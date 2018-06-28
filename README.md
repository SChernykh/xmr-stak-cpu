# This is a test repo for Cryptonight anti-ASIC/FPGA modifications

It's based on the old xmr-stak-cpu repo. Only "benchmark_mode config.txt" command line is supported.

### 1. Shuffle modification

Cryptonight is memory-intensive in terms of memory latency, but not bandwidth. Modern CPUs use 64-byte wide cache lines, but Cryptonight only loads/stores 16 bytes at a time, so 75% of available CPU cache bandwidth is wasted. ASICs are optimized for these 16 byte-wide memory accesses, so they always use 100% of whatever memory they have.

The idea is to do something computationally light and safe with the other 48 bytes of the same cache line (which is loaded in L1 cache anyway) on each step. Shuffle modification, as can be guessed by its name, treats these 48 bytes as 24 2-byte elements and shuffles them. The actual permutation is the following one:

( 0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23) ->
(18 22 19 23 16 17 20 21  2  5  3  4  6  7  0  1  9 13  8 12 10 11 14 15)

It has the following nice properties:
- this permutation can be done with only 6 simple SSE2 instructions (not counting load/store)
- every element is moved from the original position to a new one
- the cycle length of this permutation is 45, so it doesn't degenerate to the starting permutation quickly even if applied repeatedly

The shuffle modification makes Cryptonight 4 times more demanding for memory bandwidth, making ASIC/FPGA 4 times slower\*. At the same time, CPU/GPU performance stays almost the same because this bandwidth is already there, it's just not used yet. Shuffle can also be done in parallel with existing Cryptonight calculations.

\* The 4 times slowdown applies only to devices (ASIC/FPGA) that use external memory for storing the scratchpad and saturate this memory's bandwidth. Devices that use on-chip memory have no problems with bandwidth, but they'll still have to do 4 times more memory reads/writes, so they'll also become somewhat slower.

### 2. Shuffle with lag modification

It goes one step further compared to the previous modification: it shuffles not the current cache line, but one of the previously accessed cache lines which are still in L1 cache, selected randomly. All 64 bytes are shuffled as 32 2-byte elements, so this modification requires 5 times more memory bandwidth AND 2 times more random memory accesses. The actual permutation is the following one:

(  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 ) ->
( 18 22 19 23 16 17 20 21  2  5  3  4  6  7  0  1 25 30 31 24 26 27 28 29 13  9  8 12 10 11 14 15 )

Unlike the simple shuffle modification, shuffle with lag takes full advantage of large and fast L1 cache, without slowing down CPU. As for GPU, they also have cache, but the maximum lag needs to be fine tuned to find a point where it's still large enough and doesn't slow down GPU.

Current maximum lag is 256 cache lines (16 KB of previously accessed data), but as I said, it can be reduced if GPUs perform bad.

### 3. Integer math modification

It adds one 64:32 bit integer division and one 48 bit integer square root per iteration.

Adding integer division and integer square roots to the main loop ramps up the complexity of ASIC/FPGA and silicon area needed to implement it, so they'll be much less efficient with the same transistor budget and/or power consumption. Most common hardware implementations of division and square roots require a lot of clock cycles of latency: at least 8 cycles for 64:32 bit division and the same 8 cycles for 48 bit square root. These latencies are achieved for the best and fastest hardware implementations I could find. And the way this modification is made ensures that division and square root from the same main loop iteration can't be done in parallel, so their latencies add up making it staggering 16 cycles per iteration in the best case, comparing to 1 cycle per iteration in the original Crytonight. Even though this latency can be hidden in pipelined implementations, it will require A LOT of logical elements/silicon area to implement. Hiding the latency will also require many parallel scratchpads which will be a strong limiting factor for hardware with on-chip memory. They just don't have enough memory to hide the latency entirely. My rough estimates show ~15x slowdown in this case.

Good news for CPU and GPU is that division and square roots can be added to the main loop in such a way that their latency is completely hidden, so again there is almost no slowdown.

# Performance

As you can see from this data, CPUs and GPUs get less than 8% slower. On the other side, ASIC/FPGA which use external memory for scratchpad get 4 times slower due to increased bandwidth usage. ASIC/FPGA which use on-chip memory for scratchpad will also get a few times slower because of high latencies introduced with division and square root calculations: they just don't have enough on-chip memory to hide these latencies with many parallel Cryptonight calculations.

AMD Ryzen (1 thread):

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 71.1 H/s|100.0%
INT_MATH | 70.0 H/s|98.5%
SHUFFLE | 69.3 H/s|97.5%
Both mods | 67.0 H/s|94.2%
shuffle_with_lag\* | 69.1 H/s|97.2%

Intel Pentium G5400 (Coffee Lake, 2 cores, 4 MB Cache, 3.70 GHz), 2 threads

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 146.5 H/s|100.0%
INT_MATH | 144.2 H/s|98.4%
SHUFFLE | 145.5 H/s|99.3%
Both mods | 142.8 H/s|97.5%
shuffle_with_lag\* | 143.9 H/s|98.2%

Intel Core i5 3210M (Ivy Bridge, 2 cores, 3 MB Cache, 2.80 GHz), 1 thread

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 71.8 H/s|100.0%
INT_MATH | 61.7 H/s|85.9%
SHUFFLE | 70.5 H/s|98.2%
Both mods | 60.5 H/s|84.3%
shuffle_with_lag\* | 70.9 H/s|97.8%

Intel Core i7 7820X (Skylake-X, 8 cores, 11 MB Cache, 3.60 GHz), 1 thread

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 66.9 H/s|100.0%
INT_MATH | 65.3 H/s|97.6%
SHUFFLE | 65.7 H/s|98.2%
Both mods | 64.6 H/s|96.6%
shuffle_with_lag\* | 65.6 H/s|98.1%

\*shuffle_with_lag is not implemented for GPU yet

GPU performance was tested using the code from this repository: https://github.com/SChernykh/xmr-stak-amd

### XMR-STAK used is an old version, so don't expect the same numbers that you have on your mining rigs. What's important here are relative numbers of original and modified Cryptonight versions.

Radeon RX 560 on Windows 10 (overclocked): core @ 1196 MHz, memory @ 2150 MHz, 1 Click PBE Timing Straps, monitor plugged in, intensity 1024, worksize 32:

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 466.7 H/s|100.0%
INT_MATH | 438.7 H/s|94.0%
SHUFFLE | 452.4 H/s|96.9%
Both mods | 438.1 H/s|93.9%

Radeon RX 560 on Windows 10 (RX 550 simulation): core @ 595 MHz, memory @ 2150 MHz, 1 Click PBE Timing Straps, monitor plugged in, intensity 1024, worksize 32:

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 394.3 H/s|100.0%
INT_MATH | 312.8 H/s|79.3%
SHUFFLE | 355.9 H/s|90.3%
Both mods | 254.3 H/s|64.5%

**It looks like RX 550 needs GPU core overclocking to properly handle new modifications.**

GeForce GTX 1060 6 GB on Windows 10: all stock, monitor plugged in, intensity 800, worksize 8:

Mod|Hashrate|Performance level
---|--------|-----------------
\-|453.6 H/s|100.0%
INT_MATH|450.7 H/s|99.4%
SHUFFLE|421.2 H/s|92.9%
Both mods|420.9 H/s|92.8%
