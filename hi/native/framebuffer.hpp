#pragma once
#include "types.hpp"
#include "window.hpp"

#ifdef __linux__
// linux impl
#elif defined(_WIN32)
#   include <Windows.h>
#endif

namespace hi {
    namespace native {

        // ------------------------- Native Framebuffer -----------------------
        struct Framebuffer {
        private:
        #if defined(__linux__)
            // linux impl
        #elif defined(_WIN32)
            HDC     _hdc{ nullptr };
            HBITMAP _bmp{ nullptr };
            void* _pixels{ nullptr };
        #endif
        
        public:
            explicit Framebuffer() noexcept = default;
            ~Framebuffer() noexcept;
        
            Framebuffer(const Framebuffer&) = delete;
            Framebuffer& operator=(const Framebuffer&) = delete;
            Framebuffer(Framebuffer&&) = delete;
            Framebuffer& operator=(Framebuffer&&) = delete;
        
        public:
            void Render(IWindow&) noexcept;
            void SwapBuffers(const IWindow&) const noexcept;
            IO_NODISCARD bool Recreate(const IWindow& win, int width, int height, AboutError& ae) noexcept;
            void Clear(io::u32 rgba, int width, int height) const noexcept;
            void DrawPixel(int x, int y, int width, int height, io::u32 color) const noexcept;
        
        #if defined(__linux__)
        
        #elif defined(_WIN32)
            HDC getHdc() const noexcept { return _hdc; }
            void setHdc(HDC new_hdc) noexcept { _hdc = new_hdc; }
            HBITMAP getBmp() const noexcept { return _bmp; }
            void setBmp(HBITMAP new_bmp) noexcept { _bmp = new_bmp; }
            void* getPixels() const noexcept { return _pixels; }
            void setPixels(void* new_pixels) noexcept { _pixels = new_pixels; }
        #endif
        }; // struct Framebuffer
    } // namespace native
} // namespace hi

#if defined(__linux__)
    // linux impl
#elif defined(_WIN32)
#   include "../platform/windows/framebuffer_impl.hpp"
#endif