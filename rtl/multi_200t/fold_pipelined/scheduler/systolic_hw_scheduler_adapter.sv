// ============================================================================
// systolic_hw_scheduler_adapter.sv
//
// Connects the generic hardware scheduler to one existing systolic
// accelerator.
//
// Scheduler side:
//   accelerator_start[0]
//   accelerator_done[0]
//
// Accelerator side:
//   fold_start
//   c_done
//
// This adapter contains NO scheduling policy and NO cost model.
// It is only a protocol bridge.
// ============================================================================

module systolic_hw_scheduler_adapter #(
    parameter integer CYCLE_W = 32
)(
    input  wire                     clk,
    input  wire                     rst,

    // ------------------------------------------------------------------------
    // Scheduler side
    // ------------------------------------------------------------------------

    input  wire                     accelerator_start,
    output reg                      accelerator_done,

    // ------------------------------------------------------------------------
    // Existing systolic accelerator side
    // ------------------------------------------------------------------------

    output reg                      fold_start,
    input  wire                     c_done
);

    typedef enum logic [1:0] {
        ST_IDLE = 2'd0,
        ST_RUN  = 2'd1,
        ST_DONE = 2'd2
    } state_t;

    state_t state;

    always_ff @(posedge clk) begin
        if (rst) begin
            state             <= ST_IDLE;
            fold_start        <= 1'b0;
            accelerator_done  <= 1'b0;
        end
        else begin
            // Both are pulses.
            fold_start       <= 1'b0;
            accelerator_done <= 1'b0;

            case (state)

                // ------------------------------------------------------------
                // Wait for the scheduler to release this accelerator.
                // ------------------------------------------------------------
                ST_IDLE: begin
                    if (accelerator_start) begin
                        fold_start <= 1'b1;
                        state      <= ST_RUN;
                    end
                end

                // ------------------------------------------------------------
                // Existing accelerator is executing.
                // ------------------------------------------------------------
                ST_RUN: begin
                    if (c_done) begin
                        accelerator_done <= 1'b1;
                        state             <= ST_DONE;
                    end
                end

                // ------------------------------------------------------------
                // One-cycle separation before accepting another transaction.
                // ------------------------------------------------------------
                ST_DONE: begin
                    state <= ST_IDLE;
                end

                default: begin
                    state            <= ST_IDLE;
                    fold_start       <= 1'b0;
                    accelerator_done <= 1'b0;
                end

            endcase
        end
    end

endmodule
