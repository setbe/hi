#include "io.hpp"
#include "gl_loader.hpp"

#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32)
#      ifndef WIN32_LEAN_AND_MEAN
#          define WIN32_LEAN_AND_MEAN
#      endif
#      ifndef NOMINMAX
#          define NOMINMAX
#      endif
#      include <Windows.h>
#   elif defined(__linux__)
// x11
#   else
#       error "OS isn't specified"
#   endif
#endif

namespace hi {
#pragma region types
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
#ifdef IO_IMPLEMENTATION
        unsigned char key_array[static_cast<size_t>(Key::__LAST__)]{0};
#endif
    } // namespace global

    struct Key_t {
    private:
        Key _key;
    public:
        IO_CONSTEXPR Key_t(Key k) noexcept : _key{ k } { }
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

    // ============================================================================
    //                    E R R O R   P R O C E S S I N G
    // ============================================================================

    enum class Error : int {
        None = 0,

        Window,
        Opengl,
    }; // enum class error

    // Extra detailed info (must be useful for crash log)
    enum class AboutError : int {
        None = 0,
        Unknown,

        MissingOpenglFunction,         // e.g. EXT function
        MissingRequiredOpenglFunction, // e.g. ARB function

        // ------ `w_` stands for Windows ------
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
    }; // enum class about_error

    IO_NODISCARD IO_CONSTEXPR
        static const char* what(AboutError err) noexcept {
        using AE = AboutError;
        switch (err)
        {
        // General
        case AE::None: return "no error";

        // Windows OS
        case AE::w_WindowClass: return "couldn't create window class";
        case AE::w_Window: return "couldn't create window";
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
        case AE::MissingOpenglFunction:         return "optional OpenGL entry point isn't provided by the driver";
        case AE::MissingRequiredOpenglFunction: return "required OpenGL entry point isn't provided by the driver";
        default: return "unknown error";
        }
    } // what
#pragma endregion

    namespace native {
        // -- Forward declarations ---
        struct Opengl;      // Native OpenGL Context
        struct Window;      // Native Window Context
    } // namespace native

    struct IWindow {
        // --- Derived Events ---
        virtual void onRender() noexcept = 0;
        virtual void onError(Error e, AboutError ae) noexcept = 0;
        virtual void onScroll(float deltaX, float deltaY) noexcept = 0;
        virtual void onWindowResize(int width, int height) noexcept = 0;
        virtual void onMouseMove(int x, int y) noexcept = 0;
        virtual void onKeyDown(Key k) noexcept = 0;
        virtual void onKeyUp(Key k) noexcept = 0;
        virtual void onFocusChange(bool gained) noexcept = 0;

        // --- Defined by library ---
        virtual void Render() noexcept = 0;
        virtual void onGeometryChange(int w, int h) noexcept = 0;
        IO_NODISCARD virtual RendererApi api() const noexcept = 0;
        IO_NODISCARD virtual       native::Opengl& opengl()        noexcept = 0;
        IO_NODISCARD virtual const native::Opengl& opengl()  const noexcept = 0;
        IO_NODISCARD virtual       native::Window& native()       noexcept = 0;
        IO_NODISCARD virtual const native::Window& native() const noexcept = 0;
        IO_NODISCARD virtual int width() const noexcept = 0;
        IO_NODISCARD virtual int height() const noexcept = 0;
    }; // IWindow

    namespace native {
        // --------------------------- Native Window --------------------------
        struct Window {
        private:
#ifdef IO_IMPLEMENTATION
#       ifdef __linux__
            // linux impl
#       elif defined(_WIN32)
            HDC _hdc{ nullptr };
            HWND _hwnd{ nullptr };
#       endif
#endif // IO_IMPLEMENTATION

        public:
            inline explicit Window(IWindow& win, int width, int height, bool shown, bool bordless) noexcept;
            inline ~Window() noexcept;
            Window(const Window&) = delete;
            Window& operator=(const Window&) = delete;
            Window(Window&&) = delete;
            Window& operator=(Window&&) = delete;

            inline bool PollEvents(IWindow& win) const noexcept;
        public:
            inline void setTitle(const io::char_view title) const noexcept;
            inline void setShow(bool) const noexcept;
            inline void setFullscreen(bool) const noexcept;
            inline void setCursorVisible(bool) const noexcept;

        public:
#ifdef IO_IMPLEMENTATION
#       if defined(__linux__)
            // linux impl
#       elif defined(_WIN32)
            HDC getHdc() const noexcept { return _hdc; }
            void setHdc(HDC new_hdc) noexcept { _hdc = new_hdc; }
            HWND getHwnd() const noexcept { return _hwnd; }
            void setHwnd(HWND new_hwnd) noexcept { _hwnd = new_hwnd; }
#       endif
#endif // IO_IMPLEMENTATION
        }; // struct Window


#ifdef IO_IMPLEMENTATION
        IO_CONSTEXPR_VAR wchar_t WINDOW_CLASSNAME[]{ L"_" };
        static LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept;

        inline Window::Window(IWindow& win, int width, int height, bool shown, bool bordless) noexcept {
            // ---- 1. Register class
            static bool is_registered{ false };
            HINSTANCE hinstance{ GetModuleHandleW(nullptr) };
            if (!is_registered) {
                WNDCLASSEXW wc{};
                wc.cbSize = sizeof(WNDCLASSEXW);
                wc.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
                wc.lpfnWndProc = WinProc;
                wc.hInstance = hinstance;
                wc.lpszClassName = WINDOW_CLASSNAME;
                wc.hIcon = LoadIconA(hinstance, IDI_APPLICATION);
                wc.hCursor = LoadCursorA(hinstance, IDC_ARROW);

                if (!RegisterClassExW(&wc))
                    win.onError(Error::Window, AboutError::w_WindowClass);

                is_registered = true;
            }

            const DWORD style{
                // If borderless window -> use popup style (optionally visible)
                bordless ? (WS_POPUP | (shown ? WS_VISIBLE : 0))
                :
                // Otherwise -> standard windowed style with full frame controls
                (WS_OVERLAPPED | WS_CAPTION |
                 WS_SYSMENU | WS_THICKFRAME |
                 WS_MINIMIZEBOX | WS_MAXIMIZEBOX |
                 (shown ? WS_VISIBLE : 0)) }; // optionally visible

            // ---- 2. Create Window
            win.native().setHwnd(CreateWindowExW(
                /* dwExStyle    */ 0,
                /* lpClassName  */ native::WINDOW_CLASSNAME,
                /* lpWindowName */ native::WINDOW_CLASSNAME,
                /* dwStyle      */ style,
                /* x            */ CW_USEDEFAULT,
                /* y            */ CW_USEDEFAULT,
                width,
                height,
                /* hWndParent   */ nullptr,
                /* hMenu        */ nullptr,
                /* hInstance    */ hinstance,
                /* lpParam      */ static_cast<void*>(&win)));
            HWND wnd = win.native().getHwnd();
            if (!wnd) {
                win.onError(Error::Window, AboutError::w_Window);
                return;
            }
            SetWindowLongW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&win));

            // ---- 3. Get DC
            HDC new_hdc = GetDC(wnd);
            win.native().setHdc(new_hdc);
            if (!win.native().getHdc()) {
                win.onError(Error::Window, AboutError::w_WindowDC);
                return;
            }
        } // Window

        inline Window::~Window() noexcept {
            if (_hdc)  ReleaseDC(_hwnd, _hdc);
            if (_hwnd) DestroyWindow(_hwnd);
            _hwnd = nullptr; _hdc = nullptr;
        } // ~Window

        inline bool Window::PollEvents(IWindow& win) const noexcept {
            MSG msg{};
            while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                if (msg.message == WM_QUIT) return false;
                TranslateMessage(&msg);
                DispatchMessageW(&msg);
            }
            return true;
        }
        inline void Window::setTitle(const io::char_view title) const noexcept {
            if (!title) return; // empty string
            ::io::wstring temp;
            if (!::io::native::utf8_to_wide(title, temp))
                return; // conversion or alloc errors are happened
            SetWindowTextW(_hwnd, temp.data());
        }
        inline void Window::setShow(bool value) const noexcept { ShowWindow(_hwnd, value ? SW_SHOW : SW_HIDE); }
        inline void Window::setFullscreen(bool value) const noexcept {
            if (!value) {
                // Restore fixed windowed style (standard overlapped window)
                SetWindowLongW(_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW | WS_VISIBLE);
                // Optional: set default size and center
                SetWindowPos(_hwnd, HWND_NOTOPMOST,
                    100, 100, 1280, 720, SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
                return;
            }

            // TODO: maybe I should create some cache for window styles?
            // Remove window borders and make fullscreen
            SetWindowLongW(_hwnd, GWL_STYLE, GetWindowLongW(_hwnd, GWL_STYLE) & ~WS_OVERLAPPEDWINDOW);

            HMONITOR monitor = MonitorFromWindow(_hwnd, MONITOR_DEFAULTTONEAREST);
            MONITORINFO mi = { sizeof(MONITORINFO) };
            if (!GetMonitorInfoW(monitor, &mi))
                return; // just exit in case of an error

            SetWindowPos(/*   hWnd */ _hwnd,
                /* hWndInsertAfter */ HWND_TOP,
                /*      X */ mi.rcMonitor.left,
                /*      Y */ mi.rcMonitor.top,
                /*     cx */ mi.rcMonitor.right - mi.rcMonitor.left,
                /*     cy */ mi.rcMonitor.bottom - mi.rcMonitor.top,
                /* uFlags */ SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
        }
        inline void Window::setCursorVisible(bool value) const noexcept {
            // Adjusts the cursor visibility to match 'value'.
            // ShowCursor increments/decrements the internal display counter and returns the new count.
            int shown = ShowCursor(value);
            // Keep calling ShowCursor until the cursor's visibility matches the desired state.
            while ((value && shown < 0) || (!value && shown >= 0)) {
                shown = ShowCursor(value);
            }
        }


        // WPARAM -> Key
        static Key FindKeyFromWparam(WPARAM wparam) noexcept {
            Key_t k{ Key::__NONE__ };
            int _A = static_cast<int>(Key::A);
            int _0 = static_cast<int>(Key::_0);
            int _F1 = static_cast<int>(Key::F1);
            int wp = static_cast<int>(wparam);

            if (wp >= 'A' && wp <= 'Z')                 // Letters A-Z 
                return Key_t{ wp - 'A' + _A }.key();
            if (wp >= '0' && wp <= '9')                 // Digits 0-9
                return Key_t{ wp - '0' + _0 }.key();
            if (wp >= VK_F1 && wp <= VK_F12)            // F1-F12
                return Key_t{ wp - VK_F1 + _F1 }.key();
            if (wp >= VK_NUMPAD0 && wp <= VK_NUMPAD9)   // Numpad 0-9
                return Key_t{ wp - VK_NUMPAD0 + _0 }.key();

            switch (wparam) {
                // Modifiers
            case VK_SHIFT:    return Key::Shift;
            case VK_CONTROL:  return Key::Control;
            case VK_MENU:     return Key::Alt;
            case VK_LWIN:     return Key::Super;

                // TTY
            case VK_ESCAPE:   return Key::Escape;
            case VK_INSERT:   return Key::Insert;
            case VK_DELETE:   return Key::Delete;
            case VK_BACK:     return Key::Backspace;
            case VK_TAB:      return Key::Tab;
            case VK_RETURN:   return Key::Return;
            case VK_SCROLL:   return Key::ScrollLock;
            case VK_NUMLOCK:  return Key::NumLock;
            case VK_CAPITAL:  return Key::CapsLock;

                // Navigation
            case VK_HOME:     return Key::Home;
            case VK_END:      return Key::End;
            case VK_PRIOR:    return Key::PageUp;
            case VK_NEXT:     return Key::PageDown;

                // Arrows
            case VK_LEFT:     return Key::Left;
            case VK_UP:       return Key::Up;
            case VK_RIGHT:    return Key::Right;
            case VK_DOWN:     return Key::Down;

                // Mouse
            case VK_LBUTTON:  return Key::MouseLeft;
            case VK_RBUTTON:  return Key::MouseRight;
            case VK_MBUTTON:  return Key::MouseMiddle;
            case VK_XBUTTON1: return Key::MouseX1;
            case VK_XBUTTON2: return Key::MouseX2;

                // Symbols
            case VK_SPACE:      return Key::Space;
            case VK_OEM_MINUS:  return Key::Hyphen;
            case VK_OEM_PLUS:   return Key::Equal;
            case VK_OEM_1:      return Key::Semicolon;
            case VK_OEM_2:      return Key::Slash;
            case VK_OEM_3:      return Key::Grave;
            case VK_OEM_4:      return Key::BracketLeft;
            case VK_OEM_5:      return Key::Backslash;
            case VK_OEM_6:      return Key::BracketRight;
            case VK_OEM_7:      return Key::Apostrophe;
            case VK_OEM_COMMA:  return Key::Comma;
            case VK_OEM_PERIOD: return Key::Period;

            default:          return Key::__NONE__;
            }
        }

        // Key pressed/released
        static Key HandleKey(WPARAM wparam, bool pressed) noexcept {
            Key_t kt{ Key::__NONE__ };
            kt = FindKeyFromWparam(wparam);

            hi::global::key_array[static_cast<int>(kt.key())] = pressed ? 1 : 0;
            return kt.key();
        }

        static LRESULT CALLBACK WinProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) noexcept {
            IWindow* win = reinterpret_cast<IWindow*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
            if (!win) return DefWindowProcW(hwnd, msg, wparam, lparam);
            switch (msg) {
            case WM_PAINT: win->Render(); return 0;
            case WM_SIZE: {
                if (win->api() == RendererApi::None) return 0; // handled
                if (wparam == SIZE_MINIMIZED) {
                    win->onFocusChange(false);
                    return 0;
                }
                RECT r;
                GetClientRect(hwnd, &r);
                win->onGeometryChange(static_cast<int>(r.right - r.left),  // w
                    static_cast<int>(r.bottom - r.top)); // h
                PostMessageW(hwnd, WM_PAINT, 0, 0);
                return 0; // handled
            } // WM_SIZE
            case WM_MOUSEMOVE: win->onMouseMove(LOWORD(lparam), HIWORD(lparam)); return 0;
            case WM_SETFOCUS:  win->onFocusChange(true); return 0;
            case WM_KILLFOCUS: win->onFocusChange(false); return 0;
            case WM_MOUSEWHEEL: {
                int delta = GET_WHEEL_DELTA_WPARAM(wparam); // Returns 120 or -120
                // @TODO Replace `0.f` with actual horizontal scroll delta
                win->onScroll(delta / 120.f, 0.f); // Normalize before callback
                return 0;
            }
            case WM_KEYDOWN: win->onKeyDown(HandleKey(wparam, true)); return 0;
            case WM_KEYUP: win->onKeyUp(HandleKey(wparam, false)); return 0;
            case WM_SYSKEYDOWN:
                // Handle system keys as ordinary keys
                if (wparam == VK_F10)       win->onKeyDown(Key::F10);
                else if (wparam == VK_MENU) win->onKeyDown(Key::Alt);
                return 0;
            case WM_SYSKEYUP:
                // Handle system keys as ordinary keys
                if (wparam == VK_F10)       win->onKeyUp(Key::F10);
                else if (wparam == VK_MENU) win->onKeyUp(Key::Alt);
                return 0;
            case WM_NCCREATE: {
                auto cs = reinterpret_cast<CREATESTRUCTW*>(lparam);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)(cs->lpCreateParams));
#ifdef _PSAPI_H_ // Clear resources
                SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
                K32EmptyWorkingSet(GetCurrentProcess());
#endif
                return DefWindowProcW(hwnd, msg, wparam, lparam);
            } // WM_NCCREATE
            case WM_DESTROY: PostQuitMessage(0); return 0;
            } // switch (msg)

            return DefWindowProcW(hwnd, msg, wparam, lparam);
        } // WinProc
