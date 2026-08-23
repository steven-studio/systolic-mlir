module systolic_array_4x4 #(
    parameter int DATA_W = 32
) (
    input  logic clk,
    input  logic rst,

    input  logic [DATA_W-1:0] a_in [0:3],
    input  logic [DATA_W-1:0] b_in [0:3],

    input  logic a_valid_in [0:3],
    input  logic b_valid_in [0:3],

    input  logic [3:0] acc_sel,

    input  logic reduce_start,

    output logic c_valid_out,
    output logic [DATA_W-1:0] c_out [0:3][0:3],

    output logic [DATA_W-1:0] dbg_acc_out [0:3][0:3][0:15]
);

    logic [DATA_W-1:0] a_bus [0:3][0:4];
    logic [DATA_W-1:0] b_bus [0:4][0:3];

    logic a_valid_bus [0:3][0:4];
    logic b_valid_bus [0:4][0:3];

    logic reduce_valid [0:3][0:3];
    logic reduce_busy  [0:3][0:3];

    genvar r, c, k;

    generate
        for (r = 0; r < 4; r = r + 1) begin : INIT_A
            assign a_bus[r][0]       = a_in[r];
            assign a_valid_bus[r][0] = a_valid_in[r];
        end

        for (c = 0; c < 4; c = c + 1) begin : INIT_B
            assign b_bus[0][c]       = b_in[c];
            assign b_valid_bus[0][c] = b_valid_in[c];
        end
    endgenerate


    generate
        for (r = 0; r < 4; r = r + 1) begin : ROW
            for (c = 0; c < 4; c = c + 1) begin : COL

                logic [DATA_W-1:0] dbg_acc [0:15];

                systolic_pe u_pe (
                    .clk         (clk),
                    .rst         (rst),

                    .a_valid_in  (a_valid_bus[r][c]),
                    .b_valid_in  (b_valid_bus[r][c]),

                    .acc_sel     (acc_sel),

                    .a_in        (a_bus[r][c]),
                    .b_in        (b_bus[r][c]),

                    .a_valid_out (a_valid_bus[r][c+1]),
                    .b_valid_out (b_valid_bus[r+1][c]),

                    .a_out       (a_bus[r][c+1]),
                    .b_out       (b_bus[r+1][c]),

                    .dbg_acc     (dbg_acc)
                );

                for (k = 0; k < 16; k = k + 1) begin : EXPORT_ACC
                    assign dbg_acc_out[r][c][k] = dbg_acc[k];
                end

                fp_reduce8 u_reduce (
                    .clk       (clk),
                    .rst       (rst),

                    .start     (reduce_start),

                    .acc       (dbg_acc[0:7]),

                    .busy      (reduce_busy[r][c]),
                    .valid_out (reduce_valid[r][c]),
                    .result    (c_out[r][c])
                );

            end
        end
    endgenerate

    assign c_valid_out = reduce_valid[0][0];

endmodule
