# Практикум C++: как шаг за шагом написать `ByteBuffer`

В начале `ByteBuffer` — два поля и несколько методов. Затем он получает собственное хранилище, учится копироваться и перемещаться, после чего мы передаём владение стандартной библиотеке. В конце исследуем виртуальные вызовы и ABI, обобщаем хранилище через шаблоны и добавляем фабрику на C++23.

**Как работать.** Начните с минимального примера ниже. В каждой фазе вносите изменения по одному: предскажите результат, соберите, запустите и сравните наблюдение с ожиданием. Полный листинг после шагов — рабочая контрольная версия. Если ведёте один `byte_buffer.cpp`, целиком заменяйте его содержимое при переходе к контрольной версии: каждый листинг содержит свой `main()`. Если ведёте отдельные файлы, используйте имена `stage01.cpp`–`stage10.cpp`. Фазы 7–8 — отдельные эксперименты с полиморфизмом; фаза 9 возвращается к модели значения из фазы 6.

Команды `make` предполагают, что вы находитесь в `/Users/user/Mentorship/cpp-oop-workshop` и используете `Makefile` из комплекта. Для одного файла на macOS достаточно:

```bash
clang++ -std=c++20 -O0 -g -Wall -Wextra -Wpedantic byte_buffer.cpp -o byte_buffer
./byte_buffer
```

В фазе 10 замените `-std=c++20` на `-std=c++23`. Для лабораторных проверок не задавайте `-DNDEBUG`: он отключает `assert`.

## Минимальная точка входа

Сохраните этот код как `byte_buffer.cpp` и запустите командой выше. Пока поля публичные, поэтому можно сразу создать объект с двумя значениями и сравнить размеры объекта и его полей. Затем перейдите к фазе 1.

<!-- source: starter.cpp -->
```cpp
#include <cstddef>
#include <cstdint>
#include <iostream>

class ByteBuffer {
public:
    std::uint8_t identifier{};
    std::size_t byte_count{};
};

int main() {
    ByteBuffer buffer{7, 100};
    std::cout << "bytes=" << buffer.byte_count
              << ", object size=" << sizeof(buffer) << '\n';
}
```

## Фаза 01. Простой класс и его представление

**Цель.** Начните с двух полей и выведите `sizeof`. Пока `ByteBuffer` описывает данные; памяти под полезную нагрузку у него нет.

**Что добавить по шагам:**

1. Объявите `BufferRecord` с публичными полями и создайте его как `BufferRecord{7, 100}`. Затем используйте те же типы полей в `class ByteBuffer` с закрытым состоянием.
2. Инициализируйте поля через `{}`. Добавьте `byte_count() const`, общий `static constexpr` предел и `set_byte_count()`: число выше предела должно отклоняться без изменения предыдущего значения.
3. Добавьте две перегрузки `identifier()` для обычного и `const` объекта. Оставьте доступ к `count_` через проверяющий метод. Объясните, почему такой же изменяемый доступ к `count_` нарушил бы инвариант.
4. В `main()` проверьте инвариант. Затем выведите размер, выравнивание и адреса полей, добавьте `static_assert` для свойств типов и вынесите определение `byte_count()` из класса для анализа символов.

**Запуск:** `make CXX=clang++ run-01`

**Ожидаемый результат.** После принятия `100` и отказа для `4097` размер остаётся `100`. В типичной 64-битной конфигурации `sizeof(ByteBuffer) == 16`, выравнивание — `8`, а смещение счётчика — `8`; проверьте результат на своей машине.

**Для обсуждения.** Поменяйте порядок полей и предскажите, где окажется padding. Почему `BufferRecord` — агрегат, а `ByteBuffer` при этом может быть standard-layout и trivially copyable?

**Полный пример: `stage01.cpp`**

<!-- source: stage01.cpp -->
```cpp
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
```

## Фаза 02. Конструкторы, explicit и порядок инициализации

**Цель.** Перенесите проверку начального количества байт в конструктор. Ошибочный параметр теперь означает, что объект не будет создан.

**Что добавить по шагам:**

1. Добавьте `explicit ByteBuffer(std::size_t)` и инициализируйте `count_` в списке инициализации через проверяющую функцию.
2. Добавьте конструктор без аргументов, делегирующий в конструктор с размером `0`. Сравните прямую и копирующую инициализацию.
3. Временно добавьте два поля `Trace`, которые печатают сообщения при создании и уничтожении. Создайте два корректных объекта в блоке, затем попробуйте создать объект с размером `4097`.

**Запуск:** `make CXX=clang++ run-02`

