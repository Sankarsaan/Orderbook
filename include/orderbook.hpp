#pragma once
#include "order.hpp"
#include "memory_pool.hpp"
#include <map>
#include <unordered_map>
#include <cstdint>

// A custom Doubly-Linked List manager for our intrusive pointers
struct OrderList {
    Order* head = nullptr;
    Order* tail = nullptr;

    bool empty() const { return head == nullptr; }
    void push_back(Order* order);
    void pop_front();
    void erase(Order* order); // O(1) unlinking
};

class OrderBook {
private:
    OrderPool pool;

    // Bids (highest price first), Asks (lowest price first)
    std::map<double, OrderList, std::greater<double>> bids;
    std::map<double, OrderList> asks;

    // Metadata Cache for O(1) lookup: Order ID -> Exact Memory Address
    std::unordered_map<uint64_t, Order*> order_map;

    void match();

public:
    // Pre-allocate 1,000,000 orders
    OrderBook(size_t pool_size = 2000000) : pool(pool_size) {}

    void add_order(uint64_t id, Side side, OrderType type, double price, uint32_t qty);
    void cancel_order(uint64_t id);
};