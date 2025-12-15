# hi — freestanding C++ framework

`hi` is a **freestanding-friendly C++ framework** for building low-level applications, tools, GUI systems, and runtimes **without relying on the C++ standard library, libc, or CRT**.

The project is designed for **explicit control over memory, object lifetime, and OS interaction**, with a clean separation between platform-agnostic core code and platform-specific implementations.

This is **not** a wrapper around STL, and **not** a convenience library.  
It is a foundation for systems-level software.

---

## Design Goals

- No dependency on the C++ Standard Library containers or strings
- Freestanding-friendly (custom allocators, no implicit CRT usage)
- Explicit ownership and lifetime semantics
- Minimal abstractions with predictable cost
- Clear OS boundary (`native/` vs `platform/`)
- Suitable for GUI frameworks, engines, and low-level tools

---

## Non-Goals

- STL compatibility or drop-in replacements;
- Maximum performance via heavy templates;
- Automatic memory management;
- Cross-platform portability at any cost;
- Header-only convenience abstractions.

---

## Project Structure

```text
hi/
├── native/                 # Platform-agnostic low-level APIs
│   ├── atomic.hpp          # Freestanding atomic wrapper
│   ├── containers.hpp      # vector, deque, list, basic_string<char / wchar_t>
│   ├── filesystem.hpp      # Native filesystem interface
│   ├── syscalls.hpp        # OS syscalls (alloc, sleep, exit, time)
│   ├── out.hpp             # Output stream
│   ├── gl_loader.hpp       # OpenGL loader and gl::Functions
│   ├── socket.hpp          # Async client/listener
│   │   ..........
│   └── types.hpp           # Fixed-width and core types
│
├── platform/
│   └── windows/
│       ├── filesystem_impl.hpp
│       ├── window_impl.hpp
│       ├── opengl_impl.hpp       # OpenGL handler and context creation
│       └── framebuffer_impl.hpp  # Software renderer via OS
│
├── filesystem.hpp          # High-level filesystem API
├── io.hpp                  # General header for everything that `native` dir has
└── window.hpp              # Window + event loop abstraction

## Core Components

### Containers

Custom containers implemented without STL:

- `io::vector<T>` - dynamic array with explicit lifetime control;
- `io::deque<T>` - ring-buffer based deque;
- `io::list<T>` - doubly-linked list;
- `io::view<T>` - non-owning span-like view.

All containers are *move-only* by design.

### Strings

Unified string implementation:

```cpp
io::string   // UTF-8 string
io::wstring  // wide string (UTF-16 on Windows)
```

Both are implemented as:

```cpp
io::basic_string<CharT>
```

Key properties:

- Always NUL-terminated;
- Built on top of io::vector;
- No Small String Optimization;
- Safe for OS interop (c_str() always valid).

## Filesystem

Cross-platform filesystem API with platform-specific backends.

Features:

- File and directory status;
- Directory iteration;
- UTF-8 paths in public API;
- UTF-16 / UTF-8 conversion handled internally;
- No fixed-size path buffers;
- RAII-based directory handles.

Example:

```cpp
io::string cwd;
fs::current_directory(cwd);
io::out << cwd << io::out.endl;
```

## Atomic & Syscalls

- `io::atomic<T>` - freestanding atomic abstraction
- Fallbacks for non-STL environments

OS syscalls:
- alloc / free
- exit_process
- sleep_ms
- monotonic_seconds

## Windowing & Rendering

- CRTP-based window abstraction
- Platform-specific implementations
- OpenGL backend (Windows)
- No implicit message pumps or hidden globals

Example:

```cpp
struct MainWindow : public hi::Window<MainWindow> {
    void onRender() noexcept override {
        gl::Clear(gl::BufferBit::Color);
        this->SwapBuffers();
    }
};
```

## Testing

The project includes extensive Catch2 tests covering:

- Container lifetime and move semantics
- Filesystem behavior on real OS paths
- Directory iteration correctness
- Syscalls and process behavior
- String and view invariants

Tests are designed to catch lifetime bugs and ABI-level issues, not just logic errors.

## Supported Platforms

- Windows (primary target)
- Linux (in progress)
- Android (in planning)

The codebase is structured to allow additional platforms without affecting core APIs.

## Philosophy

- Ownership must be explicit
- Lifetime must be visible
- Abstractions must have predictable cost
- OS interaction must be explicit and isolated
- Simplicity over convenience

## Status

Active development.
APIs may evolve, but design principles are stable.