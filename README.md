# Практикум C++: 10 фаз развития ByteBuffer

Каждая фаза — самостоятельный файл с `main()`. Можно запускать любую фазу
отдельно и изменять её, не затрагивая остальные. Фазы 01–09 используют C++20;
фаза 10 требует C++23. Сторонних C++ библиотек нет.

Пошаговый разбор всех десяти фаз — цели, листинги, ожидаемый результат и
вопросы для обсуждения — лежит в
[`docs/cpp-mentorship-workshop.md`](docs/cpp-mentorship-workshop.md).

## Установка на macOS

Нужны компилятор C++ и редактор — сборку берёт на себя `Makefile`.

Проверьте инструменты в Terminal:

```sh
clang++ --version
make --version
```

`make` уже есть в системе. Если `clang++` не найден, поставьте LLVM через
[Homebrew][llvm]:

```sh
brew install llvm
```

Системный компилятор Homebrew не подменяет, поэтому путь к нему указывают явно:

```sh
make CXX="$(brew --prefix llvm)/bin/clang++" run-01
```

Распакуйте архив в папку для занятий (пример для загрузки в Downloads):

```sh
mkdir -p /Users/user/Mentorship
unzip -n "$HOME/Downloads/cpp-mentorship.zip" -d /Users/user/Mentorship
cd /Users/user/Mentorship/cpp-mentorship
clang++ --version
make --version
```

Если имя пользователя на Mac отличается, замените `/Users/user/Mentorship`
на `$HOME/Mentorship`. Опция `unzip -n` сохраняет существующие файлы.

Откройте папку комплекта в VS Code:

```sh
code /Users/user/Mentorship/cpp-mentorship
```

Если команда `code` недоступна, нажмите в VS Code `⇧⌘P` и выполните
`Shell Command: Install 'code' command in PATH`, либо откройте папку через
`File → Open Folder…`.

Для работы в редакторе поставьте два расширения:

| Расширение | Зачем |
|---|---|
| **C/C++** (`ms-vscode.cpptools`) | IntelliSense: подсказки, переходы к определениям, диагностика |
| **CodeLLDB** (`vadimcn.vscode-lldb`) | отладка через LLDB: точки останова, шаг с заходом, переменные |

Сборку и запуск выполняйте во встроенном терминале VS Code
(`Terminal → New Terminal`) командами `make` из раздела «Запуск»: отдельная
конфигурация сборки не нужна, всё описано в `Makefile`. Для отладки выберите
`Run and Debug` с CodeLLDB и цель `build/stage08` — либо запустите
`lldb build/stage08` в терминале.

## Запуск

```sh
make CXX=clang++ run-01     # собрать и запустить первую фазу
make CXX=clang++ run-08     # запустить отдельную фазу ABI
make CXX=clang++ check      # собрать и запустить все десять фаз
```

При запуске без Makefile:

```sh
clang++ -std=c++20 -O0 -g -Wall -Wextra -Wpedantic stage01.cpp -o /tmp/mentorship-stage01
/tmp/mentorship-stage01
```

## Если фаза 10 не собирается

Поддержка `-std=c++23` и наличие `std::expected` в стандартной библиотеке
проверяются отдельно: `make CXX=clang++ run-10` проверяет оба требования.
Если установленный Apple Clang или его стандартная библиотека слишком старые,
поставьте свежий LLVM через [Homebrew][llvm] (см. раздел «Установка на macOS»).
Если Homebrew отсутствует, установите его по инструкции на [brew.sh][brew].

```sh
make CXX="$(brew --prefix llvm)/bin/clang++" BUILD=build-llvm check
```

Отдельный каталог сборки не даст Make переиспользовать бинарники,
собранные другим компилятором. При смене флагов в том же каталоге
выполните `make clean` или выберите новый `BUILD`.

Если компилятор поддерживает `std::expected` и `if consteval`, но ещё не
поддерживает explicit object parameter, фаза 10 автоматически использует
обычные перегрузки и сообщает об этом при запуске. Доступность C++23 зависит
от версии компилятора, SDK и стандартной библиотеки.

## Фазы и задания

| Фаза | Пример | Что проверить или изменить |
|---|---|---|
| 01 | Метаданные, инвариант, const, layout | Предскажите размер и смещения; переставьте поля; объясните разницу между aggregate и standard-layout. |
| 02 | Конструкторы, explicit, порядок инициализации | Предскажите порядок сообщений, включая неудачный конструктор; сравните прямую и копирующую инициализацию. |
| 03 | Владение памятью и RAII | Попробуйте скопировать буфер; объясните диагностику и нарисуйте результат гипотетического поверхностного копирования. |
| 04 | Rule of Three и copy-and-swap | Объясните, почему ошибка выделения памяти оставляет целевой объект неизменным. |
| 05 | Rule of Five и перенос владения | Сравните адрес хранилища до и после перемещения; исследуйте поведение `vector` при изменении `noexcept`. |
| 06 | Rule of Zero, vector, span и ref-qualifiers | Проверьте запрет представления из временного объекта; объясните, когда полученный span становится недействительным. |
| 07 | Абстрактный интерфейс, override, final и clone | Объясните разницу между заимствованным представлением и владеющим снимком. Заимствованное хранилище должно пережить BorrowedBuffer. |
| 08 | RTTI, cross-cast, ромб и ABI | Сравните адреса подобъектов, найдите vtable, VTT и корректирующие переходники. |
| 09 | Concepts, шаблоны, constexpr и storage policy | Сравните `array` и `vector`; попробуйте подставить хранилище без `data()`. |
| 10 | C++23: expected, if consteval, explicit object parameter | Проверьте корректный пакет, пустой ввод и неверную длину. Объясните, почему `expected` не исключает `bad_alloc`. |

## Инспекция ABI на macOS

```sh
make CXX=clang++ run-01 run-08
nm -C build/stage01 | grep ByteBuffer
nm -C build/stage08 | grep -E 'vtable|VTT|thunk|typeinfo'
otool -tvV build/stage08
lldb build/stage08
```

В LLDB: `breakpoint set -n dispatch`, затем `run` и
`disassemble --frame`. Для сравнения с оптимизацией:

```sh
make CXX=clang++ BUILD=build-opt CXXFLAGS='-O2 -g -Wall -Wextra -Wpedantic' run-08
otool -tvV build-opt/stage08
```

На macOS используется Mach-O, поэтому имена символов и ассемблер
могут отличаться от Linux ELF. На Apple Silicon инструкции AArch64,
на Intel Mac — x86-64. Сравнивайте структуру вызова, а не конкретные адреса.

Для Linux x86-64 с подходящим GCC:

```sh
make CXX=g++ BUILD=build-linux check
nm -C build-linux/stage08 | grep -E 'vtable|VTT|thunk|typeinfo'
objdump -dC build-linux/stage08
```

[llvm]: https://formulae.brew.sh/formula/llvm
[brew]: https://brew.sh/
