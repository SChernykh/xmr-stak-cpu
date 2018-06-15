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

The shuffle modification makes Cryptonight 4 times more demanding for memory bandwidth, making ASIC/FPGA 4 times slower\*. At the same time, CPU/GPU performance stays almost the same because this bandwidth is already there, it's just not used yet. Shuffle can also be done in parallel with existing Cryptonight calculations. My tests on CPU showed only 2.5% slowdown.

\* The 4 times slowdown applies only to devices (ASIC/FPGA) that use external memory for storing the scratchpad and saturate this memory's bandwidth. Devices that use on-chip memory have no problems with bandwidth, but they'll still have to do 4 times more memory reads/writes, so they'll also become somewhat slower.

### 2. Shuffle with lag modification

It goes one step further compared to the previous modification: it shuffles not the current cache line, but one of the previously accessed cache lines which are still in L1 cache, selected randomly. All 64 bytes are shuffled as 32 2-byte elements, so this modification requires 5 times more memory bandwidth AND 2 times more random memory accesses. The actual permutation is the following one:

(  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 ) ->
( 18 22 19 23 16 17 20 21  2  5  3  4  6  7  0  1 25 30 31 24 26 27 28 29 13  9  8 12 10 11 14 15 )

Unlike the simple shuffle modification, shuffle with lag takes full advantage of large and fast L1 cache, without slowing down CPU (~2.5% slowdown, just like the simple shuffle). As for GPU, they also have cache, but the maximum lag needs to be fine tuned to find a point where it's still large enough and doesn't slow down GPU.

Current maximum lag is 256 cache lines (16 KB of previously accessed data), but as I said, it can be reduced if GPUs perform bad.

### 3. Integer math modification

It adds one 64:32 bit integer division and two 52 bit integer square roots per iteration.

Adding integer division and integer square roots to the main loop ramps up the complexity of ASIC/FPGA and silicon area needed to implement it, so they'll be much less efficient with the same transistor budget and/or power consumption. Most common hardware implementations of division and square roots require a lot of clock cycles of latency (at least 16 cycles for division and at least 26 cycles for 52 bit square root). Even though this latency can be hidden in pipelined implementations, it will require A LOT of logical elements/silicon area to implement.

Good news for CPU and GPU is that division and square roots can be added to the main loop in such a way that their latency is completely hidden, so again there is almost no slowdown. My tests on CPU showed 3% slowdown with division and no additional slowdown at all when division is added together with shuffle modification.
