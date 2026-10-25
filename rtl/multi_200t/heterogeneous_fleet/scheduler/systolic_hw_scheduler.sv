// ============================================================================
// systolic_hw_scheduler.sv
//
// Minimal hardware schedule controller, one execution context per physical
// accelerator.
//
// The compiler supplies an already-decided schedule per tile:
//   - which accelerator (implicit: the request arrives on that accelerator's
//     own request line)
//   - start cycle
//   - expected compute duration
//
// The controller enforces the temporal schedule in hardware.
//
// This module deliberately does NOT:
//   - perform DMA
//   - calculate tile cost
//   - choose an accelerator
//   - perform decomposition
//
// Those decisions belong to the compiler.
//
// Hardware responsibility, per accelerator:
//   request -> (wait for the start cycle and for the datapath to be ready)
//           -> one-cycle start pulse -> the slot is free for the next request
//
// The contexts are independent: accelerator i's request, start, run and
// completion never touch accelerator j's.  There is no shared "active
// accelerator" -- that single slot is what used to serialize the fleet.  The
// only shared state is cycle_counter, the schedule clock the start cycles are
// measured against; it runs while any accelerator is busy.
//
// RELEASE ON START, NOT ON DONE (Oct 2026).  The slot used to stay in ST_RUN
// from the start pulse until that accelerator's done -- the array's
// c_valid_out, which comes after every PE's bank reduction.  That made the
// scheduler the first place where consecutive invocations of one accelerator
// were serialized: the next tile's request could not even be taken until the
// previous tile's reduction had finished, although the PEs' ping-pong
// accumulator sets let the next tile be fed while the previous one reduces.
// Now the slot returns to ST_IDLE as soon as it has pulsed the start; WHEN the
// datapath can take another transaction is the context's decision
// (accelerator_ready: operands landed, the PE quiet gap elapsed, a result bank
// free), exactly as before -- the slot only ever pulses start while
// accelerator_ready is high.  Completions are counted per accelerator: busy[i]
// stays high while a request is held or any start has not yet been answered
// by done, so the schedule clock keeps running through a pipelined stream, and
// schedule_done[i] still pulses once per completion.
// ============================================================================

