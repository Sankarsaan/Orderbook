#include "../include/orderbook.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

void benchmark_limit_orders(int iterations) {
    OrderBook book;
    
    // Warm up the CPU cache and pre-allocate initial pool elements
    for(int i = 0; i < 1000; ++i) {
        book.add_order(i, Side::BUY, OrderType::LIMIT, 100.0, 10);
    }

    // Capture start time
    auto start = std::chrono::high_resolution_clock::now();
    
    // Execute the insertion operations
    for (int i = 1000; i < 1000 + iterations; ++i) {
        book.add_order(i, Side::BUY, OrderType::LIMIT, 100.0, 10);
    }
    
    // Capture end time
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> elapsed = end - start;
    
    double avg_time = elapsed.count() / iterations;
    
    std::cout << std::left << std::setw(30) << "BM_AddLimitOrder" 
              << std::setw(20) << avg_time << " ns" 
              << std::setw(15) << iterations << "\n";
}

void benchmark_market_sweeps(int iterations) {
    OrderBook book;
    
    // Pre-populate the book with resting Limit liquidity
    for(int i = 0; i < iterations; ++i) {
        book.add_order(i, Side::SELL, OrderType::LIMIT, 100.0 + (i * 0.01), 10);
    }
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Blast Market orders that sweep through the levels
    for (int i = 0; i < iterations; ++i) {
        book.add_order(iterations + i, Side::BUY, OrderType::MARKET, 0.0, 10);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::nano> elapsed = end - start;
    
    double avg_time = elapsed.count() / iterations;
    
    std::cout << std::left << std::setw(30) << "BM_MarketOrderSweep" 
              << std::setw(20) << avg_time << " ns" 
              << std::setw(15) << iterations << "\n";
}

int main() {
    std::cout << "=========================================================\n";
    std::cout << std::left << std::setw(30) << "Benchmark Target" 
              << std::setw(23) << "Latency (Avg)" 
              << std::setw(15) << "Iterations" << "\n";
    std::cout << "=========================================================\n";
    
    // Profile 1 million limit additions and 100k aggressive sweeps
    benchmark_limit_orders(1000000);
    benchmark_market_sweeps(100000);
    
    std::cout << "=========================================================\n";
    return 0;
}