#pragma once

#include "../../native/types.hpp"
#include "../../native/window.hpp"

#if defined(__linux__)
// linux impl
#elif defined(_WIN32)
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#   undef MessageBox
#   undef CreateWindow
#endif

namespace hi {
    namespace native {
        // Helper: Color converter for Windows' GDI
        static inline io::u32 RGBAtoBGRA(io::u32 color) noexcept {
            io::u8 r = (color >> 16) & 0xFF;
            io::u8 g = (color >> 8) & 0xFF;
            io::u8 b = (color) & 0xFF;
            io::u8 a = (color >> 24) & 0xFF;
            return (a << 24) | (r << 0) | (g << 8) | (b << 16);
        }

        void Framebuffer::Render(IWindow& win) noexcept {
            PAINTSTRUCT ps;
            BeginPaint(win.native().getHwnd(), &ps);
            win.onRender();
            BitBlt(/* HDC hdc */ ps.hdc,
                /* int x  */ 0,
                /* int y  */ 0,
                /* int cx */ win.width(),
                /* int cy */ win.height(),
                /* HDC src */ win.framebuffer().getHdc(),
                /* int x1 */ 0,
                /* int x2 */ 0,
                /* DWORD rop */ SRCCOPY);
            EndPaint(win.native().getHwnd(), &ps);
        }

        void Framebuffer::SwapBuffers(const IWindow& win) const noexcept {
            InvalidateRect(win.native().getHwnd(), nullptr, FALSE);
            UpdateWindow(win.native().getHwnd());
        }

        void Framebuffer::Clear(io::u32 rgba, int win_width, int win_height) const noexcept {
            if (!_pixels) return;

            io::u32* pixels = static_cast<io::u32*>(_pixels);
            const size_t count = static_cast<size_t>(win_width) * win_height;
            const io::u32 bgra = RGBAtoBGRA(rgba);

            for (size_t i = 0; i < count; ++i)
                pixels[i] = bgra;
        }

        Framebuffer::~Framebuffer() noexcept {
            if (_bmp) DeleteObject(_bmp);
            if (_hdc) DeleteDC(_hdc);
        }

        IO_NODISCARD bool Framebuffer::Recreate(
            const IWindow& win, int width_, int height_, AboutError& ae) noexcept {
            // 1. Free old resources
            if (_bmp) {
                DeleteObject(_bmp);
                _bmp = nullptr;
            }
            if (_hdc) {
                DeleteDC(_hdc);
                _hdc = nullptr;
            }

            // 2. Reject invalid dimensions
            if (width_ <= 0 || height_ <= 0)
                return true; // Exit without error and without recreating

            // 3. Create compatible DC
            {
                HDC device = GetDC(win.native().getHwnd());
                if (!device) {
                    ae = AboutError::w_GetCurrentDC;
                    return false;
                }
                _hdc = CreateCompatibleDC(device);
                ReleaseDC(win.native().getHwnd(), device);
            }
            if (!_hdc) {
                ae = AboutError::w_CreateCompatibleDC;
                return false;
            }

            // 4. Create DIB section
            // Prepare bitmap info (top-down DIB)
            BITMAPINFO bmi{};
            bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
            bmi.bmiHeader.biWidth = width_;
            bmi.bmiHeader.biHeight = -height_; // top-down
            bmi.bmiHeader.biPlanes = 1;
            bmi.bmiHeader.biBitCount = 32;
            bmi.bmiHeader.biCompression = BI_RGB;

            void* ppv_bits{};
            _bmp = CreateDIBSection(
                /*        HDC */ _hdc,
                /* BITMAPINFO */ &bmi,
                /*       UINT */ DIB_RGB_COLORS,
                /*    ppvBits */ &ppv_bits,
                /*   hSection */ nullptr,
                /*     offset */ 0);

            // Check `bitmap` && `received pixel buffer`
            if (!_bmp || !ppv_bits) {
                ae = AboutError::w_CreateDibSection;
                return false;
            }

            // Check selected object
            if (!SelectObject(_hdc, _bmp)) {
                ae = AboutError::w_SelectObject;
                return false;
            }

            _pixels = ppv_bits;

            return true;
        } // Framebuffer::Recreate

        void Framebuffer::DrawPixel(int x, int y, int width, int height, io::u32 rgba_color) const noexcept {
            // Defensive: don't draw out of bounds
            if (x < 0 || x >= width || y < 0 || y >= height || !_pixels)
                return;

            // Each pixel is 4 bytes (BGRA32)
            const int pitch = width*4;
            io::u8* dst = static_cast<io::u8*>(_pixels);
            io::u32* pixel_ptr = reinterpret_cast<io::u32*>(dst + (y*pitch) + x*4);
            *pixel_ptr = RGBAtoBGRA(rgba_color); // draw
        }
    } // namespace native
} // namespace hi