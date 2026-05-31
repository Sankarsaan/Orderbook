#include "../include/orderbook.hpp"
#include <iostream>
#include <chrono>

int main() {
    OrderBook book;
    const int NUM_ORDERS = 1000000;

    std::cout << "Generating and matching " << NUM_ORDERS << " orders...\n";

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < NUM_ORDERS; ++i) {
        Side side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        double price = 100.0 + (i % 10); // Spreads orders across prices $100 - $109
        uint32_t qty = 10;
        
        book.add_order(i, side, OrderType::LIMIT, price, qty);
        
        // Simulate aggressive cancellations
        if (i % 5 == 0) {
            book.cancel_order(i - 1); 
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "Time elapsed: " << duration.count() << " ms\n";
    std::cout << "Throughput: " << (NUM_ORDERS / (duration.count() / 1000.0)) << " ops/sec\n";

    return 0;
}