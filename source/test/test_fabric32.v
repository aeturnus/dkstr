module test_fabric32();

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


    reg ctrl_wr;
    reg [31:0] ctrl_in;
    wire [31:0] ctrl_out;
    wire int_done;
    fabric32 fabric(
            .clk(clk),
            .arst_n(arst_n),
            .ctrl_wr(ctrl_wr),
            .ctrl_in(ctrl_in),
            .ctrl_out(ctrl_out),

            .txn_req(req),
            .txn_wr(wr),
            .txn_addr(addr),
            .txn_wdata(data_wr),
            .txn_rdata(data_rd),
            .txn_rdy(data_rdy),

            .int_done(int_done)
        );

    integer i;
    always begin
        $dumpfile("wave_fabric32.vcd");
        $dumpvars(0, test_fabric32);
        clk = 0;
        arst_n = 1;
        ctrl_wr = 0;
        ctrl_in = 0;
        #10;

        ctrl_wr = 1;
        ctrl_in = {1'd1, 1'd1, 20'd0, 5'd0, 5'd0};
        #10;
        ctrl_wr = 0;

        //#100;
        #20000;
        // should have populated already

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
        $readmemh("mem.hex", chip0);
        cs = 0;
    end
    reg [31:0] rd_addr;
    reg [31:0] wr_addr;
    reg [31:0] wr_data;

    always @(posedge clk) begin
        case (cs)
        0: begin
            data_rdy <= 1;
            if (req && !wr) begin
                rd_addr <= ((addr - 32'h40000000) >> 2);
                cs <= 1;
                data_rdy <= 0;
            end
            else if (req && wr) begin
                wr_addr <= ((addr - 32'h40002000) >> 2);
                wr_data <= data_wr;
                cs <= 2;
                data_rdy <= 0;
            end
        end
        1: begin
            data_rd <= chip0[rd_addr];
            cs <= 0;
            data_rdy <= 1;
        end
        2: begin
            chip1[rd_addr] <= wr_data;
            cs <= 0;
            data_rdy <= 1;
        end
        endcase
    end

endmodule