**Ожидаемый результат.** Поля `Trace` создаются в порядке объявления и уничтожаются в обратном. Если проверка размера выбрасывает исключение, оба уже созданных `Trace` уничтожаются, а тело конструктора буфера не выполняется.

**Для обсуждения.** Почему `ByteBuffer b{16};` допустимо, а `ByteBuffer b = 16;` — нет? Какие подобъекты уже существуют к моменту ошибки и почему?

**Полный пример: `stage02.cpp`**

<!-- source: stage02.cpp -->
```cpp
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
```

## Фаза 03. Владение памятью и RAII

**Цель.** Замените описатель на буфер, который действительно владеет байтами. С этого этапа идентификатор и учебный предел 4096 не нужны: фокус смещается на владение.

**Что добавить по шагам:**

1. Добавьте `size_` и `data_`. Для ненулевого размера выделите массив `std::byte[]`, для нулевого оставьте указатель `nullptr`.
2. Освободите массив в деструкторе через `delete[]`. Удалите копирующий конструктор и копирующее присваивание, пока не определили глубокое копирование.
3. Добавьте минимальный доступ по индексу. Зафиксируйте его предусловие: индекс меньше размера. В `main()` запишите байт и прочитайте его обратно.

**Запуск:** `make CXX=clang++ run-03`

**Ожидаемый результат.** Буфер из четырёх байт хранит записанное значение; пустой буфер не выделяет массив. Копирование не компилируется.

**Для обсуждения.** Нарисуйте два объекта после гипотетического копирования поля `data_`. Какой адрес попал бы в оба объекта и что произошло бы при их уничтожении?

**Полный пример: `stage03.cpp`**

<!-- source: stage03.cpp -->
```cpp
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
```

## Фаза 04. Глубокое копирование и гарантия при исключении

**Цель.** Разрешите копирование: каждый объект должен владеть отдельным массивом.

**Что добавить по шагам:**

1. Добавьте копирующий конструктор: выделите новый массив того же размера и скопируйте байты. Сохраните деструктор из фазы 3.
2. Добавьте `swap() noexcept`, который меняет местами размер и указатель. Для копирующего присваивания сначала создайте временную копию, затем обменяйте её состояние с целевым объектом.
3. В учебной функции выделения памяти добавьте однократный управляемый сбой `std::bad_alloc`. Проверьте независимость копий, успешное присваивание и состояние цели при сбое.

**Запуск:** `make CXX=clang++ run-04`

**Ожидаемый результат.** Если источник содержит `7`, а цель — `9`, то после неудачного присваивания цель по-прежнему содержит `9`. После успешного присваивания она содержит `7`.

**Для обсуждения.** В какой строке операция меняет целевой объект? Кто освобождает старый массив цели после успешного обмена?

**Полный пример: `stage04.cpp`**

<!-- source: stage04.cpp -->
```cpp
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>
#include <utility>

class ByteBuffer {
public:
    // Deterministic fault injection for this single-threaded lab.
    static inline bool fail_next_allocation = false;

    explicit ByteBuffer(std::size_t size) : size_(size), data_(allocate(size)) {}
    ~ByteBuffer() { delete[] data_; }

    ByteBuffer(const ByteBuffer& other) : ByteBuffer(other.size_) {
        if (size_) std::copy_n(other.data_, size_, data_);
    }

    ByteBuffer& operator=(const ByteBuffer& other) {
        if (this != &other) {
            ByteBuffer temporary(other);
            swap(temporary);
        }
        return *this;
    }

    void swap(ByteBuffer& other) noexcept {
        std::swap(size_, other.size_);
        std::swap(data_, other.data_);
    }

    std::size_t size() const { return size_; }
    std::byte& operator[](std::size_t index) { return data_[index]; }

private:
    static std::byte* allocate(std::size_t size) {
        if (!size) return nullptr;
        if (std::exchange(fail_next_allocation, false)) throw std::bad_alloc{};
        return new std::byte[size]{};
    }
    std::size_t size_;
    std::byte* data_;
};

int main() {
    ByteBuffer source{4};
    source[0] = std::byte{7};
    ByteBuffer target = source;
    target[0] = std::byte{9};
    assert(source[0] == std::byte{7});

    ByteBuffer::fail_next_allocation = true;
    try {
        target = source;
        return 1;
    } catch (const std::bad_alloc&) {
        assert(target.size() == 4 && target[0] == std::byte{9});
        std::cout << "copy failed: target unchanged\n";
    }
    target = source;
    assert(target[0] == std::byte{7});
}
```

## Фаза 05. Перемещение и Rule of Five

**Цель.** Сохраните глубокое копирование и добавьте перенос владения без выделения нового массива.

**Что добавить по шагам:**