#endif // IO_IMPLEMENTATION

        // -------------------- Native Opengl Context -------------------------
        struct Opengl {
        private:
#ifdef IO_IMPLEMENTATION
#   if defined(__linux__)
            // linux impl
#   elif defined(_WIN32)
            HGLRC _hglrc{ nullptr };
#   endif
#endif // IO_IMPLEMENTATION

            io::u8 _major;
            io::u8 _minor;

        public:
            Opengl() noexcept = delete;
            explicit inline Opengl(io::u8 major, io::u8 minor) noexcept
                : _major{ major }, _minor{ minor } {};
            ~Opengl() noexcept;
            // Non-copyable, non-movable
            Opengl(const Opengl&) = delete;
            Opengl& operator=(const Opengl&) = delete;
            Opengl(Opengl&&) = delete;
            Opengl& operator=(Opengl&&) = delete;

        public:
            inline void Render(IWindow&) const noexcept;
            inline void SwapBuffers(const IWindow&) const noexcept;
            IO_NODISCARD AboutError CreateContext(IWindow&) noexcept;

            IO_NODISCARD inline io::u8 currentMajorVersion() const noexcept { return _major; }
            IO_NODISCARD inline io::u8 currentMinorVersion() const noexcept { return _minor; }

#ifdef IO_IMPLEMENTATION
#   if defined(__linux__)

#   elif defined(_WIN32)
            HGLRC getHglrc() const noexcept { return _hglrc; }
            void setHglrc(HGLRC new_hglrc) noexcept { _hglrc = new_hglrc; }
#   endif
#endif // IO_IMPLEMENTATION
        }; // struct OpenglContext

