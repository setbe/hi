#pragma once
#include "../../native/i_window.hpp"

// ------------------- OS-dependent Includes -------------------------
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <Psapi.h> // link with psapi.lib
#undef MessageBox
#undef CreateWindow
#include <gl/GL.h>

namespace hi {
    namespace native {
        inline void Opengl::Render(IWindow& win) noexcept {
            PAINTSTRUCT ps;
            BeginPaint(win.native().getHwnd(), &ps);
            if (wglGetCurrentContext() != getHglrc() ||
                wglGetCurrentDC() != win.native().getHdc()) {
                wglMakeCurrent(win.native().getHdc(), getHglrc());
            }
            win.onRender();
            EndPaint(win.native().getHwnd(), &ps);
        } // Render

        inline void Opengl::SwapBuffers(const IWindow& win) const noexcept {
#ifdef __LA_CXX17 // Wrap it in macro: Cannot access PF_DESCRIPTOR at runtime
            static_assert(native::PF_DESCRIPTOR.dwFlags & PFD_DOUBLEBUFFER,
                "OpenGL renderer requires double buffering");
#endif
            ::SwapBuffers(reinterpret_cast<HDC>(win.native().getHdc()));
        } // SwapBuffers


#pragma region _dummy window
        // ----------------- Dummy Window for Modern OpenGL -----------------------

        IO_CONSTEXPR_VAR LPCSTR DUMMY_CLASS_NAME = "d";
        struct DummyWindow {
            // Constructor
            inline DummyWindow() noexcept :
                _hinstance{ GetModuleHandleA(nullptr) },
                _hwnd{ nullptr }, _hdc{ nullptr }, _ctx{ nullptr } {

                WNDCLASSA wc{};
                wc.style = CS_OWNDC;
                wc.lpfnWndProc = DefWindowProcA;
                wc.hInstance = _hinstance;
                wc.lpszClassName = DUMMY_CLASS_NAME;

                if (!RegisterClassA(&wc))
                    return;

                _hwnd = CreateWindowExA(
                    /*    dwExStyle */ 0,
                    /*  lpClassName */ DUMMY_CLASS_NAME,
                    /* lpWindowName */ " ",
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
                if (_hdc)  { ReleaseDC(_hwnd, _hdc); _hdc = nullptr; }
                if (_hwnd) { DestroyWindow(_hwnd);  _hwnd = nullptr; }
                UnregisterClassA(DUMMY_CLASS_NAME, _hinstance);
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

            static inline AboutError CreateModernContext(IWindow& win,
                wgl::PFNWGLCHOOSEPIXELFORMATARBPROC    choose,
                wgl::PFNWGLCREATECONTEXTATTRIBSARBPROC create) noexcept
            {
                int format;
                UINT num_formats;
                HDC main_dc{ reinterpret_cast<HDC>(win.native().getHdc()) };
                if (!choose(main_dc, wgl::PIXEL_ATTRS, nullptr, 1, &format, &num_formats))
                    return AboutError::w_ChoosePixelFormatARB;

                PIXELFORMATDESCRIPTOR pfd{};
                if (!SetPixelFormat(main_dc, format, &pfd)) return AboutError::w_SetPixelFormat;

                HGLRC ctx = create(main_dc, nullptr, wgl::CONTEXT_ATTRS);
                if (!ctx) return AboutError::w_CreateContextAttribsARB;
                if (!wglMakeCurrent(main_dc, ctx)) return AboutError::w_CreateModernContext;

                if (!wglGetCurrentContext()) return AboutError::w_GetCurrentContext;
                if (!wglGetCurrentDC())      return AboutError::w_GetCurrentDC;
                win.opengl().setHglrc(ctx);

                return AboutError::None;
            } // CreateModernContext
        } // namespace wgl
#pragma endregion WGL

        // --- Create OpenGL Context ----
        IO_NODISCARD AboutError Opengl::CreateContext(IWindow& win) noexcept {
            DummyWindow dummy{};

            wgl::PFNWGLCHOOSEPIXELFORMATARBPROC choose{ nullptr };
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
    } // namespace native
} // namespace hi