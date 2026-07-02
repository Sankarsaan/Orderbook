#pragma once
#include <atomic>
#include <vector>
#include <cstddef>

using namespace std;

template <typename T>
class SPSCRingBuffer {
private:
    vector<T> buffer;
    const size_t capacity;
    alignas(64) atomic<size_t> head{0};
    alignas(64) atomic<size_t> tail{0};

public:
    SPSCRingBuffer(size_t cap) : buffer(cap), capacity(cap) {}

    bool push(const T& item) {
        size_t current_head = head.load(memory_order_relaxed);
        size_t next_head = (current_head + 1) % capacity;
        
        if (next_head == tail.load(memory_order_acquire)) {
            return false;
        }
        
        buffer[current_head] = item;
        head.store(next_head, memory_order_release);
        return true;
    }

    bool pop(T& item) {
        size_t current_tail = tail.load(memory_order_relaxed);
        
        if (current_tail == head.load(memory_order_acquire)) {
            return false;
        }
        
        item = buffer[current_tail];
        tail.store((current_tail + 1) % capacity, memory_order_release);
        return true;
    }
};