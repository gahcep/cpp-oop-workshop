#include <array>
#include <cassert>
#include <concepts>
#include <cstddef>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

template<class Storage, class T>
concept ContiguousStorage = requires(Storage& storage, const Storage& immutable) {
    { storage.data() } -> std::same_as<T*>;
    { immutable.data() } -> std::same_as<const T*>;
    { immutable.size() } -> std::convertible_to<std::size_t>;
};

template<class T, class Storage>
    requires ContiguousStorage<Storage, T>
class BasicBuffer {
public:
    template<class... Args>
        requires std::constructible_from<Storage, Args...>
    explicit constexpr BasicBuffer(Args&&... args)
        noexcept(std::is_nothrow_constructible_v<Storage, Args...>)
        : storage_(std::forward<Args>(args)...) {}

    constexpr std::size_t size() const noexcept(noexcept(storage_.size())) {
        return storage_.size();
    }
    constexpr T& operator[](std::size_t index) & { return storage_.data()[index]; }

private:
    Storage storage_;
};

constexpr bool fixed_buffer_works() {
    BasicBuffer<std::byte, std::array<std::byte, 4>> buffer;
    buffer[0] = std::byte{42};
    return buffer.size() == 4 && buffer[0] == std::byte{42};
}

static_assert(fixed_buffer_works());

int main() {
    BasicBuffer<std::byte, std::vector<std::byte>> dynamic(8u);
    assert(dynamic.size() == 8);
    std::cout << "same interface: fixed storage at compile time, vector at runtime\n";
}
