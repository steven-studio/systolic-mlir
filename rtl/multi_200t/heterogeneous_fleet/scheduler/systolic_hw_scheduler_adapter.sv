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
//
// It used to be a three-state machine (idle / running / done) that dropped a
// start arriving while c_done was still pending.  With the context pipelining
// its transactions (the next tile fed while the previous one reduces), a start
// may legitimately arrive before the previous c_done, so the bridge now
// forwards every start and counts the transactions in flight; c_done pulses
// arrive in start order (the array publishes in order) and each one answers
// the oldest start.  Nothing is dropped and nothing is invented: a c_done with
// no start outstanding is ignored, as the old ST_IDLE ignored it.
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

    // Transactions started and not yet completed.
    logic [7:0] outstanding;

    wire done_now = c_done && (outstanding != 8'd0);

    always_ff @(posedge clk) begin
        if (rst) begin
            outstanding       <= 8'd0;
            fold_start        <= 1'b0;
            accelerator_done  <= 1'b0;
        end
        else begin
            // Both are pulses, one per start / per completion.
            fold_start       <= accelerator_start;
            accelerator_done <= done_now;

            if (accelerator_start && !done_now)      outstanding <= outstanding + 8'd1;
            else if (!accelerator_start && done_now) outstanding <= outstanding - 8'd1;
        end
    end

endmodule
