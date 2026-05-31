#include "../include/orderbook.hpp"
#include <algorithm>
#include <iostream>

// --- Intrusive List Logic ---
void OrderList::push_back(Order* order) {
    if (!head) {
        head = tail = order;
    } else {
        tail->next = order;
        order->prev = tail;
        tail = order;
    }
}

void OrderList::pop_front() {
    if (!head) return;
    Order* old_head = head;
    head = head->next;
    if (head) head->prev = nullptr;
    else tail = nullptr;
}

void OrderList::erase(Order* order) {
    if (order->prev) order->prev->next = order->next;
    if (order->next) order->next->prev = order->prev;
    if (order == head) head = order->next;
    if (order == tail) tail = order->prev;
}

// --- Core Matching Engine ---
void OrderBook::add_order(uint64_t id, Side side, OrderType type, double price, uint32_t qty) {
    
    // ==========================================
    // 1. MARKET ORDER LOGIC (The Book Sweeper)
    // ==========================================
    if (type == OrderType::MARKET) {
        uint32_t remaining_qty = qty;

        if (side == Side::BUY) {
            // Sweep the Asks (Sellers) from lowest price to highest
            while (remaining_qty > 0 && !asks.empty()) {
                auto best_ask_it = asks.begin();
                OrderList& ask_queue = best_ask_it->second;
                Order* top_ask = ask_queue.head;

                uint32_t trade_qty = std::min(remaining_qty, top_ask->quantity);
                
                 //std::cout << "Market BUY matched " << trade_qty << " @ $" << best_ask_it->first << "\n";

                remaining_qty -= trade_qty;
                top_ask->quantity -= trade_qty;

                // Cleanup if the resting order is fully filled
                if (top_ask->quantity == 0) {
                    order_map.erase(top_ask->id);
                    ask_queue.pop_front();
                    pool.deallocate(top_ask); // Recycle the seller's memory!
                }
                // Remove price level if empty
                if (ask_queue.empty()) asks.erase(best_ask_it);
            }
        } else { // Side::SELL
            // Sweep the Bids (Buyers) from highest price to lowest
            while (remaining_qty > 0 && !bids.empty()) {
                auto best_bid_it = bids.begin();
                OrderList& bid_queue = best_bid_it->second;
                Order* top_bid = bid_queue.head;

                uint32_t trade_qty = std::min(remaining_qty, top_bid->quantity);
                
                 //std::cout << "Market SELL matched " << trade_qty << " @ $" << best_bid_it->first << "\n";

                remaining_qty -= trade_qty;
                top_bid->quantity -= trade_qty;

                if (top_bid->quantity == 0) {
                    order_map.erase(top_bid->id);
                    bid_queue.pop_front();
                    pool.deallocate(top_bid); 
                }
                if (bid_queue.empty()) bids.erase(best_bid_it);
            }
        }
        
        // If remaining_qty > 0 here, the order book ran out of liquidity.
        // The unfilled portion of the market order just vanishes.
        return; 
    }

    // ==========================================
    // 2. LIMIT ORDER LOGIC (Rests in the book)
    // ==========================================
    
    // O(1) Allocation from the pool
    Order* new_order = pool.allocate(id, side, type, price, qty);
    order_map[id] = new_order;

    if (side == Side::BUY) {
        bids[price].push_back(new_order);
    } else {
        asks[price].push_back(new_order);
    }

    // Try to match immediately (in case it crosses the spread)
    match();
}

void OrderBook::cancel_order(uint64_t id) {
    // 1. O(1) Lookup
    auto it = order_map.find(id);
    if (it == order_map.end()) return; // Not found

    Order* order = it->second;

    // 2. O(1) Unlink from the Intrusive List
    if (order->side == Side::BUY) {
        bids[order->price].erase(order);
        if (bids[order->price].empty()) bids.erase(order->price);
    } else {
        asks[order->price].erase(order);
        if (asks[order->price].empty()) asks.erase(order->price);
    }

    // 3. Remove from cache
    order_map.erase(id);

    // 4. O(1) Deallocate (Recycle memory)
    pool.deallocate(order);
}

void OrderBook::match() {
    while (!bids.empty() && !asks.empty()) {
        auto best_bid_it = bids.begin();
        auto best_ask_it = asks.begin();

        double best_bid_price = best_bid_it->first;
        double best_ask_price = best_ask_it->first;

        // If the spread is positive, no match is possible
        if (best_bid_price < best_ask_price) break;

        OrderList& bid_queue = best_bid_it->second;
        OrderList& ask_queue = best_ask_it->second;

        Order* top_bid = bid_queue.head;
        Order* top_ask = ask_queue.head;

        uint32_t trade_qty = std::min(top_bid->quantity, top_ask->quantity);

        // Execute trade (In a real system, you'd log this to a network feed)
        // std::cout << "Matched " << trade_qty << " @ $" << best_ask_price << "\n";

        top_bid->quantity -= trade_qty;
        top_ask->quantity -= trade_qty;

        // Handle fully filled orders
        if (top_bid->quantity == 0) {
            order_map.erase(top_bid->id);
            bid_queue.pop_front();
            pool.deallocate(top_bid); // Recycle!
        }
        if (top_ask->quantity == 0) {
            order_map.erase(top_ask->id);
            ask_queue.pop_front();
            pool.deallocate(top_ask); // Recycle!
        }

        // Clean up empty price levels to keep Red-Black tree traversals fast
        if (bid_queue.empty()) bids.erase(best_bid_it);
        if (ask_queue.empty()) asks.erase(best_ask_it);
    }
}