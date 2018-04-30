// node execution unit
module neu #(parameter x=0, parameter y=0)
    (
        input   wire        clk,
        input   wire        rst,    // resets cost
        input   wire        clr,    // set the cost to 0
        input   wire        ld,     // load weight value/accessibility

        input   wire [3:0]  ld_weight,  // weight of 4'b1111 is inaccesible

        input   wire [11:0] n_cost,
        input   wire [11:0] ne_cost,
        input   wire [11:0] e_cost,
        input   wire [11:0] se_cost,
        input   wire [11:0] s_cost,
        input   wire [11:0] sw_cost,
        input   wire [11:0] w_cost,
        input   wire [11:0] nw_cost,

        output  wire        path_mod,    // high if a change has occurred
        output  wire [11:0] path_cost,
        output  wire [3:0]  path_dir
    );
    localparam  PERP = 2'b10;
    localparam  DIAG = 2'b11;

    reg [3:0]   weight;
    reg [11:0]  cost;
    reg [3:0]   dir;
    reg [2:0]   state;

    assign path_cost = cost;
    assign path_dir  = dir;

    wire accessible;
    assign accessible = (weight != 4'b1111);

    // combinational regs
    // compute new costs to travel to this node
    // based on the costs to travel to surrounding nodes
    reg [11:0]  adj_cost;
    reg [12:0]  travel_cost;
    reg [11:0]  new_cost;
    reg [3:0]   new_dir;
    reg         changed;

    // outside observer keeps track on how many changes are occuring
    // before determining that it is complete
    assign path_mod = changed && accessible;

    integer     i;
    always @(*) begin
        case (state)
        0: adj_cost = n_cost;
        1: adj_cost = ne_cost;
        2: adj_cost = e_cost;
        3: adj_cost = se_cost;
        4: adj_cost = s_cost;
        5: adj_cost = sw_cost;
        6: adj_cost = w_cost;
        7: adj_cost = nw_cost;
        endcase
        travel_cost = adj_cost + (weight << 1) + (state[0] ? DIAG : PERP);

        // defaults
        new_cost = cost;
        new_dir  = dir;
        changed  = 0;
        if (!travel_cost[12] && travel_cost[11:0] < cost) begin
            new_cost = travel_cost[11:0];
            new_dir  = {1'b1,state};
            changed = 1;
            /*
            $display("[%2d,%2d] travel: %0d.%0d, current: %0d.%0d", x, y,
                      travel_cost[11:1], travel_cost[0] ? 5 : 0,
                      cost[11:1], cost[0] ? 5 : 0);
            //$display("[%4d] travel: %d, current: %d", x + (y<<5), travel_cost, cost);
            $display("    n_cost: %0d.%0d",
                    n_cost[11:1], n_cost[0] ? 5 : 0);
            $display("    ne_cost: %0d.%0d",
                    ne_cost[11:1], ne_cost[0] ? 5 : 0);
            $display("    e_cost: %0d.%0d",
                    e_cost[11:1], e_cost[0] ? 5 : 0);
            $display("    se_cost: %0d.%0d",
                    se_cost[11:1], se_cost[0] ? 5 : 0);
            $display("    s_cost: %0d.%0d",
                    s_cost[11:1], s_cost[0] ? 5 : 0);
            $display("    sw_cost: %0d.%0d",
                    sw_cost[11:1], sw_cost[0] ? 5 : 0);
            $display("    w_cost: %0d.%0d",
                    w_cost[11:1], w_cost[0] ? 5 : 0);
            $display("    nw_cost: %0d.%0d",
                    nw_cost[11:1], nw_cost[0] ? 5 : 0);
            */
        end
        else changed = 0;
    end

    always @(posedge clk) begin
        // every clock cycle, update cost if node is accessible
        if (rst) begin
            cost <= 12'hFFF;
            dir <= 0;
            state <= 0;
        end
        if (clr) begin
            cost <= 0;
            dir  <= 0;
        end
        if (ld) begin
            weight <= ld_weight;
        end

        if (!(rst || clr || ld) && accessible) begin
            cost <= new_cost;
            dir  <= new_dir;
            state <= state + 1;
        end
    end

endmodule
