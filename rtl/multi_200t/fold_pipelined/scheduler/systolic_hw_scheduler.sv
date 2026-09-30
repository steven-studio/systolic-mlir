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
        end
        else begin
            // Start pulses are always one cycle.
            accelerator_start <= '0;

            // schedule_done is also a pulse.
            schedule_done <= 1'b0;

            // Global schedule clock.
            if (busy)
                cycle_counter <= cycle_counter + 1'b1;

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

                        busy <= 1'b1;

                        if (cycle_counter >= desc_start_cycle) begin
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
                    if (cycle_counter >= active_start_cycle) begin
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
                    if (accelerator_done[active_accelerator_id]) begin
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
