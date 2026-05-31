#pragma once
#include <cstdint>

enum class Side { BUY, SELL };
enum class OrderType { LIMIT, MARKET };

struct Order {
    uint64_t id;
    Side side;
    OrderType type;
    double price;
    uint32_t quantity;

    // Intrusive Doubly-Linked List pointers
    Order* prev = nullptr;
    Order* next = nullptr;

    // Reset function for when the order is recycled by the Object Pool
    void reset(uint64_t _id, Side _side, OrderType _type, double _price, uint32_t _qty) {
        id = _id;
        side = _side;
        type = _type;
        price = _price;
        quantity = _qty;
        prev = nullptr;
        next = nullptr;
    }
};