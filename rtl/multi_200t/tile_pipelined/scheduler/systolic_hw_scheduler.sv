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
//           -> one-cycle start pulse -> wait for that accelerator's done
//
// The contexts are independent: accelerator i's request, start, run and
// completion never touch accelerator j's.  There is no shared "active
// accelerator" -- that single slot is what used to serialize the fleet.  The
// only shared state is cycle_counter, the schedule clock the start cycles are
// measured against; it runs while any accelerator is busy.
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
        ST_WAIT  = 2'd1,
        ST_RUN   = 2'd2
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
        logic               busy_q;
        logic               start_q;
        logic               sched_done_q;
        // Completion may be a one-cycle pulse; keep it until consumed in ST_RUN.
        logic               done_seen;
        logic [CYCLE_W-1:0] start_cycle_q;
        logic [CYCLE_W-1:0] compute_cycles_q;

        assign req_ready[g]             = (state == ST_IDLE);
        assign req_ack[g]               = req_valid[g] && req_ready[g];
        assign busy[g]                  = busy_q;
        assign accelerator_start[g]     = start_q;
        assign schedule_done[g]         = sched_done_q;
        assign active_start_cycle[g]    = start_cycle_q;
        assign active_compute_cycles[g] = compute_cycles_q;

        always_ff @(posedge clk) begin
            if (rst) begin
                state            <= ST_IDLE;
                busy_q           <= 1'b0;
                done_seen        <= 1'b0;
                start_q          <= 1'b0;
                sched_done_q     <= 1'b0;
                start_cycle_q    <= '0;
                compute_cycles_q <= '0;
            end
            else begin
                // Start pulses and schedule_done are always one cycle.
                start_q      <= 1'b0;
                sched_done_q <= 1'b0;

                // Capture this accelerator's completion independently of the
                // FSM so a one-cycle pulse cannot be lost.
                if (busy_q && accelerator_done[g])
                    done_seen <= 1'b1;

                case (state)
                    // --------------------------------------------------------
                    // Take this accelerator's next tile.
                    // --------------------------------------------------------
                    ST_IDLE: begin
                        if (req_ack[g]) begin
                            start_cycle_q    <= req_start_cycle[g];
                            compute_cycles_q <= req_compute_cycles[g];
                            busy_q           <= 1'b1;
                            done_seen        <= 1'b0;

                            if ((cycle_counter >= req_start_cycle[g]) &&
                                accelerator_ready[g]) begin
                                start_q <= 1'b1;
                                state   <= ST_RUN;
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
                            state   <= ST_RUN;
                        end
                    end

                    // --------------------------------------------------------
                    // This accelerator is executing.  Completion comes from
                    // the hardware; the analytical cost is not assumed exact.
                    // --------------------------------------------------------
                    ST_RUN: begin
                        if (done_seen || accelerator_done[g]) begin
                            $display("SCHEDDONE t=%0t accel=%0d accelerator_done=%b",
                                     $time, g, accelerator_done);
                            busy_q       <= 1'b0;
                            sched_done_q <= 1'b1;
                            state        <= ST_IDLE;
                        end
                    end

                    default: begin
                        state  <= ST_IDLE;
                        busy_q <= 1'b0;
                    end
                endcase
            end
        end
    end
    endgenerate

endmodule
