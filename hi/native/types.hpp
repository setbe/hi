#pragma once

// --- Includeless fixed-width integer types ---
using i8 = signed char;
using u8 = unsigned char;

using i16 = short;
using u16 = unsigned short;

using i32 = int;
using u32 = unsigned int;

using i64 = long long;
using u64 = unsigned long long;


// --- Size and pointer-difference types (C++ freestanding replacements) ---
using isize = decltype(static_cast<char*>(nullptr) - static_cast<char*>(nullptr));
using usize = decltype(sizeof(0));


#pragma region macros

#if !defined(_DEBUG) && !defined(NDEBUG)
#   define _DEBUG
#endif

#define IO_DEFINE_GLOBAL_ARRAY_AND_COUNT(NAME, TYPE, COUNT) \
namespace global {                                                \
    static TYPE NAME[COUNT]{};                                    \
    static usize NAME##_count{ 0 };                               \
}


#pragma region io_keywords

// -------------------- C++17 Version Detection -------------------------------
#ifdef _MSC_VER // MSVC
#   if defined(_MSVC_LANG)
#       define IO_CPP_VERSION _MSVC_LANG
#   else
#       define IO_CPP_VERSION __cplusplus
#   endif
#elif defined(__GNUC__) || defined(__clang__) // GNU || CLang
#   if __cplusplus >= 201703L
#       define IO_CPP_VERSION __cplusplus
#   endif
#endif // C++17 feature detection

// --------------------- keyword `IO_RESTRICT` --------------------------------

#if IO_CPP_VERSION < 202302L // `restrict` was officially added in C++23
#  ifndef IO_RESTRICT
#    ifdef __GNUC__         // GNU
#      define IO_RESTRICT __restrict__
#    elif defined(_MSC_VER) // MSVC
#      define IO_RESTRICT __restrict
#    else                   // Fallback
#      define IO_RESTRICT
#    endif
#  endif // IO_RESTRICT
#endif // IO_CPP_VERSION < 202302L

// Define `IO_CXX_17` macro if C++17 or above used
#if IO_CPP_VERSION >= 201703L
#    define IO_CXX_17
#endif


// -------------------------- IO_FALLTHROUGH ----------------------------------
#if defined(IO_CXX_17)
#   define IO_FALLTHROUGH [[fallthrough]]
#elif defined(__GNUC__) && __GNUC__ >= 7
#   define IO_FALLTHROUGH __attribute__((fallthrough))
#else
#   define IO_FALLTHROUGH ((void)0)
#endif // defined(IO_CXX_17)

// ------------- IO_NODISCARD / IO_CONSTEXPR / IO_CONSTEXPR_VAR ---------------
#ifdef IO_CXX_17
#   ifndef IO_NODISCARD
#       define IO_NODISCARD [[nodiscard]]
#   endif
#   ifndef IO_CONSTEXPR
#       define IO_CONSTEXPR constexpr
#   endif
#   ifndef IO_CONSTEXPR_VAR
#       define IO_CONSTEXPR_VAR constexpr
#   endif
#else // fallback
#   ifndef IO_NODISCARD
#       define IO_NODISCARD
#   endif
#   ifndef IO_CONSTEXPR
#       define IO_CONSTEXPR inline
#   endif
#   ifndef IO_CONSTEXPR_VAR
#       define IO_CONSTEXPR_VAR const
#   endif
#endif // // Attribute/IO_CONSTEXPR defines
#pragma endregion // io_keywords

// ------------------------------ IO_API --------------------------------------
#ifndef IO_API
#  if defined(_WIN32) && defined(IO_BUILD_DLL)
#    define IO_API __declspec(dllexport)
#  elif defined(_WIN32) && defined(IO_USE_DLL)
#    define IO_API __declspec(dllimport)
#  else
#    define IO_API
#  endif
#endif

#pragma endregion // macros

namespace io {

    IO_CONSTEXPR usize len(const char* s) noexcept {
        if (!s) return 0; // avoid UB by returning 0 for null pointers
        const char* p = s;
        while (*p != char(0)) ++p;
        return static_cast<usize>(p - s);
    }
    // ----------------- Basic SFINAE Primitives --------------------

    template<bool B, typename T = void> struct enable_if {};

    template<typename T> struct enable_if<true, T> { using type = T; };

    template<bool B, typename T = void>
    using enable_if_t = typename enable_if<B, T>::type;

    // ----------------- Constant / true/false types ----------------

    template<typename T, T v>
    struct constant {
        static IO_CONSTEXPR_VAR T value = v;
        using type = constant;
        IO_CONSTEXPR operator T() const noexcept { return value; }
    };

    using true_t = constant<bool, true>;
    using false_t = constant<bool, false>;

    // ----------------------- is_same ------------------------------

