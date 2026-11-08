#include <cassert>
#include <cstddef>
#include <iostream>
#include <type_traits>

class ByteBuffer {
public:
    explicit ByteBuffer(std::size_t size)
        : size_(size), data_(size ? new std::byte[size]{} : nullptr) {}

    ~ByteBuffer() { delete[] data_; }

    ByteBuffer(const ByteBuffer&) = delete;
    ByteBuffer& operator=(const ByteBuffer&) = delete;

    std::size_t size() const { return size_; }
    std::byte& operator[](std::size_t index) { return data_[index]; }

private:
    std::size_t size_;
    std::byte* data_;
};

static_assert(!std::is_copy_constructible_v<ByteBuffer>);

int main() {
    ByteBuffer empty{0};
    ByteBuffer buffer{4};
    buffer[0] = std::byte{42};
    assert(empty.size() == 0 && buffer[0] == std::byte{42});
    std::cout << "owned bytes: " << buffer.size() << '\n';
    // Exercise: predict what a generated shallow copy would do before enabling it.
}
