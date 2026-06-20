// marco2_opcode_leds.v — ver copia em marco2_base/ (fonte para Quartus).
// LEDR[7:0]: one-hot da ULTIMA instrucao (so um LED). LEDR[8]=busy, [9]=done.

`timescale 1ns/1ns

module marco2_opcode_leds (
    input  wire        clk,
    input  wire        rst_n,
    input  wire [31:0] data_in,
    input  wire [2:0]  ctrl,
    output reg  [9:0]  LEDR
);

    localparam integer BUSY_CYCLES = 50000;

    wire        enable     = ctrl[0];
    reg         enable_d1;
    wire        enable_rise = enable & ~enable_d1;

    reg  [7:0]  last_op_onehot;
    reg         done_sticky;
    reg         busy_led;

    reg         fsm_busy;
    integer     cnt;

    always @(posedge clk or negedge rst_n) begin
        if (!rst_n) begin
            enable_d1       <= 1'b0;
            last_op_onehot  <= 8'b0;
            done_sticky     <= 1'b0;
            busy_led        <= 1'b0;
            fsm_busy        <= 1'b0;
            cnt             <= 0;
            LEDR            <= 10'b0;
        end else begin
            enable_d1 <= enable;

            if (enable_rise) begin
                last_op_onehot <= (8'b1 << data_in[2:0]);
                fsm_busy       <= 1'b1;
                cnt            <= BUSY_CYCLES;
            end

            if (fsm_busy) begin
                busy_led <= 1'b1;
                if (cnt > 0)
                    cnt <= cnt - 1;
                else begin
                    fsm_busy    <= 1'b0;
                    busy_led    <= 1'b0;
                    done_sticky <= 1'b1;
                end
            end

            LEDR[7:0] <= last_op_onehot;
            LEDR[8]   <= busy_led;
            LEDR[9]   <= done_sticky;
        end
    end

endmodule