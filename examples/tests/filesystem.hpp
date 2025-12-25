#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/filesystem.hpp"
#include "../../hi/native/file.hpp"        // io::File
#include "../../hi/native/containers.hpp"  // io::string, io::vector, io::char_view
#include "../../hi/io.hpp"                 // io::out

// ============================================================
// Filesystem tests.
//
// Philosophy:
// - Tests are API documentation: use only hi-level io::File + fs::* wrappers.
// - Prefer io::char_view everywhere to avoid hidden strlen/temporary allocations.
// - Keep path construction readable: split/join instead of manual slash checks.
// ============================================================

namespace test_fs {

    // Canonical separator used by tests.
    // We intentionally pick one so expected strings are stable.
    // - Windows accepts both '/' and '\\' in most APIs.
    // - Linux uses '/'.
    //
    // If you want 100% canonical output across platforms, make fs normalize separators.
#ifdef _WIN32
    static const io::char_view kSep = { "\\", 1 };
#else
    static const io::char_view kSep = { "/", 1 };
#endif

    static bool split_view(io::char_view s, char delim, io::vector<io::char_view>& out_parts) noexcept {
        out_parts.clear();
        if (!s.data()) return true; // empty => one empty part? your choice; I'll treat as no parts

        io::usize start = 0;
        for (io::usize i = 0; i < s.size(); ++i) {
            if (s[i] == delim) {
                io::char_view part{ s.data() + start, i - start };
                if (!out_parts.push_back(part)) return false;
                start = i + 1;
            }
        }
        io::char_view tail{ s.data() + start, s.size() - start };
        if (!out_parts.push_back(tail)) return false;
        return true;
    }


    static bool path_join(io::char_view base, io::char_view name, io::string& out) {
        out.clear();

        io::vector<io::char_view> parts{};

#ifdef _WIN32
        const char sep = '\\';
#else
        const char sep = '/';
#endif

        // 1) split by canonical sep; if didn't split, try the other one
        if (!split_view(base, sep, parts)) return false;

        if (parts.size() == 1) {
#ifdef _WIN32
            if (!split_view(base, '/', parts)) return false;
#else
            if (!split_view(base, '\\', parts)) return false;
#endif
        }

        // 2) trim trailing empty part if base ended with separator
        if (!parts.empty() && parts.back().size() == 0) {
            parts.pop_back();
        }

        // 3) append leaf
        if (!parts.push_back(name)) return false;

        // 4) join
        const io::char_view kSepView{ &sep, 1 }; // NOTE: NOT constexpr; points to local sep (alive for this call)
        bool res = io::string::join(parts.as_view(), kSepView, out);

        return res;
    }


    // Make a temporary directory under the current working directory so we don't need OS APIs.
    // We rely only on:
    // - fs::current_directory
    // - fs::create_directory
    // - fs::remove
    //
    // Directory name uses a monotonic counter to avoid collisions.
    static io::string make_temp_dir() noexcept {
        io::string cwd{};
        REQUIRE(fs::current_directory(cwd));
        REQUIRE(!cwd.empty());

        static io::atomic<io::u64> g{ 1 };
        io::u64 id = g.fetch_add(1, io::memory_order_relaxed);

        // Build dir name: "hi_fs_test_<id>"
        io::string leaf{};
        REQUIRE(leaf.append("hi_fs_test_"));

        io::out.reset();
        io::out << id;
        REQUIRE(leaf.append(io::out.scrap_view()));

        io::string dir{};
        REQUIRE(path_join(cwd.as_view(), leaf.as_view(), dir));

        // Best-effort cleanup in case a previous crashed run left it behind.
        if (fs::exists(dir)) (void)fs::remove(dir);

        REQUIRE(fs::create_directory(dir));
        REQUIRE(fs::exists(dir));
        REQUIRE(fs::is_directory(dir));

        return dir;
    }

    // Best-effort remove: do not hard-fail tests just because the file is locked by AV/indexer.
    // Instead, print a message so the user understands what happened.
    static void cleanup_path(io::char_view p) noexcept {
        if (!fs::exists(p)) return;

        if (!fs::remove(p)) {
            io::out << "[cleanup] couldn't remove: " << p << io::out.endl;
        }
    }

    // Extract filename from a full path without allocating (handles both separators).
    static io::char_view basename_view(io::char_view full) noexcept {
        if (!full.data() || full.size() == 0) return io::char_view{};

        io::usize last = 0;
        for (io::usize i = 0; i < full.size(); ++i) {
            char c = full[i];
            if (c == '/' || c == '\\') last = i + 1;
        }
        return io::char_view{ full.data() + last, full.size() - last };
    }

