// node execution unit
module neu(
        input   wire        clk,
        input   wire        rst,    // resets cost
        input   wire        clr,    // set the cost to 0
        input   wire        ld,     // load weight value/accessibility

        input   wire [3:0]  ld_weight,  // weight of 4'b1111 is inaccesible

        input   wire [15:0] n_cost,
        input   wire [15:0] ne_cost,
        input   wire [15:0] e_cost,
        input   wire [15:0] se_cost,
        input   wire [15:0] s_cost,
        input   wire [15:0] sw_cost,
        input   wire [15:0] w_cost,
        input   wire [15:0] nw_cost,

        output  wire        path_mod,    // high if a change has occurred
        output  wire [15:0] path_cost,
        output  wire [2:0]  path_dir
    );
    localparam  PERP = 2'b10;
    localparam  DIAG = 2'b11;

    reg [3:0]   weight;
    reg [15:0]  cost;
    reg [2:0]   dir;

    assign path_cost = cost;
    assign path_dir  = dir;

    wire accessible;
    assign accessible = (weight != 4'b1111);

    // combinational regs
    // compute new costs to travel to this node
    // based on the costs to travel to surrounding nodes
    reg [15:0]  adj_costs[0:7];
    reg [15:0]  new_cost;
    reg [3:0]   new_dir;
    reg         changed; assign path_mod  = changed;
    integer     i;
    always @(*) begin
        adj_costs[0] = n_cost  + (weight << 1) + PERP;
        adj_costs[1] = ne_cost + (weight << 1) + DIAG;
        adj_costs[2] = e_cost  + (weight << 1) + PERP;
        adj_costs[3] = se_cost + (weight << 1) + DIAG;
        adj_costs[4] = s_cost  + (weight << 1) + PERP;
        adj_costs[5] = sw_cost + (weight << 1) + DIAG;
        adj_costs[6] = w_cost  + (weight << 1) + PERP;
        adj_costs[7] = nw_cost + (weight << 1) + DIAG;

        // defaults
        new_cost = cost;
        new_dir  = dir;
        changed = 0;

        for (i = 0; i < 7; i = i + 1) begin
            if (adj_costs[i] < cost) begin
                new_cost = adj_costs[i];
                new_dir  = i;
                changed = 1;
            end
        end
    end

    always @(posedge clk) begin
        // every clock cycle, update cost if node is accessible
        if (accessible) begin
            cost <= new_cost;
            dir  <= new_dir;
        end

        if (rst) begin
            cost <= 16'hFFFF;
            dir <= 0;
        end
        if (clr) begin
            cost <= 0;
            dir  <= 0;
        end
        if (ld) begin
            weight <= ld_weight;
        end
    end

endmodule
