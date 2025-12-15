#pragma once

#include "i_window.hpp"

#ifdef __linux__
// linux impl
#elif defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   undef MessageBox
#   undef CreateWindow
#endif

namespace hi {

    // Forward declaration
    template <typename Derived>
    struct Window; // note: remark this is not native::Window

    namespace native {
        // --------------------------- Native Window --------------------------
        struct Window {
        private:
#       ifdef __linux__
            // linux impl
#       elif defined(_WIN32)
            HDC _hdc{ nullptr };
            HWND _hwnd{ nullptr };
#       endif
    
        public:
            inline explicit Window(IWindow& win, int width, int height, bool shown, bool bordless) noexcept;
            inline ~Window() noexcept;
    
            Window(const Window&) = delete;
            Window& operator=(const Window&) = delete;
            Window(Window&&) = delete;
            Window& operator=(Window&&) = delete;

            inline bool PollEvents(IWindow& win) const noexcept;
    
        public:
            inline void setTitle(const char* title) const noexcept;
            inline void setShow(bool) const noexcept;
            inline void setFullscreen(bool) const noexcept;
            inline void setCursorVisible(bool) const noexcept;
    
        public:
#       if defined(__linux__)
            // linux impl
#       elif defined(_WIN32)
            HDC getHdc() const noexcept { return _hdc; }
            void setHdc(HDC new_hdc) noexcept { _hdc = new_hdc; }
            HWND getHwnd() const noexcept { return _hwnd; }
            void setHwnd(HWND new_hwnd) noexcept { _hwnd = new_hwnd; }
#       endif
        }; // struct Window


    } // namespace native
} // namespace hi

#ifdef __linux__
// linux impl
#elif defined(_WIN32)
#   include "../platform/windows/window_impl.hpp"
#endif