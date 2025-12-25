#pragma once
#define CATCH_CONFIG_MAIN
#include "catch.hpp"

#include "../../hi/native/types.hpp"

// ------------------------------------------------------------
//              Compile-time sanity checks
// ------------------------------------------------------------
static_assert(sizeof(io::i8) == 1, "i8 must be 1 byte");
static_assert(sizeof(io::u8) == 1, "u8 must be 1 byte");
static_assert(sizeof(io::i16) == 2, "i16 must be 2 bytes");
static_assert(sizeof(io::u16) == 2, "u16 must be 2 bytes");
static_assert(sizeof(io::i32) == 4, "i32 must be 4 bytes");
static_assert(sizeof(io::u32) == 4, "u32 must be 4 bytes");
static_assert(sizeof(io::i64) == 8, "i64 must be 8 bytes");
static_assert(sizeof(io::u64) == 8, "u64 must be 8 bytes");

static_assert(sizeof(io::usize) == sizeof(sizeof(0)), "usize must match sizeof(...) result type size");
static_assert(sizeof(io::isize) == sizeof(static_cast<char*>(nullptr) - static_cast<char*>(nullptr)),
    "isize must match pointer-difference expression type size");

// ------------------------------------------------------------
//                          Tests
// ------------------------------------------------------------
TEST_CASE("io::len counts bytes until NUL and handles nullptr", "[types][io][len]") {
    REQUIRE(io::len(nullptr) == 0);

    REQUIRE(io::len("") == 0);
    REQUIRE(io::len("a") == 1);
    REQUIRE(io::len("hello") == 5);

    const char s[] = "abc\0zzz";
    REQUIRE(io::len(s) == 3);
}

TEST_CASE("io::is_same_v works", "[types][io][traits]") {
    STATIC_REQUIRE(io::is_same_v<int, int>);
    STATIC_REQUIRE(!io::is_same_v<int, long>);
    STATIC_REQUIRE(io::is_same_v<io::true_t, io::constant<bool, true>>);
    STATIC_REQUIRE(io::is_same_v<io::false_t, io::constant<bool, false>>);
}

TEST_CASE("io::enable_if_t SFINAE basic usage compiles", "[types][io][sfinae]") {
    struct X {};

    auto f_int_only = [](auto v) -> io::enable_if_t<io::is_same_v<decltype(v), int>, int> {
        return v + 1;
    };

    REQUIRE(f_int_only(1) == 2);

    // Important: Do NOT call f_int_only(X{}), otherwise it will be a hard error, and we need compile-time SFINAE.
    SUCCEED();
}

TEST_CASE("io::remove_reference_t removes references", "[types][io][traits]") {
    STATIC_REQUIRE(io::is_same_v<io::remove_reference_t<int>, int>);
    STATIC_REQUIRE(io::is_same_v<io::remove_reference_t<int&>, int>);
    STATIC_REQUIRE(io::is_same_v<io::remove_reference_t<int&&>, int>);
}

TEST_CASE("io::move casts to rvalue ref (type-level check)", "[types][io][move]") {
    int x = 7;
    // Сheck the type of the result: remove_reference_t<T>&&
    STATIC_REQUIRE(io::is_same_v<decltype(io::move(x)), int&&>);
    (void)x;
    SUCCEED();
}

TEST_CASE("io::forward preserves value category (type-level checks)", "[types][io][forward]") {
    // lvalue forward
    int a = 1;
    STATIC_REQUIRE(io::is_same_v<decltype(io::forward<int&>(a)), int&>);

    // rvalue forward
    STATIC_REQUIRE(io::is_same_v<decltype(io::forward<int>(io::move(a))), int&&>);
    SUCCEED();
}

TEST_CASE("io::view default ctor is empty", "[types][io][view]") {
    io::view<int> v{};
    REQUIRE(v.data() == nullptr);
    REQUIRE(v.size() == 0);
    REQUIRE(v.empty());
}

TEST_CASE("io::view from pointer+len", "[types][io][view]") {
    int arr[] = { 1,2,3,4 };
    io::view<int> v(arr, 4);

    REQUIRE(v.size() == 4);
    REQUIRE(!v.empty());
    REQUIRE(v.data() == arr);
    REQUIRE(v[0] == 1);
    REQUIRE(v.back() == 4);
    REQUIRE(v.front() == 1);
}

TEST_CASE("io::view from C-array", "[types][io][view]") {
    int arr[] = { 10,20,30 };
    io::view<int> v(arr);

    REQUIRE(v.size() == 3);
    REQUIRE(v[1] == 20);
}

TEST_CASE("io::view cstr ctor works only for const char", "[types][io][view]") {
    io::view<const char> s("hello");
    REQUIRE(s.size() == 5);
    REQUIRE(s[0] == 'h');
    REQUIRE(s.back() == 'o');
}

TEST_CASE("io::view slicing helpers", "[types][io][view]") {
    int arr[] = { 1,2,3,4,5 };
    io::view<int> v(arr);

    auto first2 = v.first(2);
    REQUIRE(first2.size() == 2);
    REQUIRE(first2[0] == 1);
    REQUIRE(first2[1] == 2);

    auto last2 = v.last(2);
    REQUIRE(last2.size() == 2);
    REQUIRE(last2[0] == 4);
    REQUIRE(last2[1] == 5);

    auto drop3 = v.drop(3);
    REQUIRE(drop3.size() == 2);
    REQUIRE(drop3[0] == 4);
    REQUIRE(drop3[1] == 5);

    auto slice = v.slice(1, 3);
    REQUIRE(slice.size() == 3);
    REQUIRE(slice[0] == 2);
    REQUIRE(slice[1] == 3);
    REQUIRE(slice[2] == 4);

    auto oob1 = v.slice(100, 1);
    REQUIRE(oob1.size() == 0);

    auto too_much = v.first(999);
    REQUIRE(too_much.size() == 5);
}

TEST_CASE("hi::Key_t::map returns expected strings for a few keys", "[types][hi][key]") {
    REQUIRE(                 hi::Key_t::map(hi::Key::A) == std::string("a"));
    REQUIRE(                hi::Key_t::map(hi::Key::F1) == std::string("f1"));
    REQUIRE(            hi::Key_t::map(hi::Key::Escape) == std::string("escape"));
    REQUIRE(          hi::Key_t::map(hi::Key::__NONE__) == std::string("__NONE__"));
    REQUIRE(   hi::Key_t::map(static_cast<hi::Key>(-1)) == std::string("unknown"));
    REQUIRE(hi::Key_t::map(static_cast<hi::Key>(99999)) == std::string("unknown"));
}
