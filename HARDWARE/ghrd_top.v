`define ENABLE_HPS

module ghrd_top(

      ///////// ADC /////////
      output             ADC_CONVST,
      output             ADC_DIN,
      input              ADC_DOUT,
      output             ADC_SCLK,

      ///////// AUD /////////
      input              AUD_ADCDAT,
      inout              AUD_ADCLRCK,
      inout              AUD_BCLK,
      output             AUD_DACDAT,
      inout              AUD_DACLRCK,
      output             AUD_XCK,

      ///////// CLOCK2 /////////
      input              CLOCK2_50,
      ///////// CLOCK3 /////////
      input              CLOCK3_50,
      ///////// CLOCK4 /////////
      input              CLOCK4_50,
      ///////// CLOCK /////////
      input              CLOCK_50,

      ///////// DRAM /////////
      output      [12:0] DRAM_ADDR,
      output      [1:0]  DRAM_BA,
      output             DRAM_CAS_N,
      output             DRAM_CKE,
      output             DRAM_CLK,
      output             DRAM_CS_N,
      inout       [15:0] DRAM_DQ,
      output             DRAM_LDQM,
      output             DRAM_RAS_N,
      output             DRAM_UDQM,
      output             DRAM_WE_N,

      ///////// FAN /////////
      output             FAN_CTRL,

      ///////// FPGA /////////
      output             FPGA_I2C_SCLK,
      inout              FPGA_I2C_SDAT,

      ///////// GPIO /////////
      inout     [35:0]   GPIO_0,
      inout     [35:0]   GPIO_1,

      ///////// HEX /////////
      output      [6:0]  HEX0,
      output      [6:0]  HEX1,
      output      [6:0]  HEX2,
      output      [6:0]  HEX3,
      output      [6:0]  HEX4,
      output      [6:0]  HEX5,

`ifdef ENABLE_HPS
      ///////// HPS /////////
      inout              HPS_CONV_USB_N,
      output      [14:0] HPS_DDR3_ADDR,
      output      [2:0]  HPS_DDR3_BA,
      output             HPS_DDR3_CAS_N,
      output             HPS_DDR3_CKE,
      output             HPS_DDR3_CK_N,
      output             HPS_DDR3_CK_P,
      output             HPS_DDR3_CS_N,
      output      [3:0]  HPS_DDR3_DM,
      inout       [31:0] HPS_DDR3_DQ,
      inout       [3:0]  HPS_DDR3_DQS_N,
      inout       [3:0]  HPS_DDR3_DQS_P,
      output             HPS_DDR3_ODT,
      output             HPS_DDR3_RAS_N,
      output             HPS_DDR3_RESET_N,
      input              HPS_DDR3_RZQ,
      output             HPS_DDR3_WE_N,
      output             HPS_ENET_GTX_CLK,
      inout              HPS_ENET_INT_N,
      output             HPS_ENET_MDC,
      inout              HPS_ENET_MDIO,
      input              HPS_ENET_RX_CLK,
      input       [3:0]  HPS_ENET_RX_DATA,
      input              HPS_ENET_RX_DV,
      output      [3:0]  HPS_ENET_TX_DATA,
      output             HPS_ENET_TX_EN,
      inout       [3:0]  HPS_FLASH_DATA,
      output             HPS_FLASH_DCLK,
      output             HPS_FLASH_NCSO,
      inout              HPS_GSENSOR_INT,
      inout              HPS_I2C1_SCLK,
      inout              HPS_I2C1_SDAT,
      inout              HPS_I2C2_SCLK,
      inout              HPS_I2C2_SDAT,
      inout              HPS_I2C_CONTROL,
      inout              HPS_KEY,
      inout              HPS_LED,
      inout              HPS_LTC_GPIO,
      output             HPS_SD_CLK,
      inout              HPS_SD_CMD,
      inout       [3:0]  HPS_SD_DATA,
      output             HPS_SPIM_CLK,
      input              HPS_SPIM_MISO,
      output             HPS_SPIM_MOSI,
      inout              HPS_SPIM_SS,
      input              HPS_UART_RX,
      output             HPS_UART_TX,
      input              HPS_USB_CLKOUT,
      inout       [7:0]  HPS_USB_DATA,
      input              HPS_USB_DIR,
      input              HPS_USB_NXT,
      output             HPS_USB_STP,
