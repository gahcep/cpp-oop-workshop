#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <type_traits>

struct Trace {
    const char* label;
    explicit Trace(const char* name) : label(name) { std::cout << "+ " << label << '\n'; }
    ~Trace() { std::cout << "- " << label << '\n'; }
};

class ByteBuffer {
public:
    ByteBuffer() : ByteBuffer(0) {}
    explicit ByteBuffer(std::size_t count) : count_(checked(count)) {
        std::cout << "constructor body: " << count_ << '\n';
    }
    std::size_t byte_count() const { return count_; }
    std::uint8_t identifier() const { return id_; }

private:
    static std::size_t checked(std::size_t count) {
        if (count > 4096) throw std::invalid_argument("byte count > 4096");
        return count;
    }
    Trace first_{"first member"};
    Trace second_{"second member"};
    std::uint8_t id_{};
    std::size_t count_{};
};

static_assert(std::is_constructible_v<ByteBuffer, std::size_t>);
static_assert(!std::is_convertible_v<std::size_t, ByteBuffer>);

int main() {
    {
        ByteBuffer empty;
        ByteBuffer sized{16};
        assert(empty.byte_count() == 0 && sized.byte_count() == 16);
        assert(sized.identifier() == 0);
    }
    try {
        ByteBuffer invalid{4097};
        return 1;
    } catch (const std::invalid_argument& error) {
        std::cout << "caught: " << error.what() << '\n';
    }
}
