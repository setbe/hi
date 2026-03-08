#include "io.hpp"
#include "gl_loader.hpp"
#include "../3rd_party/stb/stb_truetype_stream/stb_truetype_stream.hpp"
#include "../3rd_party/stb/stb_truetype_stream/codepoints/stbtt_codepoints_stream.hpp"

#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32)
#       ifndef WIN32_LEAN_AND_MEAN
#           define WIN32_LEAN_AND_MEAN
#       endif
#       ifndef NOMINMAX
#           define NOMINMAX
#       endif
#       include <Windows.h>
#       ifdef DrawText
#           undef DrawText
#       endif
#   elif defined(__linux__)
#       include <X11/Xlib.h>
#       include <X11/Xatom.h>
#       include <X11/keysym.h>
#       include <X11/Xutil.h>
#       include <GL/gl.h>
#       include <GL/glx.h>
#       ifdef None
#           undef None
#       endif
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
        extern unsigned char key_array[static_cast<io::usize>(Key::__LAST__)];
#ifdef IO_IMPLEMENTATION
        unsigned char key_array[static_cast<io::usize>(Key::__LAST__)]{0};
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
        static IO_CONSTEXPR io::usize size() noexcept { return static_cast<unsigned char>(Key::__LAST__); }
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

        Window,
        WindowDevice,

        // Missing functions
        MissingOpenglFunction,         // e.g. EXT function
        MissingRequiredOpenglFunction, // e.g. ARB function
        MissingChoosePixelFormatARB,
        MissingCreateContextAttribsARB,

        CreateContextAttribsARB,
        CreateModernContext,
        GetCurrentContext,

        // ------ `w_` stands for Windows ------
        w_WindowClass,
        // Opengl Window
        w_ChoosePixelFormatARB,
        w_SetPixelFormat,
        w_GetCurrentDC,
        // Dummy Window
        w_DummyWindowClass,
        w_DummyWindow,
        w_DummyWindowDC,
        w_DummyChoosePixelFormat,
        w_DummySetPixelFormat,
        w_DummyCreateContext,

        // ------ `l_` stands for Linux ------
        l_Display
    }; // enum class about_error

    IO_NODISCARD IO_CONSTEXPR
        static const char* what(AboutError err) noexcept {
        using AE = AboutError;
        switch (err)
        {
        // General
        case AE::None: return "no error";

        case AE::MissingOpenglFunction:          return "optional OpenGL entry point isn't provided by the driver";
        case AE::MissingRequiredOpenglFunction:  return "required OpenGL entry point isn't provided by the driver";
        case AE::MissingChoosePixelFormatARB:    return "missing wglChoosePixelFormatARB";
        case AE::MissingCreateContextAttribsARB: return "missing wgCreateContextAttribsARB";

        case AE::CreateContextAttribsARB: return "couldn't create context attribs (ARB)";
        case AE::CreateModernContext: return "couldn't create modern context";
        case AE::GetCurrentContext: return "couldn't get current context";

        case AE::Window: return "couldn't create window";
        case AE::WindowDevice: return "couldn't create window device";

        // Windows OS
        case AE::w_WindowClass: return "couldn't create window class";
        case AE::w_ChoosePixelFormatARB: return "couldn't choose pixel format (ARB)";
        case AE::w_SetPixelFormat: return "couldn't set pixel format";
        case AE::w_GetCurrentDC: return "couldn't get current DC";
        case AE::w_DummyWindowClass: return "couldn't create dummy window class";
        case AE::w_DummyWindow: return "couldn't create dummy window object";
        case AE::w_DummyWindowDC: return "couldn't create dummy window DC";
        case AE::w_DummyChoosePixelFormat: return "couldn't choose dummy pixel format";
        case AE::w_DummySetPixelFormat: return "couldn't set dummy pixel format";
        case AE::w_DummyCreateContext: return "couldn't create dummy context";
        // Linux
        case AE::l_Display: return "couldn't open XDisplay";
        
        default: return "unknown error";
        }
    } // what

    enum class Cursor {
        Arrow,      // Default arrow cursor
        IBeam,      // Text input cursor
        Crosshair,  // Crosshair cursor
        Hand,       // Hand pointer for links/buttons
        HResize,    // Horizontal resize
        VResize,    // Vertical resize
        Hidden      // Invisible cursor
    };
    enum class CursorState {
        None,
        Default, // Arrow
        TextEdit, // IBeam
        HResizing, // HResize
        VResizing, // VResize
        HoversButton, // Hand
    };
#pragma endregion

namespace native {
        // -- Forward declarations ---
        struct Opengl;      // Native OpenGL Context
        struct Window;      // Native Window Context
} // namespace native

struct IWindow {
    friend struct native::Window;

    // --- Derived Events ---
    virtual void onRender() noexcept = 0;
    virtual void onError(Error, AboutError) noexcept = 0;
    virtual void onScroll(float deltaX, float deltaY) noexcept = 0;
    virtual void onWindowResize(int width, int height) noexcept = 0;
    virtual void onMouseMove(int x, int y) noexcept = 0;
    virtual void onKeyDown(Key) noexcept = 0;
    virtual void onKeyUp(Key) noexcept = 0;
    virtual void onFocusChange(bool gained) noexcept = 0;
    // --- Defined by library ---
    virtual void Render() noexcept = 0;
    virtual void onGeometryChange(int w, int h) noexcept = 0;
    virtual void setCursor(Cursor) noexcept = 0;
    virtual void resetCursorState() noexcept = 0;

    IO_NODISCARD virtual RendererApi api() const noexcept = 0;
    IO_NODISCARD virtual       native::Opengl& opengl()        noexcept = 0;
    IO_NODISCARD virtual const native::Opengl& opengl()  const noexcept = 0;
    IO_NODISCARD virtual       native::Window& native()       noexcept = 0;
    IO_NODISCARD virtual const native::Window& native() const noexcept = 0;
    IO_NODISCARD virtual int width() const noexcept = 0;
    IO_NODISCARD virtual int height() const noexcept = 0;
    IO_NODISCARD virtual float mouseX() const noexcept = 0;
    IO_NODISCARD virtual float mouseY() const noexcept = 0;
    IO_NODISCARD virtual CursorState getCursorState() const noexcept = 0;

    IO_NODISCARD virtual bool isShown() const noexcept = 0;
    IO_NODISCARD virtual bool isFullscreen() const noexcept = 0;
    IO_NODISCARD virtual bool isCursorVisible() const noexcept = 0;
    IO_NODISCARD virtual bool isMouseDown() const noexcept = 0;
    IO_NODISCARD virtual bool isPrevMouseDown() const noexcept = 0;
    IO_NODISCARD virtual float UiScale() const noexcept = 0;

protected:
    virtual void setMouseX(float) noexcept = 0;
    virtual void setMouseY(float) noexcept = 0;
    virtual void setMouseDown(bool) noexcept = 0;
}; // IWindow

namespace native {
    // --------------------------- Native Window --------------------------
    struct Window {
        private:
#ifdef IO_IMPLEMENTATION
#       ifdef __linux__
            ::Display* _dpy{ nullptr };
            ::Window   _xwnd{ 0 };
            ::Atom     _wm_delete{ 0 };
            ::Colormap _cmap{ 0 };
            bool       _mapped{ false };
#       elif defined(_WIN32)
            HDC _hdc{ nullptr };
            HWND _hwnd{ nullptr };
#       endif
#endif // IO_IMPLEMENTATION
        CursorState _last_cursor_st{ CursorState::None };

        public:
            inline explicit Window(IWindow& win, int width, int height, bool shown, bool bordless) noexcept;
            inline ~Window() noexcept;
            Window(const Window&) = delete;
            Window& operator=(const Window&) = delete;
            Window(Window&&) = delete;
            Window& operator=(Window&&) = delete;

            inline bool PollEvents(IWindow& win) const noexcept;
        public:
            inline void setTitle(io::char_view title) const noexcept;
            inline void setShow(bool) const noexcept;
            inline void setFullscreen(bool) const noexcept;
            inline void setCursor(Cursor c = Cursor::Arrow) const noexcept;
            inline void setCursor(CursorState cs) const noexcept {
                switch (cs)
                {
                case hi::CursorState::TextEdit: setCursor(hi::Cursor::IBeam); break;
                case hi::CursorState::HResizing: setCursor(hi::Cursor::HResize); break;
                case hi::CursorState::VResizing: setCursor(hi::Cursor::VResize); break;
                case hi::CursorState::HoversButton: setCursor(hi::Cursor::Hand); break;
                case hi::CursorState::Default: setCursor(hi::Cursor::Arrow); break;
                default: break;
                }
            }

            inline void updateLastCursorState(CursorState cs) noexcept { _last_cursor_st = cs; }
            inline CursorState getLastCursorState() const noexcept { return _last_cursor_st; }

            static void handleMouseButton(IWindow* w, bool pressed) noexcept { w->setMouseDown(pressed); }
            static void handleMouseMove(IWindow* w, int x, int y) noexcept {
                w->setMouseX(static_cast<float>(x)); w->setMouseY(static_cast<float>(y));
            }

