module test_fabric32();

    reg clk;
    reg arst_n;

    always #5 clk = ~clk;

    wire req_rd, req_wr, data_rdy;
    wire [31:0] addr_wr, addr_rd, data_wr, data_rd;
    mem memory(
            .clk(clk),
            .req_rd(req_rd),
            .req_wr(req_wr),
            .addr_wr(addr_wr),
            .addr_rd(addr_rd),
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

            .req_rd(req_rd),
            .req_wr(req_wr),
            .addr_wr(addr_wr),
            .addr_rd(addr_rd),
            .data_wr(data_wr),
            .data_rd(data_rd),
            .data_rdy(data_rdy),

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

        $finish();
    end

endmodule

module mem(
        input   wire    clk,
        input   wire    req_rd,
        input   wire    req_wr,
        input   wire [31:0] addr_wr,
        input   wire [31:0] addr_rd,
        input   wire [31:0] data_wr,
        output  reg  [31:0] data_rd,
        output  reg     data_rdy
    );

    reg [31:0] chip0[0:127];
    reg [31:0] chip1[0:127];

    reg [1:0] cs;
    reg [1:0] ns;

    initial begin
        $readmemh("mem.hex", chip0);
        cs <= 0;
    end

    reg o_sv;
    reg [31:0] rd_addr;
    reg [31:0] wr_addr;
    reg [31:0] wr_data;
    always @(*) begin
        data_rdy = 0;
        o_sv = 0;

        case (cs)
        0: begin
            data_rdy = 1;
            if (req_rd) begin
                ns = 1; // read
                o_sv = 1;
            end
            else if (req_wr) begin
                ns = 2; // write
                o_sv = 1;
            end
        end

        1: ns = 0;
        2: ns = 0;
        endcase
    end

    always @(clk) begin
        cs <= ns;

        if (o_sv) begin
            rd_addr <= ((addr_rd - 32'h4000000) >> 2);
            wr_addr <= ((addr_wr - 32'h4002000) >> 2);
            wr_data <= data_wr;
        end

        case (cs)
        1: begin
            data_rd <= chip0[rd_addr];
        end
        2: begin
            chip1[rd_addr] <= wr_data;
        end
        endcase
    end

endmodule
