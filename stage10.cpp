#include <array>
#include <cassert>
#include <cstddef>
#include <expected>
#include <iostream>
#include <span>
#include <vector>

enum class ParseError { missing_length, length_mismatch };

class ByteBuffer {
public:
    static std::expected<ByteBuffer, ParseError> parse(std::span<const std::byte> frame) {
        if (frame.empty()) return std::unexpected(ParseError::missing_length);
        const auto size = std::to_integer<std::size_t>(frame.front());
        const auto payload = frame.subspan(1);
        if (size != payload.size()) return std::unexpected(ParseError::length_mismatch);
        return ByteBuffer(payload);
    }

#if defined(__cpp_explicit_this_parameter) && __cpp_explicit_this_parameter >= 202110L
    // Explicit object parameter preserves constness. The lvalue reference
    // rejects both const and nonconst temporaries.
    template<class Self>
    auto bytes(this Self& self) noexcept { return std::span{self.bytes_}; }
#else
    std::span<std::byte> bytes() & noexcept { return bytes_; }
    std::span<const std::byte> bytes() const & noexcept { return bytes_; }
    std::span<const std::byte> bytes() const && = delete;
#endif

private:
    explicit ByteBuffer(std::span<const std::byte> bytes) : bytes_(bytes.begin(), bytes.end()) {}
    std::vector<std::byte> bytes_;
};

constexpr unsigned first_byte(std::span<const std::byte, 1> data) {
    if consteval {
        return std::to_integer<unsigned>(data[0]);
    } else {
        return std::to_integer<unsigned>(data.front());
    }
}

constexpr std::array one{std::byte{42}};
static_assert(first_byte(one) == 42);

int main() {
    const std::array frame{std::byte{2}, std::byte{7}, std::byte{9}};
    auto parsed = ByteBuffer::parse(frame);
    assert(parsed && parsed->bytes().size() == 2);
    auto bad = ByteBuffer::parse(std::span{frame}.first(2));
    assert(!bad && bad.error() == ParseError::length_mismatch);
    assert(first_byte(one) == 42);
    std::cout << "C++23: std::expected, if consteval; ";
#if defined(__cpp_explicit_this_parameter) && __cpp_explicit_this_parameter >= 202110L
    std::cout << "explicit object parameter enabled\n";
#else
    std::cout << "ref-qualified accessor fallback\n";
#endif
}