`endif

      ///////// IRDA /////////
      input              IRDA_RXD,
      output             IRDA_TXD,

      ///////// KEY /////////
      input       [3:0]  KEY,

      ///////// LEDR /////////
      output      [9:0]  LEDR,

      ///////// PS2 /////////
      inout              PS2_CLK,
      inout              PS2_CLK2,
      inout              PS2_DAT,
      inout              PS2_DAT2,

      ///////// SW /////////
      input       [9:0]  SW,

      ///////// TD /////////
      input              TD_CLK27,
      input       [7:0]  TD_DATA,
      input              TD_HS,
      output             TD_RESET_N,
      input              TD_VS,

      ///////// VGA /////////
      output      [7:0]  VGA_B,
      output             VGA_BLANK_N,
      output             VGA_CLK,
      output      [7:0]  VGA_G,
      output             VGA_HS,
      output      [7:0]  VGA_R,
      output             VGA_SYNC_N,
      output             VGA_VS,

      ///////// UART /////////
      output             UART_TX,
      input              UART_RX,
      output             UART_RTS,
      input              UART_CTS,

      ///////// QSPI /////////
      output             QSPI_FLASH_SCLK,
      inout       [3:0]  QSPI_FLASH_DATA,
      output             QSPI_FLASH_CE_n,

      ///////// RISC-V JTAG /////////
      input              RISCV_JTAG_TCK,
      input              RISCV_JTAG_TDI,
      output             RISCV_JTAG_TDO,
      input              RISCV_JTAG_TMS
);

// =======================================================
//  Fios internos originais
// =======================================================
wire [3:0]  fpga_debounced_buttons;
wire [9:0]  fpga_led_internal;
wire        hps_fpga_reset_n;
wire [2:0]  hps_reset_req;
wire        hps_cold_reset;
wire        hps_warm_reset;
wire        hps_debug_reset;
wire [27:0] stm_hw_events;

assign stm_hw_events = {{3{1'b0}}, SW, fpga_led_internal, fpga_debounced_buttons};

// =======================================================
//  Fios dos PIOs — Marco 2
//  Offsets na ponte leve: DATA_IN 0x40, CTRL 0x60 (ver Marco2-driver/hps_0.h).
// =======================================================
wire [31:0] data_in_w;
wire [2:0]  ctrl_w;
wire [9:0] marco2_ledr_probe;

// =======================================================
//  Fios dos PIOs — Marco 3
//  Offsets na ponte leve: vga_data 0x70, vga_ctrl 0x80 , vga_status 0x90
// =======================================================
wire [31:0] vga_data_w;
wire vga_ctrl_w;
wire vga_status_w;



// =======================================================
//  Sistema Qsys — soc_system
// =======================================================
soc_system u0 (
    .clk_clk                               ( CLOCK_50          ),
    .reset_reset_n                         ( hps_fpga_reset_n  ),

    .memory_mem_a                          ( HPS_DDR3_ADDR     ),
    .memory_mem_ba                         ( HPS_DDR3_BA       ),
    .memory_mem_ck                         ( HPS_DDR3_CK_P     ),
    .memory_mem_ck_n                       ( HPS_DDR3_CK_N     ),
    .memory_mem_cke                        ( HPS_DDR3_CKE      ),
    .memory_mem_cs_n                       ( HPS_DDR3_CS_N     ),
    .memory_mem_ras_n                      ( HPS_DDR3_RAS_N    ),
    .memory_mem_cas_n                      ( HPS_DDR3_CAS_N    ),
    .memory_mem_we_n                       ( HPS_DDR3_WE_N     ),
    .memory_mem_reset_n                    ( HPS_DDR3_RESET_N  ),
    .memory_mem_dq                         ( HPS_DDR3_DQ       ),
    .memory_mem_dqs                        ( HPS_DDR3_DQS_P    ),
    .memory_mem_dqs_n                      ( HPS_DDR3_DQS_N    ),
    .memory_mem_odt                        ( HPS_DDR3_ODT      ),
    .memory_mem_dm                         ( HPS_DDR3_DM       ),
    .memory_oct_rzqin                      ( HPS_DDR3_RZQ      ),

    .hps_0_hps_io_hps_io_emac1_inst_TX_CLK ( HPS_ENET_GTX_CLK    ),
    .hps_0_hps_io_hps_io_emac1_inst_TXD0   ( HPS_ENET_TX_DATA[0] ),
    .hps_0_hps_io_hps_io_emac1_inst_TXD1   ( HPS_ENET_TX_DATA[1] ),
    .hps_0_hps_io_hps_io_emac1_inst_TXD2   ( HPS_ENET_TX_DATA[2] ),
    .hps_0_hps_io_hps_io_emac1_inst_TXD3   ( HPS_ENET_TX_DATA[3] ),
    .hps_0_hps_io_hps_io_emac1_inst_RXD0   ( HPS_ENET_RX_DATA[0] ),
    .hps_0_hps_io_hps_io_emac1_inst_MDIO   ( HPS_ENET_MDIO       ),
    .hps_0_hps_io_hps_io_emac1_inst_MDC    ( HPS_ENET_MDC        ),
    .hps_0_hps_io_hps_io_emac1_inst_RX_CTL ( HPS_ENET_RX_DV      ),
    .hps_0_hps_io_hps_io_emac1_inst_TX_CTL ( HPS_ENET_TX_EN      ),
    .hps_0_hps_io_hps_io_emac1_inst_RX_CLK ( HPS_ENET_RX_CLK     ),
    .hps_0_hps_io_hps_io_emac1_inst_RXD1   ( HPS_ENET_RX_DATA[1] ),
    .hps_0_hps_io_hps_io_emac1_inst_RXD2   ( HPS_ENET_RX_DATA[2] ),
    .hps_0_hps_io_hps_io_emac1_inst_RXD3   ( HPS_ENET_RX_DATA[3] ),
    .hps_0_hps_io_hps_io_qspi_inst_IO0     ( HPS_FLASH_DATA[0]   ),
    .hps_0_hps_io_hps_io_qspi_inst_IO1     ( HPS_FLASH_DATA[1]   ),
    .hps_0_hps_io_hps_io_qspi_inst_IO2     ( HPS_FLASH_DATA[2]   ),
    .hps_0_hps_io_hps_io_qspi_inst_IO3     ( HPS_FLASH_DATA[3]   ),
    .hps_0_hps_io_hps_io_qspi_inst_SS0     ( HPS_FLASH_NCSO      ),
    .hps_0_hps_io_hps_io_qspi_inst_CLK     ( HPS_FLASH_DCLK      ),
    .hps_0_hps_io_hps_io_sdio_inst_CMD     ( HPS_SD_CMD          ),
    .hps_0_hps_io_hps_io_sdio_inst_D0      ( HPS_SD_DATA[0]      ),
    .hps_0_hps_io_hps_io_sdio_inst_D1      ( HPS_SD_DATA[1]      ),
    .hps_0_hps_io_hps_io_sdio_inst_CLK     ( HPS_SD_CLK          ),
    .hps_0_hps_io_hps_io_sdio_inst_D2      ( HPS_SD_DATA[2]      ),
    .hps_0_hps_io_hps_io_sdio_inst_D3      ( HPS_SD_DATA[3]      ),
    .hps_0_hps_io_hps_io_usb1_inst_D0      ( HPS_USB_DATA[0]     ),
    .hps_0_hps_io_hps_io_usb1_inst_D1      ( HPS_USB_DATA[1]     ),
    .hps_0_hps_io_hps_io_usb1_inst_D2      ( HPS_USB_DATA[2]     ),
    .hps_0_hps_io_hps_io_usb1_inst_D3      ( HPS_USB_DATA[3]     ),
    .hps_0_hps_io_hps_io_usb1_inst_D4      ( HPS_USB_DATA[4]     ),
    .hps_0_hps_io_hps_io_usb1_inst_D5      ( HPS_USB_DATA[5]     ),
    .hps_0_hps_io_hps_io_usb1_inst_D6      ( HPS_USB_DATA[6]     ),
    .hps_0_hps_io_hps_io_usb1_inst_D7      ( HPS_USB_DATA[7]     ),
    .hps_0_hps_io_hps_io_usb1_inst_CLK     ( HPS_USB_CLKOUT      ),
    .hps_0_hps_io_hps_io_usb1_inst_STP     ( HPS_USB_STP         ),
    .hps_0_hps_io_hps_io_usb1_inst_DIR     ( HPS_USB_DIR         ),
    .hps_0_hps_io_hps_io_usb1_inst_NXT     ( HPS_USB_NXT         ),
    .hps_0_hps_io_hps_io_spim1_inst_CLK    ( HPS_SPIM_CLK        ),
    .hps_0_hps_io_hps_io_spim1_inst_MOSI   ( HPS_SPIM_MOSI       ),
    .hps_0_hps_io_hps_io_spim1_inst_MISO   ( HPS_SPIM_MISO       ),
    .hps_0_hps_io_hps_io_spim1_inst_SS0    ( HPS_SPIM_SS         ),
    .hps_0_hps_io_hps_io_uart0_inst_RX     ( HPS_UART_RX         ),
    .hps_0_hps_io_hps_io_uart0_inst_TX     ( HPS_UART_TX         ),
    .hps_0_hps_io_hps_io_i2c0_inst_SDA     ( HPS_I2C1_SDAT       ),
    .hps_0_hps_io_hps_io_i2c0_inst_SCL     ( HPS_I2C1_SCLK       ),
    .hps_0_hps_io_hps_io_i2c1_inst_SDA     ( HPS_I2C2_SDAT       ),
    .hps_0_hps_io_hps_io_i2c1_inst_SCL     ( HPS_I2C2_SCLK       ),
    .hps_0_hps_io_hps_io_gpio_inst_GPIO09  ( HPS_CONV_USB_N      ),
    .hps_0_hps_io_hps_io_gpio_inst_GPIO35  ( HPS_ENET_INT_N      ),
    .hps_0_hps_io_hps_io_gpio_inst_GPIO40  ( HPS_LTC_GPIO        ),
    .hps_0_hps_io_hps_io_gpio_inst_GPIO48  ( HPS_I2C_CONTROL     ),
    .hps_0_hps_io_hps_io_gpio_inst_GPIO53  ( HPS_LED             ),
    .hps_0_hps_io_hps_io_gpio_inst_GPIO54  ( HPS_KEY             ),
    .hps_0_hps_io_hps_io_gpio_inst_GPIO61  ( HPS_GSENSOR_INT     ),

    .hps_0_f2h_stm_hw_events_stm_hwevents  ( stm_hw_events       ),
    .hps_0_h2f_reset_reset_n               ( hps_fpga_reset_n    ),
    .hps_0_f2h_warm_reset_req_reset_n      ( ~hps_warm_reset     ),
    .hps_0_f2h_debug_reset_req_reset_n     ( ~hps_debug_reset    ),
    .hps_0_f2h_cold_reset_req_reset_n      ( ~hps_cold_reset     ),

    // PIOs do Marco 2
    .data_in_external_connection_export    ( data_in_w           ),
    .data_out_external_connection_export   ( data_out_w    ),
    .ctrl_external_connection_export       ( ctrl_w             ),
	 
	 //PIOS Do marco 3
	 .vga_data_external_connection_export   (vga_data_w),   //   vga_data_external_connection.export
    .vga_status_external_connection_export (vga_status_w), // vga_status_external_connection.export
    .vga_ctrl_external_connection_export   (vga_ctrl_w)  
	 
	 
	 
);

// =======================================================
//  Reset do HPS
// =======================================================
hps_reset hps_reset_inst (
    .source_clk ( CLOCK_50      ),
    .source     ( hps_reset_req )
);

altera_edge_detector pulse_cold_reset (
    .clk       ( CLOCK_50         ),
    .rst_n     ( hps_fpga_reset_n ),
    .signal_in ( hps_reset_req[0] ),
    .pulse_out ( hps_cold_reset   )
);
defparam pulse_cold_reset.PULSE_EXT = 6;
defparam pulse_cold_reset.EDGE_TYPE = 1;
defparam pulse_cold_reset.IGNORE_RST_WHILE_BUSY = 1;

altera_edge_detector pulse_warm_reset (
    .clk       ( CLOCK_50         ),
    .rst_n     ( hps_fpga_reset_n ),
    .signal_in ( hps_reset_req[1] ),
    .pulse_out ( hps_warm_reset   )
);
defparam pulse_warm_reset.PULSE_EXT = 2;
defparam pulse_warm_reset.EDGE_TYPE = 1;
defparam pulse_warm_reset.IGNORE_RST_WHILE_BUSY = 1;

altera_edge_detector pulse_debug_reset (
    .clk       ( CLOCK_50         ),
    .rst_n     ( hps_fpga_reset_n ),
    .signal_in ( hps_reset_req[2] ),
    .pulse_out ( hps_debug_reset  )
);
defparam pulse_debug_reset.PULSE_EXT = 32;
defparam pulse_debug_reset.EDGE_TYPE = 1;
defparam pulse_debug_reset.IGNORE_RST_WHILE_BUSY = 1;

// =======================================================
//  Modo teste: decodificador de opcode + busy/done em marco2_opcode_leds.v
//  Para voltar ao CoProcessor + HEX + LEDR do ELM, comente o bloco abaixo e
//  descomente o bloco marcado /* ELM COMPLETO */ no fim do ficheiro.
// =======================================================
marco2_opcode_leds u_opcode_probe (
    .clk   ( CLOCK_50         ),
    .rst_n ( hps_fpga_reset_n ),
    .data_in ( data_in_w      ),
    .ctrl    ( ctrl_w         ),
    .LEDR    ( marco2_ledr_probe )
);

assign LEDR = marco2_ledr_probe;

assign HEX0 = 7'h7F;
assign HEX1 = 7'h7F;
assign HEX2 = 7'h7F;
assign HEX3 = 7'h7F;
assign HEX4 = 7'h7F;
assign HEX5 = 7'h7F;


// -------- ELM COMPLETO (descomente se quiser) --------
wire [31:0] data_out_w;
wire elm_enable = ctrl_w[0];
wire elm_clr    = ctrl_w[1];
wire elm_rst    = ctrl_w[2];

CoProcessor elm_inst (
    .clk           ( CLOCK_50   ),
    .data_in       ( data_in_w  ),
    .enable        ( elm_enable ),
    .clr_operation ( elm_clr    ),
    .rst           ( elm_rst    ),
    .data_out      ( data_out_w )
);

/*VGA
*/

wire [8:0] posx;
wire [7:0] posy;
wire [2:0] red;
wire [2:0] green;
wire [2:0] blue;

assign posx = vga_data_w[8:0];
assign posy = vga_data_w[16:9];

assign red   = vga_data_w[19:17];
assign green = vga_data_w[22:20];
assign blue  = vga_data_w[25:23];




controller_vga_to_sd vga0 (
    .posx(posx),
    .posy(posy),

    .red(red),
    .green(green),
    .blue(blue),

    .enable(vga_ctrl_w),

    .clk(CLOCK_50),
    .rst(reset),

    .hs(VGA_HS),
    .vs(VGA_VS),
    .sync(VGA_SYNC_N),
    .blank(VGA_BLANK_N),
    .vga_clk(VGA_CLK),

    .vga_red(VGA_R),
    .vga_green(VGA_G),
    .vga_blue(VGA_B),

    .done(vga_status_w)
);


endmodule