#ifdef IO_IMPLEMENTATION
        inline void Opengl::Render(IWindow& win) const noexcept {
            PAINTSTRUCT ps;
            HWND wnd = win.native().getHwnd();
            HDC dc = win.native().getHdc();
            HGLRC glc = getHglrc();

            BeginPaint(wnd, &ps);

            if (wglGetCurrentContext() != glc ||
                wglGetCurrentDC() != dc) {
                wglMakeCurrent(dc, glc);
            }
            win.onRender();

            EndPaint(wnd, &ps);
        } // Render
        inline void Opengl::SwapBuffers(const IWindow& win) const noexcept {
#ifdef IO_CXX_17
            static_assert(native::PF_DESCRIPTOR.dwFlags & PFD_DOUBLEBUFFER,
                "OpenGL renderer requires double buffering");
#endif
            ::SwapBuffers(reinterpret_cast<HDC>(win.native().getHdc()));
        } // SwapBuffers
#pragma region _dummy window
        // ----------------- Dummy Window for Modern OpenGL -----------------------

        IO_CONSTEXPR_VAR LPCWSTR DUMMY_CLASS_NAME = L"d";
        struct DummyWindow {
            // Constructor
            inline DummyWindow() noexcept :
                _hinstance{ GetModuleHandleW(nullptr) },
                _hwnd{ nullptr }, _hdc{ nullptr }, _ctx{ nullptr } {

                WNDCLASSW wc{};
                wc.style = CS_OWNDC;
                wc.lpfnWndProc = DefWindowProcW;
                wc.hInstance = _hinstance;
                wc.lpszClassName = DUMMY_CLASS_NAME;

                if (!RegisterClassW(&wc))
                    return;

                _hwnd = CreateWindowExW(
                    /*    dwExStyle */ 0,
                    /*  lpClassName */ DUMMY_CLASS_NAME,
                    /* lpWindowName */ L" ",
                    /*      dwStyle */ WS_OVERLAPPEDWINDOW,
                    /*            X */ CW_USEDEFAULT,
                    /*            Y */ CW_USEDEFAULT,
                    /*       nWidth */ 1,
                    /*      nHeight */ 1,
                    /*   hWndParent */ nullptr,
                    /*        hMenu */ nullptr,
                    /*    hInstance */ _hinstance,
                    /*      lpParam */ nullptr);
                if (!_hwnd)
                    return;

                _hdc = GetDC(_hwnd);
            } // DummyWindow 

            inline ~DummyWindow() noexcept {
                // Fully clear members in this class
                if (_ctx) {
                    if (wglGetCurrentContext() == _ctx)
                        wglMakeCurrent(nullptr, nullptr);
                    wglDeleteContext(_ctx);
                    _ctx = nullptr;
                }
                if (_hdc) { ReleaseDC(_hwnd, _hdc); _hdc = nullptr; }
                if (_hwnd) { DestroyWindow(_hwnd);  _hwnd = nullptr; }
                UnregisterClassW(DUMMY_CLASS_NAME, _hinstance);
                _hinstance = nullptr;
            } // ~DummyWindow

            // --- Getters ---
            inline HINSTANCE hinstance() const noexcept { return _hinstance; }
            inline HWND hwnd() const noexcept { return _hwnd; }
            inline HDC hdc() const noexcept { return _hdc; }
            inline HGLRC ctx() const noexcept { return _ctx; }
            // --- Setters ---
            inline void setCtx(HGLRC ctx) noexcept { _ctx = ctx; }

        private:
            HINSTANCE _hinstance;
            HWND _hwnd;
            HDC _hdc;
            HGLRC _ctx;
        }; // struct DummyWindow
