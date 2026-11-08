#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>
#include <utility>

class ByteBuffer {
public:
    // Deterministic fault injection for this single-threaded lab.
    static inline bool fail_next_allocation = false;

    explicit ByteBuffer(std::size_t size) : size_(size), data_(allocate(size)) {}
    ~ByteBuffer() { delete[] data_; }

    ByteBuffer(const ByteBuffer& other) : ByteBuffer(other.size_) {
        if (size_) std::copy_n(other.data_, size_, data_);
    }

    ByteBuffer& operator=(const ByteBuffer& other) {
        if (this != &other) {
            ByteBuffer temporary(other);
            swap(temporary);
        }
        return *this;
    }

    void swap(ByteBuffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

    std::size_t size() const { return size_; }
    std::byte& operator[](std::size_t index) { return data_[index]; }

private:
    static std::byte* allocate(std::size_t size) {
        if (!size) return nullptr;
        if (std::exchange(fail_next_allocation, false)) throw std::bad_alloc{};
        return new std::byte[size]{};
    }
    std::size_t size_;
    std::byte* data_;
};

int main() {
    ByteBuffer source{4};
    source[0] = std::byte{7};
    ByteBuffer target = source;
    target[0] = std::byte{9};
    assert(source[0] == std::byte{7});

    ByteBuffer::fail_next_allocation = true;
    try {
        target = source;
        return 1;
    } catch (const std::bad_alloc&) {
        assert(target.size() == 4 && target[0] == std::byte{9});
        std::cout << "copy failed: target unchanged\n";
    }
    target = source;
    assert(target[0] == std::byte{7});
}
