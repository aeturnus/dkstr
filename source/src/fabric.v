module fabric #(parameter DIM=32, parameter COST_SIZE=9)
    (
        input   wire    clk,
        input   wire    arst_n,

        // registers
        // control register
        // [31] run
        // [30] load
        // ...
        // [9:5] start_y
        // [4:0] start_x
        input   wire            ctrl_wr,
        input   wire    [31:0]  ctrl_in,
        output  wire    [31:0]  ctrl_out,

        // memory interface
        input                   txn_rdy,
        input   wire    [31:0]  txn_rdata,
        output  wire    [31:0]  txn_wdata,
        output  reg     [31:0]  txn_addr,
        output  reg             txn_req,
        output  reg             txn_wr,

        // interrupts
        output  wire    int_done
    );
    // params
    localparam N = (DIM * DIM);
    localparam N_SIZE = $clog2(N);
    localparam S = COST_SIZE;
    localparam BORDER_COST = {S{1'b1}};

    localparam ADDR_MAP = 32'h40000000;
    localparam ADDR_DIR = 32'h40002000;

    // states
    localparam ST_IDLE          = 0;
    localparam ST_FIRST_LOAD    = 1;
    localparam ST_W_FIRST_LOAD  = 2;
    localparam ST_LOAD_WEIGHT   = 3;
    localparam ST_W_LOAD        = 4;
    localparam ST_RUNNING       = 5;
    localparam ST_PACK_DIR      = 6;
    localparam ST_W_WRITE       = 7;
    localparam ST_W_LAST_WRITE  = 8;

    // visble registers
    reg [4:0] reg_start_x;
    reg [4:0] reg_start_y;
    reg reg_load;
    reg reg_run;
    assign ctrl_out = {reg_run, reg_load, 20'd0, reg_start_y, reg_start_x};

    wire rst;

    // fabric variables
    reg [N-1:0] clr;
    reg [N-1:0] ld;
    reg [3:0] ld_weight;
    wire [N-1:0] mod;
    wire [S-1:0] cost[0:N-1];
    wire [3:0] dir[0:N-1];

    wire activity;
    assign activity = |mod;

    reg [3:0] cs;
    reg [3:0] ns;

    // internal registers and signals
    reg [4:0] curr_x, curr_y;   // current coordinates, for writing and reading
    reg [N_SIZE-1:0] curr;      // current node
    reg [6:0] addr_off;     // word address offset
    reg [31:0] data_word;   // data word to read/pass
    reg [15:0] history;
    wire [3:0] curr_dir;
    //assign curr_dir = {1'b0, dir[curr]};
    assign curr_dir = dir[curr];
    assign txn_wdata = data_word;

    wire [31:0] addr_rd, addr_wr;
    assign addr_rd = ADDR_MAP + (addr_off << 2);
    assign addr_wr = ADDR_DIR + (addr_off << 2);

    // SM outputs
    reg o_clr_curr;         // clear the curr register
    reg o_clr_addr_off;     // clear the addr_off register
    reg o_init_rd;          // initialize a read
    reg o_inc_addr_off;     // increment the word addr offset
    reg o_sv_word;          // save the data word from a read request
    reg o_ld_weight;        // load the weight in the node pointed by curr
    reg o_inc_curr;         // increment the curr register
    reg o_clr_load_map;     // clear the load map register
    reg o_rst; assign rst = o_rst;// resets the nodes
    reg o_clr_his;          // clears the history
    reg o_up_word;          // updates the data word based on first 3 bits of curr
    reg o_init_wr;          //
    reg o_clr_run;          // clears the run register
    reg o_int_done; assign int_done = o_int_done;

    // SM qualifiers
    wire q_load_map; assign q_load_map = reg_load;
    wire q_run; assign q_run = reg_run;
    wire q_data_done; assign q_data_done = txn_rdy;
    wire q_curr_0; assign q_curr_0 = (curr == 0);
    wire q_curr_mod8_last; assign q_curr_mod8_last = (curr[2:0] == 3'b111);
    wire q_activity; assign q_activity = (history != 0);

    // state combinational logic
    integer i;
    always @(*) begin
        ns = cs;
        o_rst = 0;
        o_clr_curr = 0;
        o_clr_addr_off = 0;
        o_init_rd = 0;
        o_inc_addr_off = 0;
        o_sv_word = 0;
        o_ld_weight = 0;
        o_inc_curr = 0;
        o_clr_load_map = 0;
        o_clr_his = 0;
        o_up_word = 0;
        o_init_wr = 0;
        o_clr_run = 0;
        o_int_done = 0;

        // state logic
        case (cs)

        ST_IDLE: begin
            if (q_load_map) begin
                o_clr_curr = 1;
                o_clr_addr_off = 1;
                ns = ST_FIRST_LOAD;
            end
            else if (q_run) begin
                ns = ST_RUNNING;
                o_rst = 1;
                o_clr_his = 1;
            end
        end

        ST_FIRST_LOAD: begin
            o_init_rd = 1;
            o_inc_addr_off = 1;
            ns = ST_W_FIRST_LOAD;
        end

        ST_W_FIRST_LOAD: begin
            ns = ST_W_FIRST_LOAD;
            if (q_data_done) begin
                ns = ST_LOAD_WEIGHT;
                o_sv_word = 1;
                o_init_rd = 1;
            end
        end

        ST_LOAD_WEIGHT: begin
            o_ld_weight = 1;
            o_inc_curr = 1;
            if (q_curr_mod8_last) begin
                ns = ST_W_LOAD;
                o_inc_addr_off = 1;
            end
            else ns = ST_LOAD_WEIGHT;
        end

        ST_W_LOAD: begin
            if (!q_data_done) begin
                ns = ST_W_LOAD;
            end
            else if (!q_curr_0) begin
                ns = ST_LOAD_WEIGHT;
                o_sv_word = 1;
                o_init_rd = 1;
            end
            else begin
                ns = ST_IDLE;
                o_clr_load_map = 1;
            end
        end

        ST_RUNNING: begin
            if (q_activity) begin
                ns = ST_RUNNING;
            end
            else begin
                ns = ST_PACK_DIR;
                o_clr_curr = 1;
                o_clr_addr_off = 1;
            end
        end

        ST_PACK_DIR: begin
            ns = q_curr_mod8_last ? ST_W_WRITE : ST_PACK_DIR;
            o_up_word = 1;
            o_inc_curr = 1;
        end

        ST_W_WRITE: begin
            if (!q_data_done) begin
                ns = ST_W_WRITE;
            end
            else if (!q_curr_0) begin
                ns = ST_PACK_DIR;
                o_inc_addr_off = 1;
                o_init_wr = 1;
            end
            else begin
                ns = ST_W_LAST_WRITE;
                o_init_wr = 1;
            end
        end

        ST_W_LAST_WRITE: begin
            if (!q_data_done) begin
                ns = ST_W_LAST_WRITE;
            end
            else begin
                ns = ST_IDLE;
                o_clr_run = 1;
                o_int_done = 1;
            end
        end

        endcase
    end

    // other combination signals
    always @(*) begin
        // default ld and clr signals
        for (i = 0; i < N-1; i = i + 1) begin
            ld[i] = 0;
            clr[i] = 0;
        end
        if (o_ld_weight) ld[curr] = 1;
        if (reg_run) clr[reg_start_x + (reg_start_y * DIM)] = 1;
        case (curr[2:0])
        0: ld_weight = data_word[3:0];
        1: ld_weight = data_word[7:4];
        2: ld_weight = data_word[11:8];
        3: ld_weight = data_word[15:12];
        4: ld_weight = data_word[19:16];
        5: ld_weight = data_word[23:20];
        6: ld_weight = data_word[27:24];
        7: ld_weight = data_word[31:28];
        endcase

        if (o_init_rd) begin
            txn_addr = addr_rd;
            txn_req = 1;
            txn_wr = 0;
        end
        else if (o_init_wr) begin
            txn_addr = addr_wr;
            txn_req = 1;
            txn_wr = 1;
        end
        else begin
            txn_addr = addr_rd;
            txn_req = 0;
            txn_wr = 0;
        end

    end

    // sequential logic
    initial begin
        cs <= ST_IDLE;
        reg_start_x <= 0;
        reg_start_y <= 0;
        reg_load <= 0;
        reg_run <= 0;
    end

    always @(posedge clk or negedge arst_n) begin
        if (!arst_n) begin
            cs <= ST_IDLE;
            reg_start_x <= 0;
            reg_start_y <= 0;
            reg_load <= 0;
            reg_run <= 0;
        end else begin
            // load the control register if not running
            if (ctrl_wr) begin
                if (!reg_run || !ctrl_in[31]) begin
                    reg_run  <= ctrl_in[31];
                    reg_load <= ctrl_in[30];
                    reg_start_y <= ctrl_in[9:5];
                    reg_start_x <= ctrl_in[4:0];
                end
            end
            cs <= ns;

            if (o_clr_curr) begin
                curr <= 0;
            end
            if (o_clr_addr_off) begin
                addr_off <= 0;
            end
            if (o_inc_addr_off) begin
                addr_off <= addr_off + 1;
            end
            if (o_sv_word) begin
                data_word <= txn_rdata;
            end

            if (o_inc_curr) begin
                if (curr == N-1)
                    curr <= 0;
                else
                    curr <= curr + 1;
            end

            if (o_clr_load_map) begin
                reg_load <= 0;
            end

            if (o_clr_run) begin
                reg_run <= 0;
            end

            if (o_clr_his) begin
                history <= 16'hffff;
            end
            else begin
                history <= {history[14:0], activity};
            end

            if (o_up_word) begin
                case (curr[2:0])
                0: data_word[3:0] <= curr_dir;
                1: data_word[7:4] <= curr_dir;
                2: data_word[11:8] <= curr_dir;
                3: data_word[15:12] <= curr_dir;
                4: data_word[19:16] <= curr_dir;
                5: data_word[23:20] <= curr_dir;
                6: data_word[27:24] <= curr_dir;
                7: data_word[31:28] <= curr_dir;
                endcase
            end

        end
    end

    // generate the fabric
    genvar r, c;
    generate
    for (r = 0; r < DIM; r = r + 1) begin:fab_neu
        for (c = 0; c < DIM; c = c + 1) begin
            if (r == 0 && c == 0) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(BORDER_COST), .ne_cost(BORDER_COST),
                         .e_cost(cost[(c+1) + (r)*DIM]), .se_cost(cost[(c+1) + (r+1)*DIM]),
                         .s_cost(cost[(c) + (r+1)*DIM]), .sw_cost(BORDER_COST),
                         .w_cost(BORDER_COST), .nw_cost(BORDER_COST),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else if (r == 0 && c == DIM-1) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(BORDER_COST), .ne_cost(BORDER_COST),
                         .e_cost(BORDER_COST), .se_cost(BORDER_COST),
                         .s_cost(cost[(c) + (r+1)*DIM]), .sw_cost(cost[(c-1) + (r+1)*DIM]),
                         .w_cost(cost[(c-1) + (r)*DIM]), .nw_cost(BORDER_COST),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else if (r == DIM - 1 && c == 0) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(cost[(c) + (r-1)*DIM]), .ne_cost(cost[(c+1) + (r-1)*DIM]),
                         .e_cost(cost[(c+1) + (r)*DIM]), .se_cost(BORDER_COST),
                         .s_cost(BORDER_COST), .sw_cost(BORDER_COST),
                         .w_cost(BORDER_COST), .nw_cost(BORDER_COST),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else if (r == DIM - 1 && c == DIM - 1) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(cost[(c) + (r-1)*DIM]), .ne_cost(BORDER_COST),
                         .e_cost(BORDER_COST), .se_cost(BORDER_COST),
                         .s_cost(BORDER_COST), .sw_cost(BORDER_COST),
                         .w_cost(cost[(c-1) + (r)*DIM]), .nw_cost(cost[(c-1) + (r-1)*DIM]),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else if (r == 0) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(BORDER_COST), .ne_cost(BORDER_COST),
                         .e_cost(cost[(c+1) + (r)*DIM]), .se_cost(cost[(c+1) + (r+1)*DIM]),
                         .s_cost(cost[(c) + (r+1)*DIM]), .sw_cost(cost[(c-1) + (r+1)*DIM]),
                         .w_cost(cost[(c-1) + (r)*DIM]), .nw_cost(BORDER_COST),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else if (r == DIM - 1) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(cost[(c) + (r-1)*DIM]), .ne_cost(cost[(c+1) + (r-1)*DIM]),
                         .e_cost(cost[(c+1) + (r)*DIM]), .se_cost(BORDER_COST),
                         .s_cost(BORDER_COST), .sw_cost(BORDER_COST),
                         .w_cost(cost[(c-1) + (r)*DIM]), .nw_cost(cost[(c-1) + (r-1)*DIM]),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else if (c == 0) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(cost[(c) + (r-1)*DIM]), .ne_cost(cost[(c+1) + (r-1)*DIM]),
                         .e_cost(cost[(c+1) + (r)*DIM]), .se_cost(cost[(c+1) + (r+1)*DIM]),
                         .s_cost(cost[(c) + (r+1)*DIM]), .sw_cost(BORDER_COST),
                         .w_cost(BORDER_COST), .nw_cost(BORDER_COST),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else if (c == DIM - 1) begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(cost[(c) + (r-1)*DIM]), .ne_cost(BORDER_COST),
                         .e_cost(BORDER_COST), .se_cost(BORDER_COST),
                         .s_cost(cost[(c) + (r+1)*DIM]), .sw_cost(cost[(c-1) + (r+1)*DIM]),
                         .w_cost(cost[(c-1) + (r)*DIM]), .nw_cost(cost[(c-1) + (r-1)*DIM]),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
            else begin
                neu #(.X(c), .Y(r), .N(c+r*DIM), .COST_SIZE(COST_SIZE))
                    neu (.clk(clk), .rst(rst), .clr(clr[c+r*DIM]), .ld(ld[c+r*DIM]), .ld_weight(ld_weight),
                         .n_cost(cost[(c) + (r-1)*DIM]), .ne_cost(cost[(c+1) + (r-1)*DIM]),
                         .e_cost(cost[(c+1) + (r)*DIM]), .se_cost(cost[(c+1) + (r+1)*DIM]),
                         .s_cost(cost[(c) + (r+1)*DIM]), .sw_cost(cost[(c-1) + (r+1)*DIM]),
                         .w_cost(cost[(c-1) + (r)*DIM]), .nw_cost(cost[(c-1) + (r-1)*DIM]),
                         .path_mod(mod[c+r*DIM]), .path_dir(dir[c+r*DIM]), .path_cost(cost[c+r*DIM])
                        );
            end
        end
    end
    endgenerate

endmodule
