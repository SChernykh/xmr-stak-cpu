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

**!!! Tests have shown that this modification kills GPU performance, so it's not viable. The description is only left here for the reference.**

~~It goes one step further compared to the previous modification: it shuffles not the current cache line, but one of the previously accessed cache lines which are still in L1 cache, selected randomly. All 64 bytes are shuffled as 32 2-byte elements, so this modification requires 5 times more memory bandwidth AND 2 times more random memory accesses. The actual permutation is the following one:~~

~~(  0  1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 ) ->
( 18 22 19 23 16 17 20 21  2  5  3  4  6  7  0  1 25 30 31 24 26 27 28 29 13  9  8 12 10 11 14 15 )~~

~~Unlike the simple shuffle modification, shuffle with lag takes full advantage of large and fast L1 cache, without slowing down CPU. As for GPU, they also have cache, but the maximum lag needs to be fine tuned to find a point where it's still large enough and doesn't slow down GPU.~~

~~Current maximum lag is 256 cache lines (16 KB of previously accessed data), but as I said, it can be reduced if GPUs perform bad.~~

### 3. Integer math modification

It adds one 64:32 bit integer division and one 48 bit integer square root per iteration.

Adding integer division and integer square roots to the main loop ramps up the complexity of ASIC/FPGA and silicon area needed to implement it, so they'll be much less efficient with the same transistor budget and/or power consumption. Most common hardware implementations of division and square roots require a lot of clock cycles of latency: at least 8 cycles for 64:32 bit division and the same 8 cycles for 48 bit square root. These latencies are achieved for the best and fastest hardware implementations I could find. And the way this modification is made ensures that division and square root from the same main loop iteration can't be done in parallel, so their latencies add up making it staggering 16 cycles per iteration in the best case, comparing to 1 cycle per iteration in the original Crytonight. Even though this latency can be hidden in pipelined implementations, it will require A LOT of logical elements/silicon area to implement. Hiding the latency will also require many parallel scratchpads which will be a strong limiting factor for hardware with on-chip memory. They just don't have enough memory to hide the latency entirely. My rough estimates show ~15x slowdown in this case.

Good news for CPU and GPU is that division and square roots can be added to the main loop in such a way that their latency is completely hidden, so again there is almost no slowdown.

# Performance

Overall, it seems that all Radeon cards will be in 85-90+% range of their CryptonightV1 performance in the end after the community has more time to get familiar with these mods and come up with tuning manuals. All GeForce cards will be in 90-95+% range of their CryptonightV1 performance, so their relative performance will improve a few percent comparing to Radeons. And there is always some room for further performance improvement in the mining software itself, so overall network hashrate will only drop 5-10%.

On the other side, ASIC/FPGA which use external memory for scratchpad will get 4 times slower due to increased bandwidth usage. ASIC/FPGA which use on-chip memory for scratchpad will get ~15 times slower because of high latencies introduced with division and square root calculations: they just don't have enough on-chip memory to hide these latencies with many parallel Cryptonight calculations.

**AMD Ryzen 7 1700 @ 3.6 GHz**, 8 threads

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 600.8 H/s|100.0%
INT_MATH | 588.0 H/s|97.9%
SHUFFLE | 586.6 H/s|97.6%
Both mods | 572.0 H/s|95.2%
shuffle_with_lag\* | 587.1 H/s|97.7%

**AMD Ryzen 5 2600 @ 3.9 GHz**, 1 thread

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 94.6 H/s|100.0%
INT_MATH | 92.3 H/s|97.6%
SHUFFLE | 92.2 H/s|97.5%
Both mods | 89.5 H/s|94.6%
shuffle_with_lag\* | 91.0 H/s|96.2%

**Intel Pentium G5400 (Coffee Lake, 2 cores, 4 MB Cache, 3.70 GHz)**, 2 threads

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 146.5 H/s|100.0%
INT_MATH | 141.0 H/s|96.2%
SHUFFLE | 145.3 H/s|99.2%
Both mods | 140.5 H/s|95.9%
shuffle_with_lag\* | 143.9 H/s|98.2%

**Intel Core i5 3210M (Ivy Bridge, 2 cores, 3 MB Cache, 2.80 GHz)**, 1 thread

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 71.6 H/s|100.0%
INT_MATH | 69.3 H/s|96.8%
SHUFFLE | 70.5 H/s|98.5%
Both mods | 66.7 H/s|93.2%
shuffle_with_lag\* | 70.3 H/s|98.2%

**Intel Core i7 7820X (Skylake-X, 8 cores, 11 MB Cache, 3.60 GHz)**, 1 thread

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 68.3 H/s|100.0%
INT_MATH | 65.9 H/s|96.5%
SHUFFLE | 67.3 H/s|98.5%
Both mods | 65.0 H/s|95.2%
shuffle_with_lag\* | 67.3 H/s|98.5%

\*shuffle_with_lag is not implemented for GPU yet

GPU performance was tested using the code from this repository: https://github.com/SChernykh/xmr-stak-amd

### XMR-STAK used is an old version, so don't expect the same numbers that you have on your mining rigs. What's important here are relative numbers of original and modified Cryptonight versions.

**Radeon RX 560** on Windows 10 (overclocked): core @ 1196 MHz, memory @ 2200 MHz, 1 Click PBE Timing Straps, monitor plugged in, intensity 1024, worksize 32:

