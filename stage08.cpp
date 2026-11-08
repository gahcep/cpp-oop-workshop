#include <cassert>
#include <cstddef>
#include <iostream>
#include <typeinfo>

struct BufferIdentity {
    virtual ~BufferIdentity() = default;
    virtual int identifier() const = 0;
};

struct Readable : virtual BufferIdentity {
    virtual std::size_t size() const = 0;
};

struct Writable : virtual BufferIdentity {
    virtual void resize(std::size_t size) = 0;
};

class ByteBuffer final : public Readable, public Writable {
public:
    ~ByteBuffer() override;
    int identifier() const override { return 7; }
    std::size_t size() const override { return size_; }
    void resize(std::size_t size) override { size_ = size; }
private:
    std::size_t size_{};
};

ByteBuffer::~ByteBuffer() = default;

// Inspect this function at -O0 and -O2. No direct call to ByteBuffer here.
std::size_t dispatch(const Readable& buffer) { return buffer.size(); }

int main() {
    ByteBuffer object;
    Readable* reader = &object;
    Writable* writer = &object;
    BufferIdentity* identity = reader;
    writer->resize(64);
    assert(dispatch(*reader) == 64);
    assert(dynamic_cast<Writable*>(reader) == writer);
    assert(dynamic_cast<void*>(writer) == static_cast<void*>(&object));
    assert(identity == static_cast<BufferIdentity*>(writer));

    std::cout << "object   " << static_cast<void*>(&object)
              << "\nreadable " << static_cast<void*>(reader)
              << "\nwritable " << static_cast<void*>(writer)
              << "\nidentity " << static_cast<void*>(identity)
              << "\nRTTI     " << typeid(*reader).name() << '\n';
}
