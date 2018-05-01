#include <stdio.h>

const char * border_cost = "9'h1ff";
#define calc_off(r,c) ((r) * w + c)
int main(int argc, char * argv[])
{

    if (argc <= 3) {
        fprintf(stderr, "gen <file to write to> <width> <height> [register size]\n");
        return 1;
    }

    int size = 12;
    if (argc >= 5) {
        sscanf(argv[4], "%d", &size);
    }
    char border_cost[128];
    sprintf(border_cost, "%d'h%x", size, 0xFFFFFFFF >> (32-size));

    int w, h;
    const char * out_path = argv[1];
    sscanf(argv[2], "%d", &w);
    sscanf(argv[3], "%d", &h);

    FILE * f = fopen(out_path, "w");

    int max = w * h - 1;
    fprintf(f,"//auto-generated wires\n");
    fprintf(f,"wire clk;\n");
    fprintf(f,"wire rst;\n");
    fprintf(f,"wire [%d:0] clr;\n", max);
    fprintf(f,"wire [%d:0] ld;\n", max);
    fprintf(f,"wire [3:0] ld_weight;\n");
    fprintf(f,"wire [%d:0] mod;\n", max);
    fprintf(f,"wire [%d:0] cost[0:%d];\n", size-1, max);
    fprintf(f,"wire [3:0] dir[0:%d];\n", max);
    fprintf(f,"localparam COST_SIZE = %d;\n", size);
    fprintf(f,"localparam BORDER_COST = %s;\n", border_cost);
    fprintf(f,"\n//auto-generated fabric\n");
    for (int r = 0; r < h; ++r) {
        for (int c = 0; c < w; ++c) {
            int num = calc_off(r,c);
            fprintf(f,"neu #(.X(%d), .Y(%d), .N(%d), .COST_SIZE(COST_SIZE)) neu%d(", c, r, c + r*32, num);
            //fprintf(f,"neu neu%d(", num);
            fprintf(f,".clk(clk),");
            fprintf(f,".rst(rst),");
            fprintf(f,".clr(clr[%d]),", num);
            fprintf(f,".ld(ld[%d]),", num);
            fprintf(f,".ld_weight(ld_weight),");

            // n
            if (r == 0)
                fprintf(f,".n_cost(BORDER_COST),");
            else
                fprintf(f,".n_cost(cost[%d]),", calc_off(r-1,c));

            // ne
            if (r == 0 || c == w - 1)
                fprintf(f,".ne_cost(BORDER_COST),");
            else
                fprintf(f,".ne_cost(cost[%d]),", calc_off(r-1,c+1));

            // e
            if (c == w - 1)
                fprintf(f,".e_cost(BORDER_COST),");
            else
                fprintf(f,".e_cost(cost[%d]),", calc_off(r,c+1));

            // se
            if (r == h - 1 || c == w - 1)
                fprintf(f,".se_cost(BORDER_COST),");
            else
                fprintf(f,".se_cost(cost[%d]),", calc_off(r+1,c+1));

            // s
            if (r == h - 1)
                fprintf(f,".s_cost(BORDER_COST),");
            else
                fprintf(f,".s_cost(cost[%d]),", calc_off(r+1,c));

            // sw
            if (r == h - 1 || c == 0)
                fprintf(f,".sw_cost(BORDER_COST),");
            else
                fprintf(f,".sw_cost(cost[%d]),", calc_off(r+1,c-1));

            // w
            if (c == 0)
                fprintf(f,".w_cost(BORDER_COST),");
            else
                fprintf(f,".w_cost(cost[%d]),", calc_off(r,c-1));

            // nw
            if (r == 0 || c == 0)
                fprintf(f,".nw_cost(BORDER_COST),");
            else
                fprintf(f,".nw_cost(cost[%d]),", calc_off(r-1,c-1));

            fprintf(f,".path_mod(mod[%d]),", num);
            fprintf(f,".path_dir(dir[%d]),", num);
            fprintf(f,".path_cost(cost[%d]));", num);

            fprintf(f,"\n");
        }
    }

    fclose(f);
    return 0;
}
