#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/filesystem.hpp"

#ifdef _WIN32
#  define WIN32_LEAN_AND_MEAN
#  include <Windows.h>
#  undef CreateDirectory
#  undef DeleteFile
#else
#  include <unistd.h>
#  include <fcntl.h>
#  include <sys/stat.h>
#endif

// -------- helpers --------

static void make_temp_dir(char* out, usize cap) {
#ifdef _WIN32
    DWORD n = GetTempPathA((DWORD)cap, out);
    REQUIRE(n > 0);
    // unique name
    char name[MAX_PATH];
    UINT ok = GetTempFileNameA(out, "hi", 0, name);
    REQUIRE(ok != 0);
    // temp file -> delete and make dir
    DeleteFileA(name);
    REQUIRE(fs::native::create_directory(name));
    // copy to out
    usize i = 0;
    for (; name[i] && i + 1 < cap; ++i) out[i] = name[i];
    out[i] = '\0';
#else
    // mkdtemp style, but minimal
    const char* base = "/tmp/hi_fs_test_XXXXXX";
    usize i = 0;
    for (; base[i] && i + 1 < cap; ++i) out[i] = base[i];
    out[i] = '\0';
    // use mkstemp then mkdir
    int fd = mkstemp(out);
    REQUIRE(fd != -1);
    close(fd);
    unlink(out);
    REQUIRE(mkdir(out, 0700) == 0);
#endif
}

static void create_file_with_bytes(const char* path, const void* data, usize n) {
#ifdef _WIN32
    HANDLE h = CreateFileA(path, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    REQUIRE(h != INVALID_HANDLE_VALUE);
    DWORD wrote = 0;
    BOOL ok = WriteFile(h, data, (DWORD)n, &wrote, nullptr);
    CloseHandle(h);
    REQUIRE(ok);
    REQUIRE(wrote == n);
#else
    int fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0644);
    REQUIRE(fd != -1);
    ssize_t w = write(fd, data, n);
    close(fd);
    REQUIRE((usize)w == n);
#endif
}

// -------- tests --------

TEST_CASE("filesystem basic create/stat/rename/remove + directory iteration", "[io][fs]") {
    char dir[512];
    make_temp_dir(dir, sizeof(dir));

    // join_path to create file paths
    char file1[512];
    fs::native::join_path(dir, "a.txt", file1, sizeof(file1));

    char file2[512];
    fs::native::join_path(dir, "b.txt", file2, sizeof(file2));

    REQUIRE(fs::exists(fs::path_view(dir)));
    REQUIRE(fs::is_directory(fs::path_view(dir)));

    // create a file
    const char payload[] = "hello";
    create_file_with_bytes(file1, payload, 5);

    REQUIRE(fs::exists(fs::path_view(file1)));
    REQUIRE(fs::is_regular_file(fs::path_view(file1)));
    REQUIRE(fs::file_size(fs::path_view(file1)) == 5);

    // rename
    REQUIRE(fs::rename(fs::path_view(file1), fs::path_view(file2)));
    REQUIRE(!fs::exists(fs::path_view(file1)));
    REQUIRE(fs::exists(fs::path_view(file2)));

    // iterate directory and find b.txt
    bool found = false;
    fs::directory_iterator it{ fs::path_view(dir) };
    for (; !it.is_end(); ++it) {
        auto& e = *it;
        // entry.path.c_str is full path; just search suffix "b.txt"
        const char* p = e.path.c_str;
        if (!p) continue;

        // naive suffix check
        const char* suf = "b.txt";
        usize lenp = 0; while (p[lenp]) ++lenp;
        usize lens = 0; while (suf[lens]) ++lens;

        if (lenp >= lens) {
            bool ok = true;
            for (usize i = 0; i < lens; ++i) {
                if (p[lenp - lens + i] != suf[i]) { ok = false; break; }
            }
            if (ok) {
                found = true;
                REQUIRE(e.type == fs::file_type::regular);
                REQUIRE(e.size == 5);
                break;
            }
        }
    }
    REQUIRE(found);

    // cleanup: remove file then dir
    REQUIRE(fs::remove(fs::path_view(file2)));
    REQUIRE(!fs::exists(fs::path_view(file2)));
    REQUIRE(fs::remove(fs::path_view(dir)));
    REQUIRE(!fs::exists(fs::path_view(dir)));
}
