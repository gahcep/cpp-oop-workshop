#include <cassert>
#include <cstddef>
#include <iostream>
#include <span>
#include <vector>

class ByteBuffer {
public:
    explicit ByteBuffer(std::size_t count) : bytes_(count) {}
    [[nodiscard]] std::size_t size() const noexcept { return bytes_.size(); }
    std::byte& at(std::size_t index) & { return bytes_.at(index); }
    const std::byte& at(std::size_t index) const & { return bytes_.at(index); }
    const std::byte& at(std::size_t) const && = delete;

    [[nodiscard]] std::span<std::byte> bytes() & noexcept { return bytes_; }
    [[nodiscard]] std::span<const std::byte> bytes() const & noexcept { return bytes_; }
    std::span<const std::byte> bytes() const && = delete;

    friend bool operator==(const ByteBuffer&, const ByteBuffer&) = default;

private:
    std::vector<std::byte> bytes_;
};

template<class T>
concept HasTemporaryView = requires { T{1}.bytes(); };

static_assert(!HasTemporaryView<ByteBuffer>);

int main() {
    ByteBuffer original{4};
    original.at(0) = std::byte{42};
    ByteBuffer copy = original;
    assert(copy == original);
    copy.bytes()[0] = std::byte{7};
    assert(copy != original);
    std::cout << "Rule of Zero: copy is independent; view borrows storage\n";
}
