#include "../hi/window.hpp"
#include "../hi/filesystem.hpp"
#include "../hi/native/containers.hpp"
#include "../hi/native/file.hpp"

struct MainWindow : public hi::Window<MainWindow> {
    MainWindow() noexcept {
        this->setTitle("My Window");
        this->setApi(hi::RendererApi::Opengl);
    }

    void onRender() noexcept override {
        static double red = 0.;
        static double now = io::monotonic_seconds();
        static double prev = now;
        prev = io::monotonic_seconds();

        red += prev - now;
        now = prev;
        if (red > 1.) red = 0.;

        gl::ClearColor(static_cast<float>(red), 0.f, 0.f, 0.f);
        gl::Clear(gl::BufferBit.Color | gl::BufferBit.Depth);
        this->SwapBuffers();
    }
}; // struct MainWindow

int main() {
    MainWindow win;

    while (win.PollEvents()) {
        win.Render();
    }
    io::exit_process(0);
}