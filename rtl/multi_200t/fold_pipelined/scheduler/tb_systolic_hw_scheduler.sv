`timescale 1ns/1ps

module tb_systolic_hw_scheduler;

    localparam integer NUM_ACCEL = 2;
    localparam integer CYCLE_W   = 8;
    localparam integer ACC_W     = (NUM_ACCEL <= 1) ? 1 : $clog2(NUM_ACCEL);

    logic clk;
    logic rst;

    logic                     desc_valid;
    wire                      desc_ready;

    logic [ACC_W-1:0]         desc_accelerator_id;
    logic [CYCLE_W-1:0]       desc_start_cycle;
    logic [CYCLE_W-1:0]       desc_compute_cycles;

    wire [NUM_ACCEL-1:0]      accelerator_start;
    logic [NUM_ACCEL-1:0]     accelerator_done;

    wire                      busy;
    wire                      schedule_done;

    wire [CYCLE_W-1:0]        cycle_counter;
    wire [CYCLE_W-1:0]        active_start_cycle;
    wire [CYCLE_W-1:0]        active_compute_cycles;
    wire [ACC_W-1:0]          active_accelerator_id;

    integer errors;
    integer starts_seen;
    integer done_seen;

    // ------------------------------------------------------------------------
    // Clock
    // ------------------------------------------------------------------------

    initial clk = 1'b0;
    always #5 clk = ~clk;

    // ------------------------------------------------------------------------
    // DUT
    // ------------------------------------------------------------------------

    systolic_hw_scheduler #(
        .NUM_ACCEL(NUM_ACCEL),
        .CYCLE_W(CYCLE_W),
        .ACC_W(ACC_W)
    ) dut (
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
    // Helpers
    // ------------------------------------------------------------------------

    task automatic reset_dut;
        begin
            rst = 1'b1;

            desc_valid          = 1'b0;
            desc_accelerator_id = '0;
            desc_start_cycle    = '0;
            desc_compute_cycles = '0;

            accelerator_done    = '0;

            repeat (3) @(posedge clk);

            rst = 1'b0;

            @(posedge clk);
        end
    endtask

    // Submit exactly one descriptor.
    //
    // The important rule:
    //   descriptor is held until desc_valid && desc_ready.
    //
    // The TB never assumes when the scheduler will accept it.
    task automatic submit_descriptor(
        input integer acc,
        input integer start_cycle,
        input integer compute_cycles
    );
        begin
            @(negedge clk);

            desc_accelerator_id = acc;
            desc_start_cycle    = start_cycle;
            desc_compute_cycles = compute_cycles;
            desc_valid          = 1'b1;

            while (!desc_ready)
                @(negedge clk);

            @(posedge clk);

            @(negedge clk);
            desc_valid = 1'b0;
        end
    endtask

    // Wait for one accelerator start pulse.
    //
    // Return the accelerator selected by the scheduler.
    task automatic wait_for_start(
        output integer started_acc
    );
        integer guard;
        begin
            started_acc = -1;
            guard = 0;

            while (started_acc < 0 && guard < 1000) begin
                @(posedge clk);

                if (accelerator_start[0])
                    started_acc = 0;

                if (accelerator_start[1])
                    started_acc = 1;

                guard = guard + 1;
            end

            if (started_acc < 0) begin
                $display("FAIL: timeout waiting for accelerator_start");
                errors = errors + 1;
            end
        end
    endtask

    // Complete the currently running accelerator.
    task automatic complete_accelerator(
        input integer acc
    );
        begin
            @(negedge clk);
            accelerator_done = '0;
            accelerator_done[acc] = 1'b1;

            @(posedge clk);

            @(negedge clk);
            accelerator_done = '0;
        end
    endtask

    // Wait for scheduler completion pulse.
    task automatic wait_for_schedule_done;
        integer guard;
        begin
            guard = 0;

            while (!schedule_done && guard < 100) begin
                @(posedge clk);
                guard = guard + 1;
            end

            if (!schedule_done) begin
                $display("FAIL: timeout waiting for schedule_done");
                errors = errors + 1;
            end
        end
    endtask

    // ------------------------------------------------------------------------
    // Continuous protocol assertions
    // ------------------------------------------------------------------------

    always @(posedge clk) begin
        if (!rst) begin

            // Start must be one-hot-or-zero.
            if ((accelerator_start & (accelerator_start - 1'b1)) != 0) begin
                $display("FAIL: accelerator_start is not one-hot: %b",
                         accelerator_start);
                errors = errors + 1;
            end

            // Scheduler must not assert start while idle without a transaction.
            if (!busy && (accelerator_start != '0)) begin
                $display("FAIL: accelerator_start asserted while scheduler idle");
                errors = errors + 1;
            end

            // If busy, an active accelerator must be valid.
            if (busy && (active_accelerator_id >= NUM_ACCEL)) begin
                $display("FAIL: invalid active accelerator id %0d",
                         active_accelerator_id);
                errors = errors + 1;
            end
        end
    end

    // ------------------------------------------------------------------------
    // Test 1:
    //
    // Descriptor starts in the future.
    //
    // Verify:
    //   1. descriptor is accepted;
    //   2. correct accelerator starts;
    //   3. wrong accelerator does not start;
    //   4. completion causes schedule_done.
    //
    // We intentionally do NOT hard-code the internal cycle-counter observation.
    // The scheduler's externally visible contract is what matters.
    // ------------------------------------------------------------------------

    task automatic test_future_start;
        integer started_acc;
        integer start_cycle_observed;
        begin
            $display("");
            $display("TEST 1: future scheduled start");

            reset_dut();

            submit_descriptor(
                0,      // accelerator
                5,      // requested start cycle
                10      // analytical compute duration
            );

            if (!busy) begin
                $display("FAIL: scheduler is not busy after descriptor acceptance");
                errors = errors + 1;
            end

            if (active_accelerator_id !== 0) begin
                $display("FAIL: active accelerator = %0d, expected 0",
                         active_accelerator_id);
                errors = errors + 1;
            end

            if (active_start_cycle !== 5) begin
                $display("FAIL: active_start_cycle = %0d, expected 5",
                         active_start_cycle);
                errors = errors + 1;
            end

            if (active_compute_cycles !== 10) begin
                $display("FAIL: active_compute_cycles = %0d, expected 10",
                         active_compute_cycles);
                errors = errors + 1;
            end

            // Observe the registered start pulse after NBA updates have
            // settled.  At negedge, both cycle_counter and
            // accelerator_start reflect the state produced by the
            // preceding posedge.
            start_cycle_observed = -1;

            while (start_cycle_observed < 0) begin
                @(negedge clk);

                if (accelerator_start[0]) begin
                    start_cycle_observed = cycle_counter;
                    started_acc = 0;
                end
            end

            if (start_cycle_observed != 5) begin
                $display("FAIL: accelerator_start at cycle %0d, expected 5",
                         start_cycle_observed);
                errors = errors + 1;
            end
            else begin
                $display("PASS: accelerator_start at scheduled cycle 5");
            end

            if (started_acc !== 0) begin
                $display("FAIL: started accelerator = %0d, expected 0",
                         started_acc);
                errors = errors + 1;
            end

            if (accelerator_start !== 2'b01) begin
                $display("FAIL: start vector = %b, expected 01",
                         accelerator_start);
                errors = errors + 1;
            end

            complete_accelerator(0);
            wait_for_schedule_done();

            if (busy) begin
                $display("FAIL: scheduler still busy after completion");
                errors = errors + 1;
            end

            $display("PASS: TEST 1");
        end
    endtask

    // ------------------------------------------------------------------------
    // Test 2:
    //
    // Start cycle is already in the past when the descriptor is accepted.
    //
    // This checks the "late descriptor" behavior:
    // the scheduler must start immediately rather than deadlock waiting
    // for a cycle that has already passed.
    // ------------------------------------------------------------------------

    task automatic test_late_descriptor;
        integer started_acc;
        begin
            $display("");
            $display("TEST 2: late descriptor");

            reset_dut();

            // Give the global counter time to advance.
            repeat (8) @(posedge clk);

            submit_descriptor(
                1,      // accelerator
                0,      // already in the past
                7
            );

            wait_for_start(started_acc);

            if (started_acc !== 1) begin
                $display("FAIL: started accelerator = %0d, expected 1",
                         started_acc);
                errors = errors + 1;
            end

            complete_accelerator(1);
            wait_for_schedule_done();

            if (busy) begin
                $display("FAIL: scheduler still busy after late descriptor");
                errors = errors + 1;
            end

            $display("PASS: TEST 2");
        end
    endtask

    // ------------------------------------------------------------------------
    // Test 3:
    //
    // Descriptor must remain pending until desc_ready.
    //
    // This verifies the actual producer/consumer protocol.
    // ------------------------------------------------------------------------

    task automatic test_descriptor_handshake;
        integer started_acc;
        begin
            $display("");
            $display("TEST 3: descriptor handshake");

            reset_dut();

            // First transaction occupies scheduler.
            submit_descriptor(
                0,
                3,
                4
            );

            wait_for_start(started_acc);

            if (started_acc !== 0) begin
                $display("FAIL: first transaction started on %0d",
                         started_acc);
                errors = errors + 1;
            end

            // While busy, desc_ready must be low.
            if (desc_ready) begin
                $display("FAIL: desc_ready asserted while scheduler busy");
                errors = errors + 1;
            end

            complete_accelerator(0);
            wait_for_schedule_done();

            // Now scheduler must become available again.
            @(posedge clk);

            if (!desc_ready) begin
                $display("FAIL: desc_ready did not return after completion");
                errors = errors + 1;
            end

            // Second transaction.
            submit_descriptor(
                1,
                cycle_counter + 3,
                6
            );

            wait_for_start(started_acc);

            if (started_acc !== 1) begin
                $display("FAIL: second transaction started on %0d",
                         started_acc);
                errors = errors + 1;
            end

            complete_accelerator(1);
            wait_for_schedule_done();

            $display("PASS: TEST 3");
        end
    endtask

    // ------------------------------------------------------------------------
    // Main
    // ------------------------------------------------------------------------

    initial begin
        errors = 0;
        starts_seen = 0;
        done_seen = 0;

        test_future_start();
        test_late_descriptor();
        test_descriptor_handshake();

        $display("");
        $display("==============================================");

        if (errors == 0) begin
            $display("PASS: systolic_hw_scheduler");
            $display("  all scheduler protocol tests passed");
        end
        else begin
            $display("FAIL: systolic_hw_scheduler");
            $display("  errors = %0d", errors);
        end

        $display("==============================================");
        $finish;
    end

endmodule
