// node execution unit
module neu(
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
        output  wire [2:0]  path_dir
    );
    localparam  PERP = 2'b10;
    localparam  DIAG = 2'b11;

    reg [3:0]   weight;
    reg [11:0]  cost;
    reg [2:0]   dir;
    reg [2:0]   state;

    assign path_cost = cost;
    assign path_dir  = dir;

    wire accessible;
    assign accessible = (weight != 4'b1111);

    // combinational regs
    // compute new costs to travel to this node
    // based on the costs to travel to surrounding nodes
    reg [11:0]  adj_cost;
    reg [11:0]  travel_cost;
    reg [11:0]  new_cost;
    reg [3:0]   new_dir;
    reg         changed;

    // outside observer keeps track on how many changes are occuring
    // before determining that it is complete
    assign path_mod = changed;

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
        if (travel_cost < new_cost) begin
            new_cost = travel_cost;
            new_dir  = state;
            changed = 1;
        end
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
