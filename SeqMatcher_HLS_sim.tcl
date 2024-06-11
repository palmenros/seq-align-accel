source "SeqMatcher_HLS.tcl"
add_files -tb HLS/testbench.cpp
csim_design -O
quit
