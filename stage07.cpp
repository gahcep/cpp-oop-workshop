#include <cassert>
#include <cstddef>
#include <iostream>
#include <memory>
#include <span>
#include <vector>

class IBuffer {
public:
    virtual ~IBuffer() = default;
    [[nodiscard]] virtual std::span<const std::byte> bytes() const noexcept = 0;
    [[nodiscard]] virtual std::unique_ptr<IBuffer> clone() const = 0;

    IBuffer(const IBuffer&) = delete;
    IBuffer& operator=(const IBuffer&) = delete;

protected:
    IBuffer() = default;
};

class MemoryBuffer final : public IBuffer {
public:
    explicit MemoryBuffer(std::span<const std::byte> source)
        : storage_(source.begin(), source.end()) {}

    std::span<const std::byte> bytes() const noexcept override { return storage_; }
    std::unique_ptr<IBuffer> clone() const override {
        return std::make_unique<MemoryBuffer>(bytes());
    }
private:
    std::vector<std::byte> storage_;
};

class BorrowedBuffer final : public IBuffer {
public:
    explicit BorrowedBuffer(std::span<const std::byte> source) : view_(source) {}

    std::span<const std::byte> bytes() const noexcept override { return view_; }
    std::unique_ptr<IBuffer> clone() const override {
        // Clone is an owning snapshot, regardless of the original storage type.
        return std::make_unique<MemoryBuffer>(view_);
    }
private:
    std::span<const std::byte> view_;
};

int main() {
    std::byte raw[]{std::byte{42}};
    std::unique_ptr<IBuffer> buffer = std::make_unique<BorrowedBuffer>(raw);
    auto snapshot = buffer->clone();
    raw[0] = std::byte{7};
    assert(buffer->bytes()[0] == std::byte{7});
    assert(snapshot->bytes()[0] == std::byte{42});
    std::cout << "virtual dispatch: borrowed view and owning snapshot\n";
}
