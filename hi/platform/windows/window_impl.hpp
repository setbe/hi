#pragma once

#include "../../native/types.hpp"
#include "../../native/i_window.hpp"

// ------------------- OS-dependent Includes -------------------------
#if defined(__linux__)
    // linux impl
#elif defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   include <Psapi.h> // link with psapi.lib
#   undef MessageBox
#   undef CreateWindow
#   include <gl/GL.h>
#endif

namespace hi {
    namespace native {
        IO_CONSTEXPR_VAR wchar_t WINDOW_CLASSNAME[]{L"_"};
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
                wc.hIcon = LoadIconW(hinstance, IDI_APPLICATION);
                wc.hCursor = LoadCursorW(hinstance, IDC_ARROW);

                if (!RegisterClassExW(&wc))
                    win.onError(Error::Window, AboutError::w_WindowClass);
            }
            is_registered = true;

            DWORD style{
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
            if (!win.native().getHwnd()) {
                win.onError(Error::Window, AboutError::w_Window);
                return;
            }
            SetWindowLongPtrW(win.native().getHwnd(), GWLP_USERDATA,
                reinterpret_cast<LONG_PTR>(&win));

            // ---- 3. Get DC
            HDC new_hdc = GetDC(win.native().getHwnd());
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
        inline void Window::setTitle(const char* title) const noexcept {
            IO_CONSTEXPR_VAR usize BUF_SIZE = 128;
            wchar_t wide_buf[BUF_SIZE] = {};
            int len = MultiByteToWideChar(
                /*       CodePage */ CP_UTF8,
                /*        dwFlags */ 0,
                /* lpMultiByteStr */ title,
                /*    cbMultiByte */ -1,
                /*  lpWideCharStr */ wide_buf,
                /*    cchWideChar */ BUF_SIZE);
            SetWindowTextW(_hwnd, len > 0 ? wide_buf : L""); // Fallback: empty title
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
            int _A  = static_cast<int>(Key::A);
            int _0  = static_cast<int>(Key::_0);
            int _F1 = static_cast<int>(Key::F1);
            int wp = static_cast<int>(wparam);

            if (wp>='A' && wp<='Z')                 // Letters A-Z 
                return Key_t{ wp - 'A' + _A }.key();
            if (wp>='0' && wp<='9')                 // Digits 0-9
                return Key_t{ wp - '0' + _0 }.key();
            if (wp>=VK_F1 && wp<=VK_F12)            // F1-F12
                return Key_t{ wp - VK_F1 + _F1 }.key();
            if (wp>=VK_NUMPAD0 && wp<=VK_NUMPAD9)   // Numpad 0-9
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

            case WM_PAINT: {
                win->Render();
                return 0;
            } // WM_PAINT

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

            case WM_MOUSEMOVE:
                win->onMouseMove(LOWORD(lparam), HIWORD(lparam));
                return 0;

            case WM_SETFOCUS:
                win->onFocusChange(true);
                return 0;

            case WM_KILLFOCUS:
                win->onFocusChange(false);
                return 0;

            case WM_MOUSEWHEEL: {
                int delta = GET_WHEEL_DELTA_WPARAM(wparam); // Returns 120 or -120
                // @TODO Replace `0.f` with actual horizontal scroll delta
                win->onScroll(delta / 120.f, 0.f); // Normalize before callback
                return 0;
            }
            case WM_KEYDOWN:
                win->onKeyDown(HandleKey(wparam, true));
                return 0;

            case WM_KEYUP:
                win->onKeyUp(HandleKey(wparam, false));
                return 0;

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
                auto create_struct = reinterpret_cast<CREATESTRUCT*>(lparam);
                SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)(create_struct->lpCreateParams));
#ifdef _PSAPI_H_ // Clear resources
                SetProcessWorkingSetSize(GetCurrentProcess(), -1, -1);
                K32EmptyWorkingSet(GetCurrentProcess());
#endif
                return TRUE;
            } // WM_NCCREATE

            case WM_DESTROY:
                PostQuitMessage(0);
                return 0;
            } // switch (msg)

            return DefWindowProcW(hwnd, msg, wparam, lparam);
        } // WinProc
    } // namespace native
} // namespace hi