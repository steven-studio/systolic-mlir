// ============================================================================
// systolic_hw_scheduler.sv
//
// Minimal hardware schedule controller.
//
// The compiler supplies an already-decided schedule:
//   - accelerator ID
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
// Hardware responsibility:
//   descriptor -> start pulse -> wait for done
// ============================================================================

module systolic_hw_scheduler #(
    parameter integer NUM_ACCEL = 2,
    parameter integer CYCLE_W   = 32,
    parameter integer ACC_W     = (NUM_ACCEL <= 1) ? 1 : $clog2(NUM_ACCEL)
)(
    input  wire                     clk,
    input  wire                     rst,

    // ------------------------------------------------------------------------
    // Schedule descriptor interface.
    //
    // A descriptor is accepted when desc_valid && desc_ready.
    // ------------------------------------------------------------------------
    input  wire                     desc_valid,
    output wire                     desc_ready,

    input  wire [ACC_W-1:0]         desc_accelerator_id,
    input  wire [CYCLE_W-1:0]       desc_start_cycle,
    input  wire [CYCLE_W-1:0]       desc_compute_cycles,

    // ------------------------------------------------------------------------
    // One-cycle start pulse for each physical accelerator.
    // ------------------------------------------------------------------------
    output reg  [NUM_ACCEL-1:0]     accelerator_start,

    // Accelerator completion inputs.
    //
    // accelerator_done[i] corresponds to accelerator i.
    // ------------------------------------------------------------------------
    input  wire [NUM_ACCEL-1:0]     accelerator_done,

    // Accelerator/datapath readiness.
    //
    // The scheduler must not consume its one-cycle accelerator_start pulse
    // until the datapath is actually ready to accept the invocation.
    // Otherwise the start pulse can be emitted during P_READ and lost.
    // ------------------------------------------------------------------------
    input  wire [NUM_ACCEL-1:0]     accelerator_ready,

    // ------------------------------------------------------------------------
    // Scheduler status.
    // ------------------------------------------------------------------------
    output reg                      busy,
    output reg                      schedule_done,

    output reg [CYCLE_W-1:0]        cycle_counter,

    output reg [CYCLE_W-1:0]        active_start_cycle,
    output reg [CYCLE_W-1:0]        active_compute_cycles,
    output reg [ACC_W-1:0]          active_accelerator_id
);

    typedef enum logic [1:0] {
        ST_IDLE  = 2'd0,
        ST_WAIT  = 2'd1,
        ST_RUN   = 2'd2
    } state_t;

    state_t state;

    // Accelerator completion may be a one-cycle pulse.  Keep it latched
    // until the scheduler consumes it in ST_RUN, so completion cannot be
    // lost at the ST_IDLE/ST_WAIT -> ST_RUN boundary.
    logic accelerator_done_seen;

    assign desc_ready = (state == ST_IDLE) && !busy;

    always_ff @(posedge clk) begin
        if (rst) begin
            state                  <= ST_IDLE;

            accelerator_start      <= '0;

            busy                   <= 1'b0;
            schedule_done          <= 1'b0;

            cycle_counter          <= '0;

            active_start_cycle     <= '0;
            active_compute_cycles  <= '0;
            active_accelerator_id  <= '0;

            accelerator_done_seen  <= 1'b0;
        end
        else begin
            // Start pulses are always one cycle.
            accelerator_start <= '0;

            // schedule_done is also a pulse.
            schedule_done <= 1'b0;

            // Global schedule clock.
            if (busy)
                cycle_counter <= cycle_counter + 1'b1;

            // Accelerator completion is a pulse at the hardware boundary.
            // Capture it independently of the scheduler FSM state so a
            // one-cycle pulse cannot be lost.
            if (busy && |accelerator_done)
                accelerator_done_seen <= 1'b1;

            case (state)

                // ------------------------------------------------------------
                // Accept a compiler-generated descriptor.
                // ------------------------------------------------------------
                ST_IDLE: begin
                    busy <= 1'b0;

                    if (desc_valid && desc_ready) begin
                        active_start_cycle    <= desc_start_cycle;
                        active_compute_cycles <= desc_compute_cycles;
                        active_accelerator_id <= desc_accelerator_id;

                        // Start a fresh completion window for this job.
                        accelerator_done_seen <= 1'b0;

                        busy <= 1'b1;

                        if ((cycle_counter >= desc_start_cycle) &&
                            accelerator_ready[desc_accelerator_id]) begin
                            accelerator_start[desc_accelerator_id] <= 1'b1;
                            state <= ST_RUN;
                        end
                        else begin
                            state <= ST_WAIT;
                        end
                    end
                end

                // ------------------------------------------------------------
                // Wait until the scheduled start cycle.
                // ------------------------------------------------------------
                ST_WAIT: begin
                    // cycle_counter is incremented in the same clock edge
                    // while busy.  Therefore, the next visible counter
                    // value is cycle_counter + 1.  Fire the start pulse
                    // when that next cycle reaches the requested
                    // compiler schedule cycle.
                    if (((cycle_counter + 1'b1) >= active_start_cycle) &&
                        accelerator_ready[active_accelerator_id]) begin
                        accelerator_start[active_accelerator_id] <= 1'b1;
                        state <= ST_RUN;
                    end
                end

                // ------------------------------------------------------------
                // Accelerator is executing.
                //
                // Completion comes from the actual hardware accelerator.
                // We intentionally trust the hardware done signal rather
                // than assuming the analytical cost is exact.
                // ------------------------------------------------------------
                ST_RUN: begin
                    if (accelerator_done_seen ||
                        accelerator_done[active_accelerator_id]) begin
                        $display(
                            "SCHEDDONE t=%0t state=%0d active_dev=%0d accelerator_done=%b busy=%0b",
                            $time,
                            state,
                            active_accelerator_id,
                            accelerator_done,
                            busy
                        );

                        busy          <= 1'b0;
                        schedule_done <= 1'b1;
                        state         <= ST_IDLE;
                    end
                end

                default: begin
                    state <= ST_IDLE;
                    busy  <= 1'b0;
                end

            endcase
        end
    end

endmodule
