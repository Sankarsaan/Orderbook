#pragma once
#include <cstdint>
#include "order.hpp"

using namespace std;

struct OrderCommand {
    uint64_t id;
    Side side;
    OrderType type;
    double price;
    uint32_t qty;
};