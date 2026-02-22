
# HI — ULTIMATE README

This document explains **everything** about the repository:
- how to build
- how the build system works
- configurations
- targets
- architecture
- framework philosophy

---

# How to Build

## Windows (Recommended for beginners)

### Requirements
Install:

- Visual Studio 2022 (Desktop C++ workload)
- CMake
- Ninja
- Git

### Clone repository
```
git clone https://github.com/setbe/hi.git
cd hi
```

### Configure
```
cmake --list-presets
cmake . -B build --preset win64
```

### Build examples
use generated `.sln` project file in `build` folder, select target in Visual Studio then build it

or via `ninja` and using  [Command Prompt for Developers x64](https://learn.microsoft.com/en-us/visualstudio/ide/reference/command-prompt-powershell?view=visualstudio):
```
cd build
ninja -f build-ReleaseMini.ninja client server
```

Find:

```
build/ReleaseMini/server.exe
build/ReleaseMini/client.exe
```

---

## Linux (x64 primary target)

Supported:
- x86_64
- partial x86 (l32 preset)

Not supported:
- ARM
- exotic architectures

### Install development packages

#### Debian / Ubuntu
```
sudo apt install build-essential cmake ninja-build git libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev libgl1-mesa-dev libglu1-mesa-dev mesa-common-dev
```

#### Arch
```
sudo pacman -S base-devel cmake ninja git libx11 libxrandr libxinerama libxcursor libxi mesa glu
```

#### Fedora
```
sudo dnf install gcc gcc-c++ cmake ninja-build git libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel mesa-libGL-devel mesa-libGLU-devel
```

### Configure
```
cmake --list-presets
cmake . -B build --preset l
```

### Build
```
cd build
ninja -f build-ReleaseMini.ninja client server
```

---

# What CMake Does

CMake does NOT build your project.

It:
1. Detects compiler
2. Detects OS
3. Generates Ninja build files
4. Selects configuration flags
5. Creates targets

Output:
```
build/*.ninja
```

---

# What Ninja Does

Ninja is the **actual builder**.

It:
- compiles sources
- links binaries
- tracks dependencies
- rebuilds only changed files

List targets:
```
ninja -t targets
```

---

# Configurations

## Debug
Full std + libc.

Used for:
- tests
- debugging

## Release
Optimized hosted build.

## ReleaseNoConsole
Same as Release but GUI subsystem.

## ReleaseMini
True freestanding build:
- no CRT
- no std
- no libc

## ReleaseMiniNoConsole
Freestanding GUI build.
---

Note: GUI builds anyway could be build with hosted link

# Targets

## Examples

client — UDP client example  
server — UDP server example  
gl — Window + OpenGL example

## Tests

test_io_hi  
test_server  
test_client  
test_crypto  
test_gl  

---

# Header Layout

The framework is mostly header-only.

## io.hpp

Core runtime:
- memory
- views
- containers
- syscalls
- platform abstraction

## hi.hpp

Window system:
- CRTP windows
- event callbacks
- OpenGL context

## socket.hpp

Freestanding UDP stack:
- handshake
- reliability
- keepalive
- peer lifecycle

---

# Core Concept: io::view

`io::view` is the most important type.

It represents:

- non-owning memory
- span-like access
- zero allocations
- explicit lifetime

Example:

```
io::byte_view payload(ptr, size);
```

Rules:
- never owns memory
- caller manages lifetime
- always cheap to copy

Everything in the framework passes data using views.

---

# Memory Philosophy

No hidden allocations.

Memory comes from:
- arenas
- OS alloc
- user buffers

Ownership must always be visible.

---

# Networking Model

EventLoop drives networking.

Flow:

```
Socket -> EventLoop -> Callbacks
```

Server and client automatically:
- handshake
- send keepalive
- detect timeouts

---

# GUI Model

CRTP window:

```
struct Window : hi::Window<Window>
```

Callbacks:
- onRender
- onKeyDown
- onError

No hidden message loop.

---

# Freestanding Philosophy

Freestanding means:

- no std::vector
- no iostream
- no exceptions
- no RTTI
- no CRT startup

You control everything.

---

# Typical Development Workflow

1. Debug build
2. Write tests
3. Switch to ReleaseMini
4. Verify freestanding compatibility

---

# Project Goals

- deterministic behavior
- explicit ownership
- predictable performance
- minimal runtime dependencies

---

# Status

Active development.
APIs may evolve.

