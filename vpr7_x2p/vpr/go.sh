#!/bin/sh
# Example of how to run vpr

# Pack, place, and route a heterogeneous FPGA
# Packing uses the AAPack algorithm
./vpr ../../fpga_flow/arch/fpga_spice/k6_N10_sram_tsmc40nm_TT.xml Circuits/s298_prevpr.blif --full_stats --nodisp --activity_file Circuits/s298_prevpr.act --route_chan_width 30 --fpga_spice --fpga_spice_rename_illegal_port --fpga_spice_dir ./spice_test --fpga_spice_print_top_testbench --fpga_spice_print_grid_testbench --fpga_spice_print_cb_testbench --fpga_spice_print_sb_testbench --fpga_spice_print_lut_testbench --fpga_spice_print_hardlogic_testbench --fpga_spice_print_pb_mux_testbench --fpga_spice_print_cb_mux_testbench --fpga_spice_print_sb_mux_testbench --fpga_verilog --fpga_verilog_dir ./verilog_test


