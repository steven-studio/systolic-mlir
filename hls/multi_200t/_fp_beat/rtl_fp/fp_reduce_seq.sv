module fp_reduce_seq #(
    parameter int DATA_W = 32,
    parameter int N       = 8
) (
    input  logic clk,
    input  logic rst,

    input  logic start,
    input  logic [DATA_W-1:0] in [0:N-1],

    output logic done,
    output logic [DATA_W-1:0] result
);

    logic [DATA_W-1:0] sum;
    logic [3:0] index;

    logic add_valid_in;
    logic add_valid_out;
    logic [DATA_W-1:0] add_result;

    logic busy;

    fp_add u_add (
        .clk       (clk),
        .rst       (rst),

        .valid_in  (add_valid_in),
        .a         (sum),
        .b         (in[index]),

        .valid_out (add_valid_out),
        .result    (add_result)
    );

    always_ff @(posedge clk) begin
        if (rst) begin
            sum          <= '0;
            index        <= '0;
            add_valid_in <= 1'b0;
            busy         <= 1'b0;
            done         <= 1'b0;
            result       <= '0;
        end
        else begin
            add_valid_in <= 1'b0;
            done         <= 1'b0;

            if (start && !busy) begin
                sum          <= '0;
                index        <= 0;
                add_valid_in <= 1'b1;
                busy         <= 1'b1;
            end

            if (busy && add_valid_out) begin
                if (index == N-1) begin
                    result <= add_result;
                    done   <= 1'b1;
                    busy   <= 1'b0;
                end
                else begin
                    sum          <= add_result;
                    index        <= index + 1'b1;
                    add_valid_in <= 1'b1;
                end
            end
        end
    end

endmodule