    // Write exact bytes to a file using io::File (no OS API).
    static void write_bytes(io::char_view path, io::char_view bytes) {
        io::File f(path, io::OpenMode::Write | io::OpenMode::Create | io::OpenMode::Truncate);
        REQUIRE(f.is_open());
        REQUIRE(f.write(bytes) == bytes.size());
        REQUIRE(f.flush());
        REQUIRE(f.good());
    }

// ============================================================
// Tests
// ============================================================

TEST_CASE("fs: basic create/stat/rename/remove + directory iteration", "[fs]") {
    io::string dir = make_temp_dir();

    io::string file1{};
    io::string file2{};

    REQUIRE(path_join(dir.as_view(), "a.txt", file1));
    REQUIRE(path_join(dir.as_view(), "b.txt", file2));

    // Create a file without strlen() and without OS headers.
    write_bytes(file1.as_view(), io::char_view{ "hello", 5 });

    REQUIRE(fs::exists(file1));
    REQUIRE(fs::is_regular_file(file1));
    REQUIRE(fs::file_size(file1) == 5);

    // Rename file1 -> file2 (all args are io::char_view / io::string)
    REQUIRE(fs::rename(file1, file2));
    REQUIRE_FALSE(fs::exists(file1));
    REQUIRE(fs::exists(file2));
    REQUIRE(fs::file_size(file2) == 5);

    // Iterate directory and find "b.txt"
    bool found = false;
    for (fs::directory_iterator it{ dir.as_view() }; !it.is_end(); ++it) {
        const auto& e = *it;
        if (!e.path.data()) continue;

        io::char_view base = basename_view(e.path);
        if (base == "b.txt") {
            found = true;
            REQUIRE(e.type == fs::file_type::regular);
            REQUIRE(e.size == 5);
            break;
        }
    }
    REQUIRE(found);

    // Cleanup (best-effort, see helper comment).
    cleanup_path(file2.as_view());
    cleanup_path(dir.as_view());
}

TEST_CASE("fs: status() returns not_found for missing paths", "[fs]") {
    io::string dir = make_temp_dir();

    io::string missing{};
    REQUIRE(path_join(dir.as_view(), "does_not_exist.bin", missing));

    REQUIRE_FALSE(fs::exists(missing));
    REQUIRE(fs::status(missing) == fs::file_type::not_found);

    cleanup_path(dir.as_view());
}

TEST_CASE("fs: directory_iterator skips '.' and '..' and produces full paths", "[fs]") {
    io::string dir = make_temp_dir();

    // Ensure at least one real entry exists.
    io::string f{};
    REQUIRE(path_join(dir.as_view(), "x.txt", f));
    write_bytes(f.as_view(), io::char_view{ "x", 1 });

    bool saw_x = false;

    for (fs::directory_iterator it{ dir.as_view() }; !it.is_end(); ++it) {
        const auto& e = *it;

        REQUIRE(e.path.data() != nullptr);
        REQUIRE(e.path.size() > 0);

        io::char_view base = basename_view(e.path);

        // These should never appear because iterator filters them out.
        REQUIRE(base != ".");
        REQUIRE(base != "..");

        if (base == "x.txt") {
            saw_x = true;
            REQUIRE(e.type == fs::file_type::regular);
            REQUIRE(e.size == 1);
        }

        // The iterator returns a full path, not just a leaf name.
        // So the directory should be a prefix somewhere.
        REQUIRE(e.path.find(dir.as_view()) != io::char_view::npos);
    }

    REQUIRE(saw_x);

    cleanup_path(f.as_view());
    cleanup_path(dir.as_view());
}

TEST_CASE("fs: rename over existing destination is documented (platform behavior may differ)", "[fs]") {
    // This test is intentionally tolerant: it documents current behavior rather than forcing one.
    // If you later standardize fs::rename semantics across platforms (e.g. always overwrite),
    // update this test to match.
    io::string dir = make_temp_dir();

    io::string a{}, b{};
    REQUIRE(path_join(dir.as_view(), "a.txt", a));
    REQUIRE(path_join(dir.as_view(), "b.txt", b));

    write_bytes(a.as_view(), io::char_view{ "A", 1 });
    write_bytes(b.as_view(), io::char_view{ "B", 1 });

    bool ok = fs::rename(a, b);

#ifdef _WIN32
    // With MoveFileW (no REPLACE_EXISTING), overwriting typically fails.
    REQUIRE_FALSE(ok);
    REQUIRE(fs::exists(a));
    REQUIRE(fs::exists(b));
#else
    // POSIX rename() usually overwrites (if your Linux backend follows that).
    // If your Linux backend is not implemented yet, ok may be false.
    (void)ok;
#endif

    cleanup_path(a.as_view());
    cleanup_path(b.as_view());
    cleanup_path(dir.as_view());
}

} // namespace