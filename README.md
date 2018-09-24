# This is a test repo for Cryptonight anti-ASIC/FPGA modifications

It's based on the old xmr-stak-cpu repo. Only "benchmark_mode config.txt" command line is supported.

### 1. Shuffle and add modification

Cryptonight is memory-intensive in terms of memory latency, but not bandwidth. Modern CPUs use 64-byte wide cache lines, but Cryptonight only loads/stores 16 bytes at a time, so 75% of available CPU cache bandwidth is wasted. ASICs are optimized for these 16 byte-wide memory accesses, so they always use 100% of whatever memory they have.

The idea is to do something computationally light and safe with the other 48 bytes of the same cache line (which is loaded in L1 cache anyway) on each step. Shuffle modification, as can be guessed by its name, treats these 48 bytes as 3 16-byte elements, shuffles them and performs 6x64-bit integer additions on them to make sure ASIC can't do it via simple rewiring.

The actual shuffle and add logic has 4 different cases depending on where in the 64-byte line the AES round is performed. Here are these 4 cases with variable names just like in the code. "+" operation on a 16-byte chunk means it's treated as 2 separate 64-bit unsigned integers.

\-|Bytes 0-15|Bytes 16-31|Bytes 32-47|Bytes 48-63
-|----------|-----------|-----------|-----------
Before|AES input|chunk1|chunk2|chunk3
After|AES output|chunk3+b1|chunk1+b|chunk2+a

\-|Bytes 0-15|Bytes 16-31|Bytes 32-47|Bytes 48-63
-|----------|-----------|-----------|-----------
Before|chunk1|AES input|chunk3|chunk2
After|chunk3+b1|AES output|chunk2+a|chunk1+b

\-|Bytes 0-15|Bytes 16-31|Bytes 32-47|Bytes 48-63
-|----------|-----------|-----------|-----------
Before|chunk2|chunk3|AES input|chunk1
After|chunk1+b|chunk2+a|AES output|chunk3+b1

\-|Bytes 0-15|Bytes 16-31|Bytes 32-47|Bytes 48-63
-|----------|-----------|-----------|-----------
Before|chunk3|chunk2|chunk1|AES input
After|chunk2+a|chunk1+b|chunk3+b1|AES output

The shuffle modification makes Cryptonight 4 times more demanding for memory bandwidth, making ASIC/FPGA 4 times slower\*. At the same time, CPU/GPU performance stays almost the same because this bandwidth is already there, it's just not used yet. Shuffle can also be done in parallel with existing Cryptonight calculations.

\* The 4 times slowdown applies only to devices (ASIC/FPGA) that use external memory for storing the scratchpad and saturate this memory's bandwidth. Devices that use on-chip memory have no problems with bandwidth, but they'll still have to do 4 times more memory reads/writes, so they'll also become somewhat slower.

### 2. Integer math modification

It adds one 64:32 bit integer division and one 64 bit integer square root per iteration.

Integer division is defined as follows:
1) `Divisor(bits 0-31) = (AES_ROUND_OUTPUT(bits 0-31) + sqrt_result * 2) | 0x80000001` where sqrt_result is 32-bit unsigned integer from the previous iteration. Divisor being dependent on `sqrt_result` creates dependency chain and ensures that two different iterations can't be run in parallel.
2) `Dividend(bits 0-63) = AES_ROUND_OUTPUT(bits 64-127)`
3) `Quotient(bits 0-31 of division_result) = (Dividend / Divisor)(bits 0-31)`
4) `Remainder(bits 32-63 of division_result) = (Dividend % Divisor)(bits 0-31)`

Square root is defined as follows:
1) `sqrt_input(bits 0-63) = AES_ROUND_OUTPUT(bits 0-63) + division_result(bits 0-63)`. Sqrt input being dependent on `division_result` creates dependency chain and ensures that sqrt can't be run in parallel with division.
2) `sqrt_result(bits 0-31) = Integer part of "sqrt(2^64 + sqrt_input) * 2 - 2^33"`

Both `division_result` and `sqrt_result` are blended in the main loop of Cryptonight as follows:
1) Data from second memory read is taken - it's 16 bytes, but only first 8 bytes are changed
2) Bits 0-63 are XOR'ed with bits 0-63 of `division_result`
3) Bits 32-63 are XOR'ed with bits 0-31 of `sqrt_result`
4) Changing bits 0-63 of the second memory read's data guarantees that every bit of `division_result` and `sqrt_result` influences all bits of the address for the next memory read and is also stored to the scratchpad - see the code of the original Cryptonight algorithm. ASIC can't skip calculating div+sqrt to continue the main loop.

Adding integer division and integer square roots to the main loop ramps up the complexity of ASIC/FPGA and silicon area needed to implement it, so they'll be much less efficient with the same transistor budget and/or power consumption. Most common hardware implementations of division and square roots require a lot of clock cycles of latency: at least 8 cycles for 64:32 bit division and the same 8 cycles for 64 bit square root. These latencies are achieved for the best and fastest hardware implementations I could find. And the way this modification is made ensures that division and square root from the same main loop iteration can't be done in parallel, so their latencies add up making it staggering 16 cycles per iteration in the best case, comparing to 1 cycle per iteration in the original Crytonight. Even though this latency can be hidden in pipelined implementations, it will require A LOT of logical elements/silicon area to implement. Hiding the latency will also require many parallel scratchpads which will be a strong limiting factor for hardware with on-chip memory. They just don't have enough memory to hide the latency entirely. My rough estimates show ~15x slowdown in this case.

Good news for CPU and GPU is that division and square roots can be added to the main loop in such a way that their latency is completely hidden, so again there is almost no slowdown.

# Performance

The following table summarizes performance level for different hardware, relative to current Monero PoW (CryptonightV1).

Source:

- https://github.com/fireice-uk/xmr-stak/issues/1851 for most of this table
- https://github.com/monero-project/monero/pull/4404#issuecomment-424084477 for Intel Core i7 7700

Hardware|Performance
-----------|-------------
AMD Ryzen 5 2600|99.5%
AMD Radeon RX 560|99.05%
Intel Core i7 7700|99.0%
Intel Core i7 2600k|91.9%
Intel Core i5 3210M|91.9%


Overall, it seems that all hardware will get a minimal performance hit.

On the other side, ASIC/FPGA which use external memory for scratchpad will get 4 times slower due to increased bandwidth usage. ASIC/FPGA which use on-chip memory for scratchpad will get ~15 times slower because of high latencies introduced with division and square root calculations: they just don't have enough on-chip memory to hide these latencies with many parallel Cryptonight calculations.