1. Добавьте перемещающий конструктор. Через `std::exchange` заберите указатель и размер, а источник переведите в пустое состояние.
2. Добавьте перемещающее присваивание: переместите источник во временный объект и обменяйтесь с ним состоянием. Обработайте случай присваивания самому себе.
3. Пометьте обе операции `noexcept`, проверьте это через type traits. Сравните адрес выделенного массива до и после переноса и попробуйте расширить `std::vector<ByteBuffer>`.

**Запуск:** `make CXX=clang++ run-05`

**Ожидаемый результат.** После переноса новый владелец хранит исходный адрес, старый владелец пуст. При расширении вектора содержимое сохраняется; для наблюдения выбора операции добавьте трассировку копирующего и перемещающего конструкторов.

**Для обсуждения.** Что именно делает `std::move`? Какие действия выполняет сам конструктор? Как изменится наблюдаемое поведение `vector::reserve`, если разрешить выбрасывать исключение из перемещающего конструктора?

**Полный пример: `stage05.cpp`**

<!-- source: stage05.cpp -->
```cpp
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
```

## Фаза 06. Rule of Zero и заимствованный доступ

**Цель.** Замените ручное владение на `std::vector<std::byte>` и отдайте клиентам представление `std::span`.

**Что добавить по шагам:**

1. Удалите указатель, отдельный размер и все пять написанных вручную операций. Храните только `std::vector<std::byte>` и получайте размер из него.
2. Добавьте проверяемый `at()`, `operator==` и перегрузки `bytes()` для изменяемого и константного объектов. Значения копируются вместе с вектором; `span` лишь ссылается на их хранилище.
3. Ограничьте выдачу ссылок и представлений объектами lvalue. Объявите перегрузку `const &&` с `= delete` и добавьте проверку, что вызов `bytes()` на временном объекте недоступен.

**Запуск:** `make CXX=clang++ run-06`

**Ожидаемый результат.** Изменение байта через представление копии не меняет оригинал. Попытка получить `span` из временного буфера отвергается компилятором.

**Для обсуждения.** Когда представление может стать недействительным даже после корректного вызова на lvalue? Что меняется при уничтожении владельца или замене его хранилища?

**Полный пример: `stage06.cpp`**

<!-- source: stage06.cpp -->
```cpp
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
```

## Фаза 07. Абстрактный интерфейс и виртуальные вызовы

**Цель.** Откройте отдельный эксперимент: общий интерфейс для владеющего буфера и представления на внешнюю память.

**Что добавить по шагам:**

1. Объявите `IBuffer` с чисто виртуальными `bytes()` и `clone()`, публичным виртуальным деструктором и запретом обычного копирования.
2. Реализуйте `MemoryBuffer` на векторе и `BorrowedBuffer` на `span`. Используйте `override` для проверки сигнатур и `final` для конкретных реализаций.
3. Определите контракт `clone()`: результат всегда владеет снимком байтов. Через `std::unique_ptr<IBuffer>` вызовите `clone()` и измените исходную внешнюю память.

**Запуск:** `make CXX=clang++ run-07`

**Ожидаемый результат.** Заимствованный буфер видит новое значение `7`, а владеющий снимок сохраняет прежнее `42`. Внешняя память в примере живёт дольше объекта, который её заимствует.

**Для обсуждения.** Почему копирование через `clone()` здесь может изменить конкретный тип? Зачем деструктор интерфейса виртуальный при удалении через `std::unique_ptr<IBuffer>`?

**Полный пример: `stage07.cpp`**

<!-- source: stage07.cpp -->
```cpp
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
```

## Фаза 08. RTTI, ромб наследования и ABI

**Цель.** Постройте отдельный эксперимент с подобъектами: эту иерархию не требуется переносить в прикладной буфер.

**Что добавить по шагам:**

1. Создайте общий виртуальный базовый класс `BufferIdentity`, интерфейсы `Readable` и `Writable` и реализующий их `ByteBuffer`.
2. Сравните адреса полного объекта и его базовых подобъектов. Проверьте боковое приведение `dynamic_cast<Writable*>(reader)` и получение адреса полного объекта через `dynamic_cast<void*>`.
3. Вынесите деструктор из определения класса, оставьте отдельную функцию `dispatch()` и исследуйте её вызов. Найдите таблицы, RTTI, VTT и функции корректировки `this` инструментами своей платформы.

**Запуск:** `make CXX=clang++ run-08`

**Ожидаемый результат.** Обе ветви дают доступ к одному `BufferIdentity`. Адреса базовых указателей могут различаться. На проверенной сборке Linux x86-64 видны construction vtables, VTT и thunks.

