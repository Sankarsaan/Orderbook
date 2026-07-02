# High-Performance Order Matching Engine

A C++17 implementation of an ultra-low latency Limit Order Book (LOB) matching engine. This project focuses on mechanical sympathy and deterministic execution by systematically eliminating Operating System overhead during the critical trading path.

## Architecture & Optimizations

This engine deviates from standard library abstractions to achieve nanosecond-level performance:

1. **Zero-Allocation Object Pool (Arena Allocator)**
   Standard `new`/`malloc` calls introduce unpredictable OS-level context switches and heap fragmentation. This engine pre-allocates a massive contiguous block of `Order` structs at startup. During runtime, allocations and deallocations are strictly $O(1)$ pointer swaps.
   
2. **Intrusive Doubly-Linked Lists for O(1) Cancellations**
   Using `std::deque` or `std::list` for price levels forces $O(N)$ linear scans for order cancellations. By embedding `prev` and `next` pointers directly inside the `Order` struct and caching their exact memory addresses in an `std::unordered_map`, this engine unplugs cancelled orders from the queue in absolute $O(1)$ time.

3. **Optimized Market Order Sweeping**
   Market orders are processed independently from resting limit orders. They bypass the memory pool entirely, aggressively sweeping the opposing `std::map` price levels to maximize L1 cache locality and minimize execution branches.

## Core Financial Concepts

To understand the architecture, you must understand the rules of the exchange. This engine strictly enforces **Price-Time Priority**: orders are matched first by who offers the best price, and second by who arrived first.

* **The Order Book:** A centralized, electronic ledger that matches buyers (Bids) and sellers (Asks). Bids are sorted descending (highest price wins), and Asks are sorted ascending (lowest price wins). 
* **Limit Order:** An order with a strict price constraint. A Limit Buy at $100 means the trader will pay *no more* than $100. If no matching seller exists, the order "rests" in the book, providing liquidity.
* **Market Order:** An order driven by extreme urgency. It specifies a quantity but no price constraint. A Market Buy says "give me shares right now, sweeping through as many sellers as necessary until my quantity is filled." Market orders never rest in the book.

---

## How This Engine Handles Execution

The engine is fundamentally split between orders that *rest* and orders that *sweep*, treating memory management completely differently for both.

### 1. Handling Limit Orders (Resting)
When a Limit Order arrives:
1. **O(1) Allocation:** The engine requests an `Order` struct from the pre-allocated `OrderPool`, avoiding an expensive OS `new` call.
2. **Indexing:** The exact memory address of the `Order` is cached in an `std::unordered_map` tied to its unique Order ID.
3. **Insertion:** The order is pushed into the back of an Intrusive Doubly-Linked List residing at its specific price level inside the `std::map`.
4. **Matching:** The engine checks if the new order crosses the spread (e.g., a buyer offers more than the lowest seller). If it does, a trade executes, and filled orders are recycled back into the Object Pool.

### 2. Handling Market Orders (Sweeping)
Because Market Orders demand instant execution and never rest in the book, **they bypass the Object Pool entirely**. 
When a Market Order arrives, the engine simply takes its quantity and loops through the opposing side of the `std::map` (starting at the best price). It aggressively subtracts quantity from resting limit orders. If a resting order drops to 0 quantity, it is instantly unlinked and its memory is recycled. Any un-filled portion of a Market Order is immediately dropped, keeping the operation lightning-fast.

### 3. Handling Cancellations (O(1) Deletion)
Standard order books use `std::deque` or `std::vector` for price levels, meaning canceling an order forces an $O(N)$ linear scan to find it. This engine eliminates that bottleneck:
1. **Lookup:** The engine checks the `std::unordered_map` to get the exact memory address of the resting order instantly.
2. **Unplugging:** Because the order is a node in an Intrusive Doubly-Linked List, the engine simply rewires the `prev` and `next` pointers to bypass it. This removes the order from the queue in true $O(1)$ time.
3. **Recycling:** The unlinked pointer is immediately tossed back onto the Object Pool's free-list, ready to be used by the very next network packet.

## Performance Benchmarks

Benchmarking was conducted locally using a custom `std::chrono` high-resolution timing harness across millions of iterations. 

* **Compiler Flags:** `-O3 -march=native`
* **BM_AddLimitOrder (1,000,000 iterations):** ~164 ns / operation
* **BM_MarketOrderSweep (100,000 iterations):** ~80 ns / operation


## Concurrency: Lock-Free SPSC Ring Buffer
To simulate a real-world exchange pipeline, this engine operates across two independent threads: a Network thread (Producer) and a Matching Engine thread (Consumer). 

To avoid the massive latency overhead of OS-level `std::mutex` locks, the threads communicate exclusively through a custom Lock-Free Single-Producer Single-Consumer (SPSC) Ring Buffer. By utilizing C++17 `std::atomic` memory orders (`acquire` and `release`) and aligning pointers to 64-byte boundaries to prevent False Sharing, the system achieves safe, concurrent order routing in ~317 nanoseconds end-to-end.

## Build Instructions

This project requires a C++17 compatible compiler (GCC/Clang).

```bash
# Compile the functional simulation and the benchmarking suite
make all

# Run the performance benchmark
./run_benchmarks

# Run the functional logic simulation
./engine
