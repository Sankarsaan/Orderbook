#include "../include/orderbook.hpp"
#include "../include/spsc_queue.hpp"
#include "../include/order_command.hpp"
#include <iostream>
#include <thread>
#include <chrono>

using namespace std;

void network_producer(SPSCRingBuffer<OrderCommand>& ring_buffer, int num_orders) {
    for (int i = 0; i < num_orders; ++i) {
        OrderCommand cmd;
        cmd.id = i + 1;
        cmd.side = (i % 2 == 0) ? Side::BUY : Side::SELL;
        cmd.type = OrderType::LIMIT;
        cmd.price = 100.0 + (i % 10);
        cmd.qty = 10;

        while (!ring_buffer.push(cmd)) {
            
        }
    }
}

void engine_consumer(SPSCRingBuffer<OrderCommand>& ring_buffer, OrderBook& book, int num_orders) {
    int processed = 0;
    OrderCommand cmd;

    while (processed < num_orders) {
        if (ring_buffer.pop(cmd)) {
            book.add_order(cmd.id, cmd.side, cmd.type, cmd.price, cmd.qty);
            processed++;
        }
    }
}

int main() {
    const int NUM_ORDERS = 1000000;
    const int QUEUE_CAPACITY = 65536;

    SPSCRingBuffer<OrderCommand> ring_buffer(QUEUE_CAPACITY);
    OrderBook book(2000000);

    auto start = chrono::high_resolution_clock::now();

    thread producer_thread(network_producer, ref(ring_buffer), NUM_ORDERS);
    thread consumer_thread(engine_consumer, ref(ring_buffer), ref(book), NUM_ORDERS);

    producer_thread.join();
    consumer_thread.join();

    auto end = chrono::high_resolution_clock::now();
    chrono::duration<double, nano> elapsed = end - start;

    cout << "Processed " << NUM_ORDERS << " orders through SPSC pipeline.\n";
    cout << "Total pipeline time: " << elapsed.count() / 1000000.0 << " ms\n";
    cout << "Average latency per order: " << elapsed.count() / NUM_ORDERS << " ns\n";

    return 0;
}