    template<typename A, typename B>
    struct is_same : false_t {};

    template<typename A>
    struct is_same<A, A> : true_t {};

    template<typename A, typename B>
    IO_CONSTEXPR_VAR bool is_same_v = is_same<A, B>::value;

    // ------------------------ void_t -----------------------------

    template<typename...>
    using void_t = void;

    // -------- Convenience macro for enabling functions -----------

#define IO_REQUIRES(...) typename = enable_if_t<(__VA_ARGS__)>
#define IO_REQUIRES_T(...) enable_if_t<(__VA_ARGS__), int> = 0

    // -------------------- remove_reference --------------------
    template<typename T> struct remove_reference { using type = T; };
    template<typename T> struct remove_reference<T&> { using type = T; };
    template<typename T> struct remove_reference<T&&> { using type = T; };

    template<typename T>
    using remove_reference_t = typename remove_reference<T>::type;

    // ---------------------- move, forward ------------------------
    
    template<typename T>
    IO_CONSTEXPR remove_reference_t<T>&& move(T&& t) noexcept { return static_cast<remove_reference_t<T>&&>(t); }
    template<typename T>
    IO_CONSTEXPR T&& forward(remove_reference_t<T>& t) noexcept { return static_cast<T&&>(t); }
    template<typename T>
    IO_CONSTEXPR T&& forward(remove_reference_t<T>&& t) noexcept { return static_cast<T&&>(t); }


    // ============================================================
    //                   universal view (like span)
    // ============================================================
    template<typename T>
    struct view {
        using value_type = T;

        // -------------------- Constructors ---------------------------------

        // 1. Default
        IO_CONSTEXPR view() noexcept : _ptr(nullptr), _len(0) {}

        // 2. Raw pointer + length
        IO_CONSTEXPR view(T* ptr, usize len) noexcept : _ptr(ptr), _len(len) {}

        // 3a) From C-array (generic, but NOT for const char)
        template<usize N, typename U = T,
            IO_REQUIRES(!is_same_v<U, const char>)>
        IO_CONSTEXPR view(U(&arr)[N]) noexcept : _ptr(arr), _len(N) {}

        // 3b) From char literal / char array for const char: trim '\0' if present
        template<usize N, typename U = T,
            IO_REQUIRES(is_same_v<U, const char>)>
        IO_CONSTEXPR view(const char(&arr)[N]) noexcept : _ptr(arr),
            _len((N&& arr[N - 1] == '\0') ? (N - 1) : N) {}

        // -------------------- Access / Info -------------------------------

        IO_CONSTEXPR T& operator[](usize i) const noexcept { return _ptr[i]; }
        IO_CONSTEXPR T* data()    const noexcept { return _ptr; }
        IO_CONSTEXPR usize size() const noexcept { return _len; }
        IO_CONSTEXPR bool empty() const noexcept { return _len == 0; }

        // -------------------- Iteration -----------------------------------

        IO_CONSTEXPR T* begin() const noexcept { return _ptr; }
        IO_CONSTEXPR T* end()   const noexcept { return _ptr + _len; }

        // -------------------- Convenience operations ----------------------

        IO_CONSTEXPR T& front() const noexcept { return _ptr[0]; }
        IO_CONSTEXPR T& back()  const noexcept { return _ptr[_len - 1]; }

        IO_CONSTEXPR view first(usize n) const noexcept { return view(_ptr, (n <= _len ? n : _len)); }
        IO_CONSTEXPR view last(usize n) const noexcept { return view(_ptr + (_len>n ? _len-n : 0), (n<=_len ? n : _len)); }
        IO_CONSTEXPR view drop(usize n) const noexcept { if (n >= _len) return view(); return view(_ptr + n, _len - n); }
        IO_CONSTEXPR view slice(usize pos, usize count) const noexcept {
            if (pos >= _len) return view();
            usize r = (_len - pos);
            if (count < r) r = count;
            return view(_ptr + pos, r);
        }
        IO_CONSTEXPR view subview(usize pos, usize count) const noexcept { return slice(pos, count); }

    protected:
        T* _ptr;
        usize _len;
    };

    // --- convenience aliases ---
    using char_view = view<const char>;
    using byte_view = view<const u8>;
} // namespace io








namespace hi {
    enum class Key {
        __NONE__,
        // --------------------------- FUNCTIONAL ---------------------------
        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        // --------------------------- MODIFIERS ---------------------------
        Shift,
        Control,
        Alt,
        Super,
        // --------------------------- TTY ---------------------------
        Escape,
        Insert,
        Delete,
        Backspace,
        Tab,
        Return,
        ScrollLock,
        NumLock,
        CapsLock,
        // --------------------------- MOTION ---------------------------
        Home,
        End,
        PageUp,
        PageDown,
        Left,
        Up,
        Right,
        Down,
        // --------------------------- MOUSE ---------------------------
        MouseLeft,
        MouseRight,
        MouseMiddle,
        MouseX1,
        MouseX2,

