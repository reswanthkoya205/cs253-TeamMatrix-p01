#pragma once

#include <vector>
#include <utility>
#include "counters.hpp"

// Handwritten min-heap of vertex IDs.
// The smallest vertex ID is always removed first.
class MinHeap {
private:
    std::vector<int> data;
    Counters* counters;

    bool smaller(int a, int b) {
        if (counters) ++counters->comparisons;
        return a < b;
    }

public:
    explicit MinHeap(Counters* c = nullptr) : counters(c) {}

    bool empty() const {
        return data.empty();
    }

    void push(int value) {
        data.push_back(value);
        if (counters) ++counters->heapPushes;

        int i = static_cast<int>(data.size()) - 1;
        while (i > 0) {
            int parent = (i - 1) / 2;
            if (!smaller(data[i], data[parent])) break;
            std::swap(data[i], data[parent]);
            i = parent;
        }
    }

    int pop() {
        int answer = data[0];
        if (counters) ++counters->heapPops;

        if (data.size() == 1) {
            data.pop_back();
            return answer;
        }

        data[0] = data.back();
        data.pop_back();

        int i = 0;
        int n = static_cast<int>(data.size());

        while (true) {
            int left = 2 * i + 1;
            int right = 2 * i + 2;
            int smallest = i;

            if (left < n && smaller(data[left], data[smallest]))
                smallest = left;

            if (right < n && smaller(data[right], data[smallest]))
                smallest = right;

            if (smallest == i) break;

            std::swap(data[i], data[smallest]);
            i = smallest;
        }

        return answer;
    }
};
