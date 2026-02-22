#define IO_IMPLEMENTATION
#include "../hi/hi.hpp"

struct MainWindow : public hi::Window<MainWindow> {
    bool is_cursor{ true };
    bool is_fullscreen{ false };

    MainWindow() noexcept {
        this->setTitle(IO_U8("Hello World! Привіт Світе!"));
        this->setGlCore(2, 0);
    }

    void onRender() noexcept override {
        using io::u64;
        static float red = 0.;
        red += .001f;
        if (red > 1.f) red = 0.f;

        gl::ClearColor(red, 0.f, 0.f, 0.f);
        gl::Clear(gl::buffer_bit.Color | gl::buffer_bit.Depth);
    }

    void onError(hi::Error err, hi::AboutError ae) noexcept override {
        setTitle(hi::what(ae));
    }

    void onKeyDown(hi::Key k) noexcept override {
        if (k==hi::Key::_1) { is_cursor!=is_cursor; setCursorVisible(is_cursor); }
        if (k==hi::Key::_2) { is_fullscreen!=is_fullscreen; setFullscreen(is_fullscreen); }
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