open_project SeqMatcher_HW_HLS
set_top SeqMatcher_HW
add_files HLS/seqMatcher.h
add_files HLS/seqMatcher.cpp
add_files -tb HLS/testbench.cpp
open_solution "solution1" -flow_target vivado
set_part {xc7z020clg400-1}
create_clock -period 10 -name default
set_clock_uncertainty 1
config_export -display_name SeqMatcher_HW -format ip_catalog -output ./IP-catalog/SeqMatcher_HW_HLS.zip -rtl vhdl -vendor EPFL -vivado_clock 10
config_interface -m_axi_addr64=0
quit
