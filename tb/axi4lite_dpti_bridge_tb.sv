`timescale 1ns/1ps

module axi4lite_dpti_bridge_tb;

  reg         clk;
  reg         rst;

  // DPTI_TB_WATCHDOG
  // Fail fast if an AXI handshake never completes.
  initial begin
    #100000;
    $display("");
    $display("ERROR: TB WATCHDOG TIMEOUT");
    $display("AXI4-Lite bridge test did not complete within 100000 time units.");
    $display("awready=%0b wready=%0b bvalid=%0b",
             s_axi_awready, s_axi_wready, s_axi_bvalid);
    $display("arready=%0b rvalid=%0b",
             s_axi_arready, s_axi_rvalid);
    $display("wr_valid=%0b wr_ready=%0b",
             wr_valid, wr_ready);
    $fatal(1);
  end

  reg  [7:0]  s_axi_awaddr;
  reg         s_axi_awvalid;
  wire        s_axi_awready;

  reg  [31:0] s_axi_wdata;
  reg  [3:0]  s_axi_wstrb;
  reg         s_axi_wvalid;
  wire        s_axi_wready;

  wire [1:0]  s_axi_bresp;
  wire        s_axi_bvalid;
  reg         s_axi_bready;

  reg  [7:0]  s_axi_araddr;
  reg         s_axi_arvalid;
  wire        s_axi_arready;

  wire [31:0] s_axi_rdata;
  wire [1:0]  s_axi_rresp;
  wire        s_axi_rvalid;
  reg         s_axi_rready;

  wire        wr_valid;
  wire        wr_ready;
  wire [7:0]  wr_addr;
  wire [31:0] wr_data;

  // ----------------------------------------------------------
  // DPTI downstream model.
  //
  // The AXI4-Lite bridge is the DUT here.  The real hardware
  // connects wr_ready to dpti_descriptor_bridge.  For this
  // unit test, model the downstream consumer as always ready.
  // ----------------------------------------------------------
  assign wr_ready = 1'b1;

  wire [31:0] status;

  // ------------------------------------------------------------
  // DUT
  // ------------------------------------------------------------

  axi4lite_dpti_bridge dut (
    .aclk            (clk),
    .aresetn         (rst),

    .s_axi_awaddr   (s_axi_awaddr),
    .s_axi_awvalid  (s_axi_awvalid),
    .s_axi_awready  (s_axi_awready),

    .s_axi_wdata    (s_axi_wdata),
    .s_axi_wstrb    (s_axi_wstrb),
    .s_axi_wvalid   (s_axi_wvalid),
    .s_axi_wready   (s_axi_wready),

    .s_axi_bresp    (s_axi_bresp),
    .s_axi_bvalid   (s_axi_bvalid),
    .s_axi_bready   (s_axi_bready),

    .s_axi_araddr   (s_axi_araddr),
    .s_axi_arvalid  (s_axi_arvalid),
    .s_axi_arready  (s_axi_arready),

    .s_axi_rdata    (s_axi_rdata),
    .s_axi_rresp    (s_axi_rresp),
    .s_axi_rvalid   (s_axi_rvalid),
    .s_axi_rready   (s_axi_rready),

    .wr_valid       (wr_valid),
    .wr_ready       (wr_ready),
    .wr_addr        (wr_addr),
    .wr_data        (wr_data),

    .status         (status)
  );

  // ------------------------------------------------------------
  // Clock
  // ------------------------------------------------------------

  initial begin
    clk = 1'b0;
    forever #5 clk = ~clk;
  end

  // ------------------------------------------------------------
  // AXI4-Lite write helper
  //
  // Sends AW and W independently, as required by AXI4-Lite.
  // ------------------------------------------------------------

  task automatic axi_write;
    input [7:0]  addr;
    input [31:0] data;
    begin
      @(posedge clk);

      s_axi_awaddr  <= addr;
      s_axi_awvalid <= 1'b1;

      s_axi_wdata   <= data;
      s_axi_wstrb   <= 4'b1111;
      s_axi_wvalid  <= 1'b1;

      // AW handshake
      $display("TB: waiting for AWREADY t=%0t", $time);

      while (!s_axi_awready) begin
        @(posedge clk);
        if (($time % 1000) == 0)
          $display("TB: still waiting AWREADY t=%0t", $time);
      end

      $display("TB: AWREADY observed t=%0t", $time);

      @(posedge clk);
      s_axi_awvalid <= 1'b0;

      // W handshake
      $display("TB: waiting for WREADY t=%0t", $time);

      while (!s_axi_wready) begin
        @(posedge clk);
        if (($time % 1000) == 0)
          $display("TB: still waiting WREADY t=%0t", $time);
      end

      $display("TB: WREADY observed t=%0t", $time);

      @(posedge clk);
      s_axi_wvalid <= 1'b0;

      // Accept write response.
      s_axi_bready <= 1'b1;

      $display("TB: waiting for BVALID t=%0t", $time);

      while (!s_axi_bvalid) begin
        @(posedge clk);
        if (($time % 1000) == 0)
          $display("TB: still waiting BVALID t=%0t", $time);
      end

      $display("TB: BVALID observed t=%0t", $time);

      if (s_axi_bresp !== 2'b00) begin
        $display(
          "ERROR: AXI write response addr=0x%02x resp=%b",
          addr, s_axi_bresp
        );
        $fatal;
      end

      @(posedge clk);
      s_axi_bready <= 1'b0;

      $display(
        "AXI WRITE OK  addr=0x%02x data=0x%08x",
        addr, data
      );
    end
  endtask

  // ------------------------------------------------------------
  // DPTI write monitor
  // ------------------------------------------------------------

  always @(posedge clk) begin
    if (wr_valid && wr_ready) begin
      $display(
        "DPTI WRITE     addr=0x%02x data=0x%08x",
        wr_addr,
        wr_data
      );
    end
  end

  // ------------------------------------------------------------
  // Test
  // ------------------------------------------------------------

  initial begin

    s_axi_awaddr  = 8'd0;
    s_axi_awvalid = 1'b0;

    s_axi_wdata   = 32'd0;
    s_axi_wstrb   = 4'b0000;
    s_axi_wvalid  = 1'b0;

    s_axi_bready  = 1'b0;

    s_axi_araddr  = 8'd0;
    s_axi_arvalid = 1'b0;
    s_axi_rready  = 1'b0;

    // aresetn is active-low:
    //   0 = reset asserted
    //   1 = normal operation
    rst = 1'b0;

    repeat (3)
      @(posedge clk);

    rst = 1'b1;

    @(posedge clk);

    $display("");
    $display("============================================================");
    $display(" AXI4-Lite -> DPTI descriptor bridge test");
    $display("============================================================");
    $display("");

    // ----------------------------------------------------------
    // Descriptor
    // ----------------------------------------------------------

    axi_write(8'h04, 32'h00000001); // JOB_ID
    axi_write(8'h08, 32'h00000000); // DEVICE_ID
    axi_write(8'h0c, 32'd8);         // M
    axi_write(8'h10, 32'd8);         // N
    axi_write(8'h14, 32'd16);        // K
    axi_write(8'h18, 32'd100);       // START_CYCLE
    axi_write(8'h1c, 32'd125);       // EST_CYCLES

    axi_write(8'h20, 32'h00001000); // A_BASE_LO
    axi_write(8'h24, 32'h00000000); // A_BASE_HI

    axi_write(8'h28, 32'h00002000); // B_BASE_LO
    axi_write(8'h2c, 32'h00000000); // B_BASE_HI

    axi_write(8'h30, 32'h00003000); // C_BASE_LO
    axi_write(8'h34, 32'h00000000); // C_BASE_HI

    // ----------------------------------------------------------
    // Submit
    // ----------------------------------------------------------

    $display("");
    $display("SUBMIT JOB");
    axi_write(8'h00, 32'h00000001);

    repeat (3)
      @(posedge clk);

    $display("");
    $display("============================================================");
    $display(" PASS: AXI4-Lite writes completed");
    $display("============================================================");
    $display("");

    $finish;
  end

endmodule
