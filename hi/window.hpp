#pragma once
#include "native/window.hpp"
#include "native/framebuffer.hpp"
#include "native/opengl.hpp"

namespace hi {
    // --- CRTP base ---
    template <typename Derived>
    struct Window : public IWindow {
    public:
        union {
            native::Framebuffer fb;
            native::Opengl g;
        };

        explicit inline Window(
            RendererApi  api = RendererApi::Software,
            int            w = 440,
            int            h = 320,
            bool       shown = true,
            bool    bordless = false) noexcept;
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
        IO_NODISCARD inline       native::Framebuffer& framebuffer()       noexcept override { return fb; }
        IO_NODISCARD inline const native::Framebuffer& framebuffer() const noexcept override { return fb; }
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
#ifdef _WIN32
            PostMessageW(reinterpret_cast<HWND>(native().getHwnd()), WM_QUIT, 0, 0);
#else
#   error "Not implemented"
#endif
        }

        // --- Setters ---
        inline void setShow(bool value) const noexcept { native().setShow(value); }
        inline void setTitle(const char* new_title) const noexcept { native().setTitle(new_title); }
        inline void setFullscreen(bool value) const noexcept { native().setFullscreen(value); }
        inline void setCursorVisible(bool value) const noexcept { native().setCursorVisible(value); }
        void setApi(RendererApi api) noexcept;

    private:
        native::Window _native_window;
        RendererApi _renderer_api{ RendererApi::None };
        
        int _width;
        int _height;
    }; // struct Window



    template <typename Derived>
    Window<Derived>::Window(RendererApi api, int w, int h, bool shown, bool bordless) noexcept
        : _width{ w }, _height{ h }, fb{}, _native_window{ *this,w,h,shown,bordless } {
        setApi(api);
    }

    template <typename Derived>
    inline Window<Derived>::~Window() noexcept {
        switch (api()) {
        case RendererApi::Software: fb.~Framebuffer(); break;
        case RendererApi::Opengl: g.~Opengl(); break;
        case RendererApi::Vulkan: break; // @TODO
        default: break;
        } // switch
    }

    template <typename Derived>
    inline void Window<Derived>::onGeometryChange(int w, int h) noexcept {
        HDC hdc = reinterpret_cast<HDC>(native().getHdc());
        // Update window size
        _width = w;
        _height = h;

        // Switch between APIs
        switch (api()) {
        case RendererApi::Software: { // Recreate framebuffer
            AboutError about{ AboutError::None };
            if (!fb.Recreate(*this, w, h, about))
                onError(Error::WindowFramebuffer, about);
            break;
        } // software
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
        switch (api()) {
        case RendererApi::Software: fb.Render(*this); break;
        case RendererApi::Opengl: g.Render(*this); break;
        case RendererApi::Vulkan: break; // @TODO
        } // switch
    } // render

    template <typename Derived>
    inline void Window<Derived>::SwapBuffers() const noexcept {
        switch (api())
        {
        case RendererApi::Software: fb.SwapBuffers(*this); break;
        case RendererApi::Opengl: g.SwapBuffers(*this); break;
        case RendererApi::Vulkan:   break; // @TODO
        default: break;
        } // switch
    } // render

    template <typename Derived>
    void Window<Derived>::setApi(RendererApi api) noexcept {
        if (api == _renderer_api) return;

        // Destroy old renderer
        switch (_renderer_api) {
        case RendererApi::Software: fb.~Framebuffer(); break;
        case RendererApi::Opengl: g.~Opengl(); break;
        case RendererApi::Vulkan: break; // @TODO
        }

        _renderer_api = api;
        HDC hdc = reinterpret_cast<HDC>(native().getHdc());
        AboutError about = AboutError::None;
        Error err;

        switch (_renderer_api) {
        case RendererApi::Software:
            err = Error::WindowFramebuffer;
            if (!fb.Recreate(*this, _width, _height, about))
                onError(err, about);
            break;
        case RendererApi::Opengl:
            err = Error::Opengl;
            about = g.CreateContext(*this);
            if (about != AboutError::None)
                onError(err, about);
            else
                gl::Viewport(0, 0, _width, _height);
            break;
        case RendererApi::Vulkan: break; // @TODO
        default:
            break;
        } // switch
    } // set_api
} // namespace hi