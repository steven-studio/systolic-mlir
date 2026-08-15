module gemm_controller (
    input  logic clk,
    input  logic rst,

    input  logic start,

    input  logic [31:0] A [0:7][0:7],
    input  logic [31:0] B [0:7][0:7],

    output logic [31:0] a_in [0:7],
    output logic [31:0] b_in [0:7],

    output logic a_valid_in [0:7],
    output logic b_valid_in [0:7],

    output logic [3:0] acc_sel,

    output logic reduce_start,

    input  logic c_valid_in,
    input  logic [31:0] c_in [0:7][0:7],

    output logic done,
    output logic [31:0] C [0:7][0:7]
);

    typedef enum logic [2:0] {
        IDLE,
        INJECT,
        DRAIN,
        REDUCE,
        WAIT_C,
        DONE
    } state_t;

    state_t state;

    integer t;
    integer drain_count;


    /*
     * ============================================================
     * Combinational injection
     *
     * A[r][k] enters at t = r + k
     * B[k][c] enters at t = c + k
     * ============================================================
     */
    always_comb begin
        for (int i = 0; i < 8; i++) begin
            a_in[i]       = 32'd0;
            b_in[i]       = 32'd0;

            a_valid_in[i] = 1'b0;
            b_valid_in[i] = 1'b0;
        end

        if (state == INJECT) begin

            for (int r = 0; r < 8; r++) begin
                int k_idx;

                k_idx = t - r;

                if (
                    (k_idx >= 0)
                    &&
                    (k_idx < 8)
                ) begin
                    a_in[r]       = A[r][k_idx];
                    a_valid_in[r] = 1'b1;
                end
            end

            for (int c = 0; c < 8; c++) begin
                int k_idx;

                k_idx = t - c;

                if (
                    (k_idx >= 0)
                    &&
                    (k_idx < 8)
                ) begin
                    b_in[c]       = B[k_idx][c];
                    b_valid_in[c] = 1'b1;
                end
            end

        end
    end


    /*
     * ============================================================
     * Controller
     * ============================================================
     */
    always_ff @(posedge clk) begin
        if (rst) begin
            state        <= IDLE;
            t            <= 0;
            drain_count  <= 0;

            acc_sel      <= 0;
            reduce_start <= 0;
            done         <= 0;

            for (int r = 0; r < 8; r++) begin
                for (int c = 0; c < 8; c++) begin
                    C[r][c] <= 0;
                end
            end
        end
        else begin
            reduce_start <= 0;
            done         <= 0;

            case (state)

                IDLE: begin
                    t           <= 0;
                    drain_count <= 0;
                    acc_sel     <= 0;

                    if (start)
                        state <= INJECT;
                end


                /*
                 * 8x8 systolic skew takes:
                 *
                 * k + row / col
                 *
                 * t = 0 ... 14
                 */
                INJECT: begin
                    acc_sel <= acc_sel + 1'b1;

                    if (t == 14) begin
                        t           <= 0;
                        drain_count <= 0;
                        state       <= DRAIN;
                    end
                    else begin
                        t <= t + 1;
                    end
                end


                /*
                 * Give MUL + ADD pipeline enough time to drain.
                 * Conservative for first board bring-up.
                 */
                DRAIN: begin
                    if (drain_count == 40) begin
                        drain_count <= 0;
                        state       <= REDUCE;
                    end
                    else begin
                        drain_count <= drain_count + 1;
                    end
                end


                REDUCE: begin
                    reduce_start <= 1'b1;
                    state        <= WAIT_C;
                end


                WAIT_C: begin
                    if (c_valid_in) begin

                        for (int r = 0; r < 8; r++) begin
                            for (int c = 0; c < 8; c++) begin
                                C[r][c] <= c_in[r][c];
                            end
                        end

                        state <= DONE;
                    end
                end


                DONE: begin
                    done  <= 1'b1;
                    state <= IDLE;
                end

            endcase
        end
    end

endmodule