#pragma endregion DummyWindow
#pragma region WGL
        // ========================================================================
        //                          Load WGL Extensions
        // ========================================================================

        namespace wgl {
            // minimal ARB
            IO_CONSTEXPR_VAR struct /* arb */ {
                IO_CONSTEXPR_VAR static int DRAW_TO_WINDOW = 0x2001;
                IO_CONSTEXPR_VAR static int SUPPORT_OPENGL = 0x2010;
                IO_CONSTEXPR_VAR static int DOUBLE_BUFFER = 0x2011;
                IO_CONSTEXPR_VAR static int PIXEL_TYPE = 0x2013;
                IO_CONSTEXPR_VAR static int TYPE_RGBA = 0x202B;
                IO_CONSTEXPR_VAR static int COLOR_BITS = 0x2014;
                IO_CONSTEXPR_VAR static int DEPTH_BITS = 0x2022;
                IO_CONSTEXPR_VAR static int STENCIL_BITS = 0x2023;

                IO_CONSTEXPR_VAR static int CONTEXT_MAJOR_VERSION = 0x2091;
                IO_CONSTEXPR_VAR static int CONTEXT_MINOR_VERSION = 0x2092;
                IO_CONSTEXPR_VAR static int CONTEXT_PROFILE_MASK = 0x9126;
                IO_CONSTEXPR_VAR static int CONTEXT_CORE_PROFILE_BIT = 0x00000001;

            } arb;

            typedef HGLRC(WINAPI* PFNWGLCREATECONTEXTATTRIBSARBPROC)(
                HDC hdc, HGLRC shareContext, const int* attribList);

            typedef BOOL(WINAPI* PFNWGLCHOOSEPIXELFORMATARBPROC)(
                HDC hdc, const int* piAttribIList, const FLOAT* pfAttribFList,
                UINT nMaxFormats, int* piFormats, UINT* nNumFormats);
            typedef BOOL(WINAPI* PFNWGLSWAPINTERVALEXTPROC)(int interval);

            // For future
            // Enable VSync
            /*typedef BOOL(APIENTRY* PFNWGLSWAPINTERVALEXTPROC)(int interval);
            PFNWGLSWAPINTERVALEXTPROC wglSwapIntervalEXT =
                (PFNWGLSWAPINTERVALEXTPROC)(void*)wglGetProcAddress(
                    "wglSwapIntervalEXT");
            if (!wglSwapIntervalEXT)
                return AboutError::Win32_Missing_SwapIntervalEXT;

            wglSwapIntervalEXT(1);*/

            IO_CONSTEXPR_VAR int PIXEL_ATTRS[]{
                arb.DRAW_TO_WINDOW, GL_TRUE,
                arb.SUPPORT_OPENGL, GL_TRUE,
                arb.DOUBLE_BUFFER,  GL_TRUE,
                arb.PIXEL_TYPE,     arb.TYPE_RGBA,
                arb.COLOR_BITS,     32,
                arb.DEPTH_BITS,     24,
                arb.STENCIL_BITS,   8,
                0 /* The end of the array */ }; // PIXEL_ATTRS
            IO_CONSTEXPR_VAR int CONTEXT_ATTRS[]{
                arb.CONTEXT_MAJOR_VERSION, 3,
                arb.CONTEXT_MINOR_VERSION, 3,
                arb.CONTEXT_PROFILE_MASK,  arb.CONTEXT_CORE_PROFILE_BIT,
                0 /* The end of the array */ }; // CONTEXT_ATTRS
            IO_CONSTEXPR_VAR PIXELFORMATDESCRIPTOR PF_DESCRIPTOR{
                /*        nSize */ sizeof(PF_DESCRIPTOR),
                /*     nVersion */ 1,
                /*      dwFlags */ PFD_DRAW_TO_WINDOW
                                 | PFD_SUPPORT_OPENGL
                                 | PFD_DOUBLEBUFFER,
                /*   iPixelType */ PFD_TYPE_RGBA,
                /*   cColorBits */ 32,
                /*   cDepthBits */ 24,
                /* cStencilBits */ 8,
                /*   iLayerType */ PFD_MAIN_PLANE }; // PF_DESCRIPTOR

            static inline AboutError LoadExtensions(DummyWindow& dummy,
                wgl::PFNWGLCHOOSEPIXELFORMATARBPROC& choose,
                wgl::PFNWGLCREATECONTEXTATTRIBSARBPROC& create) noexcept
            {
                // Create dummy context
                PIXELFORMATDESCRIPTOR pfd{};
                pfd.nSize = sizeof(pfd);
                pfd.nVersion = 1;
                pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
                pfd.iPixelType = PFD_TYPE_RGBA;
                pfd.cColorBits = 32;
                int format = ChoosePixelFormat(dummy.hdc(), &pfd);
                if (format == 0) return AboutError::w_DummyChoosePixelFormat;

                if (!SetPixelFormat(dummy.hdc(), format, &pfd)) return AboutError::w_DummySetPixelFormat;

                dummy.setCtx(wglCreateContext(dummy.hdc()));
                if (!dummy.ctx() || !wglMakeCurrent(dummy.hdc(), dummy.ctx()))
                    return AboutError::w_DummyCreateContext;

                // Load extensions
                choose = (wgl::PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
                create = (wgl::PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");

                if (!choose) return AboutError::w_MissingChoosePixelFormatARB;
                if (!create) return AboutError::w_MissingCreateContextAttribsARB;

                return AboutError::None;
            } // LoadExtensions

            // ---------------------- Create Modern Context ---------------------------

            static inline void* OpenglLoader(const char* name) noexcept {
                void* p = (void*)wglGetProcAddress(name);
                if (!p || p == (void*)0x1 ||
                    p == (void*)0x2 ||
                    p == (void*)0x3 ||
                    p == (void*)-1)
                {
                    static HMODULE module = LoadLibraryA("opengl32.dll");
                    p = (void*)GetProcAddress(module, name);
                }
                return p;
            } // OpenglLoader

            static inline AboutError CreateModernContext(IWindow& win,
                wgl::PFNWGLCHOOSEPIXELFORMATARBPROC    choose,
                wgl::PFNWGLCREATECONTEXTATTRIBSARBPROC create) noexcept
            {
                int format;
                UINT num_formats;
                HDC main_dc{ reinterpret_cast<HDC>(win.native().getHdc()) };
                PIXELFORMATDESCRIPTOR pfd{};
                HGLRC ctx;

                if (!choose(main_dc, wgl::PIXEL_ATTRS, nullptr, 1, &format, &num_formats))
                    return AboutError::w_ChoosePixelFormatARB;
                if (!SetPixelFormat(main_dc, format, &pfd)) return AboutError::w_SetPixelFormat;

                ctx = create(main_dc, nullptr, wgl::CONTEXT_ATTRS);
                if (!ctx) return AboutError::w_CreateContextAttribsARB;
                if (!wglMakeCurrent(main_dc, ctx)) return AboutError::w_CreateModernContext;

                if (!wglGetCurrentContext()) return AboutError::w_GetCurrentContext;
                if (!wglGetCurrentDC())      return AboutError::w_GetCurrentDC;

                win.opengl().setHglrc(ctx);
                const io::u8 major = win.opengl().currentMajorVersion();
                const io::u8 minor = win.opengl().currentMinorVersion();
                gl::loader = OpenglLoader;
                if (!::gl::load(major, minor).empty()) return AboutError::MissingRequiredOpenglFunction;
                // here we can also load EXT functions

                return AboutError::None;
            } // CreateModernContext
        } // namespace wgl
#pragma endregion WGL
    // ========================================================================
    //                          namespace hi::native::*
    // ========================================================================
        IO_NODISCARD AboutError Opengl::CreateContext(IWindow& win) noexcept {
            DummyWindow dummy{};

            wgl::PFNWGLCHOOSEPIXELFORMATARBPROC    choose{ nullptr };
            wgl::PFNWGLCREATECONTEXTATTRIBSARBPROC create{ nullptr };
            if (!dummy.hinstance()) return AboutError::w_DummyWindow;
            AboutError ae = wgl::LoadExtensions(dummy, choose, create);
            if (ae != AboutError::None) return ae;
            return wgl::CreateModernContext(win, choose, create);
        }
        Opengl::~Opengl() noexcept {
            HGLRC ctx = reinterpret_cast<HGLRC>(_hglrc);
            // Unbind context from any HDC (just in case it's current)
            if (wglGetCurrentContext() == _hglrc) wglMakeCurrent(nullptr, nullptr);
            if (_hglrc) wglDeleteContext(_hglrc); // Delete OpenGL rendering context
        }
    
#endif // IO_IMPLEMENTATION
    } // namespace native

    // --- CRTP base ---
    template <typename Derived>
    struct Window : public IWindow {
    public:
        union { native::Opengl g; };

        explicit inline Window(int w = 440, int h = 320, bool shown = true, bool bordless = false) noexcept;
        inline ~Window() noexcept;

        // Non-copyable, non-movable
        Window(const Window&) = delete;
        Window& operator=(const Window&) = delete;
        Window(Window&&) = delete;
        Window& operator=(Window&&) = delete;

        Derived* self() noexcept { return static_cast<Derived*>(this); }

        // --- Implement interface using CRTP dispatch ---
        inline void onRender() noexcept override { }
        inline void onError(Error e, AboutError ae)       noexcept override { }
        inline void onScroll(float deltaX, float deltaY)  noexcept override { }
        inline void onWindowResize(int width, int height) noexcept override { }
        inline void onMouseMove(int x, int y)  noexcept override { }
        inline void onKeyDown(Key k)           noexcept override { }
        inline void onKeyUp(Key k)             noexcept override { }
        inline void onFocusChange(bool gained) noexcept override { }
        IO_NODISCARD RendererApi api() const noexcept override { return _renderer_api; }

        // Getters
        IO_NODISCARD inline       native::Opengl& opengl()       noexcept override { return g; }
        IO_NODISCARD inline const native::Opengl& opengl() const noexcept override { return g; }
        IO_NODISCARD inline       native::Window& native()       noexcept override { return _native_window; }
        IO_NODISCARD inline const native::Window& native() const noexcept override { return _native_window; }
        IO_NODISCARD inline int width() const noexcept override { return _width; }
        IO_NODISCARD inline int height() const noexcept override { return _height; }
        inline void onGeometryChange(int w, int h) noexcept override;

    public:
        IO_NODISCARD inline bool PollEvents() const noexcept {
            // const Window<Derived>* -> IWindow&
            auto* self_nc = const_cast<Window<Derived>*>(this);
            return native().PollEvents(static_cast<IWindow&>(*self_nc));
        }
        inline void Render() noexcept override;
        inline void SwapBuffers() const noexcept;
        void Quit() const noexcept {
#ifdef IO_IMPLEMENTATION
#   ifdef _WIN32
               PostMessageW(reinterpret_cast<HWND>(native().getHwnd()), WM_QUIT, 0, 0);
#   else
#      error "Not implemented"
#   endif
#endif // IO_IMPLEMENTATION
        }

        // --- Setters ---
        inline void setShow(bool value) const noexcept { native().setShow(value); }
        inline void setTitle(const io::char_view new_title) const noexcept { native().setTitle(new_title); }
        inline void setFullscreen(bool value) const noexcept { native().setFullscreen(value); }
        inline void setCursorVisible(bool value) const noexcept { native().setCursorVisible(value); }
        void setApi(RendererApi api, io::u8 major, io::u8 minor) noexcept;

    private:
        native::Window _native_window;
        int _width;
        int _height;

        RendererApi _renderer_api{ RendererApi::None };
        bool _renderer_alive{ false };
    }; // struct Window



    template <typename Derived>
    Window<Derived>::Window(int w, int h, bool shown, bool bordless) noexcept
        : _width{ w }, _height{ h }, _native_window{ *this,w,h,shown,bordless } { }

    template <typename Derived>
    inline Window<Derived>::~Window() noexcept {
        if (_renderer_alive) {
            switch (api()) {
            case RendererApi::Opengl: g.~Opengl(); break;
            case RendererApi::Vulkan: break; // @TODO
            default: break;
            } // switch
            _renderer_alive = false;
        }
    }

#ifdef IO_IMPLEMENTATION
    template <typename Derived>
    inline void Window<Derived>::onGeometryChange(int w, int h) noexcept {
        HDC hdc = reinterpret_cast<HDC>(native().getHdc());
        // Update window size
        _width = w;
        _height = h;

        // Switch between APIs
        switch (api()) {
        case RendererApi::Opengl: // Call viewport    
            io::sleep_ms(7); // Hack: slow down the program.
            gl::Viewport(0, 0, w, h);
            break;
        default:
            break;
        } // switch renderer

        onWindowResize(w, h); // Call user defined callback
        onRender(); // Rerender the window
        SwapBuffers();
    } // onGeometryChange

    template <typename Derived>
    inline void Window<Derived>::Render() noexcept {
        if (!_renderer_alive) return;
        switch (api()) {
        case RendererApi::Opengl: g.Render(*this); break;
        case RendererApi::Vulkan: break; // @TODO
        } // switch
    } // render

    template <typename Derived>
    inline void Window<Derived>::SwapBuffers() const noexcept {
        if (!_renderer_alive) return;
        switch (api())
        {
        case RendererApi::Opengl: g.SwapBuffers(*this); break;
        case RendererApi::Vulkan:   break; // @TODO
        default: break;
        } // switch
    } // render

    template <typename Derived>
    void Window<Derived>::setApi(RendererApi api, io::u8 major, io::u8 minor) noexcept {
        if (api == _renderer_api) return;

        if (_renderer_alive) { // Destroy old renderer
            switch (_renderer_api) {
            case RendererApi::Opengl: {
                g.~Opengl();
                break;
            }
            case RendererApi::Vulkan: break; // @TODO
            }
            _renderer_alive = false;
        }

        _renderer_api = api;
        HDC hdc = reinterpret_cast<HDC>(native().getHdc());

        switch (_renderer_api) {
        case RendererApi::Opengl: {
            new (&g) native::Opengl(major, minor);
            _renderer_alive = true;

            Error err = Error::Opengl;
            AboutError about = g.CreateContext(*this);
            if (about != AboutError::None) {
                onError(Error::Opengl, about);
                g.~Opengl();
                _renderer_alive = false;
                _renderer_api = RendererApi::None;
                return;
            }
            gl::Viewport(0, 0, _width, _height);
            break;
        }
        case RendererApi::Vulkan: break; // @TODO
        default:
            break;
        } // switch
    } // set_api
#endif // IO_IMPLEMENTATION
} // namespace hi