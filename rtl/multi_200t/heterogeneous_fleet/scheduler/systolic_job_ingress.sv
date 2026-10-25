`default_nettype none

module systolic_job_ingress (
  input  wire        clk,
  input  wire        rst_n,

  // Descriptor producer interface.
  input  wire        in_valid,
  output wire        in_ready,

  input  wire [31:0] in_job_id,
  input  wire [31:0] in_device_id,

  input  wire [31:0] in_m,
  input  wire [31:0] in_n,
  input  wire [31:0] in_k,

  input  wire [31:0] in_start_cycle,
  input  wire [31:0] in_est_cycles,

  input  wire [63:0] in_a_base,
  input  wire [63:0] in_b_base,
  input  wire [63:0] in_c_base,

  // Consumer interface.
  input  wire        consumer_ready,
  output wire        job_valid,

  output wire [31:0] job_id,
  output wire [31:0] job_device_id,

  output wire [31:0] job_m,
  output wire [31:0] job_n,
  output wire [31:0] job_k,

  output wire [31:0] job_start_cycle,
  output wire [31:0] job_est_cycles,

  output wire [63:0] job_a_base,
  output wire [63:0] job_b_base,
  output wire [63:0] job_c_base
);

  reg        valid_r;

  reg [31:0] job_id_r;
  reg [31:0] job_device_id_r;

  reg [31:0] job_m_r;
  reg [31:0] job_n_r;
  reg [31:0] job_k_r;

  reg [31:0] job_start_cycle_r;
  reg [31:0] job_est_cycles_r;

  reg [63:0] job_a_base_r;
  reg [63:0] job_b_base_r;
  reg [63:0] job_c_base_r;

  assign in_ready = ~valid_r | consumer_ready;

  assign job_valid = valid_r;

  assign job_id         = job_id_r;
  assign job_device_id  = job_device_id_r;

  assign job_m = job_m_r;
  assign job_n = job_n_r;
  assign job_k = job_k_r;

  assign job_start_cycle = job_start_cycle_r;
  assign job_est_cycles  = job_est_cycles_r;

  assign job_a_base = job_a_base_r;
  assign job_b_base = job_b_base_r;
  assign job_c_base = job_c_base_r;

  always @(posedge clk) begin
    if (!rst_n) begin
      valid_r <= 1'b0;

      job_id_r        <= 32'd0;
      job_device_id_r <= 32'd0;

      job_m_r <= 32'd0;
      job_n_r <= 32'd0;
      job_k_r <= 32'd0;

      job_start_cycle_r <= 32'd0;
      job_est_cycles_r  <= 32'd0;

      job_a_base_r <= 64'd0;
      job_b_base_r <= 64'd0;
      job_c_base_r <= 64'd0;
    end
    else begin
      // A new descriptor may replace an already-consumed descriptor in
      // the same cycle.  This keeps the interface one-entry elastic.
      if (in_valid && in_ready) begin
        valid_r <= 1'b1;

        job_id_r        <= in_job_id;
        job_device_id_r <= in_device_id;

        job_m_r <= in_m;
        job_n_r <= in_n;
        job_k_r <= in_k;

        job_start_cycle_r <= in_start_cycle;
        job_est_cycles_r  <= in_est_cycles;

        job_a_base_r <= in_a_base;
        job_b_base_r <= in_b_base;
        job_c_base_r <= in_c_base;
      end
      else if (valid_r && consumer_ready) begin
        valid_r <= 1'b0;
      end
    end
  end

endmodule

`default_nettype wire
