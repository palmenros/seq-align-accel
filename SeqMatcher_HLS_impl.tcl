source "SeqMatcher_HLS.tcl"
csynth_design
export_design -rtl vhdl -format ip_catalog -output ./IP-catalog/SeqMatcher_HW_HLS.zip
quit