Mod|Hashrate|Performance level
---|--------|-----------------
\- | 477.1 H/s|100.0%
INT_MATH | 448.3 H/s|94.0%
SHUFFLE | 458.7 H/s|96.1%
Both mods | 446.9 H/s|93.7%
Both mods, core @ 1450 MHz | 456.9 H/s|95.8%

**GeForce GTX 1080 Ti 11 GB** on Windows 10: core 2000 MHz, memory 11800 MHz, monitor plugged in, intensity 1280, worksize 8:

Mod|Hashrate|Performance level
---|--------|-----------------
\-|908.4 H/s|100.0%
INT_MATH|902.7 H/s|99.4%
SHUFFLE|848.6 H/s|93.4%
Both mods|846.7 H/s|93.2%

**GeForce GTX 1060 6 GB** on Windows 10: all stock, monitor plugged in, intensity 800, worksize 8:

Mod|Hashrate|Performance level
---|--------|-----------------
\-|453.6 H/s|100.0%
INT_MATH|452.2 H/s|99.7%
SHUFFLE|422.6 H/s|93.2%
Both mods|421.5 H/s|92.9%

**GeForce GTX 1050 2 GB** on Windows 10: core 1721 MHz, memory 1877 MHz, monitor unplugged, intensity 448, worksize 8:

Mod|Hashrate|Performance level
---|--------|-----------------
\-|319.9 H/s|100.0%
INT_MATH|318.1 H/s|99.4%
SHUFFLE|292.5 H/s|91.4%
Both mods|291.0 H/s|91.0%

**Results from \@mobilepolice**

**XFX RX560D 14CU** 1150Mhz Core 1750Mhz Mem https://pastebin.com/HC1TchsL

Best Result:

threads | intensity | worksize | unroll | math | shuffle | hashrate | performance
-- | -- | -- | -- | -- | -- | -- | --
2 | 448 | 8 | 4 | FALSE | TRUE | 503.6 | 100.0%



Best Result with Mods:

threads | intensity | worksize | unroll | math | shuffle | hashrate | performance
-- | -- | -- | -- | -- | -- | -- | --
2 | 448 | 8 | 8 | TRUE | TRUE | 447.3 | 88.8%
2 | 448 | 32 | 2 | TRUE | TRUE | 446.5 | 88.7%



**XFX RX560 16CU** 1150Mhz Core 1750Mhz Mem https://pastebin.com/pfajCwkC

Best Result:

threads | intensity | worksize | unroll | math | shuffle | hashrate | performance
-- | -- | -- | -- | -- | -- | -- | --
2 | 448 | 4 | 2 | FALSE | FALSE | 530.8 | 100.0%




Best Result with Mods:

threads | intensity | worksize | unroll | math | shuffle | hashrate | performance
-- | -- | -- | -- | -- | -- | -- | --
2 | 448 | 8 | 8 | TRUE | TRUE | 454.7 | 85.7%


**Sapphire RX550 8CU** 1325Mhz Core 1760Mhz Mem https://pastebin.com/NZbqLcV4

Best Result:

threads | intensity | worksize | unroll | math | shuffle | hashrate | performance
-- | -- | -- | -- | -- | -- | -- | --
2 | 448 | 16 | 8 | FALSE | TRUE | 496.1 | 100.0%




Best Result with Mods:

threads | intensity | worksize | unroll | math | shuffle | hashrate | performance
-- | -- | -- | -- | -- | -- | -- | --
2 | 448 | 16 | 4 | TRUE | TRUE | 444.1 | 89.5%

**Results from MoneroCrusher**

**RX 550 Gigabyte, 8 CU**, 2200 mckl, 1250 sckl, 1 click PBE timings, Ubuntu 16.04 LTS Server, Same settings as CN7 (low electricity consumption, ~40-45W at the wall)
Best Results (full results https://pastebin.com/Nr2N139a)

Mod | Threads/Intensity | Worksize | Unroll | Hashrate | Performance Level
-- | -- | -- | -- | -- | --
Reference (latest xmr-stak) | 2/432 | 8 | - | 525 H/s | -
No Mod | 2/432 | 8 | - | 507.8 | 100%
SHUFFLE | 2/448 | 32 | 1 | 459.7 H/s | 90.5%
SHUFFLE | 2/448 | 32 | 8 | 459.7 H/s | 90.5%
INT_MATH | 2/448 | 16 | 8 | 464 H/s | 91.4%
Both mods | 2/448 | 32 | 1 | 417.2 H/s | 82.2%
Both mods | 2/448 | 32 | 8 | 415.2 H/s | 81.8%

Here I have to note that 432 intensity works better for no mods while 448 works better for mods (about 3% better).

**RX Vega 56, 56 CU**, 915mckl, 1590sckl, stock BIOS, Windows 10, 18.5.2 Drivers,
Best Results (full results: https://pastebin.com/kz0fYhrr)

Mod | Threads/Intensity | Worksize | Unroll | Hashrate | Performance Level
-- | -- | -- | -- | -- | --
Reference (latest xmr-stak) | 2/1800 | 8 | - | 1865 H/s | -
No Mod | 2/1800 | 16 | - | 1869 H/s | 100%
SHUFFLE | 2/1800 | 16 | 8 | 1714 H/s | 92.2%
INT_MATH | 2/1800 | 16 | 1 | 1763 H/s | 94.3%
Both mods | 2/1800 | 32 | 8 | 1595 H/s | 85.3%
