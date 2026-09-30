`timescale 1ns/1ps

module tb_systolic_hw_scheduler_integration;

    localparam integer NUM_ACCEL = 1;
    localparam integer CYCLE_W   = 8;
    localparam integer ACC_W     = 1;

    logic clk;
    logic rst;

    logic                     desc_valid;
    wire                      desc_ready;

    logic [ACC_W-1:0]         desc_accelerator_id;
    logic [CYCLE_W-1:0]       desc_start_cycle;
    logic [CYCLE_W-1:0]       desc_compute_cycles;

    wire [NUM_ACCEL-1:0]      accelerator_start;
    wire [NUM_ACCEL-1:0]      accelerator_done;

    wire                      busy;
    wire                      schedule_done;

    wire [CYCLE_W-1:0]        cycle_counter;
    wire [CYCLE_W-1:0]        active_start_cycle;
    wire [CYCLE_W-1:0]        active_compute_cycles;
    wire [ACC_W-1:0]          active_accelerator_id;

    wire                      fold_start;
    logic                     c_done;

    integer errors;

    always #5 clk = ~clk;

    // ------------------------------------------------------------------------
    // Scheduler
    // ------------------------------------------------------------------------

    systolic_hw_scheduler #(
        .NUM_ACCEL(NUM_ACCEL),
        .CYCLE_W(CYCLE_W),
        .ACC_W(ACC_W)
    ) scheduler (
        .clk                    (clk),
        .rst                    (rst),

        .desc_valid             (desc_valid),
        .desc_ready             (desc_ready),

        .desc_accelerator_id    (desc_accelerator_id),
        .desc_start_cycle       (desc_start_cycle),
        .desc_compute_cycles    (desc_compute_cycles),

        .accelerator_start      (accelerator_start),
        .accelerator_done       (accelerator_done),

        .busy                   (busy),
        .schedule_done          (schedule_done),

        .cycle_counter          (cycle_counter),

        .active_start_cycle     (active_start_cycle),
        .active_compute_cycles  (active_compute_cycles),
        .active_accelerator_id  (active_accelerator_id)
    );

    // ------------------------------------------------------------------------
    // Scheduler <-> accelerator bridge
    // ------------------------------------------------------------------------

    systolic_hw_scheduler_adapter adapter (
        .clk                (clk),
        .rst                (rst),

        .accelerator_start  (accelerator_start[0]),
        .accelerator_done   (accelerator_done[0]),

        .fold_start         (fold_start),
        .c_done             (c_done)
    );

    // ------------------------------------------------------------------------
    // Fake accelerator.
    //
    // This represents the existing systolic array boundary:
    //
    //     fold_start -> accelerator executes -> c_done
    //
    // The real FPGA array will replace only this block.
    // ------------------------------------------------------------------------

    integer compute_count;

    always @(posedge clk) begin
        if (rst) begin
            compute_count <= 0;
            c_done        <= 1'b0;
        end
        else begin
            c_done <= 1'b0;

            if (fold_start) begin
                compute_count <= 4;
            end
            else if (compute_count > 0) begin
                compute_count <= compute_count - 1;

                if (compute_count == 1)
                    c_done <= 1'b1;
            end
        end
    end

    // ------------------------------------------------------------------------
    // Test
    // ------------------------------------------------------------------------

    initial begin
        errors = 0;

        clk = 0;
        rst = 1;

        desc_valid          = 0;
        desc_accelerator_id = 0;
        desc_start_cycle    = 3;
        desc_compute_cycles = 4;

        repeat (3) @(posedge clk);
        rst = 0;

        // Submit one compiler-generated descriptor.
        @(negedge clk);

        desc_valid = 1'b1;

        while (!desc_ready)
            @(negedge clk);

        @(posedge clk);

        @(negedge clk);
        desc_valid = 1'b0;

        // Scheduler must eventually generate accelerator_start.
        //
        // accelerator_start is a one-cycle registered pulse.
        // The adapter samples that pulse on the following rising edge
        // and generates fold_start.
        wait (accelerator_start[0]);

        $display("PASS: scheduler -> accelerator_start");

        // accelerator_start has just become visible after a rising edge.
        // The adapter cannot consume it until the NEXT rising edge.
        @(posedge clk);
        @(negedge clk);

        if (!fold_start) begin
            $display("FAIL: accelerator_start was not converted to fold_start");
            errors = errors + 1;
        end
        else begin
            $display("PASS: accelerator_start -> fold_start");
        end

        // fold_start must disappear at the next rising edge.
        @(posedge clk);
        @(negedge clk);

        if (fold_start) begin
            $display("FAIL: fold_start remained high for more than one cycle");
            errors = errors + 1;
        end
        else begin
            $display("PASS: fold_start is a one-cycle pulse");
        end

        // Wait for fake accelerator completion.
        wait (c_done);

        $display("PASS: accelerator produced c_done");

        // Adapter must return done to scheduler.
        wait (accelerator_done[0]);

        $display("PASS: c_done -> accelerator_done");

        // Scheduler must finish the transaction.
        wait (schedule_done);

        $display("PASS: accelerator_done -> schedule_done");

        $display("");

        if (errors == 0)
            $display("PASS: scheduler/accelerator integration");
        else
            $display("FAIL: scheduler/accelerator integration errors=%0d",
                     errors);

        $finish;
    end

endmodule
