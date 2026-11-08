#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

class ByteBuffer {
public:
    explicit ByteBuffer(std::size_t size)
        : size_(size), data_(size ? new std::byte[size]{} : nullptr) {}
    ~ByteBuffer() { delete[] data_; }

    ByteBuffer(const ByteBuffer& other) : ByteBuffer(other.size_) {
        if (size_) std::copy_n(other.data_, size_, data_);
    }

    ByteBuffer& operator=(const ByteBuffer& other) {
        if (this != &other) {
            ByteBuffer copy(other);
            swap(copy);
        }
        return *this;
    }

    ByteBuffer(ByteBuffer&& other) noexcept
        : size_(std::exchange(other.size_, 0)),
          data_(std::exchange(other.data_, nullptr)) {}

    ByteBuffer& operator=(ByteBuffer&& other) noexcept {
        if (this != &other) {
            ByteBuffer incoming(std::move(other));
            swap(incoming);
        }
        return *this;
    }

    void swap(ByteBuffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

    std::size_t size() const noexcept { return size_; }
    const std::byte* data() const noexcept { return data_; }

private:
    std::size_t size_;
    std::byte* data_;
};

static_assert(std::is_nothrow_move_constructible_v<ByteBuffer>);
static_assert(std::is_nothrow_move_assignable_v<ByteBuffer>);

int main() {
    ByteBuffer a{8};
    const auto* initial_storage = a.data();
    ByteBuffer b{std::move(a)};
    assert(a.size() == 0 && a.data() == nullptr);
    assert(b.data() == initial_storage);
    a = std::move(b);
    assert(b.size() == 0 && a.data() == initial_storage);

    std::vector<ByteBuffer> buffers;
    buffers.push_back(std::move(a));
    buffers.reserve(buffers.capacity() + 1);
    assert(buffers.front().size() == 8);
    std::cout << "moves preserve the allocation; moved-from buffer is empty\n";
}
