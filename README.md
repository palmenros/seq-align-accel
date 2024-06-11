# Sequence Matcher Accelerator

Code for a Smith-Waterman (DNA sequence matching) hardware accelerator implemented using Vivado High-Level Synthesis. It is highly optimized including BRAM caching, compression, score quantization, efficient systolic-array score computation and scheduling across multiple parallel workers.

It also includes a Linux kernel module driver to communicate with the Pynq-Z2 software.

Our accelerator is 60% faster than a 490W 80-core Xeon while consuming 2.5W on a Pynq-Z2.

Authors:
- Pedro Palacios Almendros
- Alejandro López Rodríguez

A report can be found in `Report.pdf`, where the different techniques used are explored and a comparison is made between different parallel worker scheduling paradigms.
