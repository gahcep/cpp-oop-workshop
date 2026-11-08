#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <type_traits>

struct BufferRecord {
    std::uint8_t id{};
    std::size_t count{};
};

class ByteBuffer {
public:
    static constexpr std::size_t limit = 4096;
    std::size_t byte_count() const;
    std::uint8_t& identifier() { return id_; }
    const std::uint8_t& identifier() const { return id_; }

    bool set_byte_count(std::size_t count) {
        if (count > limit) return false;
        count_ = count;
        return true;
    }

    void print_addresses() const {
        std::cout << "object=" << static_cast<const void*>(this)
                  << " id=" << static_cast<const void*>(&id_)
                  << " count=" << static_cast<const void*>(&count_) << '\n';
    }

private:
    std::uint8_t id_{};
    std::size_t count_{};
};

// Out-of-line definition gives nm a symbol to inspect.
std::size_t ByteBuffer::byte_count() const { return count_; }

static_assert(std::is_aggregate_v<BufferRecord>);
static_assert(!std::is_aggregate_v<ByteBuffer>);
static_assert(std::is_standard_layout_v<ByteBuffer>);
static_assert(std::is_trivially_copyable_v<ByteBuffer>);
static_assert(!std::is_trivially_default_constructible_v<ByteBuffer>);

int main() {
    ByteBuffer buffer;
    assert(buffer.byte_count() == 0);
    const bool accepted = buffer.set_byte_count(100);
    const bool rejected = !buffer.set_byte_count(4097);
    assert(accepted && rejected && buffer.byte_count() == 100);
    buffer.identifier() = 7;
    const ByteBuffer& observer = buffer;
    assert(observer.identifier() == 7);
    std::cout << "size=" << sizeof(ByteBuffer) << " alignment="
              << alignof(ByteBuffer) << '\n';
    buffer.print_addresses();
}
