`timescale 1ns/1ps
// Real context, banks, packer and FIFO; synthetic array publication and DMA
// source isolate writeback ownership from floating-point compute (covered by
// tb_pipeline_overlap). Every tile word and completion identity is checked.
module tb_shared_wb_context #(parameter integer N = 8);
  logic clk=0, rst_n=0;
  always #5 clk=~clk;
  localparam integer JOBS=300, BEATS=N*N/4;
  logic fire=0, cv=0, done=0;
  logic [31:0] id=0;
  logic [31:0] c [0:N-1][0:N-1];
  wire accept, ready, req, op_valid, full, wvalid, valid, job_done;
  wire [15:0] op_beats, beats;
  wire [28:0] addr;
  wire [127:0] data;
  wire [31:0] complete_id;
  wire [63:0] complete_addr;
  logic op_ready=0, dst_wr=0, read_done=0, desc_ready=0, src_ready=0;
  logic [15:0] dst_beat=0;
  wire fold = ready && (dut.n_req != dut.n_fed);
  systolic_accel_context #(.N_ARRAY(N), .N_WIRE(8), .K_MAX(16),
    .K_DIM(16), .USE_V2(1), .USE_EXTERNAL_SCHEDULER(1)) dut (
    .clk(clk), .ui_rst_n(rst_n), .init_calib_complete(1'b1),
    .n_inv_probe(4'd1), .rerun_pulse(1'b0),
    .job_fire(fire), .in_job_id(id), .in_device_id(32'd0), .in_k(32'd16),
    .in_start_cycle(32'd0), .in_est_cycles(32'd0),
    .in_a_base(64'd0), .in_b_base(64'd0), .in_c_base(64'(4096+id*256)),
    .job_accept_ready(accept), .job_done(job_done), .job_id_reg(complete_id),
    .job_c_base_reg(complete_addr), .tile_req(req), .tile_ack(req),
    .accelerator_ready(ready), .fold_start_ext(fold), .c_valid_out(cv), .c_out(c),
    .op_desc_valid(op_valid), .op_desc_beats(op_beats), .op_desc_ready(op_ready),
    .dst_wr_en(dst_wr), .dst_wr_beat(dst_beat), .dst_wr_data(128'd0),
    .dst_full(full), .read_done(read_done),
    .wb_desc_valid(valid), .wb_desc_addr(addr), .wb_desc_beats(beats),
    .wb_desc_ready(desc_ready), .wb_done(done),
    .src_valid(wvalid), .src_data(data), .src_ready(src_ready), .seed_done(1'b0)
  );
  integer cycle=0, accepted=0, published=0, issued=0, completed=0, words=0;
  integer fill_left=0, fill_index=0, publish_delay=0;
  integer active=-1, sent=0, delay_b=0;
  integer meta_full=0, fifo_full=0, empty_seen=0, early_reuse=0, simultaneous=0;
  integer stalled=0, meta_empty=0, finished_cycles=0;
  function automatic [31:0] pattern(input integer job, input integer word_index);
    return 32'h51000000 + job*128 + word_index;
  endfunction
  // Drive on falling edges; all counts below describe following rising edges.
  always @(negedge clk) if (rst_n) begin
    cycle++;
    fire=0; op_ready=0; dst_wr=0; read_done=0; cv=0;
    desc_ready=0; src_ready=0; done=0;
    if (accepted<JOBS && accept) begin id=accepted; fire=1; accepted++; end
    if (fill_left==0 && op_valid) begin
      op_ready=1; fill_left=op_beats; fill_index=0;
    end else if (fill_left>0 && !full) begin
      dst_wr=1; dst_beat=16'(fill_index); fill_index++; fill_left--;
      if (fill_left==0) read_done=1;
    end
    if (publish_delay>0) begin
      publish_delay--;
      if (publish_delay==0) begin
        cv=1;
        for (int r=0;r<N;r++) for (int col=0;col<N;col++)
          c[r][col]=pattern(published,r*N+col);
        published++;
      end
    end
    // Initially hold service long enough for both complete tiles to queue.
    if (cycle>1600 && active<0 && valid) begin
      if (addr !== 29'(4096+issued*256) || beats !== 16'(BEATS))
        $fatal(1,"descriptor mismatch job %0d addr %h beats %0d",issued,addr,beats);
      active=issued; issued++; sent=0; desc_ready=1;
    end else if (active>=0) begin
      if (sent<BEATS) begin
        src_ready=(cycle%5!=0);
        if (wvalid && !src_ready) stalled++;
        if (wvalid && src_ready) begin
          for (int lane=0;lane<4;lane++) begin
            if (data[32*lane+:32] !== pattern(active,sent*4+lane))
              $fatal(1,"data mismatch job %0d word %0d",active,sent*4+lane);
            words++;
          end
          sent++;
          if (sent==BEATS) delay_b=400;
        end
      end else begin
        delay_b--;
        // Deliberately retire on a second tile's commit to cover push/pop.
        if (delay_b<=0 || (dut.rdr_done && dut.wb_meta_count==1)) begin
          done=1;
        end
      end
    end
    if (dut.wb_meta_count==2) meta_full++;
    if (dut.wb_meta_count==0) meta_empty++;
    if (dut.rdr_wfull) fifo_full++;
    if (dut.fifo_rempty) empty_seen++;
    if (dut.enqueue_commit && done) simultaneous++;
    if (cycle>1000000) $fatal(1,"timeout completed=%0d",completed);
  end
  always @(posedge clk) if (rst_n) begin
    if (fold) begin
      if (publish_delay!=0) $fatal(1,"synthetic array overlap");
      publish_delay=50;
      if (int'(dut.n_fed)-completed>=2) early_reuse++;
    end
    if (job_done) begin
      if (!done || complete_id !== 32'(completed) ||
          complete_addr !== 64'(4096+completed*256))
        $fatal(1,"completion identity mismatch job %0d",completed);
      completed++; active=-1;
    end
    if (completed==JOBS) finished_cycles++;
    if (finished_cycles==5) begin
      if (dut.wb_meta_count!=0 || !dut.fifo_rempty ||
          dut.n_wb !== 8'(JOBS) || dut.n_enqueued !== 8'(JOBS))
        $fatal(1,"final queue/counter state did not drain");
      if (issued!=JOBS || published!=JOBS || words!=JOBS*N*N ||
          meta_full==0 || meta_empty==0 || empty_seen==0 || early_reuse==0 || simultaneous==0 ||
          stalled==0 || (N==8 && fifo_full==0))
        $fatal(1,"coverage missing issued=%0d published=%0d words=%0d meta_full=%0d fifo_full=%0d empty=%0d reuse=%0d simultaneous=%0d stalls=%0d",
          issued,published,words,meta_full,fifo_full,empty_seen,early_reuse,simultaneous,stalled);
      $display("PASS context N=%0d jobs=%0d words=%0d mismatches=0 meta_full=%0d fifo_full=%0d empty=%0d early_reuse=%0d simultaneous=%0d stalls=%0d meta_empty=%0d",
        N,completed,words,meta_full,fifo_full,empty_seen,early_reuse,simultaneous,stalled,meta_empty);
      $finish;
    end
  end
  initial begin
    for (int r=0;r<N;r++) for (int col=0;col<N;col++) c[r][col]=0;
    repeat(5) @(negedge clk);
    rst_n=1;
  end
endmodule
