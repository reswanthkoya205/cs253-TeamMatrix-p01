#pragma once

#include <vector>

// Simple FIFO queue implemented with a vector and a moving front index.
// Each item is inserted and removed in O(1) amortized time.
class FifoQueue {
private:
    std::vector<int> data;
    std::size_t frontIndex = 0;

public:
    bool empty() const {
        return frontIndex == data.size();
    }

    void push(int value) {
        data.push_back(value);
    }

    int pop() {
        int value = data[frontIndex];
        ++frontIndex;
        return value;
    }
};
