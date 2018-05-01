`timescale 1ns/1ns

module test_fabric();

    reg clk;
    reg arst_n;

    always #5 clk = ~clk;

    wire req, wr, data_rdy;
    wire [31:0] addr, data_wr, data_rd;
    mem memory(
            .clk(clk),
            .req(req),
            .wr(wr),
            .addr(addr),
            .data_wr(data_wr),
            .data_rd(data_rd),
            .data_rdy(data_rdy)
        );

    //localparam DIM = 32;
    localparam DIM = 28;
    localparam N   = DIM * DIM;
    localparam COST_SIZE = 9;

    reg ctrl_wr;
    reg [31:0] ctrl_in;
    wire [31:0] ctrl_out;
    wire int_done;
    wire [31:0] cnt_ld, cnt_run, cnt_st;
    fabric #(.DIM(DIM), .COST_SIZE(COST_SIZE)) fabric(
            .clk(clk),
            .arst_n(arst_n),
            .ctrl_wr(ctrl_wr),
            .ctrl_in(ctrl_in),
            .ctrl_out(ctrl_out),

            .cnt_ld_cycles(cnt_ld),
            .cnt_run_cycles(cnt_run),
            .cnt_st_cycles(cnt_st),

            .txn_req(req),
            .txn_wr(wr),
            .txn_addr(addr),
            .txn_wdata(data_wr),
            .txn_rdata(data_rd),
            .txn_rdy(data_rdy),

            .int_done(int_done)
        );

    integer i,j,f,r,c;
    always begin
        $dumpfile("wave_fabric.vcd");
        $dumpvars(0, test_fabric);
        clk = 0;
        arst_n = 1;
        ctrl_wr = 0;
        ctrl_in = 0;
        #10;

        ctrl_wr = 1;
        //ctrl_in = {1'd1, 1'd1, 20'd0, 5'd0, 5'd0};
        ctrl_in = {1'd1, 1'd1, 20'd0, 5'd1, 5'd1};
        #10;
        ctrl_wr = 0;

        // run until completion
        //#30000;
        @(posedge int_done);
        #10000;

        ///*

        ctrl_in = {1'd1, 1'd0, 20'd0, 5'd1, 5'd1};
        ctrl_wr = 1;
        #10;
        ctrl_wr = 0;

        @(posedge int_done);
        #10000;
        //*/


        f = $fopen("paths.hex","w");
        $fdisplay(f, "%08x", fabric.reg_start_x);
        $fdisplay(f, "%08x", fabric.reg_start_y);
        for (i = 0; i < N; i = i + 1) begin
            $fdisplay(f, "%08x", memory.chip1[i]);
        end

        for (r = 0; r < DIM; r = r + 1) begin
            for (c = 0; c < DIM; c = c + 1) begin
                $display("NEU[%2d,%2d]: %0d.%0d", c, r,
                (fabric.cost[c + r*DIM] >> 1), (fabric.cost[c + r*DIM][0] ? 5 : 0));
            end
        end

        $finish();
    end

endmodule

module mem(
        input   wire    clk,
        input   wire    req,
        input   wire    wr,
        input   wire [31:0] addr,
        input   wire [31:0] data_wr,
        output  reg  [31:0] data_rd,
        output  reg         data_rdy
    );

    reg [31:0] chip0[0:127];
    reg [31:0] chip1[0:127];

    reg [1:0] cs;
    reg [1:0] ns;

    initial begin
        $readmemh("test28.hex", chip0);
        cs = 0;
    end
    reg [31:0] rd_addr;
    reg [31:0] wr_addr;
    reg [31:0] wr_data;
    reg [4:0] cnt;

    localparam CNT = 16;
    always @(posedge clk) begin
        case (cs)
        0: begin
            data_rdy <= 1;
            if (req && !wr) begin
                rd_addr <= ((addr - 32'h40000000) >> 2);
                cs <= 1;
                data_rdy <= 0;
                cnt <= CNT;
            end
            else if (req && wr) begin
                wr_addr <= ((addr - 32'h40001000) >> 2);
                wr_data <= data_wr;
                cs <= 2;
                data_rdy <= 0;
                cnt <= CNT;
            end
        end
        1: begin
            if (cnt != 0) begin
                cs <= 1;
                data_rdy <= 0;
                cnt <= cnt - 1;
            end
            else begin
                data_rd <= chip0[rd_addr];
                cs <= 0;
                data_rdy <= 1;
            end
        end
        2: begin
            if (cnt != 0) begin
                cs <= 2;
                data_rdy <= 0;
                cnt <= cnt - 1;
            end
            else begin
                chip1[wr_addr] <= wr_data;
                cs <= 0;
                data_rdy <= 1;
            end
        end
        endcase
    end

endmodule
