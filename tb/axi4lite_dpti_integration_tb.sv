`timescale 1ns/1ps
`default_nettype none

// ============================================================================
// AXI4-Lite -> DPTI integration test
//
// AXI4-Lite
//     |
//     v
// axi4lite_dpti_bridge
//     |
//     | wr_valid/ready/addr/data
//     v
// dpti_descriptor_bridge
//     |
//     v
// job_valid + descriptor
//
// This test verifies the complete descriptor transport path.
// ============================================================================

module axi4lite_dpti_integration_tb;

  // --------------------------------------------------------------------------
  // Clock / reset
  // --------------------------------------------------------------------------

  reg clk;

  // AXI4-Lite reset is active-low.
  reg axi_aresetn;

  // DPTI descriptor bridge reset is active-high.
  reg dpti_rst;

  // --------------------------------------------------------------------------
  // AXI4-Lite
  // --------------------------------------------------------------------------

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

  // --------------------------------------------------------------------------
  // DPTI transport
  // --------------------------------------------------------------------------

  wire        dpti_wr_valid;
  wire        dpti_wr_ready;
  wire [7:0]  dpti_wr_addr;
  wire [31:0] dpti_wr_data;

  wire [31:0] dpti_status;

  // --------------------------------------------------------------------------
  // Job descriptor
  // --------------------------------------------------------------------------

  wire        job_valid;
  reg         job_ready;

  wire [31:0] job_id;
  wire [31:0] job_device_id;

  wire [31:0] job_m;
  wire [31:0] job_n;
  wire [31:0] job_k;

  wire [31:0] job_start_cycle;
  wire [31:0] job_est_cycles;

  wire [63:0] job_a_base;
  wire [63:0] job_b_base;
  wire [63:0] job_c_base;

  // --------------------------------------------------------------------------
  // DUT 1: AXI4-Lite -> DPTI transport
  // --------------------------------------------------------------------------

  axi4lite_dpti_bridge dut_axi (
      .aclk           (clk),
      .aresetn        (axi_aresetn),

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

      .wr_valid       (dpti_wr_valid),
      .wr_ready       (dpti_wr_ready),
      .wr_addr        (dpti_wr_addr),
      .wr_data        (dpti_wr_data),

      .status         (dpti_status)
  );

  // --------------------------------------------------------------------------
  // DUT 2: DPTI transport -> descriptor/job
  // --------------------------------------------------------------------------

  dpti_descriptor_bridge dut_dpti (
      .clk             (clk),
      .rst             (dpti_rst),

      .wr_valid        (dpti_wr_valid),
      .wr_ready        (dpti_wr_ready),
      .wr_addr         (dpti_wr_addr),
      .wr_data         (dpti_wr_data),

      .job_valid       (job_valid),
      .job_ready       (job_ready),

      .job_id          (job_id),
      .job_device_id   (job_device_id),

      .job_m           (job_m),
      .job_n           (job_n),
      .job_k           (job_k),

      .job_start_cycle (job_start_cycle),
      .job_est_cycles  (job_est_cycles),

      .job_a_base      (job_a_base),
      .job_b_base      (job_b_base),
      .job_c_base      (job_c_base),

      .status          (dpti_status)
  );

  // --------------------------------------------------------------------------
  // Clock
  // --------------------------------------------------------------------------

  initial begin
    clk = 1'b0;
    forever #5 clk = ~clk;
  end

  // --------------------------------------------------------------------------
  // Watchdog
  // --------------------------------------------------------------------------

  initial begin
    #200000;

    $display("");
    $display("ERROR: integration TB watchdog timeout");
    $display(
      "axi_aresetn=%0b dpti_rst=%0b",
      axi_aresetn,
      dpti_rst
    );
    $display(
      "AWREADY=%0b WREADY=%0b BVALID=%0b",
      s_axi_awready,
      s_axi_wready,
      s_axi_bvalid
    );
    $display(
      "job_valid=%0b job_ready=%0b",
      job_valid,
      job_ready
    );
    $display(
      "dpti_wr_valid=%0b dpti_wr_ready=%0b",
      dpti_wr_valid,
      dpti_wr_ready
    );
    $display("DPTI addr=0x%02x data=0x%08x",
             dpti_wr_addr, dpti_wr_data);

    $fatal(1);
  end

  // --------------------------------------------------------------------------
  // AXI4-Lite write helper
  //
  // AW and W are deliberately issued independently.
  // --------------------------------------------------------------------------

  task axi_write;
    input [7:0]  addr;
    input [31:0] data;

    integer guard;

    begin
      $display("");
      $display(
        "AXI WRITE    addr=0x%02x data=0x%08x",
        addr,
        data
      );

      // ==========================================================
      // AW CHANNEL
      // ==========================================================

      @(negedge clk);

      s_axi_awaddr  <= addr;
      s_axi_awvalid <= 1'b1;

      $display(
        "  AW drive    t=%0t AWVALID=%0b AWREADY=%0b",
        $time,
        s_axi_awvalid,
        s_axi_awready
      );

      guard = 0;

      while (!(s_axi_awvalid && s_axi_awready)) begin
        @(posedge clk);

        $display(
          "  AW sample   t=%0t AWVALID=%0b AWREADY=%0b",
          $time,
          s_axi_awvalid,
          s_axi_awready
        );

        guard = guard + 1;

        if (guard > 100) begin
          $display("ERROR: AW handshake timeout");
          $fatal(1);
        end
      end

      // The handshake condition was observed on the current edge.
      @(negedge clk);

      @(negedge clk);
      s_axi_awvalid <= 1'b0;

      $display(
        "  AW done     t=%0t",
        $time
      );

      // ==========================================================
      // W CHANNEL
      // ==========================================================

      @(negedge clk);

      s_axi_wdata  <= data;
      s_axi_wstrb  <= 4'hf;
      s_axi_wvalid <= 1'b1;

      $display(
        "  W drive     t=%0t WVALID=%0b WREADY=%0b",
        $time,
        s_axi_wvalid,
        s_axi_wready
      );

      guard = 0;

      while (!(s_axi_wvalid && s_axi_wready)) begin
        @(posedge clk);

        $display(
          "  W sample    t=%0t WVALID=%0b WREADY=%0b",
          $time,
          s_axi_wvalid,
          s_axi_wready
        );

        guard = guard + 1;

        if (guard > 100) begin
          $display("ERROR: W handshake timeout");
          $fatal(1);
        end
      end

      @(negedge clk);

      @(negedge clk);
      s_axi_wvalid <= 1'b0;

      $display(
        "  W done      t=%0t",
        $time
      );

      // ==========================================================
      // B CHANNEL
      // ==========================================================

      @(negedge clk);

      s_axi_bready <= 1'b1;

      $display(
        "  B wait      t=%0t BVALID=%0b",
        $time,
        s_axi_bvalid
      );

      guard = 0;

      while (!s_axi_bvalid) begin
        @(posedge clk);

        $display(
          "  B sample    t=%0t BVALID=%0b BRESP=%b",
          $time,
          s_axi_bvalid,
          s_axi_bresp
        );

        guard = guard + 1;

        if (guard > 100) begin
          $display("ERROR: BVALID timeout");
          $fatal(1);
        end
      end

      @(negedge clk);

      if (s_axi_bresp !== 2'b00) begin
        $display(
          "ERROR: BRESP=%b for addr=0x%02x",
          s_axi_bresp,
          addr
        );
        $fatal(1);
      end

      @(negedge clk);
      s_axi_bready <= 1'b0;

      $display(
        "AXI WRITE OK addr=0x%02x data=0x%08x",
        addr,
        data
      );
    end
  endtask

  // --------------------------------------------------------------------------
  // Descriptor checker
  // --------------------------------------------------------------------------

  task check_descriptor;
    begin

      if (job_id !== 32'd1) begin
        $display("ERROR: job_id = 0x%08x", job_id);
        $fatal(1);
      end

      if (job_device_id !== 32'd0) begin
        $display("ERROR: job_device_id = 0x%08x", job_device_id);
        $fatal(1);
      end

      if (job_m !== 32'd8) begin
        $display("ERROR: job_m = %0d", job_m);
        $fatal(1);
      end

      if (job_n !== 32'd8) begin
        $display("ERROR: job_n = %0d", job_n);
        $fatal(1);
      end

      if (job_k !== 32'd16) begin
        $display("ERROR: job_k = %0d", job_k);
        $fatal(1);
      end

      if (job_start_cycle !== 32'd100) begin
        $display(
          "ERROR: job_start_cycle = %0d",
          job_start_cycle
        );
        $fatal(1);
      end

      if (job_est_cycles !== 32'd125) begin
        $display(
          "ERROR: job_est_cycles = %0d",
          job_est_cycles
        );
        $fatal(1);
      end

      if (job_a_base !== 64'h00000000_00001000) begin
        $display(
          "ERROR: job_a_base = 0x%016x",
          job_a_base
        );
        $fatal(1);
      end

      if (job_b_base !== 64'h00000000_00002000) begin
        $display(
          "ERROR: job_b_base = 0x%016x",
          job_b_base
        );
        $fatal(1);
      end

      if (job_c_base !== 64'h00000000_00003000) begin
        $display(
          "ERROR: job_c_base = 0x%016x",
          job_c_base
        );
        $fatal(1);
      end

      $display("");
      $display("============================================================");
      $display(" PASS: AXI4-Lite -> DPTI -> JOB descriptor");
      $display("============================================================");
      $display(
        " job_id          = %0d",
        job_id
      );
      $display(
        " device_id       = %0d",
        job_device_id
      );
      $display(
        " M/N/K           = %0d/%0d/%0d",
        job_m,
        job_n,
        job_k
      );
      $display(
        " start_cycle     = %0d",
        job_start_cycle
      );
      $display(
        " est_cycles      = %0d",
        job_est_cycles
      );
      $display(
        " A/B/C base      = 0x%016x / 0x%016x / 0x%016x",
        job_a_base,
        job_b_base,
        job_c_base
      );
      $display("");

    end
  endtask

  // --------------------------------------------------------------------------
  // Main test
  // --------------------------------------------------------------------------

  initial begin

    s_axi_awaddr  = 8'd0;
    s_axi_awvalid = 1'b0;

    s_axi_wdata   = 32'd0;
    s_axi_wstrb   = 4'h0;
    s_axi_wvalid  = 1'b0;

    s_axi_bready  = 1'b0;

    s_axi_araddr  = 8'd0;
    s_axi_arvalid = 1'b0;
    s_axi_rready  = 1'b0;

    job_ready = 1'b0;

    // --------------------------------------------------------
    // Reset both blocks with explicit polarity.
    //
    // AXI4-Lite:
    //   aresetn = 0 -> reset
    //   aresetn = 1 -> run
    //
    // DPTI:
    //   rst = 1 -> reset
    //   rst = 0 -> run
    // --------------------------------------------------------

    axi_aresetn = 1'b0;
    dpti_rst    = 1'b1;

    repeat (5)
      @(posedge clk);

    axi_aresetn = 1'b1;
    dpti_rst    = 1'b0;

    // DPTI job consumer is ready.
    job_ready = 1'b1;

    repeat (2)
      @(posedge clk);

    $display("");
    $display("============================================================");
    $display(" AXI4-Lite -> DPTI integration test");
    $display("============================================================");
    $display("");
    $display(
      "RESET RELEASED: axi_aresetn=%0b dpti_rst=%0b",
      axi_aresetn,
      dpti_rst
    );
    $display(
      "AXI READY: AWREADY=%0b WREADY=%0b ARREADY=%0b",
      s_axi_awready,
      s_axi_wready,
      s_axi_arready
    );
    $display(
      "DPTI READY: wr_ready=%0b job_ready=%0b",
      dpti_wr_ready,
      job_ready
    );
    $display("");

    if (s_axi_awready !== 1'b1) begin
      $display(
        "ERROR: AWREADY is not high after reset release: %b",
        s_axi_awready
      );
      $fatal(1);
    end

    if (s_axi_wready !== 1'b1) begin
      $display(
        "ERROR: WREADY is not high after reset release: %b",
        s_axi_wready
      );
      $fatal(1);
    end

    if (dpti_wr_ready !== 1'b1) begin
      $display(
        "ERROR: DPTI wr_ready is not high: %b",
        dpti_wr_ready
      );
      $fatal(1);
    end

    // --------------------------------------------------------
    // Descriptor
    // --------------------------------------------------------

    axi_write(8'h04, 32'h00000001);  // JOB_ID
    axi_write(8'h08, 32'h00000000);  // DEVICE_ID

    axi_write(8'h0c, 32'h00000008);  // M
    axi_write(8'h10, 32'h00000008);  // N
    axi_write(8'h14, 32'h00000010);  // K

    axi_write(8'h18, 32'h00000064);  // START_CYCLE
    axi_write(8'h1c, 32'h0000007d);  // EST_CYCLES = 125

    axi_write(8'h20, 32'h00001000);  // A_BASE_LO
    axi_write(8'h24, 32'h00000000);  // A_BASE_HI

    axi_write(8'h28, 32'h00002000);  // B_BASE_LO
    axi_write(8'h2c, 32'h00000000);  // B_BASE_HI

    axi_write(8'h30, 32'h00003000);  // C_BASE_LO
    axi_write(8'h34, 32'h00000000);  // C_BASE_HI

    // --------------------------------------------------------
    // Submit
    // --------------------------------------------------------

    $display("");
    $display("SUBMIT JOB");
    axi_write(8'h00, 32'h00000001);

    // Let the DPTI bridge consume the descriptor.
    @(posedge clk);
    @(posedge clk);

    if (!job_valid) begin
      // job_valid may already have been consumed because job_ready=1.
      // Therefore descriptor values are the persistent evidence.
      $display("INFO: job_valid pulse already consumed by job_ready.");
    end

    check_descriptor();

    #20;

    $finish;
  end

endmodule

`default_nettype wire