        public:
#ifdef IO_IMPLEMENTATION
#       if defined(__linux__)
            ::Display* dpy() const noexcept { return _dpy; }
            ::Window xwnd() const noexcept { return _xwnd; }
#       elif defined(_WIN32)
            HDC getHdc() const noexcept { return _hdc; }
            void setHdc(HDC new_hdc) noexcept { _hdc = new_hdc; }
            HWND getHwnd() const noexcept { return _hwnd; }
            void setHwnd(HWND new_hwnd) noexcept { _hwnd = new_hwnd; }
#       endif
#endif // IO_IMPLEMENTATION
    }; // struct Window

// WinApi Window
#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
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
                win.onError(Error::Window, AboutError::Window);
                return;
            }
            SetWindowLongW(wnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(&win));

            // ---- 3. Get DC
            HDC new_hdc = GetDC(wnd);
            win.native().setHdc(new_hdc);
            if (!win.native().getHdc()) {
                win.onError(Error::Window, AboutError::WindowDevice);
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
        inline void Window::setTitle(io::char_view title) const noexcept {
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
            case WM_SETCURSOR: {
                if (!win->isCursorVisible()) {
                    ::SetCursor(nullptr);
                    return TRUE;
                }
                const CursorState cs = win->getCursorState();

                if (cs != win->native().getLastCursorState()) {
                    win->native().setCursor(cs);
                    win->native().updateLastCursorState(cs);
                }
                win->resetCursorState();
                return TRUE;
            }
            case WM_MOUSEMOVE: {
                int mouseX = LOWORD(lparam);
                int mouseY = HIWORD(lparam);
                ::hi::native::Window::handleMouseMove(win, mouseX, mouseY);
                win->onMouseMove(mouseX, mouseY);
                return 0;
            }
            case WM_SETFOCUS:  win->onFocusChange(true); return 0;
            case WM_KILLFOCUS: win->onFocusChange(false); return 0;
            case WM_MOUSEWHEEL: {
                int delta = GET_WHEEL_DELTA_WPARAM(wparam); // Returns 120 or -120
                // @TODO Replace `0.f` with actual horizontal scroll delta
                win->onScroll(0.f, delta / 120.f); // Normalize before callback
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

            // --- mouse ---
            case WM_LBUTTONDOWN: SetCapture(hwnd); ::hi::native::Window::handleMouseButton(win, true);  win->onKeyDown(Key::MouseLeft);   return 0;
            case WM_LBUTTONUP:   ReleaseCapture(); ::hi::native::Window::handleMouseButton(win, false); win->onKeyUp(Key::MouseLeft);     return 0;
            case WM_RBUTTONDOWN: SetCapture(hwnd); ::hi::native::Window::handleMouseButton(win, true);  win->onKeyDown(Key::MouseRight);  return 0;
            case WM_RBUTTONUP:   ReleaseCapture(); ::hi::native::Window::handleMouseButton(win, false); win->onKeyUp(Key::MouseRight);    return 0;
            case WM_MBUTTONDOWN: SetCapture(hwnd); ::hi::native::Window::handleMouseButton(win, true);  win->onKeyDown(Key::MouseMiddle); return 0;
            case WM_MBUTTONUP:   ReleaseCapture(); ::hi::native::Window::handleMouseButton(win, false); win->onKeyUp(Key::MouseMiddle);   return 0;
            case WM_XBUTTONDOWN: {
                SetCapture(hwnd);
                ::hi::native::Window::handleMouseButton(win, true);
                const WORD xb = GET_XBUTTON_WPARAM(wparam); // XBUTTON1 or XBUTTON2
                if (xb == XBUTTON1) win->onKeyDown(Key::MouseX1);
                else                win->onKeyDown(Key::MouseX2);
                return TRUE; // important for XBUTTON*
            }
            case WM_XBUTTONUP: {
                ReleaseCapture();
                ::hi::native::Window::handleMouseButton(win, false);
                const WORD xb = GET_XBUTTON_WPARAM(wparam);
                if (xb == XBUTTON1) win->onKeyUp(Key::MouseX1);
                else                win->onKeyUp(Key::MouseX2);
                return TRUE;
            }

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

// glx + FBConfig + XWindow
#if defined(IO_IMPLEMENTATION) && defined(__linux__)
namespace glx {
    // Basic FB attrs: RGBA, double buffer, depth/stencil.
    static IO_CONSTEXPR_VAR int fb_attribs[] = {
        GLX_X_RENDERABLE,   True,
        GLX_DRAWABLE_TYPE,  GLX_WINDOW_BIT,
        GLX_RENDER_TYPE,    GLX_RGBA_BIT,
        GLX_X_VISUAL_TYPE,  GLX_TRUE_COLOR,
        GLX_DOUBLEBUFFER,   True, // OpenGL needs double buffering
        GLX_RED_SIZE,       8,
        GLX_GREEN_SIZE,     8,
        GLX_BLUE_SIZE,      8,
        GLX_ALPHA_SIZE,     8,
        GLX_DEPTH_SIZE,     24,
        GLX_STENCIL_SIZE,   8,
        0 };

    struct fb_pick {
        GLXFBConfig fb{};
        XVisualInfo* vi{};
    };

    static inline fb_pick pick_fb(Display* dpy) noexcept {
        fb_pick out{};
        int n = 0;
        GLXFBConfig* fbs = glXChooseFBConfig(dpy, DefaultScreen(dpy), fb_attribs, &n);
        if (!fbs || n <= 0) return out;

        out.fb = fbs[0];
        out.vi = glXGetVisualFromFBConfig(dpy, out.fb);

        XFree(fbs);
        return out;
    }
}
    inline native::Window::Window(IWindow& win, int width, int height, bool shown, bool bordless) noexcept {
        (void)bordless; // @TODO: borderless via _MOTIF_WM_HINTS
        _dpy = XOpenDisplay(nullptr);
        if (!_dpy) { win.onError(Error::Window, AboutError::Window); return; }

        auto pick = glx::pick_fb(_dpy);
        if (!pick.fb || !pick.vi) { win.onError(Error::Window, AboutError::WindowDevice); return; }
        _cmap = XCreateColormap(_dpy, RootWindow(_dpy, pick.vi->screen), pick.vi->visual, AllocNone);

        XSetWindowAttributes swa{};
        swa.colormap = _cmap;
        swa.event_mask = ExposureMask    | StructureNotifyMask | KeyPressMask      | KeyReleaseMask |
                         ButtonPressMask | ButtonReleaseMask   | PointerMotionMask | FocusChangeMask;
        _xwnd = XCreateWindow(_dpy, RootWindow(_dpy, pick.vi->screen),
            0, 0, (unsigned)width, (unsigned)height,
            0,
            pick.vi->depth, InputOutput,
            pick.vi->visual, CWColormap | CWEventMask,
            &swa);
        XFree(pick.vi);
        if (!_xwnd) { win.onError(Error::Window, AboutError::Window); return; }
        // WM_DELETE_WINDOW -> allow graceful close
        _wm_delete = XInternAtom(_dpy, "WM_DELETE_WINDOW", False);
        XSetWMProtocols(_dpy, _xwnd, &_wm_delete, 1);

        if (shown) {
            XMapWindow(_dpy, _xwnd);
            _mapped = true;
            XFlush(_dpy);
        }
    }

    inline native::Window::~Window() noexcept {
    if (_dpy) {
        if (_xwnd) XDestroyWindow(_dpy, _xwnd);
        if (_cmap) XFreeColormap(_dpy, _cmap);
        XCloseDisplay(_dpy);
    }
    _xwnd = 0; _cmap = 0; _wm_delete = 0; _dpy = nullptr; _mapped = false;
}

    static ::hi::Key map_key_sym(::KeySym ks) noexcept {
    using K = ::hi::Key;
    // Minimal mapping; extend as needed.
    if (ks >= XK_A && ks <= XK_Z) return Key_t{ (int)K::A + (int)(ks - XK_A) }.key();
    if (ks >= XK_a && ks <= XK_z) return Key_t{ (int)K::A + (int)(ks - XK_a) }.key();
    if (ks >= XK_0 && ks <= XK_9) return Key_t{ (int)K::_0 + (int)(ks - XK_0) }.key();
    if (ks >= XK_F1 && ks <= XK_F12) return Key_t{ (int)K::F1 + (int)(ks - XK_F1) }.key();

    switch (ks) {
        case XK_Shift_L: case XK_Shift_R:   return K::Shift;
        case XK_Control_L: case XK_Control_R:return K::Control;
        case XK_Alt_L: case XK_Alt_R:       return K::Alt;
        case XK_Super_L: case XK_Super_R:   return K::Super;

        case XK_Escape:   return K::Escape;
        case XK_Return:   return K::Return;
        case XK_Tab:      return K::Tab;
        case XK_BackSpace:return K::Backspace;
        case XK_Insert:   return K::Insert;
        case XK_Delete:   return K::Delete;

        case XK_Home:     return K::Home;
        case XK_End:      return K::End;
        case XK_Page_Up:  return K::PageUp;
        case XK_Page_Down:return K::PageDown;

        case XK_Left:     return K::Left;
        case XK_Right:    return K::Right;
        case XK_Up:       return K::Up;
        case XK_Down:     return K::Down;

        case XK_space:    return K::Space;
        case XK_minus:    return K::Hyphen;
        case XK_equal:    return K::Equal;
        case XK_bracketleft:  return K::BracketLeft;
        case XK_bracketright: return K::BracketRight;
        case XK_backslash:    return K::Backslash;
        case XK_semicolon:    return K::Semicolon;
        case XK_apostrophe:   return K::Apostrophe;
        case XK_comma:        return K::Comma;
        case XK_period:       return K::Period;
        case XK_slash:        return K::Slash;
        case XK_grave:        return K::Grave;

        default: return K::__NONE__;
    }
}

    inline bool native::Window::PollEvents(IWindow& win) const noexcept {
        if (!_dpy) return false;

        IO_CONSTEXPR_VAR int MAX_EVENTS_PER_POLL = 256; // bounded work
        int processed = 0;

        bool have_resize = false;
        int last_w, last_h;
        last_w=last_h=0;

        while (XPending(_dpy) > 0 && processed < MAX_EVENTS_PER_POLL) {
            ::XEvent e{};
            XNextEvent(_dpy, &e);
            ++processed;

            if (e.type == ClientMessage) {
                if ((::Atom)e.xclient.data.l[0] == _wm_delete) return false;
            }

            switch (e.type) {
                case Expose: break; // don't render here
                case ConfigureNotify: {
                    // coalesce: keep only the last size seen in this poll
                    last_w = e.xconfigure.width;
                    last_h = e.xconfigure.height;
                    have_resize = true;

                    // optional: drain consecutive ConfigureNotify to skip storms
                    while (XPending(_dpy) > 0) {
                        ::XEvent e2{};
                        XPeekEvent(_dpy, &e2);
                        if (e2.type != ConfigureNotify) break;
                        XNextEvent(_dpy, &e2);
                        ++processed;
                        last_w = e2.xconfigure.width;
                        last_h = e2.xconfigure.height;
                        if (processed >= MAX_EVENTS_PER_POLL) break;
                    }
                    break;
                }

                case MotionNotify:
                    // optional coalesce motion too
                    ::hi::native::Window::handleMouseMove(win, e.xmotion.x, e.xmotion.y);

                    if (win->isCursorVisible()) {
                        const CursorState cs = win->getCursorState();

                        if (cs != win->native().getLastCursorState()) {
                            win->native().setCursor(cs);
                            win->native().updateLastCursorState(cs);
                        }
                        win->resetCursorState();
                    }

                    win.onMouseMove(e.xmotion.x, e.xmotion.y);
                    break;

                case FocusIn:  win.onFocusChange(true);  break;
                case FocusOut: win.onFocusChange(false); break;

                case ButtonPress:
                case ButtonRelease: {
                    const bool pressed = (e.type == ButtonPress);
                    switch (e.xbutton.button) {
                    case Button1:
                        native::Window::handleMouseButton(&win, pressed);
                        if (pressed) win.onKeyDown(Key::MouseLeft); else win.onKeyUp(Key::MouseLeft);
                        break;
                    case Button2:
                        native::Window::handleMouseButton(&win, pressed);
                        if (pressed) win.onKeyDown(Key::MouseMiddle); else win.onKeyUp(Key::MouseMiddle);
                        break;
                    case Button3:
                        native::Window::handleMouseButton(&win, pressed);
                        if (pressed) win.onKeyDown(Key::MouseRight); else win.onKeyUp(Key::MouseRight);
                        break;
                    case Button4: if (pressed) win.onScroll(0.f, +1.f); break; // wheel up
                    case Button5: if (pressed) win.onScroll(0.f, -1.f); break; // wheel down
                        // Button6/7 = horizontal wheel on some systems
                    case 6: if (pressed) win.onScroll(-1.f, 0.f); break;
                    case 7: if (pressed) win.onScroll(+1.f, 0.f); break;
                        // XButton1/XButton2 often are 8/9
                    case 8:
                        native::Window::handleMouseButton(&win, pressed);
                        if (pressed) win.onKeyDown(Key::MouseX1); else win.onKeyUp(Key::MouseX1);
                        break;
                    case 9:
                        native::Window::handleMouseButton(&win, pressed);
                        if (pressed) win.onKeyDown(Key::MouseX2); else win.onKeyUp(Key::MouseX2);
                        break;
                    }
                    break;
                }

                case KeyPress:
                case KeyRelease: {
                    const bool pressed = (e.type == KeyPress);
                    ::KeySym ks = XLookupKeysym(&e.xkey, 0);
                    ::hi::Key k = map_key_sym(ks);
                    ::hi::global::key_array[(int)k] = pressed ? 1 : 0;
                    if (pressed) win.onKeyDown(k);
                    else         win.onKeyUp(k);
                    break;
                }
            }
        }

        if (have_resize) {
            win.onGeometryChange(last_w, last_h);
        }

        return true;
    }

    inline void native::Window::setTitle(io::char_view title) const noexcept {
        if (!_dpy || !_xwnd || !title) return;
        if (!_dpy || !_xwnd || !title) return;
        const Atom utf8      = XInternAtom(_dpy, "UTF8_STRING", False);
        const Atom net_wm    = XInternAtom(_dpy, "_NET_WM_NAME", False);
        const Atom wm_name   = XInternAtom(_dpy, "WM_NAME", False);
        // Primary: EWMH UTF-8 title
        XChangeProperty(_dpy, _xwnd, net_wm, utf8, 8, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(title.data()),
                        (int)title.size());
        // Fallback: old WM_NAME (use same bytes; many WMs still read _NET_WM_NAME)
        XChangeProperty(_dpy, _xwnd, wm_name, utf8, 8, PropModeReplace,
                        reinterpret_cast<const unsigned char*>(title.data()),
                        (int)title.size());
        XFlush(_dpy);
    }

    inline void native::Window::setShow(bool v) const noexcept {
        if (!_dpy || !_xwnd) return;
        if (v) XMapWindow(_dpy, _xwnd);
        else   XUnmapWindow(_dpy, _xwnd);
        XFlush(_dpy);
    }

    inline void native::Window::setFullscreen(bool) const noexcept {
    // _NET_WM_STATE_FULLSCREEN via XSendEvent + atoms.
}
#endif

#ifdef IO_IMPLEMENTATION
#   if defined(_WIN32)

    inline void native::Window::setCursor(Cursor c) const noexcept {
        if (!_hwnd) return;

        if (c == Cursor::Hidden) {
            ::SetCursor(nullptr);
            return;
        }

        LPCSTR idc = IDC_ARROW;
        switch (c) {
        case Cursor::Arrow:     idc = IDC_ARROW;     break;
        case Cursor::IBeam:     idc = IDC_IBEAM;     break;
        case Cursor::Crosshair: idc = IDC_CROSS;     break;
        case Cursor::Hand:      idc = IDC_HAND;      break;
        case Cursor::HResize:   idc = IDC_SIZEWE;    break;
        case Cursor::VResize:   idc = IDC_SIZENS;    break;
        case Cursor::Hidden:    return;
        }

        ::HCURSOR cur = ::LoadCursorA(nullptr, idc);
        if (cur) {
            ::SetClassLongPtrW(_hwnd, GCLP_HCURSOR, reinterpret_cast<LONG_PTR>(cur));
            ::SetCursor(cur);
        }
    }

#   endif
#endif

#ifdef IO_IMPLEMENTATION
#   if defined(__linux__)

#       include <X11/cursorfont.h>

    inline void native::Window::setCursor(Cursor c) const noexcept {
        if (!_dpy || !_xwnd) return;

        if (c == Cursor::Hidden) {
            static const char empty_data[8] = { 0,0,0,0,0,0,0,0 };
            ::Pixmap blank_pixmap = ::XCreateBitmapFromData(_dpy, _xwnd, empty_data, 8, 8);
            if (!blank_pixmap) return;

            ::XColor dummy{};
            ::Cursor blank_cursor = ::XCreatePixmapCursor(_dpy, blank_pixmap, blank_pixmap, &dummy, &dummy, 0, 0);
            ::XDefineCursor(_dpy, _xwnd, blank_cursor);
            ::XFreeCursor(_dpy, blank_cursor);
            ::XFreePixmap(_dpy, blank_pixmap);
            ::XFlush(_dpy);
            return;
        }

        unsigned shape = XC_left_ptr;
        switch (c) {
        case Cursor::Arrow:     shape = XC_left_ptr;         break;
        case Cursor::IBeam:     shape = XC_xterm;            break;
        case Cursor::Crosshair: shape = XC_crosshair;        break;
        case Cursor::Hand:      shape = XC_hand2;            break;
        case Cursor::HResize:   shape = XC_sb_h_double_arrow; break;
        case Cursor::VResize:   shape = XC_sb_v_double_arrow; break;
        case Cursor::Hidden:    return;
        }

        ::Cursor cursor = ::XCreateFontCursor(_dpy, shape);
        ::XDefineCursor(_dpy, _xwnd, cursor);
        ::XFreeCursor(_dpy, cursor);
        ::XFlush(_dpy);
    }

#   endif
#endif

    // -------------------- Native Opengl Context -------------------------
    struct Opengl {
        private:
#ifdef IO_IMPLEMENTATION
#   if defined(__linux__)
            ::GLXContext _ctx{ nullptr };
            ::hi::IWindow* _win{};
#   elif defined(_WIN32)
            HGLRC _hglrc{ nullptr };
#   endif
#endif // IO_IMPLEMENTATION
            io::u8 _req_core_major{3};
            io::u8 _req_core_minor{3};

        public:
            Opengl() noexcept = default;
            explicit Opengl(io::u8 core_major, io::u8 core_minor) noexcept
                : _req_core_major(core_major), _req_core_minor(core_minor) {}

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

            IO_NODISCARD io::u8 reqCoreMajor() const noexcept { return _req_core_major; }
            IO_NODISCARD io::u8 reqCoreMinor() const noexcept { return _req_core_minor; }

            // required load of `gl::GetIntegerv` first
            IO_NODISCARD inline io::u8 currentMajorVersion() const noexcept { int v; gl::GetIntegerv(gl::GlVersionParam::major, &v); return (io::u8)v; }
            IO_NODISCARD inline io::u8 currentMinorVersion() const noexcept { int v; gl::GetIntegerv(gl::GlVersionParam::minor, &v); return (io::u8)v; }

#ifdef IO_IMPLEMENTATION
#   if defined(__linux__)
            GLXContext getCtx() const noexcept { return _ctx; }
#   elif defined(_WIN32)
            HGLRC getHglrc() const noexcept { return _hglrc; }
            void setHglrc(HGLRC new_hglrc) noexcept { _hglrc = new_hglrc; }
#   endif
#endif // IO_IMPLEMENTATION

            static inline bool gl_bootstrap_load() noexcept {
                auto must = [](auto& out, const char* name) noexcept -> bool {
                    using FnT = decltype(out);
                    void* p = ::gl::loader(name);
                    out = reinterpret_cast<FnT>(p);
                    return out != nullptr;
                };
                return must(gl::native::pGetError, "glGetError") &&
                    must(gl::native::pGetString, "glGetString") &&
                    must(gl::native::pGetIntegerv, "glGetIntegerv");
            }
        }; // struct OpenglContext

#if defined(IO_IMPLEMENTATION) && defined(_WIN32)
        inline void Opengl::Render(IWindow& win) const noexcept {
            PAINTSTRUCT ps;
            HWND wnd = win.native().getHwnd();
            HDC dc = win.native().getHdc();
            HGLRC glc = getHglrc();

            BeginPaint(wnd, &ps);
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

                if (!choose) return AboutError::MissingChoosePixelFormatARB;
                if (!create) return AboutError::MissingCreateContextAttribsARB;

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
                if (!ctx) return AboutError::CreateContextAttribsARB;
                if (!wglMakeCurrent(main_dc, ctx)) return AboutError::CreateModernContext;

                if (!wglGetCurrentContext()) return AboutError::GetCurrentContext;
                if (!wglGetCurrentDC())      return AboutError::w_GetCurrentDC;

                win.opengl().setHglrc(ctx);

                gl::loader = OpenglLoader;
                // 1) minimal load for querying version
                gl::loaded = true;
                if(!Opengl::gl_bootstrap_load()) return AboutError::MissingRequiredOpenglFunction;
            
                int real_maj=0, real_min=0;
                gl::query_core_version(real_maj, real_min);
                if (real_maj <= 0) return AboutError::MissingRequiredOpenglFunction;
            
                // Doesn't check if real >= req, we should catch nullptrs on `gl::load_core`

                // 2) now load full table based on real core version
                gl::loaded = false;
                auto miss = gl::load_core(real_maj, real_min);
                if (!miss.empty()) return AboutError::MissingRequiredOpenglFunction;
                gl::loaded = true;

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
#if defined(IO_IMPLEMENTATION) && defined(__linux__)
static inline void* glx_loader(const char* name) noexcept {
    return (void*)glXGetProcAddressARB((const GLubyte*)name);
}

namespace glx {
    typedef GLXContext (*PFN_glXCreateContextAttribsARB)(
        Display*, GLXFBConfig, GLXContext, Bool, const int*);

    static inline PFN_glXCreateContextAttribsARB get_create_attribs() noexcept {
        return (PFN_glXCreateContextAttribsARB)glXGetProcAddressARB((const GLubyte*)"glXCreateContextAttribsARB");
    }
} // namespace glx

    IO_NODISCARD AboutError native::Opengl::CreateContext(IWindow& win) noexcept {
        _win = &win;
        auto* dpy = _win->native().dpy();
        if (!dpy) return AboutError::l_Display;

        auto pick = glx::pick_fb(dpy); 
        if (!pick.fb) return AboutError::WindowDevice;

        glx::PFN_glXCreateContextAttribsARB create_attribs = glx::get_create_attribs();
        if (!create_attribs) return AboutError::MissingCreateContextAttribsARB;

        // Prefer 3.3, then 3.1, then 3.0 (compat not requested here)
        struct { int maj, min; } tries[] = { {4,1},{4,0},{3,3},{3,2},{3,1},{3,0} };

        for (auto t : tries) {
            const int attribs[] = {
                GLX_CONTEXT_MAJOR_VERSION_ARB, t.maj,
                GLX_CONTEXT_MINOR_VERSION_ARB, t.min,
                GLX_CONTEXT_PROFILE_MASK_ARB,  GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
                0
            };

            ::GLXContext ctx = create_attribs(dpy, pick.fb, nullptr, True, attribs);
            if (!ctx) continue;

            if (!glXMakeCurrent(dpy, win.native().xwnd(), ctx)) {
                glXDestroyContext(dpy, ctx);
                continue;
            }

            gl::loader = glx_loader;

            // bootstrap for version query
            gl::loaded = true;
            if (!Opengl::gl_bootstrap_load()) {
                glXDestroyContext(dpy, ctx);
                continue;
            }

            int real_maj = 0, real_min = 0;
            gl::GetIntegerv(gl::GlVersionParam::major, &real_maj);
            gl::GetIntegerv(gl::GlVersionParam::minor, &real_min);

            // accept only if real >= req
            if (!gl::ver_ge(real_maj, real_min, t.maj, t.min)) {
                glXDestroyContext(dpy, ctx);
                continue;
            }

            // now do full load based on *real* version
            gl::loaded = false;
            auto miss = gl::load_core(real_maj, real_min);
            if (!miss.empty()) {
                glXDestroyContext(dpy, ctx);
                continue;
            }
            gl::loaded = true;
            _ctx = ctx;
            return AboutError::None;
        }

        return AboutError::CreateContextAttribsARB;
    }

    native::Opengl::~Opengl() noexcept {
        glXDestroyContext(_win->native().dpy(), _ctx);
    }

    inline void native::Opengl::Render(IWindow& win) const noexcept {
        auto* dpy = win.native().dpy();
        if (!dpy || !_ctx) return;

        // Ensure current
        if (glXGetCurrentContext() != _ctx) {
            glXMakeCurrent(dpy, win.native().xwnd(), _ctx);
        }
        win.onRender();
    }

    inline void native::Opengl::SwapBuffers(const IWindow& win) const noexcept {
        auto* dpy = win.native().dpy();
        if (!dpy || !_ctx) return;
        glXSwapBuffers(dpy, win.native().xwnd());
    }
#endif
} // namespace native

    template<class Win>
    struct ContextGuardT {
        Win& w;
        RendererApi api{ RendererApi::None };
        bool alive = false;

        ContextGuardT(Win& ww, io::u8 gl_core_major, io::u8 gl_core_minor) noexcept : w(ww) {
            new (&w.g) native::Opengl(gl_core_major, gl_core_minor);
            const AboutError about = w.g.CreateContext(w);
            if (about != AboutError::None) {
                w.onError(Error::Opengl, about);
                w.g.~Opengl();
                return;
            }
            gl::Viewport(0, 0, w.width(), w.height());
            api = RendererApi::Opengl;
            alive = true;
        }

        ~ContextGuardT() noexcept {
            if (alive) {
                w.g.~Opengl();
                api = RendererApi::None;
                alive = false;
            }
        }

        ContextGuardT(const ContextGuardT&) = delete;
        ContextGuardT& operator=(const ContextGuardT&) = delete;
    }; // struct ContextGuardT


    using FontId = int;  // >=0
    using AtlasId = int;  // >=0
    enum class FontAtlasMode : io::u8 { SDF, MTSDF, DebugR, DebugRGBA };

    struct FontAtlasDesc {
        FontAtlasMode mode;
        io::u16 pixel_height; // <=64
        float  spread_px;     // e.g. 4..8
        // codepoints:
        const io::u32* codepoints;
        io::u32 codepoint_count;
    };

    struct TextStyle {
        // fill
        float r = 1.f, g = 1.f, b = 1.f, a = 1.f;
        // optional outline (works for SDF & MTSDF)
        bool  outline = false;
        float or_ = 0.f, og_ = 0.f, ob_ = 0.f, oa_ = 1.f;
        float outline_px = 1.0f;   // thickness in pixels
        float softness_px = 0.75f; // edge softness in pixels
    };

    enum class TextDock {
        TopL, TopC, TopR,
        LeftC, RightC,
        BottomL, BottomC, BottomR
    };

    struct TextDraw {
        AtlasId atlas;
        TextDock dock = TextDock::TopL;

        float x, y;         // offset (px) from dock
        float scale = 1.f;
        io::char_view text; // UTF-8
        TextStyle style{};

        float line_height = 1.0f;   // multiplier from pixel_height
        float tab_width = 4.0f;     // tab in spaces
        float space_between = 0.0f; // [-1..1]
    };

    struct ButtonStyle {
        TextStyle normal{};
        TextStyle hover{};
        TextStyle active{};
        float pad_x = 8.f; // px
        float pad_y = 4.f; // px
    };

    struct ButtonDraw {
        AtlasId  atlas = -1;
        TextDock dock = TextDock::TopL;

        float x = 0.f, y = 0.f; // offset (px) from dock
        float scale = 1.f;
        io::char_view text;

        // layout
        float line_height = 1.0f;
        float tab_width = 4.0f;
        float space_between = 0.0f;

        ButtonStyle style{};
    };

    struct ButtonState {
        bool hovered = false;
        bool held = false;
        bool clicked = false;
        float x0 = 0, y0 = 0, x1 = 0, y1 = 0; // rect (px)
    };


namespace internal {
    // ------------------------------
    // utils
    // ------------------------------
    struct glyph_map {
        static IO_CONSTEXPR_VAR io::u32 empty_key = 0xFFFFFFFFu;

        struct slot { io::u32 key; io::u32 val; };

        slot*   slots{};
        io::u32 cap{};   // power of two
        io::u32 size{};

        static inline io::u32 hash_u32(io::u32 x) noexcept {
            // cheap avalanche
            x ^= x >> 16;
            x *= 0x7feb352du;
            x ^= x >> 15;
            x *= 0x846ca68bu;
            x ^= x >> 16;
            return x;
        }

        inline void reset() noexcept { slots=nullptr; cap=size=0; }

        inline bool init(void* mem, io::u32 capacity_pow2) noexcept {
            if (!mem) return false;
            // require power-of-two
            if ((capacity_pow2 & (capacity_pow2 - 1u)) != 0u) return false;
            slots = (slot*)mem;
            cap = capacity_pow2;
            size = 0;
            for (io::u32 i=0;i<cap;++i) { slots[i].key = empty_key; slots[i].val = 0; }
            return true;
        }

        inline bool insert(io::u32 key, io::u32 val) noexcept {
            if (!slots || cap==0) return false;
            io::u32 mask = cap - 1u;
            io::u32 i = hash_u32(key) & mask;
            for (io::u32 step=0; step<cap; ++step) {
                slot& s = slots[i];
                if (s.key == empty_key || s.key == key) {
                    if (s.key == empty_key) ++size;
                    s.key = key;
                    s.val = val;
                    return true;
                }
                i = (i + 1u) & mask;
            }
            return false; // full
        }

        inline bool find(io::u32 key, io::u32& out_val) const noexcept {
            if (!slots || cap==0) return false;
            io::u32 mask = cap - 1u;
            io::u32 i = hash_u32(key) & mask;
            for (io::u32 step=0; step<cap; ++step) {
                const slot& s = slots[i];
                if (s.key == key) { out_val = s.val; return true; }
                if (s.key == empty_key) return false;
                i = (i + 1u) & mask;
            }
            return false;
        }
    }; // struct glyph_map
    static inline bool utf8_next(io::char_view s, io::usize& i, io::u32& cp) noexcept {
        cp = 0;
        if (i >= s.size()) return false;
        const io::u8* p = (const io::u8*)s.data();
        io::u8 c0 = p[i++];

        if (c0 < 0x80) { cp = c0; return true; }

        auto need = [&](int n)->bool { return (i + (io::usize)n) <= s.size(); };
        auto cont = [&](io::u8 c)->bool { return (c & 0xC0u) == 0x80u; };

        if ((c0 & 0xE0u) == 0xC0u) {
            if (!need(1)) return false;
            io::u8 c1 = p[i++];
            if (!cont(c1)) return false;
            cp = ((io::u32)(c0 & 0x1Fu) << 6) | (io::u32)(c1 & 0x3Fu);
            return true;
        }
        if ((c0 & 0xF0u) == 0xE0u) {
            if (!need(2)) return false;
            io::u8 c1 = p[i++], c2 = p[i++];
            if (!cont(c1) || !cont(c2)) return false;
            cp = ((io::u32)(c0 & 0x0Fu) << 12) | ((io::u32)(c1 & 0x3Fu) << 6) | (io::u32)(c2 & 0x3Fu);
            return true;
        }
        if ((c0 & 0xF8u) == 0xF0u) {
            if (!need(3)) return false;
            io::u8 c1 = p[i++], c2 = p[i++], c3 = p[i++];
            if (!cont(c1) || !cont(c2) || !cont(c3)) return false;
            cp = ((io::u32)(c0 & 0x07u) << 18) | ((io::u32)(c1 & 0x3Fu) << 12) |
                 ((io::u32)(c2 & 0x3Fu) << 6) | (io::u32)(c3 & 0x3Fu);
            return true;
        }
        return false;
    }
    static inline io::u32 pack_rgba8(float r,float g,float b,float a) noexcept {
        io::u32 R = io::f2u8(r), G = io::f2u8(g), B = io::f2u8(b), A = io::f2u8(a);
        return (A<<24) | (B<<16) | (G<<8) | (R<<0); // little-endian packed
    }
    
#pragma region shaders
    // ------------------------------
    // shader sources
    // ------------------------------
    static const char* k_text_vs_330 =
        "#version 330 core\n"
        "layout(location=0) in vec2 aPos;\n"
        "layout(location=1) in vec2 aUV;\n"
        "layout(location=2) in vec4 aColor;\n"
        "layout(location=3) in vec4 aOutline;\n"
        "layout(location=4) in vec2 aParams; // outline_px, softness_px\n"
        "out vec2 vUV;\n"
        "out vec4 vColor;\n"
        "out vec4 vOutline;\n"
        "out vec2 vParams;\n"
        "uniform vec2 uViewport; // (w,h)\n"
        "void main(){\n"
        "  vUV=aUV; vColor=aColor; vOutline=aOutline; vParams=aParams;\n"
        "  vec2 ndc = vec2( (aPos.x/uViewport.x)*2.0-1.0, 1.0-(aPos.y/uViewport.y)*2.0 );\n"
        "  gl_Position = vec4(ndc,0,1);\n"
        "}\n";

    static const char* k_text_fs_sdf_330 =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec4 vColor;\n"
        "in vec4 vOutline;\n"
        "in vec2 vParams; // outline_px, softness_px\n"
        "out vec4 Frag;\n"
        "uniform sampler2D uTex;\n"
        "uniform float uPxRange;     // spread_px\n"
        "uniform float uAtlasSide;   // atlas_side\n"
        "\n"
        "float screenPxRange(){\n"
        "  vec2 duv = fwidth(vUV);\n"
        "  vec2 texPerPix = duv * uAtlasSide;\n"
        "  float tpp = max(texPerPix.x, texPerPix.y);\n"
        "  return uPxRange / max(tpp, 1e-6);\n"
        "}\n"
        "\n"
        "void main(){\n"
        "  float pxr = screenPxRange();\n"
        "  float sd  = texture(uTex, vUV).r * 2.0 - 1.0;\n"
        "  sd = -sd; // ALWAYS invert\n"
        "  float dist = sd * pxr;\n"
        "\n"
        "  float w = max(fwidth(dist), vParams.y);\n"
        "  float a_fill = smoothstep(-w, +w, dist);\n"
        "  vec4 fill = vec4(vColor.rgb, vColor.a) * a_fill;\n"
        "\n"
        "  if (vOutline.a > 0.0 && vParams.x > 0.0) {\n"
        "    float o = vParams.x;\n"
        "    float a_out = smoothstep(-o-w, -o+w, dist) - smoothstep(-w, +w, dist);\n"
        "    a_out = clamp(a_out, 0.0, 1.0);\n"
        "    vec4 outc = vec4(vOutline.rgb, vOutline.a) * a_out;\n"
        "    Frag = outc + fill * (1.0 - outc.a);\n"
        "  } else {\n"
        "    Frag = fill;\n"
        "  }\n"
        "}\n";


    static const char* k_text_fs_mtsdf_330 =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "in vec4 vColor;\n"
        "in vec4 vOutline;\n"
        "in vec2 vParams; // outline_px, softness_px\n"
        "out vec4 Frag;\n"
        "uniform sampler2D uTex;\n"
        "uniform float uPxRange;     // spread_px\n"
        "uniform float uAtlasSide;   // atlas_side\n"
        "\n"
        "float screenPxRange(){\n"
        "  vec2 duv = fwidth(vUV);\n"
        "  vec2 texPerPix = duv * uAtlasSide;\n"
        "  float tpp = max(texPerPix.x, texPerPix.y);\n"
        "  return uPxRange / max(tpp, 1e-6);\n"
        "}\n"
        "\n"
        "float median3(float a,float b,float c){ return max(min(a,b), min(max(a,b),c)); }\n"
        "\n"
        "void main(){\n"
        "  vec4 t = texture(uTex, vUV);\n"
        "\n"
        "  float ms = median3(t.r, t.g, t.b) * 2.0 - 1.0;\n"
        "  float ss = t.a * 2.0 - 1.0;\n"
        "  ms = -ms; ss = -ss; // ALWAYS invert\n"
        "\n"
        "  float s = ss;\n"
        "\n"
        "  float pxr = screenPxRange();\n"
        "  float dist = s * pxr;\n"
        "\n"
        "  float w = max(fwidth(dist), vParams.y);\n"
        "  float a_fill = smoothstep(-w, +w, dist);\n"
        "  vec4 fill = vec4(vColor.rgb, vColor.a) * a_fill;\n"
        "\n"
        "  if (vOutline.a > 0.0 && vParams.x > 0.0) {\n"
        "    float o = vParams.x;\n"
        "    float a_out = smoothstep(-o-w, -o+w, dist) - smoothstep(-w, +w, dist);\n"
        "    a_out = clamp(a_out, 0.0, 1.0);\n"
        "    vec4 outc = vec4(vOutline.rgb, vOutline.a) * a_out;\n"
        "    Frag = outc + fill * (1.0 - outc.a);\n"
        "  } else {\n"
        "    Frag = fill;\n"
        "  }\n"
        "}\n";

    // Show R component as grayscale (works for R8, RGBA8)
    static const char* k_text_fs_debug_r_330 =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "out vec4 Frag;\n"
        "uniform sampler2D uTex;\n"
        "void main(){\n"
        "  float r = texture(uTex, vUV).r;\n"
        "  Frag = vec4(r, r, r, 1.0);\n"
        "}\n";

    // Show RGBA directly
    static const char* k_text_fs_debug_rgba_330 =
        "#version 330 core\n"
        "in vec2 vUV;\n"
        "out vec4 Frag;\n"
        "uniform sampler2D uTex;\n"
        "void main(){\n"
        "  Frag = texture(uTex, vUV);\n"
        "}\n";
#pragma endregion shaders

    // ------------------------------
    // GPU helpers
    // ------------------------------
    struct text_vertex {
        float x, y;
        float u, v;
        io::u32 color_rgba;       // packed
        io::u32 outline_rgba;     // packed
        float outline_px;
        float softness_px;
    };
    struct text_uniforms {
        int uViewport = -1, uPxRange = -1, uAtlasSide = -1, uTex = -1;

        static inline text_uniforms query(io::u32 prog) noexcept {
            text_uniforms u{};
            u.uViewport = gl::GetUniformLocation(prog, "uViewport");
            u.uPxRange = gl::GetUniformLocation(prog, "uPxRange");
            u.uAtlasSide = gl::GetUniformLocation(prog, "uAtlasSide");
            u.uTex = gl::GetUniformLocation(prog, "uTex");
            return u;
        }
    };
    IO_NODISCARD static inline io::u32 build_text_program(FontAtlasMode mode) noexcept {
        io::u32 vs=0, fs=0, prog=0;
        if (!gl::Shader::compile_shader(vs, gl::ShaderType::VertexShader, k_text_vs_330)) return 0;
        const char* fsrc =
                (mode==FontAtlasMode::SDF)       ? k_text_fs_sdf_330 :
                (mode==FontAtlasMode::MTSDF)     ? k_text_fs_mtsdf_330 :
                (mode==FontAtlasMode::DebugR)    ? k_text_fs_debug_r_330 :
                                                      k_text_fs_debug_rgba_330;
        if (!gl::Shader::compile_shader(fs, gl::ShaderType::FragmentShader, fsrc)) { gl::DeleteShader(vs); return 0; }
        if (!gl::Shader::link_program(prog, vs, fs)) { gl::DeleteShader(vs); gl::DeleteShader(fs); return 0; }
        gl::DeleteShader(vs);
        gl::DeleteShader(fs);
        return prog;
    }
    IO_NODISCARD static inline io::u32 gl_upload_atlas(io::u16 side, FontAtlasMode mode, const io::u8* pixels) noexcept {
        const int level = 0;
        const int w = (int)side;
        const int h = (int)side;
        const bool is_r = (mode == FontAtlasMode::SDF) || (mode == FontAtlasMode::DebugR);
        const gl::TexFormat fmt = is_r ? gl::TexFormat::R : gl::TexFormat::RGBA;
        const gl::InternalFormat int_fmt = is_r ? gl::InternalFormat::R8 : gl::InternalFormat::RGBA8;

        io::u32 tex = 0;
        gl::GenTextures(1, &tex);
        gl::BindTexture(gl::TexTarget::Tex2D, tex);

        gl::TexParameteri(gl::TexTarget::Tex2D, gl::TexParam::MinFilter, gl::MinifyingFilter::Linear);
        gl::TexParameteri(gl::TexTarget::Tex2D, gl::TexParam::MagFilter, gl::MagnifyingFilter::Linear);
        gl::TexParameteri(gl::TexTarget::Tex2D, gl::TexParam::WrapS, gl::TexWrap::ClampToEdge);
        gl::TexParameteri(gl::TexTarget::Tex2D, gl::TexParam::WrapT, gl::TexWrap::ClampToEdge);

        if (is_r) gl::PixelStorei(gl::UNPACK_ALIGNMENT, 1);
        gl::TexImage2D(gl::TexTarget::Tex2D, level, int_fmt, w, h, 0, fmt, gl::DataType::UnsignedByte, pixels);
        if (is_r) gl::PixelStorei(gl::UNPACK_ALIGNMENT, 4);

        gl::BindTexture(gl::TexTarget::Tex2D, 0);

        return tex;
    }

    // ------------------------------
    // CPU planning
    // ------------------------------
    struct font_atlas {
        font_atlas() noexcept = default;
        ~font_atlas() noexcept { destroy(); }

        font_atlas(const font_atlas&) = delete;
        font_atlas& operator=(const font_atlas&) = delete;

        font_atlas(font_atlas&& o) noexcept { move_from(o); }
        font_atlas& operator=(font_atlas&& o) noexcept {
            if (this != &o) { destroy(); move_from(o); }
            return *this;
        }

        void destroy() noexcept {
            if (tex)  gl::DeleteTextures(1, &tex);
            if (prog) gl::DeleteProgram(prog);
            if (map_mem) { io::free(map_mem); map_mem = nullptr; }
            tex = prog = 0;
            map.reset();
            glyphs.clear();
        }

        void move_from(font_atlas& o) noexcept {
            mode = o.mode; pixel_height = o.pixel_height; spread_px = o.spread_px;
            scale = o.scale; spread_fu = o.spread_fu;
            atlas_side = o.atlas_side; glyph_count = o.glyph_count;

            tex = o.tex; prog = o.prog; uniforms = o.uniforms;
            map = o.map; map_mem = o.map_mem;
            glyphs = io::move(o.glyphs);

            o.tex = o.prog = 0;
            o.uniforms = {};
            o.map.reset();
            o.map_mem = nullptr;
            o.glyph_count = 0;
            o.atlas_side = 0;
        }
        FontAtlasMode mode{};
        io::u16 pixel_height{};
        float spread_px{};
        float scale{};       // pixels per font unit (from stbtt_stream::FontPlan.scale)
        float spread_fu{};   // from plan.spread_fu

        io::u16 atlas_side{};
        io::u32 glyph_count{};

        // GPU
        io::u32 tex = 0;
        io::u32 prog = 0; // shader program (one per mode is fine too)

        glyph_map map;  // glyph lookup: codepoint -> index in glyphs[]

        struct glyph {
            io::u32 codepoint;
            io::u16 glyph_index;
            stbtt_stream::GlyphRect rect; // atlas px
            int advance; // font units? better store in pixels too
            int lsb;
            int x_min, y_min, x_max, y_max; // font units
        };
        io::vector<glyph> glyphs{};
        void* map_mem{};
        text_uniforms uniforms{};
    };
    struct planned_glyph {
        io::u32 codepoint;
        io::u16 glyph_index;
        stbtt_stream::GlyphRect rect; // atlas px
        int x_min, y_min, x_max, y_max; // font units
        int advance; // font units
        int lsb;     // font units
    };
    struct planned_font_atlas {
        FontAtlasMode mode{};
        io::u16 pixel_height{};
        float spread_px{};

        float scale{};
        float spread_fu{};
        io::u16 atlas_side{};
        io::u32 glyph_count{};

        io::u8* atlas_cpu{};
        io::u32 comp{};
        io::u32 stride{};

        planned_glyph* glyphs{}; // alloc, glyph_count
    };
    static inline void planned_destroy(planned_font_atlas& p) noexcept {
        if (p.atlas_cpu) io::free(p.atlas_cpu);
        if (p.glyphs) io::free(p.glyphs);
        p = {};
    }
    IO_NODISCARD static bool plan_font(io::char_view ttf_path, const FontAtlasDesc& desc, planned_font_atlas& out) noexcept {
        out = {};
        if (!desc.codepoints || desc.codepoint_count == 0) return false;

        io::string ttf_bytes{};
        {
            fs::File f(ttf_path, io::OpenMode::Read);
            if (!f.is_open() || !f.read_all(ttf_bytes) || ttf_bytes.empty()) return false;
        }

        stbtt_stream::Font font;
        if (!font.ReadBytes(reinterpret_cast<uint8_t*>(ttf_bytes.data()))) return false;

        stbtt_stream::PlanInput in{};
        const bool want_sdf = (desc.mode == FontAtlasMode::SDF) || (desc.mode == FontAtlasMode::DebugR);
        in.mode = want_sdf ? stbtt_stream::DfMode::SDF : stbtt_stream::DfMode::MTSDF;
        in.pixel_height = desc.pixel_height;
        in.spread_px = desc.spread_px;
        in.codepoints = desc.codepoints;
        in.codepoint_count = desc.codepoint_count;

        const size_t plan_bytes = font.PlanBytes(in);
        if (!plan_bytes) return false;

        void* plan_mem = io::alloc(plan_bytes);
        if (!plan_mem) return false;

        stbtt_stream::FontPlan plan{};
        if (!font.Plan(in, plan_mem, plan_bytes, plan)) { io::free(plan_mem); return false; }

        // ---- copy glyphs + metrics ----
        planned_glyph* glyphs = (planned_glyph*)io::alloc((size_t)plan.glyph_count * sizeof(planned_glyph));
        if (!glyphs) { io::free(plan_mem); return false; }

        for (io::u32 i = 0; i < plan.glyph_count; ++i) {
            const auto& gp = plan._glyphs[i];
            planned_glyph pg{};
            pg.codepoint = gp.codepoint;
            pg.glyph_index = gp.glyph_index;
            pg.rect = gp.rect;
            pg.x_min = gp.x_min; pg.y_min = gp.y_min;
            pg.x_max = gp.x_max; pg.y_max = gp.y_max;

            const auto hm = font.GetGlyphHorMetrics((int)gp.glyph_index);
            pg.advance = hm.advance;
            pg.lsb = hm.lsb;

            glyphs[i] = pg;
        }

        // ---- CPU atlas ----
        const io::u32 side = plan.atlas_side;
        const io::u32 comp = want_sdf ? 1u : 4u;
        const io::u32 stride = side * comp;
        const size_t atlas_bytes = (size_t)side * (size_t)side * (size_t)comp;

        io::u8* atlas_cpu = (io::u8*)io::alloc(atlas_bytes);
        if (!atlas_cpu) { io::free(glyphs); io::free(plan_mem); return false; }
        for (size_t i = 0; i < atlas_bytes; ++i) atlas_cpu[i] = 255;

        if (!font.Build(plan, atlas_cpu, stride)) {
            io::free(atlas_cpu);
            io::free(glyphs);
            io::free(plan_mem);
            return false;
        }

        io::free(plan_mem);

        out.mode = desc.mode;
        out.pixel_height = desc.pixel_height;
        out.spread_px = desc.spread_px;
        out.scale = plan.scale;
        out.spread_fu = plan.spread_fu;
        out.atlas_side = plan.atlas_side;
        out.glyph_count = plan.glyph_count;
        out.atlas_cpu = atlas_cpu;
        out.comp = comp;
        out.stride = stride;
        out.glyphs = glyphs;
        return true;
    }
    IO_NODISCARD static inline AtlasId gen_font_atlas(io::vector<font_atlas>& atlases, const planned_font_atlas& p) noexcept {
        font_atlas A{};
        A.mode = p.mode;
        A.pixel_height = p.pixel_height;
        A.spread_px = p.spread_px;
        A.scale = p.scale;
        A.spread_fu = p.spread_fu;
        A.atlas_side = p.atlas_side;
        A.glyph_count = p.glyph_count;

        A.tex = gl_upload_atlas(p.atlas_side, p.mode, p.atlas_cpu);

        if (!A.glyphs.reserve(p.glyph_count)) { if (A.tex) gl::DeleteTextures(1, &A.tex); return -1; }
        A.prog = build_text_program(p.mode);
        if (!A.prog) { if (A.tex) gl::DeleteTextures(1, &A.tex); return -1; }
        A.uniforms = text_uniforms::query(A.prog);

        io::u32 cap = 1;
        while (cap < p.glyph_count * 2u) cap <<= 1u;
        void* map_mem = io::alloc((size_t)cap * sizeof(glyph_map::slot));
        if (!map_mem || !A.map.init(map_mem, cap)) {
            if (A.prog) gl::DeleteProgram(A.prog);
            if (A.tex) gl::DeleteTextures(1, &A.tex);
            io::free(map_mem);
            return -1;
        }

        for (io::u32 i = 0; i < p.glyph_count; ++i) {
            const planned_glyph& pg = p.glyphs[i];
            font_atlas::glyph G{};
            G.codepoint = pg.codepoint;
            G.glyph_index = pg.glyph_index;
            G.rect = pg.rect;
            G.x_min = pg.x_min; G.y_min = pg.y_min;
            G.x_max = pg.x_max; G.y_max = pg.y_max;
            G.advance = pg.advance;
            G.lsb = pg.lsb;

            io::u32 idx = (io::u32)A.glyphs.size();
            A.glyphs.push_back(G);
            (void)A.map.insert(G.codepoint, idx);
        }

        if (!atlases.push_back(io::move(A))) return -1;
        return (AtlasId)(atlases.size() - 1);
    }
    template<typename... Scripts>
    static bool collect_codepoints(io::char_view ttf_path, const FontAtlasDesc& desc,
                                   io::vector<io::u32>& out, Scripts... scripts) noexcept {
        out.clear();

        // 0) read font bytes
        io::string ttf_bytes{};
        {
            fs::File f(ttf_path, io::OpenMode::Read);
            if (!f.is_open() || !f.read_all(ttf_bytes) || ttf_bytes.empty()) return false;
        }
        stbtt_stream::Font font;
        if (!font.ReadBytes(reinterpret_cast<uint8_t*>(ttf_bytes.data()))) return false;

        // 1) scripts -> count
        const io::u32 script_count = (io::u32)stbtt_codepoints::PlanGlyphs(font, scripts...);

        // 2) reserve total (scripts + desc)
        const io::u32 extra = (desc.codepoints && desc.codepoint_count) ? desc.codepoint_count : 0;
        if (!out.reserve((io::usize)script_count + (io::usize)extra)) return false;

        // 3) scripts -> fill
        io::u32 at = 0;
        auto sink = [&](io::u32 cp, int /*glyph*/) {
            (void)at;
            (void)out.push_back(cp);
            ++at;
        };
        int dummy[] = { (stbtt_codepoints::CollectGlyphs(font, sink, scripts), 0)... };
        (void)dummy;

        // 4) append desc.codepoints
        if (extra) {
            for (io::u32 i = 0; i < desc.codepoint_count; ++i)
                out.push_back(desc.codepoints[i]);
        }

        // 5) optional: unique
        // 
        // PSEUDOCODE
        // io::sort(out.begin(), out.end());
        // out.erase(io::unique(out.begin(), out.end()), out.end());

        return !out.empty();
    }
    struct text_bbox {
        float min_x, min_y, max_x, max_y;
        bool  any;
    };

    static inline text_bbox measure_text_bbox_px(const font_atlas& A, const TextDraw& d, float element_scale) noexcept {
        const float scale = d.scale * element_scale;
        const float line_px = (float)A.pixel_height * scale * ((d.line_height <= 0.f) ? 1.f : d.line_height);
        const float space_px = line_px * 0.5f;
        const float tab_px = space_px * ((d.tab_width <= 0.f) ? 4.f : d.tab_width);

        float sb = d.space_between;
        if (sb < -1.f) sb = -1.f;
        if (sb > 1.f) sb = 1.f;
        const float extra_px = sb * space_px;

        float pen_x = 0.0f;
        float pen_y = (float)A.pixel_height * scale;

        text_bbox bb{};
        bb.min_x = bb.min_y = 1e30f;
        bb.max_x = bb.max_y = -1e30f;
        bb.any = false;

        auto add_pt = [&](float x, float y) {
            bb.min_x = bb.min_x < x ? bb.min_x : x;
            bb.min_y = bb.min_y < y ? bb.min_y : y;
            bb.max_x = bb.max_x > x ? bb.max_x : x;
            bb.max_y = bb.max_y > y ? bb.max_y : y;
            bb.any = true;
        };

        io::usize it = 0;
        io::u32 cp = 0;
        while (utf8_next(d.text, it, cp)) {
            if (cp == '\n') { pen_x = 0.f; pen_y += line_px; continue; }

            if (cp == '\t') {
                const float rel = pen_x;
                const float k = (tab_px > 1e-6f) ? io::ceil(rel / tab_px) : 0.f;
                pen_x = k * tab_px;
                add_pt(pen_x, pen_y);
                continue;
            }

            if (cp == ' ') {
                pen_x += space_px + extra_px;
                add_pt(pen_x, pen_y);
                continue;
            }

            io::u32 gi = 0;
            if (!A.map.find(cp, gi)) continue;
            const auto& G = A.glyphs[gi];

            const float fu2px = A.scale * scale;
            const float gx0 = pen_x + (float)G.x_min * fu2px;
            const float gy0 = pen_y - (float)G.y_max * fu2px;
            const float gx1 = pen_x + (float)G.x_max * fu2px;
            const float gy1 = pen_y - (float)G.y_min * fu2px;

            add_pt(gx0, gy0); add_pt(gx1, gy1);

            pen_x += (float)G.advance * A.scale * scale + extra_px;
            add_pt(pen_x, pen_y);
        }

        if (!bb.any) { bb.min_x = bb.min_y = bb.max_x = bb.max_y = 0.f; }
        return bb;
    }

    static inline void dock_anchor_px(TextDock dock, float vw, float vh, float& ax, float& ay) noexcept {
        switch (dock) {
        case TextDock::TopL:    ax = 0.f;       ay = 0.f;    break;
        case TextDock::TopC:    ax = vw * 0.5f; ay = 0.f;    break;
        case TextDock::TopR:    ax = vw;        ay = 0.f;    break;
        case TextDock::LeftC:   ax = 0.f;       ay = vh * 0.5f; break;
        case TextDock::RightC:  ax = vw;        ay = vh * 0.5f; break;
        case TextDock::BottomL: ax = 0.f;       ay = vh;    break;
        case TextDock::BottomC: ax = vw * 0.5f; ay = vh;    break;
        case TextDock::BottomR: ax = vw;        ay = vh;    break;
        }
    }

    static inline void dock_align_shift(TextDock dock, const text_bbox& bb, float& dx, float& dy) noexcept {
        const bool top    = (dock == TextDock::TopL ||    dock == TextDock::TopC ||    dock == TextDock::TopR);
        const bool bottom = (dock == TextDock::BottomL || dock == TextDock::BottomC || dock == TextDock::BottomR);
        const bool left   = (dock == TextDock::TopL ||    dock == TextDock::LeftC ||   dock == TextDock::BottomL);
        const bool right  = (dock == TextDock::TopR ||    dock == TextDock::RightC ||  dock == TextDock::BottomR);

        if (left)        dx = -bb.min_x;
        else if (right)  dx = -bb.max_x;
        else             dx = -0.5f * (bb.min_x + bb.max_x); // center

        if (top)         dy = -bb.min_y;
        else if (bottom) dy = -bb.max_y;
        else             dy = -0.5f * (bb.min_y + bb.max_y); // center
    }

    // ------------------------------
    // text batching
    // ------------------------------
    static inline void push_glyph_quad(io::vector<text_vertex>& v, const font_atlas& a,
                                       const font_atlas::glyph& g,
                                       float pen_x, float pen_y, float scale,
                                       io::u32 col, io::u32 ocol,
                                       float outline_px, float softness_px) noexcept {
        const float fu2px = a.scale * scale;

        const float gx0 = pen_x + (float)g.x_min * fu2px;
        const float gy0 = pen_y - (float)g.y_max * fu2px;
        const float gx1 = pen_x + (float)g.x_max * fu2px;
        const float gy1 = pen_y - (float)g.y_min * fu2px;

        const float inv = 1.0f / (float)a.atlas_side;
        const float u0 = ((float)g.rect.x + 0.5f) * inv;
        const float v0 = ((float)g.rect.y + 0.5f) * inv;
        const float u1 = ((float)(g.rect.x + g.rect.w) - 0.5f) * inv;
        const float v1 = ((float)(g.rect.y + g.rect.h) - 0.5f) * inv;

        auto V = [&](float x, float y, float u, float v_) {
            text_vertex tv{};
            tv.x = x; tv.y = y; tv.u = u; tv.v = v_;
            tv.color_rgba = col;
            tv.outline_rgba = ocol;
            tv.outline_px = outline_px;
            tv.softness_px = softness_px;
            v.push_back(tv);
        };

        V(gx0, gy0, u0, v0);
        V(gx1, gy0, u1, v0);
        V(gx1, gy1, u1, v1);

        V(gx0, gy0, u0, v0);
        V(gx1, gy1, u1, v1);
        V(gx0, gy1, u0, v1);
    }
    // key for batching: atlas id is enough (because atlas owns tex+prog+mode+spread)
    struct text_batch {
        AtlasId atlas = -1;
        io::u32 first = 0;
        io::u32 count = 0;
    };

    // One-frame accumulator
    struct text_queue {
        io::vector<text_vertex> verts;
        io::vector<text_batch>  batches;

        // previous batch state:
        AtlasId cur_atlas = -1;
        io::u32 cur_first = 0;

        inline void clear() noexcept { verts.clear(); batches.clear(); cur_atlas = -1; cur_first = 0; }

        inline void begin_batch(AtlasId atlas) noexcept {
            if (cur_atlas == atlas) return;
            // close previous
            if (cur_atlas != -1) {
                const io::u32 cnt = (io::u32)verts.size() - cur_first;
                if (cnt) batches.push_back(text_batch{ cur_atlas, cur_first, cnt });
            }
            cur_atlas = atlas;
            cur_first = (io::u32)verts.size();
        }

        inline void end() noexcept {
            if (cur_atlas != -1) {
                const io::u32 cnt = (io::u32)verts.size() - cur_first;
                if (cnt) batches.push_back(text_batch{ cur_atlas, cur_first, cnt });
            }
            cur_atlas = -1;
            cur_first = 0;
        }
    };
    
    } // namespace internal

    // --- CRTP base ---
    template <typename Derived>
    struct Window : public IWindow {
        friend struct native::Window;
    public:
        union { native::Opengl g; };

        explicit inline Window(int w = 440, int h = 320, bool shown = true, bool bordless = false) noexcept
            : _width{ w }, _height{ h }, _shown{ shown },
            _native_window{ *this, w, h, shown, bordless }, _ctx{ *this, 3, 3 }
        {
            static const gl::Attr attrs[] = {
                gl::AttrOf<float>(2),  // pos
                gl::AttrOf<float>(2),  // uv
                gl::AttrOf<io::u8>(4, true), // color (norm)
                gl::AttrOf<io::u8>(4, true), // outline (norm)
                gl::AttrOf<float>(2),  // params
            };
            gl::MeshBinder::setup(_text_vao, _text_vbo, io::view<const gl::Attr>(attrs, sizeof(attrs) / sizeof(attrs[0])));
        }
        inline ~Window() noexcept { }

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

        // Getters
        IO_NODISCARD inline RendererApi api() const noexcept override { return _ctx.api; }
        IO_NODISCARD inline       native::Opengl& opengl()       noexcept override { return g; }
        IO_NODISCARD inline const native::Opengl& opengl() const noexcept override { return g; }
        IO_NODISCARD inline       native::Window& native()       noexcept override { return _native_window; }
        IO_NODISCARD inline const native::Window& native() const noexcept override { return _native_window; }
        IO_NODISCARD inline int width() const noexcept override { return _width; }
        IO_NODISCARD inline int height() const noexcept override { return _height; }
        IO_NODISCARD inline float mouseX() const noexcept override { return _mouse_x; }
        IO_NODISCARD inline float mouseY() const noexcept override { return _mouse_y; }

        IO_NODISCARD CursorState getCursorState() const noexcept override {
            return _is_cursor_edits_text      ? CursorState::TextEdit :
                   _is_cursor_hresize         ? CursorState::HResizing :
                   _is_cursor_vresize         ? CursorState::VResizing :
                   _is_cursor_hovering_button ? CursorState::HoversButton :
                                                CursorState::Default;
        }

        IO_NODISCARD inline bool isShown() const noexcept override { return _shown; }
        IO_NODISCARD inline bool isFullscreen() const noexcept override { return _fullscreen; }
        IO_NODISCARD inline bool isCursorVisible() const noexcept override { return _cursor; }
        IO_NODISCARD inline bool isMouseDown() const noexcept override { return _mouse_down; }
        IO_NODISCARD inline bool isPrevMouseDown() const noexcept override { return _prev_mouse_down; }
        IO_NODISCARD inline float UiScale() const noexcept override { return _ui_scale; }
        inline void onGeometryChange(int w, int h) noexcept override;

        IO_NODISCARD inline bool isMouseReleased() const noexcept { return _mouse_released; }

    public:
        IO_NODISCARD inline bool PollEvents() noexcept {
            const bool prev = _mouse_down;

            // const Window<Derived>* -> IWindow&
            auto* self_nc = const_cast<Window<Derived>*>(this);
            const bool running = native().PollEvents(static_cast<IWindow&>(*self_nc));

            _mouse_released = (prev && !_mouse_down);
            _prev_mouse_down = prev;
            return running;
        }
        inline void Render() noexcept override;
        inline void SwapBuffers() const noexcept;

        // Font files: stores path, checks file exists
        FontId LoadFont(io::char_view ttf_path) noexcept;
        // Plan/Build atlas (load TTF bytes, plan, build, upload texture)
        AtlasId GenerateFontAtlas(FontId font_id, const FontAtlasDesc& desc) noexcept;
        template<typename... Scripts>
        AtlasId GenerateFontAtlas(FontId font_id, const FontAtlasDesc& desc, Scripts... scripts) noexcept;
        
        ButtonState Button(const ButtonDraw& b) noexcept;
        
        // Immediate text
        void DrawText(const TextDraw& d) noexcept;
        // Call once per frame inside Render() (or end of Render())
        void FlushText() noexcept;

        // --- Setters ---
        inline void setShow(bool value) noexcept { _shown = value; native().setShow(value); }
        inline void setTitle(io::char_view new_title) const noexcept { native().setTitle(new_title); }
        inline void setFullscreen(bool value) noexcept { _fullscreen = value; native().setFullscreen(value); }
        inline void setElementScale(float value) noexcept { _ui_scale = value; }
        inline void setCursor(Cursor c) noexcept override {
            _cursor = (c != Cursor::Hidden);
            _native_window.setCursor(c);
        }
        inline void setCursorVisible(bool value) noexcept {
            _cursor = value;
            setCursor(value ? Cursor::Arrow : Cursor::Hidden);
        }
        inline void resetCursorState() noexcept override {
            _is_cursor_edits_text = false;
            _is_cursor_hresize = false;
            _is_cursor_vresize = false;
            _is_cursor_hovering_button = false;
        }

        inline io::u16 getAtlasSide(AtlasId id) const noexcept { return (id<0) ? 0 : (id>=(int)_atlases.size()) ? 0 : _atlases[id].atlas_side;   }

    protected:
        inline void setMouseX(float v) noexcept override { _mouse_x = v; }
        inline void setMouseY(float v) noexcept override { _mouse_y = v; }
        inline void setMouseDown(bool v) noexcept override { _mouse_down = v; }

    private:
        // --- window ---
        native::Window _native_window;
        int _width;
        int _height;

        // --- mouse ---
        float _mouse_x = 0.f;
        float _mouse_y = 0.f;
        bool  _mouse_down = false;
        bool  _prev_mouse_down = false;
        bool  _mouse_released = true;

        // --- window params ---
        bool _cursor = true;
        bool _is_cursor_hovering_button = false;
        bool _is_cursor_edits_text = false;
        bool _is_cursor_hresize = false;
        bool _is_cursor_vresize = false;
        bool _fullscreen = false;
        bool _shown;
        float _ui_scale{ 1.f };
        ContextGuardT<Window> _ctx; // RAII context

        // --- text ---
        bool _text_mesh_ready = false;
        struct text_cmd {
            AtlasId atlas;
            io::u32 first;
            io::u32 count;
        };
        gl::VertexArray _text_vao;
        gl::Buffer      _text_vbo{ gl::BufferTarget::ArrayBuffer };
        io::vector<io::string> _font_paths;
        io::vector<internal::font_atlas> _atlases;
        internal::text_queue _textq;
    }; // struct Window

#ifdef IO_IMPLEMENTATION
    template <typename Derived>
    inline void Window<Derived>::onGeometryChange(int w, int h) noexcept {
        _width=w; _height=h; // Update window size
        switch (api()) { // Switch between APIs
        case RendererApi::Opengl:
            io::sleep_ms(7); // Hack: slow down the program.
            gl::Viewport(0, 0, w, h); // Call viewport 
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
        if (!_ctx.alive) return;
        switch (api()) {
        case RendererApi::Opengl: g.Render(*this); break;
        default: break;
        } // switch
    } // render

    template <typename Derived>
    inline void Window<Derived>::SwapBuffers() const noexcept {
        if (!_ctx.alive) return;
        switch (api())
        {
        case RendererApi::Opengl: g.SwapBuffers(*this); break;
        default: break;
        } // switch
    } // render
#endif // IO_IMPLEMENTATION

    template<typename Derived>
    FontId Window<Derived>::LoadFont(io::char_view ttf_path) noexcept {
        if (!fs::exists(ttf_path)) return -1;
        if (!_font_paths.push_back(io::string{ ttf_path })) return -1;
        return (FontId)(_font_paths.size() - 1);
    }

    

    template<typename Derived>
    void Window<Derived>::DrawText(const TextDraw& d) noexcept {
        if (d.atlas < 0 || (io::u32)d.atlas >= _atlases.size()) return;
        const internal::font_atlas& A = _atlases[(io::u32)d.atlas];

        // batch switch
        _textq.begin_batch(d.atlas);

        const io::u32 col = hi::internal::pack_rgba8(d.style.r, d.style.g, d.style.b, d.style.a);
        const io::u32 ocol = d.style.outline
            ? hi::internal::pack_rgba8(d.style.or_, d.style.og_, d.style.ob_, d.style.oa_)
            : 0u;

        // ---- dock transform ----
        const float vw = (float)width();
        const float vh = (float)height();

        float ax, ay; internal::dock_anchor_px(d.dock, vw, vh, ax, ay);

        const auto bb = internal::measure_text_bbox_px(A, d, _ui_scale);

        float dx, dy; internal::dock_align_shift(d.dock, bb, dx, dy);

        const float origin_x = ax + d.x + dx;
        const float origin_y = ay + d.y + dy;

        // ---- existing layout (but use origin_*) ----
        const float scale = d.scale * _ui_scale;
        const float line_px = (float)A.pixel_height * scale * ((d.line_height <= 0.f) ? 1.f : d.line_height);
        const float space_px = line_px * 0.5f;
        const float tab_px = space_px * ((d.tab_width <= 0.f) ? 4.f : d.tab_width);

        float sb = d.space_between;
        if (sb < -1.f) sb = -1.f;
        if (sb > 1.f) sb = 1.f;
        const float extra_px = sb * space_px;

        float pen_x = origin_x;
        float pen_y = origin_y + (float)A.pixel_height * scale; // top->baseline

        io::usize it = 0;
        io::u32 cp = 0;
        while (hi::internal::utf8_next(d.text, it, cp)) {
            if (cp == '\n') { pen_x = origin_x; pen_y += line_px; continue; }

            if (cp == '\t') {
                const float rel = pen_x - origin_x;
                const float k = (tab_px > 1e-6f) ? io::ceil(rel / tab_px) : 0.f;
                pen_x = origin_x + k * tab_px;
                continue;
            }

            if (cp == ' ') { pen_x += space_px + extra_px; continue; }

            io::u32 gi = 0;
            if (!A.map.find(cp, gi)) continue;
            const auto& G = A.glyphs[gi];

            internal::push_glyph_quad(_textq.verts, A, G, pen_x, pen_y, scale,
                col, ocol, d.style.outline_px, d.style.softness_px);

            pen_x += (float)G.advance * A.scale * scale + extra_px;
        }
    }

    template<typename Derived>
    void Window<Derived>::FlushText() noexcept {
        if (_textq.verts.empty()) return;

        _textq.end();
        if (_textq.batches.empty()) { _textq.clear(); return; }

        // shared upload once
        _text_vao.bind();
        _text_vbo.bind();
        _text_vbo.data(_textq.verts.data(), _textq.verts.size() * sizeof(hi::internal::text_vertex),
                        gl::BufferUsage::DynamicDraw);

        gl::Enable(gl::Capability::Blend);
        gl::BlendFunc(gl::BlendFactor::SrcAlpha, gl::BlendFactor::OneMinusSrcAlpha);

        const float vw = (float)width();
        const float vh = (float)height();

        io::u32 last_prog = 0;
        io::u32 last_tex = 0;

        for (io::u32 bi = 0; bi < _textq.batches.size(); ++bi) {
            const auto& b = _textq.batches[bi];
            auto& A = _atlases[(io::u32)b.atlas];

            if (A.prog != last_prog) {
                gl::UseProgram(A.prog);
                last_prog = A.prog;

                const auto& u = A.uniforms;
                if (u.uViewport >= 0) gl::Uniform2f(u.uViewport, vw, vh);
                if (u.uPxRange >= 0) gl::Uniform1f(u.uPxRange, A.spread_px);
                if (u.uAtlasSide >= 0) gl::Uniform1f(u.uAtlasSide, (float)A.atlas_side);
                if (u.uTex >= 0) gl::Uniform1i(u.uTex, 0);
            }

            if (A.tex != last_tex) {
                gl::ActiveTexture(gl::TexUnit::_0);
                gl::BindTexture(gl::TexTarget::Tex2D, A.tex);
                last_tex = A.tex;
            }

            gl::DrawArrays(gl::PrimitiveMode::Triangles, (int)b.first, (int)b.count);
        }

        gl::BindTexture(gl::TexTarget::Tex2D, 0);
        gl::BindVertexArray(0);
        gl::UseProgram(0);

        _textq.clear();
    }

    
    template <typename Derived>
    AtlasId Window<Derived>::GenerateFontAtlas(FontId font_id, const FontAtlasDesc& desc) noexcept {
        if (font_id < 0 || (io::u32)font_id >= _font_paths.size()) return -1;

        internal::planned_font_atlas p{};
        if (!internal::plan_font(_font_paths[(io::u32)font_id].as_view(), desc, p)) return -1;

        AtlasId id = internal::gen_font_atlas(_atlases, p);
        internal::planned_destroy(p);
        return id;
    }

    template <typename Derived>
    template<typename... Scripts>
    AtlasId Window<Derived>::GenerateFontAtlas(FontId font_id,
                const FontAtlasDesc& desc, Scripts... scripts) noexcept {
        if (font_id < 0 || (io::u32)font_id >= _font_paths.size()) return -1;

        // 1) collect codepoints (scripts + desc)
        io::vector<io::u32> cps{};
        if (!hi::internal::collect_codepoints(_font_paths[(io::u32)font_id].as_view(), desc, cps, scripts...))
            return -1;

        // 2) create local desc with replaced codepoints
        FontAtlasDesc d2 = desc;
        d2.codepoints = cps.data();
        d2.codepoint_count = (io::u32)cps.size();

        // 3) repeat read from disk
        internal::planned_font_atlas p{};
        if (!internal::plan_font(_font_paths[(io::u32)font_id].as_view(), d2, p)) return -1;

        AtlasId id = internal::gen_font_atlas(_atlases, p);
        internal::planned_destroy(p);
        return id;
    }


    template <typename Derived>
    inline ButtonState Window<Derived>::Button(const ButtonDraw& b) noexcept {
        ButtonState out{};
        if (b.atlas < 0 || (io::u32)b.atlas >= _atlases.size()) return out;
        const internal::font_atlas& A = _atlases[(io::u32)b.atlas];

        const float mx = this->mouseX();
        const float my = this->mouseY();
        const bool mouse_released = this->isMouseReleased();

        // 1) collect temporary TextDraw for measurement/drawing
        TextDraw td{};
        td.atlas         = b.atlas;
        td.dock          = b.dock;
        td.x = b.x; td.y = b.y;
        td.scale         = b.scale;
        td.text          = b.text;
        td.line_height   = b.line_height;
        td.tab_width     = b.tab_width;
        td.space_between = b.space_between;

        // 2) text bbox (local) + dock transform => bbox in window px
        const float vw = (float)width();
        const float vh = (float)height();

        float ax = 0, ay = 0;
        internal::dock_anchor_px(b.dock, vw, vh, ax, ay);

        const auto bb = internal::measure_text_bbox_px(A, td, _ui_scale);

        float dx = 0, dy = 0;
        internal::dock_align_shift(b.dock, bb, dx, dy);

        const float origin_x = ax + b.x + dx;
        const float origin_y = ay + b.y + dy;

        float x0 = origin_x + bb.min_x;
        float y0 = origin_y + bb.min_y;
        float x1 = origin_x + bb.max_x;
        float y1 = origin_y + bb.max_y;

        // padding
        x0 -= b.style.pad_x; y0 -= b.style.pad_y;
        x1 += b.style.pad_x; y1 += b.style.pad_y;

        out.x0 = x0; out.y0 = y0; out.x1 = x1; out.y1 = y1;

        // 3) hit test
        out.hovered = (mx >= x0 && mx <= x1 && my >= y0 && my <= y1);
        out.held = out.hovered && _mouse_down;
        out.clicked = out.hovered && mouse_released; // release inside

        // 4) style
        if (out.held) {
            td.style = b.style.active;
        }
        else if (out.hovered) {
            _is_cursor_hovering_button = true;
            td.style = b.style.hover;
        }
        else {
            td.style = b.style.normal;
        }

        // 5) draw
        DrawText(td);

        return out;
    }
} // namespace hi