module systolic_hw_scheduler #(
    parameter integer NUM_ACCEL = 4,
    parameter integer CYCLE_W   = 32
)(
    input  wire                     clk,
    input  wire                     rst,

    // ------------------------------------------------------------------------
    // Per-accelerator tile requests.
    //
    // req_valid[i] is a level held by accelerator i's context; it is taken
    // (req_ack[i]) when context i here is idle, and the schedule fields are
    // latched at that moment.
    // ------------------------------------------------------------------------
    input  wire [NUM_ACCEL-1:0]     req_valid,
    output wire [NUM_ACCEL-1:0]     req_ready,
    output wire [NUM_ACCEL-1:0]     req_ack,
    input  wire [CYCLE_W-1:0]       req_start_cycle    [0:NUM_ACCEL-1],
    input  wire [CYCLE_W-1:0]       req_compute_cycles [0:NUM_ACCEL-1],

    // ------------------------------------------------------------------------
    // Accelerator/datapath readiness, per accelerator.
    //
    // The start pulse is consumed only while the datapath is actually ready
    // to accept the invocation; otherwise a one-cycle pulse could be lost.
    // ------------------------------------------------------------------------
    input  wire [NUM_ACCEL-1:0]     accelerator_ready,

    // One-cycle start pulse for each physical accelerator.
    output wire [NUM_ACCEL-1:0]     accelerator_start,

    // Completion inputs; accelerator_done[i] belongs to accelerator i only.
    // One pulse per start, in order; the context may have several starts
    // outstanding (its result pipeline) and answers them one by one.
    input  wire [NUM_ACCEL-1:0]     accelerator_done,

    // ------------------------------------------------------------------------
    // Status, per accelerator, plus the shared schedule clock.
    // ------------------------------------------------------------------------
    output wire [NUM_ACCEL-1:0]     busy,
    output wire [NUM_ACCEL-1:0]     schedule_done,
    output reg  [CYCLE_W-1:0]       cycle_counter,
    output wire [CYCLE_W-1:0]       active_start_cycle    [0:NUM_ACCEL-1],
    output wire [CYCLE_W-1:0]       active_compute_cycles [0:NUM_ACCEL-1]
);

    typedef enum logic [1:0] {
        ST_IDLE  = 2'd0,
        ST_WAIT  = 2'd1
    } state_t;

    // Global schedule clock: advances while any accelerator is busy.
    always_ff @(posedge clk) begin
        if (rst)        cycle_counter <= '0;
        else if (|busy) cycle_counter <= cycle_counter + 1'b1;
    end

    genvar g;
    generate
    for (g = 0; g < NUM_ACCEL; g++) begin : CTX
        state_t             state;
        logic               start_q;
        logic               sched_done_q;
        // Starts pulsed and not yet answered by this accelerator's done.
        // Completion comes from the hardware; the analytical cost is not
        // assumed exact.  The counter replaces the old ST_RUN wait: the slot
        // no longer holds the next request until the previous transaction
        // has completed.
        logic [7:0]         outstanding;
        logic [CYCLE_W-1:0] start_cycle_q;
        logic [CYCLE_W-1:0] compute_cycles_q;

        wire done_now = accelerator_done[g] && (outstanding != 8'd0);

        assign req_ready[g]             = (state == ST_IDLE);
        assign req_ack[g]               = req_valid[g] && req_ready[g];
        // Busy while a request is held, a start is on its way out, or any
        // start is still running -- so the schedule clock never skips the
        // cycle between the decision to start and the counter seeing it.
        assign busy[g]                  = (state != ST_IDLE) || start_q || (outstanding != 8'd0);
        assign accelerator_start[g]     = start_q;
        assign schedule_done[g]         = sched_done_q;
        assign active_start_cycle[g]    = start_cycle_q;
        assign active_compute_cycles[g] = compute_cycles_q;

        always_ff @(posedge clk) begin
            if (rst) begin
                state            <= ST_IDLE;
                outstanding      <= 8'd0;
                start_q          <= 1'b0;
                sched_done_q     <= 1'b0;
                start_cycle_q    <= '0;
                compute_cycles_q <= '0;
            end
            else begin
                // Start pulses and schedule_done are always one cycle.
                start_q      <= 1'b0;
                sched_done_q <= 1'b0;

                // One completion per start, whenever it arrives.
                if (done_now) begin
                    $display("SCHEDDONE t=%0t accel=%0d accelerator_done=%b",
                             $time, g, accelerator_done);
                    sched_done_q <= 1'b1;
                end

                // Starts and dones in the same cycle cancel.
                if (start_q && !done_now)      outstanding <= outstanding + 8'd1;
                else if (!start_q && done_now) outstanding <= outstanding - 8'd1;

                case (state)
                    // --------------------------------------------------------
                    // Take this accelerator's next tile.
                    // --------------------------------------------------------
                    ST_IDLE: begin
                        if (req_ack[g]) begin
                            start_cycle_q    <= req_start_cycle[g];
                            compute_cycles_q <= req_compute_cycles[g];

                            if ((cycle_counter >= req_start_cycle[g]) &&
                                accelerator_ready[g]) begin
                                start_q <= 1'b1;
                                // Released: the slot is free for the next
                                // request while this transaction runs.
                                state   <= ST_IDLE;
                            end
                            else begin
                                state <= ST_WAIT;
                            end
                        end
                    end

                    // --------------------------------------------------------
                    // Wait for the scheduled start cycle and for the datapath.
                    // cycle_counter advances on this same edge, so the next
                    // visible value is cycle_counter + 1.
                    // --------------------------------------------------------
                    ST_WAIT: begin
                        if (((cycle_counter + 1'b1) >= start_cycle_q) &&
                            accelerator_ready[g]) begin
                            start_q <= 1'b1;
                            state   <= ST_IDLE;
                        end
                    end

                    default: begin
                        state <= ST_IDLE;
                    end
                endcase
            end
        end
    end
    endgenerate

endmodule
