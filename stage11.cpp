#include <array>
#include <cassert>
#include <cstddef>
#include <expected>
#include <iostream>
#include <span>
#include <vector>

// Frame layout: the first byte declares the payload length, the remaining
// bytes are the payload. ParseError names the two ways a frame is rejected,
// and std::expected keeps the failure in the return value.
enum class ParseError { missing_length, length_mismatch };

class ByteBuffer {
public:
    ByteBuffer() = default;

    // The result owns a copy of the payload, so the caller's frame may be
    // modified or destroyed immediately after the call.
    [[nodiscard]] static std::expected<ByteBuffer, ParseError>
    parse(std::span<const std::byte> frame) {
        if (frame.empty()) return std::unexpected(ParseError::missing_length);
        const std::size_t declared = std::to_integer<std::size_t>(frame.front());
        const auto payload = frame.subspan(1);
        if (declared != payload.size()) {
            return std::unexpected(ParseError::length_mismatch);
        }
        return ByteBuffer{payload};
    }

    [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }

    // Views are lvalue-only: a span into a temporary would outlive nothing.
    [[nodiscard]] std::byte& at(std::size_t index) & { return bytes_.at(index); }
    [[nodiscard]] const std::byte& at(std::size_t index) const & {
        return bytes_.at(index);
    }
    const std::byte& at(std::size_t) const && = delete;

    [[nodiscard]] std::span<std::byte> bytes() & noexcept { return bytes_; }
    [[nodiscard]] std::span<const std::byte> bytes() const & noexcept {
        return bytes_;
    }
    std::span<const std::byte> bytes() const && = delete;

    friend bool operator==(const ByteBuffer&, const ByteBuffer&) = default;

private:
    explicit ByteBuffer(std::span<const std::byte> payload)
        : bytes_(payload.begin(), payload.end()) {}

    std::vector<std::byte> bytes_;
};

int main() {
    // 1. An empty frame has nowhere to read the length from.
    const auto missing = ByteBuffer::parse({});
    assert(!missing);
    assert(missing.error() == ParseError::missing_length);
    std::cout << "ok: empty frame -> missing_length\n";

    // 2. A declared length of zero is legal and owns no bytes.
    const std::array<std::byte, 1> zero{std::byte{0}};
    const auto empty = ByteBuffer::parse(zero);
    assert(empty);
    assert(empty->size() == 0 && empty->bytes().empty());
    std::cout << "ok: [0] -> empty buffer\n";

    // 3. A valid frame becomes an independent, owning copy.
    std::array<std::byte, 3> frame{std::byte{2}, std::byte{7}, std::byte{9}};
    auto parsed = ByteBuffer::parse(frame);
    assert(parsed && parsed->size() == 2);
    assert(parsed->at(0) == std::byte{7});
    assert(parsed->at(1) == std::byte{9});
    frame[1] = std::byte{99};
    assert(parsed->at(0) == std::byte{7});
    std::cout << "ok: [2, 7, 9] -> two owned bytes, later frame edits ignored\n";

    // 4. The declared length must match the bytes that follow it.
    const std::array<std::byte, 2> short_frame{std::byte{2}, std::byte{7}};
    const auto bad = ByteBuffer::parse(short_frame);
    assert(!bad);
    assert(bad.error() == ParseError::length_mismatch);
    std::cout << "ok: [2, 7] -> length_mismatch\n";

    // Lifetime: parsed->bytes() borrows parsed's vector. The span stays valid
    // while parsed lives and its storage is untouched; destroying parsed or
    // replacing the vector invalidates every span handed out before.
    const auto view = parsed->bytes();
    assert(view.size() == parsed->size() && view.data() == parsed->bytes().data());
    std::cout << "ok: span borrows the buffer and never outlives it\n";
}
