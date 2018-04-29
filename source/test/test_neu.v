module test_neu();

    reg clk;
    reg rst;
    reg clr;
    reg ld;
    reg [3:0] ld_weight;

    always #5 clk = ~clk;

    reg [11:0] cost[0:7];

    wire path_mod;
    wire [11:0] path_cost;
    wire [2:0]  path_dir;

    neu neu0(
            .clk(clk),
            .rst(rst),
            .clr(clr),
            .ld(ld),
            .ld_weight(ld_weight),
            .n_cost(cost[0]),
            .ne_cost(cost[1]),
            .e_cost(cost[2]),
            .se_cost(cost[3]),
            .s_cost(cost[4]),
            .sw_cost(cost[5]),
            .w_cost(cost[6]),
            .nw_cost(cost[7]),

            .path_mod(path_mod),
            .path_cost(path_cost),
            .path_dir(path_dir)
        );

    integer i;
    always begin
        $dumpfile("wave_neu.vcd");
        $dumpvars(0, test_neu);

        clk = 0;
        rst = 0;
        clr = 0;
        ld  = 0;
        ld_weight = 5;

        for (i = 0; i < 8; i = i + 1) begin
            cost[i] = 12'hfff;
        end

        rst = 1;
        #10;

        $display("Asserted rst: expected cost = 0xfff, actual = 0x%03x", neu0.cost);
        rst = 0;

        ld = 1;
        #10;
        $display("Asserted ld: expected weight = 5, actual = %0d", neu0.weight);
        ld = 0;

        clr = 1;
        #10;
        $display("Asserted clr: expected cost = 0, actual = %0d", neu0.cost);
        clr = 0;

        rst = 1;
        #10;
        $display("Asserted rst: expected cost = 0xfff, actual = 0x%03x", neu0.cost);
        rst = 0;

        // testing changes

        cost[0] = 20 << 1 | 0;
        #10;
        $display("Made north cost 20.0: expected cost = 26.0, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);

        cost[1] = 19 << 1 | 0;
        #10;
        $display("Made northeast cost 19.0: expected cost = 25.5, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);

        cost[2] = 18 << 1 | 0;
        #10;
        $display("Made east cost 18.0: expected cost = 24.0, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);

        cost[3] = 17 << 1 | 0;
        #10;
        $display("Made southeast cost 17.0: expected cost = 23.5, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);

        cost[4] = 16 << 1 | 0;
        #10;
        $display("Made south cost 16.0: expected cost = 22.0, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);

        cost[5] = 15 << 1 | 0;
        #10;
        $display("Made southwest cost 15.0: expected cost = 21.5, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);

        cost[6] = 14 << 1 | 0;
        #10;
        $display("Made west cost 14.0: expected cost = 20.0, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);

        cost[7] = 13 << 1 | 0;
        #10;
        $display("Made northwest cost 13.0: expected cost = 19.5, actual = %0d.%1d",
                 neu0.cost >> 1, neu0.cost[0] ?5:0);


        cost[4] = 10 << 1 | 0;
        $display("Made west cost 10.0");

        for (i = 0; i < 8; i = i + 1) begin
            #10
            $display("cost = %0d.%1d, changed = %0d",
                     neu0.cost >> 1, neu0.cost[0] ?5:0, path_mod);

        end


        $finish();
    end

endmodule
