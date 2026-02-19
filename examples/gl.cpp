#define IO_IMPLEMENTATION
#include "../hi/hi.hpp"

struct MainWindow : public hi::Window<MainWindow> {
    MainWindow() noexcept {
        this->setTitle(IO_U8("Hello World! Привіт Світе!"));
        this->setApi(hi::RendererApi::Opengl, 2, 0);
    }

    void onRender() noexcept override {
        using io::u64;
        static float red = 0.;
        red += .0001f;
        if (red > 1.f) red = 0.f;

        gl::ClearColor(red, 0.f, 0.f, 0.f);
        gl::Clear(gl::buffer_bit.Color | gl::buffer_bit.Depth);
    }

    void onError(hi::Error err, hi::AboutError ae) noexcept override {
        setTitle(hi::what(ae));
    }
}; // struct MainWindow

int main() {
    MainWindow win;

    while (win.PollEvents()) {
        win.Render();
        win.SwapBuffers();
    }
    io::exit_process(0);
}