module array8x8_impl_top (
    input  logic        clk,
    input  logic        rst,
    output logic [31:0] keep_out
);

    logic [31:0] a_in [0:7];
    logic [31:0] b_in [0:7];

    logic a_valid_in [0:7];
    logic b_valid_in [0:7];

    logic [3:0] acc_sel;

    logic reduce_start;
    logic c_valid_out;

    logic [31:0] c_out [0:7][0:7];
    logic [31:0] dbg_acc_out [0:7][0:7][0:15];

    /*
     * 暫時給內部固定/寄存器 stimulus，
     * 目的只是讓 Vivado 能 place & route 整顆 8x8。
     */
    always_ff @(posedge clk) begin
        if (rst) begin
            acc_sel      <= 4'd0;
            reduce_start <= 1'b0;

            for (int i = 0; i < 8; i++) begin
                a_in[i]       <= 32'd0;
                b_in[i]       <= 32'd0;
                a_valid_in[i] <= 1'b0;
                b_valid_in[i] <= 1'b0;
            end
        end
        else begin
            acc_sel      <= acc_sel + 1'b1;
            reduce_start <= 1'b0;

            for (int i = 0; i < 8; i++) begin
                a_in[i]       <= 32'h3f800000; // 1.0
                b_in[i]       <= 32'h3f800000; // 1.0
                a_valid_in[i] <= 1'b1;
                b_valid_in[i] <= 1'b1;
            end
        end
    end

    systolic_array_8x8 u_array (
        .clk          (clk),
        .rst          (rst),

        .a_in         (a_in),
        .b_in         (b_in),

        .a_valid_in   (a_valid_in),
        .b_valid_in   (b_valid_in),

        .acc_sel      (acc_sel),

        .reduce_start (reduce_start),

        .c_valid_out  (c_valid_out),
        .c_out        (c_out),

        .dbg_acc_out  (dbg_acc_out)
    );

    /*
     * 將計算結果接到 top-level output，
     * 避免 implementation 的 opt_design
     * 把整個 8x8 datapath 當成 unused logic 刪掉。
     */
    assign keep_out = c_out[0][0];

endmodule