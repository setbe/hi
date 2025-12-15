#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/native/syscalls.hpp"
#include "../../hi/native/types.hpp"

#ifdef _WIN32
#  include <windows.h>
#else
#  include <unistd.h>
#  include <sys/types.h>
#  include <sys/wait.h>
#endif

// ---------------------- helpers ----------------------

static bool can_write_bytes(void* p, usize n) {
    volatile unsigned char* b = static_cast<volatile unsigned char*>(p);
    for (usize i = 0; i < n; ++i) b[i] = static_cast<unsigned char>(i);
    for (usize i = 0; i < n; ++i) if (b[i] != static_cast<unsigned char>(i)) return false;
    return true;
}

// ---------------------- tests ------------------------

TEST_CASE("io::alloc returns writable memory and io::free releases it", "[io][syscalls][alloc]") {
    // test a few sizes including 0-ish and larger blocks
    const usize sizes[] = { 1, 8, 64, 4096, 65536 };

    for (usize sz : sizes) {
        void* p = io::alloc(sz);
        REQUIRE(p != nullptr);

        REQUIRE(can_write_bytes(p, (sz < 256 ? sz : 256))); // don't loop huge

        io::free(p);
        // We cannot safely dereference after free; just require no crash.
    }
}

TEST_CASE("io::alloc returns distinct regions for distinct allocations (likely)", "[io][syscalls][alloc]") {
    void* a = io::alloc(4096);
    void* b = io::alloc(4096);

    REQUIRE(a != nullptr);
    REQUIRE(b != nullptr);

    // Not a strict guarantee, but with VirtualAlloc/mmap it should be distinct.
    REQUIRE(a != b);

    io::free(a);
    io::free(b);
}

TEST_CASE("io::free(nullptr) is safe", "[io][syscalls][free]") {
    io::free(nullptr);
    SUCCEED();
}

TEST_CASE("io::monotonic_seconds is non-decreasing", "[io][syscalls][time]") {
    double t1 = io::monotonic_seconds();
    double t2 = io::monotonic_seconds();
    double t3 = io::monotonic_seconds();

    REQUIRE(t2 >= t1);
    REQUIRE(t3 >= t2);
}

TEST_CASE("io::sleep_ms sleeps at least roughly the requested time", "[io][syscalls][sleep]") {
    // Keep it short, but not too short (scheduler granularity).
    const unsigned ms = 30;

    double t1 = io::monotonic_seconds();
    io::sleep_ms(ms);
    double t2 = io::monotonic_seconds();

    double dt_ms = (t2 - t1) * 1000.0;

#ifdef _WIN32
    // Windows Sleep granularity can be ~15.6ms depending on timer resolution.
    // Allow a generous lower bound.
    REQUIRE(dt_ms >= 10.0);
#else
    // usleep is usually fine, but still scheduling exists.
    REQUIRE(dt_ms >= 10.0);
#endif

    // and must not sleep ridiculously long in normal conditions
    REQUIRE(dt_ms < 200.0);
}

TEST_CASE("io::exit_process terminates the process with given code (run in child)", "[io][syscalls][exit]") {
#ifdef _WIN32
    WARN("Skipping on Windows (needs custom main/argv or helper exe).");
    SUCCEED();
#else
    pid_t pid = fork();
    REQUIRE(pid != -1);

    if (pid == 0) {
        io::exit_process(123);
        _exit(255); // should never reach
    }

    int status = 0;
    REQUIRE(waitpid(pid, &status, 0) == pid);
    REQUIRE(WIFEXITED(status));
    REQUIRE(WEXITSTATUS(status) == 123);
#endif
}