        // --------------------------- ASCII ---------------------------
        Space,
        _0, _1, _2, _3, _4, _5, _6, _7, _8, _9,
        A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        Grave,       // `
        Hyphen,      // -
        Equal,       // =
        BracketLeft, // [
        BracketRight,// ]
        Comma,       // ,
        Period,      // .
        Slash,
        Backslash,
        Semicolon,   // ;
        Apostrophe,  // '  

        __LAST__ = 87
    }; // enum class key

    enum class RendererApi {
        None = 0,
        Software,
        Opengl,
        Vulkan,
    }; // enum class renderer_api

    enum class WindowBackend {
        Unknown = 0,
        X11 = 1,
        WindowsApi = 2,
        Cocoa = 3,
        AndroidNdk = 4,
    }; // enum class WindowBackend

    IO_CONSTEXPR_VAR WindowBackend WINDOW_BACKEND = // Choose via macros
#if defined(__linux__) // ------------- Linux
     WindowBackend::X11;
#elif defined(_WIN32) // -------------- Windows
     WindowBackend::WindowsApi;
#elif defined(__APPLE__) // ----------- Apple
     WindowBackend::Cocoa;
#elif defined(__ANDROID__) // --------- Android
     WindowBackend::AndroidNdk;
#else // ------------------------------ Unknown
     WindowBackend::Unknown;
#endif

#ifdef IO_CXX_17 // Check if `Window Backend` is known at compile-time in C++17 or above
     static_assert(WINDOW_BACKEND != WindowBackend::Unknown && "Unknown window backend");
#endif

    namespace global {
        extern unsigned char key_array[static_cast<size_t>(Key::__LAST__)];
    } // namespace global

#pragma region key
    struct Key_t {
    private:
        Key _key;
    public:
        IO_CONSTEXPR Key_t(Key k) noexcept : _key{k} { }
        IO_CONSTEXPR explicit Key_t(int k) noexcept : Key_t(static_cast<Key>(k)) { }

        
        // Key state
        IO_NODISCARD IO_CONSTEXPR static const char* map(Key) noexcept;
        IO_NODISCARD IO_CONSTEXPR const char* map() const noexcept { return map(_key); }
        IO_NODISCARD IO_CONSTEXPR Key key() const noexcept { return _key; }
        IO_NODISCARD explicit operator bool() const noexcept { return isPressed(_key); }
        IO_NODISCARD bool operator!() const noexcept { return !isPressed(_key); }

        // Static methods
        IO_NODISCARD static bool isPressed(Key k) noexcept { return hi::global::key_array[static_cast<unsigned int>(k)]; }
        static IO_CONSTEXPR size_t size() noexcept { return static_cast<unsigned char>(Key::__LAST__); }
    }; // struct Key_t

    IO_CONSTEXPR const char* Key_t::map(Key key_to_map) noexcept {
        using K = Key;
        switch (key_to_map) {
            case K::F1:  return "f1";
            case K::F2:  return "f2";
            case K::F3:  return "f3";
            case K::F4:  return "f4";
            case K::F5:  return "f5";
            case K::F6:  return "f6";
            case K::F7:  return "f7";
            case K::F8:  return "f8";
            case K::F9:  return "f9";
            case K::F10: return "f10";
            case K::F11: return "f11";
            case K::F12: return "f12";
            case K::Shift:      return "shift";
            case K::Control:    return "control";
            case K::Alt:        return "alt";
            case K::Super:      return "super";
            case K::Escape:     return "escape";
            case K::Insert:     return "insert";
            case K::Delete:     return "delete";
            case K::Backspace:  return "backspace";
            case K::Tab:        return "tab";
            case K::Return:     return "return";
            case K::ScrollLock: return "scroll lock";
            case K::NumLock:    return "num lock";
            case K::CapsLock:   return "caps lock";
            case K::Home:       return "home";
            case K::End:        return "end";
            case K::PageUp:     return "page up";
            case K::PageDown:   return "page down";
            case K::Left:       return "left";
            case K::Up:         return "up";
            case K::Right:      return "right";
            case K::Down:       return "down";
            case K::MouseLeft:  return "left mouse button";
            case K::MouseRight: return "right mouse button";
            case K::MouseMiddle: return "middle mouse button";
            case K::MouseX1:    return "mouse button 4";
            case K::MouseX2:    return "mouse button 5";
            case K::_0: return "0";
            case K::_1: return "1";
            case K::_2: return "2";
            case K::_3: return "3";
            case K::_4: return "4";
            case K::_5: return "5";
            case K::_6: return "6";
            case K::_7: return "7";
            case K::_8: return "8";
            case K::_9: return "9";
            case K::A: return "a";
            case K::B: return "b";
            case K::C: return "c";
            case K::D: return "d";
            case K::E: return "e";
            case K::F: return "f";
            case K::G: return "g";
            case K::H: return "h";
            case K::I: return "i";
            case K::J: return "j";
            case K::K: return "k";
            case K::L: return "l";
            case K::M: return "m";
            case K::N: return "n";
            case K::O: return "o";
            case K::P: return "p";
            case K::Q: return "q";
            case K::R: return "r";
            case K::S: return "s";
            case K::T: return "t";
            case K::U: return "u";
            case K::V: return "v";
            case K::W: return "w";
            case K::X: return "x";
            case K::Y: return "y";
            case K::Z: return "z";
            case K::Grave:          return "`";
            case K::Hyphen:         return "-";
            case K::Equal:          return "=";
            case K::BracketLeft:    return "[";
            case K::BracketRight:   return "]";
            case K::Comma:          return ",";
            case K::Period:         return ".";
            case K::Slash:          return "/";
            case K::Backslash:      return "\\";
            case K::Semicolon:      return ";";
            case K::Apostrophe:     return "'";
            case K::__NONE__:       return "__NONE__";
            default:                return "unknown";
        } // switch
    } // map

#pragma region error
    // ============================================================================
    //                    E R R O R   P R O C E S S I N G
    // ============================================================================

