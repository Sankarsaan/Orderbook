#pragma once
#include "order.hpp"
#include <vector>
#include <stdexcept>

class OrderPool {
private:
    std::vector<Order> pool;
    std::vector<Order*> free_list; // Acts as a stack of available pointers

public:
    OrderPool(size_t capacity) {
        pool.resize(capacity);
        free_list.reserve(capacity);
        // Push all addresses into the free list
        for (size_t i = 0; i < capacity; ++i) {
            free_list.push_back(&pool[i]);
        }
    }

    Order* allocate(uint64_t id, Side side, OrderType type, double price, uint32_t qty) {
        if (free_list.empty()) {
            throw std::runtime_error("OrderPool exhausted!");
        }
        Order* order = free_list.back();
        free_list.pop_back();
        order->reset(id, side, type, price, qty);
        return order;
    }

    void deallocate(Order* order) {
        free_list.push_back(order);
    }
};