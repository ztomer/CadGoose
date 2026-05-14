#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <cstddef>
#include <type_traits>
#include <utility>

template <typename T, size_t N>
class RingBuffer {
    static_assert(N > 0, "RingBuffer size must be > 0");
    T buf[N];
    size_t head = 0;
    size_t tail = 0;
    bool full = false;

public:
    void push(T val) {
        buf[head] = val;
        head = (head + 1) % N;
        if (full) tail = (tail + 1) % N;
        full = (head == tail);
    }

    void pop() {
        if (empty()) return;
        tail = (tail + 1) % N;
        full = false;
    }

    T& front() { return buf[tail]; }
    const T& front() const { return buf[tail]; }

    T& back() { return buf[(head + N - 1) % N]; }
    const T& back() const { return buf[(head + N - 1) % N]; }

    T& operator[](size_t i) { return buf[(tail + i) % N]; }
    const T& operator[](size_t i) const { return buf[(tail + i) % N]; }

    bool empty() const { return !full && head == tail; }
    bool isFull() const { return full; }
    size_t size() const {
        if (full) return N;
        return (head + N - tail) % N;
    }
    static constexpr size_t capacity() { return N; }

    void clear() { head = tail = 0; full = false; }

    class Iter {
        RingBuffer& rb;
        size_t idx;
        size_t count;
    public:
        Iter(RingBuffer& r, size_t i, size_t c) : rb(r), idx(i), count(c) {}
        T& operator*() { return rb.buf[(rb.tail + idx) % N]; }
        Iter& operator++() { ++idx; return *this; }
        bool operator!=(const Iter& o) const { return idx != o.idx; }
    };

    Iter begin() { return Iter(*this, 0, size()); }
    Iter end() { return Iter(*this, size(), size()); }

    class ConstIter {
        const RingBuffer& rb;
        size_t idx;
    public:
        ConstIter(const RingBuffer& r, size_t i) : rb(r), idx(i) {}
        const T& operator*() const { return rb.buf[(rb.tail + idx) % N]; }
        ConstIter& operator++() { ++idx; return *this; }
        bool operator!=(const ConstIter& o) const { return idx != o.idx; }
    };

    ConstIter begin() const { return ConstIter(*this, 0); }
    ConstIter end() const { return ConstIter(*this, size()); }
};

#endif