    enum class Error : int {
        None = 0,

        Window,
        WindowFramebuffer,
        Opengl,
    }; // enum class error

    enum class AboutError : int {
        None = 0,
        Unknown,

        ApiNotSet,

        // `w_` stands for Windows
        w_WindowClass,
        w_Window,
        w_WindowDC,
            // Opengl Window
        w_ChoosePixelFormatARB,
        w_SetPixelFormat,
        w_CreateContextAttribsARB,
        w_CreateModernContext,
        w_GetCurrentContext,
        w_GetCurrentDC,
            // Dummy Window
        w_DummyWindowClass,
        w_DummyWindow,
        w_DummyWindowDC,
        w_DummyChoosePixelFormat,
        w_DummySetPixelFormat,
        w_DummyCreateContext,
            // Missing functions
        w_MissingChoosePixelFormatARB,
        w_MissingCreateContextAttribsARB,
        w_MissingSwapIntervalEXT, // for VSync
        // Framebuffer (Software renderer)
        w_CreateCompatibleDC,
        w_CreateDibSection,
        w_SelectObject,

        // MSVC has issues in freestanding mode
#ifdef IO_NOSTD
        w_FreestandingDeletedOpCalled,
#endif
    }; // enum class about_error

    IO_NODISCARD IO_CONSTEXPR
        static const char* what(AboutError err) noexcept {
        using AE = AboutError;
        switch (err)
        {
            // General
        case AE::None: return "no error";
        case AE::ApiNotSet: return "renderer API hasn't been set";

            // Windows OS
        case AE::w_WindowClass: return "couldn't create window class";
        case AE::w_Window: return "couldn't create window object";
        case AE::w_WindowDC: return "couldn't create window DC";
        case AE::w_ChoosePixelFormatARB: return "couldn't choose pixel format (ARB)";
        case AE::w_SetPixelFormat: return "couldn't set pixel format";
        case AE::w_CreateContextAttribsARB: return "couldn't create context attribs (ARB)";
        case AE::w_CreateModernContext: return "couldn't create modern context";
        case AE::w_GetCurrentContext: return "couldn't get current context";
        case AE::w_GetCurrentDC: return "couldn't get current DC";
        case AE::w_DummyWindowClass: return "couldn't create dummy window class";
        case AE::w_DummyWindow: return "couldn't create dummy window object";
        case AE::w_DummyWindowDC: return "couldn't create dummy window DC";
        case AE::w_DummyChoosePixelFormat: return "couldn't choose dummy pixel format";
        case AE::w_DummySetPixelFormat: return "couldn't set dummy pixel format";
        case AE::w_DummyCreateContext: return "couldn't create dummy context";
        case AE::w_MissingChoosePixelFormatARB: return "missing wglChoosePixelFormatARB";
        case AE::w_MissingCreateContextAttribsARB: return "missing wgCreateContextAttribsARB";
        case AE::w_MissingSwapIntervalEXT: return "missing wglSwapIntervalEXT";
        case AE::w_CreateCompatibleDC: return "couldn't create compatibale DC";
        case AE::w_CreateDibSection: return "couldn't create DIB section";
        case AE::w_SelectObject: return "couldn't select object";
#ifdef IO_NOSTD
        case AE::w_FreestandingDeletedOpCalled: return "operator delete called in freestanding mode";
#endif
        default: return "unknown error";
        }
    } // what
#pragma endregion
} // namespace hi