**Для обсуждения.** Что гарантирует язык, а что показывает конкретный ABI? Почему функция, получившая указатель на `Writable`, иногда должна скорректировать `this` перед вызовом реализации?

**Полный пример: `stage08.cpp`**

<!-- source: stage08.cpp -->
```cpp
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
```

## Фаза 09. Шаблон, concepts и политика хранения

**Цель.** Вернитесь к value type. Вынесите тип элемента и способ хранения в параметры `BasicBuffer`.

**Что добавить по шагам:**

1. Опишите `ContiguousStorage`: у хранилища должны быть подходящие `data()` и `size()`. Концепт проверяет сигнатуры; непрерывность и корректность хранилища остаются частью его контракта.
2. Добавьте шаблонный конструктор с `std::forward`, ограничением `std::constructible_from` и условным `noexcept`. Сделайте операции доступа `constexpr`, где это допускает выбранное хранилище.
3. Проверьте фиксированный буфер на `std::array` через `static_assert`, затем создайте динамический вариант на `std::vector`.

**Запуск:** `make CXX=clang++ run-09`

**Ожидаемый результат.** Для `array` размер задан типом и проверяется в compile time; для `vector` он передаётся при создании. Попытка подставить хранилище без `data()` не удовлетворяет ограничению.

**Для обсуждения.** Что концепт может проверить, а что приходится обещать в контракте? Почему `constexpr` разрешает вычисление при компиляции, но не требует его для каждого вызова?

**Полный пример: `stage09.cpp`**

<!-- source: stage09.cpp -->
```cpp
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
```

## Фаза 10. C++23: фабрика и явные ошибки

**Цель.** Используйте простой формат кадра: первый байт задаёт длину, остальные — полезную нагрузку. Фабрика проверяет кадр и создаёт владеющий `ByteBuffer`.

**Что добавить по шагам:**

1. Объявите `ParseError` и фабрику с результатом `std::expected<ByteBuffer, ParseError>`. Сначала проверьте пустой кадр, затем совпадение длины. Только после проверок копируйте данные в вектор.
2. Объедините const/nonconst доступ через explicit object parameter, если компилятор его поддерживает. Для остальных компиляторов оставьте перегрузки из фазы 6. В обоих вариантах представление выдаётся для lvalue.
3. Изучите `if consteval` на функции чтения одного байта. Две ветви в учебном примере дают одинаковое значение; `static_assert` проверяет вычисление при компиляции. Сравните его с вызовом, получающим данные во время исполнения.

**Запуск:** `make CXX=clang++ run-10`

**Ожидаемый результат.** Кадр `[2, 7, 9]` даёт буфер из двух байт, кадр `[2, 7]` — `length_mismatch`. Добавьте проверку пустого кадра (`missing_length`) и кадра `[0]` (успешный пустой буфер).

**Для обсуждения.** Почему `std::expected` описывает ошибки формата, но не предотвращает `std::bad_alloc` при создании вектора? В каком случае отдельные ветви `if consteval` были бы полезны в реальном проекте?

**Полный пример: `stage10.cpp`**

<!-- source: stage10.cpp -->
```cpp
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
```

## Как исследовать символы и виртуальный вызов

Для фаз 1 и 8 на macOS:

```bash
make CXX=clang++ run-01 run-08
nm -C build/stage01 | grep ByteBuffer
nm -C build/stage08 | grep -E 'vtable|VTT|thunk|typeinfo'
otool -tvV build/stage08
lldb build/stage08
```

В LLDB выполните `breakpoint set -n dispatch`, `run` и `disassemble --frame`. На Linux вместо `otool` используйте `objdump -dC build/stage08`. Сравнивая `-O0` и `-O2`, собирайте в разные каталоги. В `dispatch(const Readable&)` конкретный тип не известен из сигнатуры; в месте вызова компилятор иногда может вывести его и убрать косвенный вызов. На macOS и Linux формат объектных файлов и представление имён в инструментах могут различаться.

```bash
make CXX=clang++ BUILD=build-opt CXXFLAGS='-O2 -g -Wall -Wextra -Wpedantic' run-08
```

Если фаза 10 не компилируется на Apple Clang, проверьте версию компилятора и наличие `<expected>`. Инструкции по настройке LLVM есть в `README.md` комплекта. Успешная сборка фаз 1–9 ещё не подтверждает поддержку C++23.

**Итоговое задание.** Реализуйте фабрику фазы 10 для value type из фазы 6. Добавьте тесты на пустой ввод, пустую полезную нагрузку, корректный кадр и неверную длину. Для каждого возвращаемого `span` укажите владельца и момент, после которого им больше нельзя пользоваться. Иерархии фаз 7–8 оставьте отдельными примерами для исследования ABI.
