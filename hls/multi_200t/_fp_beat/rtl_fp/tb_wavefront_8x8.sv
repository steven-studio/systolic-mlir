`timescale 1ns/1ps

module tb_wavefront_8x8;

    localparam int R = 8;
    localparam int C = 8;
    localparam int K = 64;

    logic clk;
    logic rst;

    logic a_valid_in [0:R-1];
    logic b_valid_in [0:C-1];

    logic a_valid_bus [0:R-1][0:C];
    logic b_valid_bus [0:R][0:C-1];

    integer cycle;
    integer first_cycle;
    integer last_cycle;
    integer pair_count;

    initial clk = 0;
    always #5 clk = ~clk;

    genvar r, c;

    generate
        for (r = 0; r < R; r++) begin
            assign a_valid_bus[r][0] = a_valid_in[r];
        end

        for (c = 0; c < C; c++) begin
            assign b_valid_bus[0][c] = b_valid_in[c];
        end
    endgenerate

    generate
        for (r = 0; r < R; r++) begin : ROW
            for (c = 0; c < C; c++) begin : COL

                always_ff @(posedge clk) begin
                    if (rst) begin
                        a_valid_bus[r][c+1] <= 1'b0;
                        b_valid_bus[r+1][c] <= 1'b0;
                    end
                    else begin
                        a_valid_bus[r][c+1] <= a_valid_bus[r][c];
                        b_valid_bus[r+1][c] <= b_valid_bus[r][c];
                    end
                end

            end
        end
    endgenerate

    initial begin
        rst         = 1;
        cycle       = -1;
        first_cycle = -1;
        last_cycle  = -1;
        pair_count  = 0;

        for (int i = 0; i < R; i++)
            a_valid_in[i] = 0;

        for (int i = 0; i < C; i++)
            b_valid_in[i] = 0;

        repeat (3) @(posedge clk);
        @(negedge clk);
        rst = 0;

        for (int t = 0; t < K + 7; t++) begin
            @(negedge clk);

            for (int i = 0; i < R; i++)
                a_valid_in[i] = 0;

            for (int i = 0; i < C; i++)
                b_valid_in[i] = 0;

            for (int r = 0; r < R; r++) begin
                int k_idx = t - r;
                if ((k_idx >= 0) && (k_idx < K))
                    a_valid_in[r] = 1;
            end

            for (int c = 0; c < C; c++) begin
                int k_idx = t - c;
                if ((k_idx >= 0) && (k_idx < K))
                    b_valid_in[c] = 1;
            end
        end

        @(negedge clk);

        for (int i = 0; i < R; i++)
            a_valid_in[i] = 0;

        for (int i = 0; i < C; i++)
            b_valid_in[i] = 0;

        repeat (20) @(posedge clk);

        $display("");
        $display("first PE[7][7] pair = %0d", first_cycle);
        $display("last  PE[7][7] pair = %0d", last_cycle);
        $display("pair count           = %0d", pair_count);
        $display("measured cycles      = %0d", last_cycle + 1);
        $display("expected cycles      = %0d", K + R + C - 2);

        if (first_cycle == R + C - 2 &&
            last_cycle  == K + R + C - 3 &&
            pair_count  == K) begin

            $display("PASS: K + R + C - 2 = %0d", K + R + C - 2);
        end
        else begin
            $fatal(1, "FAIL");
        end

        $finish;
    end

    always @(posedge clk) begin
        if (rst) begin
            cycle <= -1;
        end
        else begin
            cycle <= cycle + 1;

            if (a_valid_bus[7][7] && b_valid_bus[7][7]) begin
                if (pair_count == 0)
                    first_cycle = cycle;

                last_cycle = cycle;
                pair_count = pair_count + 1;
            end
        end
    end

endmodule

