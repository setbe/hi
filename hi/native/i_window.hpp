#pragma once
#include "types.hpp"

namespace hi {

    namespace native {
        // -- Forward declarations ---
        struct Framebuffer; // Native Software Context
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
        IO_NODISCARD virtual       native::Framebuffer& framebuffer()       noexcept = 0;
        IO_NODISCARD virtual const native::Framebuffer& framebuffer() const noexcept = 0;
        IO_NODISCARD virtual       native::Opengl& opengl()        noexcept = 0;
        IO_NODISCARD virtual const native::Opengl& opengl()  const noexcept = 0;
        IO_NODISCARD virtual       native::Window& native()       noexcept = 0;
        IO_NODISCARD virtual const native::Window& native() const noexcept = 0;
        IO_NODISCARD virtual int width() const noexcept = 0;
        IO_NODISCARD virtual int height() const noexcept = 0;
    }; // IWindow
} // namespace hi