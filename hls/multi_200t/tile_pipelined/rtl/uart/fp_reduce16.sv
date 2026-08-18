module fp_reduce16 #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic start,

    input  logic [DATA_W-1:0] acc [0:15],

    output logic busy,
    output logic valid_out,
    output logic [DATA_W-1:0] result
);

    typedef enum logic [1:0] {
        IDLE,
        ISSUE,
        WAIT_RESULT
    } state_t;

    state_t state;

    logic [3:0] index;

    logic [DATA_W-1:0] running_sum;

    logic add_valid_in;
    logic add_valid_out;

    logic [DATA_W-1:0] add_a;
    logic [DATA_W-1:0] add_b;
    logic [DATA_W-1:0] add_result;


    /*
     * ============================================================
     * One shared pipelined FP adder
     * ============================================================
     */
    fp_add u_reduce_add (
        .clk       (clk),
        .rst       (rst),

        .valid_in  (add_valid_in),

        .a         (add_a),
        .b         (add_b),

        .valid_out (add_valid_out),

        .result    (add_result)
    );


    /*
     * ============================================================
     * Adder input
     * ============================================================
     */
    always_comb begin

        add_valid_in = 1'b0;

        add_a = running_sum;
        add_b = acc[index];

        if (state == ISSUE)
            add_valid_in = 1'b1;

    end


    /*
     * ============================================================
     * Sequential reduction
     *
     * running_sum starts from acc[0].
     *
     * Then:
     *
     * acc[0] + acc[1]
     *        + acc[2]
     *        ...
     *        + acc[15]
     *
     * Total:
     *   15 floating-point additions.
     * ============================================================
     */
    always_ff @(posedge clk) begin

        if (rst) begin

            state       <= IDLE;

            index       <= 4'd0;

            running_sum <= '0;

            result      <= '0;

            valid_out   <= 1'b0;

            busy        <= 1'b0;

        end
        else begin

            valid_out <= 1'b0;

            case (state)

                /*
                 * ------------------------------------------------
                 * Wait for a new reduction request.
                 * ------------------------------------------------
                 */
                IDLE: begin

                    busy <= 1'b0;

                    if (start) begin

                        /*
                         * bank 0 is the initial value.
                         * First actual FP add uses bank 1.
                         */
                        running_sum <= acc[0];

                        index <= 4'd1;

                        busy <= 1'b1;

                        state <= ISSUE;

                    end

                end


                /*
                 * ------------------------------------------------
                 * Launch one FP addition.
                 * ------------------------------------------------
                 */
                ISSUE: begin

                    /*
                     * add_valid_in is asserted combinationally
                     * while state == ISSUE.
                     */
                    state <= WAIT_RESULT;

                end


                /*
                 * ------------------------------------------------
                 * Wait for pipelined fp_add result.
                 * ------------------------------------------------
                 */
                WAIT_RESULT: begin

                    if (add_valid_out) begin

                        /*
                         * bank 15 was the final operand.
                         */
                        if (index == 4'd15) begin

                            result <= add_result;

                            running_sum <= add_result;

                            valid_out <= 1'b1;

                            busy <= 1'b0;

                            state <= IDLE;

                        end
                        else begin

                            /*
                             * Feed the next bank.
                             */
                            running_sum <= add_result;

                            index <= index + 1'b1;

                            state <= ISSUE;

                        end

                    end

                end


                default: begin

                    state <= IDLE;

                end

            endcase

        end

    end

endmodule
