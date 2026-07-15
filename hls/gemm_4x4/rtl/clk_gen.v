// 100MHz -> 20MHz 時脈產生器 (Artix-7 MMCME2_BASE)
module clk_gen (
    input  wire clk_in100,
    input  wire rst_in,
    output wire clk_out20,
    output wire locked
);
    wire clkfb;
    wire clk_out20_unbuf;

    MMCME2_BASE #(
        .BANDWIDTH("OPTIMIZED"),
        .CLKFBOUT_MULT_F(10.0),
        .CLKFBOUT_PHASE(0.0),
        .CLKIN1_PERIOD(10.0),       // 100MHz 輸入
        .CLKOUT0_DIVIDE_F(50.0),    // 1000MHz / 50 = 20MHz
        .CLKOUT0_DUTY_CYCLE(0.5),
        .CLKOUT0_PHASE(0.0),
        .DIVCLK_DIVIDE(1),
        .REF_JITTER1(0.0),
        .STARTUP_WAIT("FALSE")
    ) u_mmcm (
        .CLKIN1(clk_in100),
        .CLKFBIN(clkfb),
        .RST(rst_in),
        .PWRDWN(1'b0),
        .CLKOUT0(clk_out20_unbuf),
        .CLKFBOUT(clkfb),
        .LOCKED(locked)
    );

    BUFG u_bufg (
        .I(clk_out20_unbuf),
        .O(clk_out20)
    );
endmodule
