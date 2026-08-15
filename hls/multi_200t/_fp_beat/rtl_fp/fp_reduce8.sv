module fp_reduce8 #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic start,

    input  logic [DATA_W-1:0] acc [0:7],

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

    logic [2:0] index;
    logic [DATA_W-1:0] running_sum;

    logic add_valid_in;
    logic add_valid_out;

    logic [DATA_W-1:0] add_a;
    logic [DATA_W-1:0] add_b;
    logic [DATA_W-1:0] add_result;


    fp_add u_reduce_add (
        .clk       (clk),
        .rst       (rst),

        .valid_in  (add_valid_in),
        .a         (add_a),
        .b         (add_b),

        .valid_out (add_valid_out),
        .result    (add_result)
    );


    always_comb begin
        add_valid_in = 1'b0;
        add_a        = running_sum;
        add_b        = acc[index];

        if (state == ISSUE)
            add_valid_in = 1'b1;
    end


    always_ff @(posedge clk) begin
        if (rst) begin
            state       <= IDLE;
            index       <= 3'd0;
            running_sum <= '0;
            result      <= '0;
            valid_out   <= 1'b0;
            busy        <= 1'b0;
        end
        else begin
            valid_out <= 1'b0;

            case (state)

                IDLE: begin
                    busy <= 1'b0;

                    if (start) begin
                        /*
                         * Start with bank 0.
                         * Then add banks 1..7 one at a time.
                         */
                        running_sum <= acc[0];
                        index       <= 3'd1;
                        busy        <= 1'b1;
                        state       <= ISSUE;
                    end
                end


                ISSUE: begin
                    /*
                     * add_valid_in is asserted combinationally
                     * for this cycle.
                     */
                    state <= WAIT_RESULT;
                end


                WAIT_RESULT: begin
                    if (add_valid_out) begin

                        if (index == 3'd7) begin
                            result      <= add_result;
                            running_sum <= add_result;
                            valid_out   <= 1'b1;
                            busy        <= 1'b0;
                            state       <= IDLE;
                        end
                        else begin
                            running_sum <= add_result;
                            index       <= index + 1'b1;
                            state       <= ISSUE;
                        end

                    end
                end

                default: state <= IDLE;

            endcase
        end
    end

endmodule
