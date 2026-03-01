#define IO_IMPLEMENTATION
#include "../hi/hi.hpp"
#include "../3rd_party/stb_truetype_stream/codepoints/stbtt_codepoints_stream.hpp"

static const char* DEFAULT_FONT_FILENAME{ "FreeSansBold.ttf" };

struct MainWindow : public hi::Window<MainWindow> {
    float text_scale{ 1.f };
    float move_offset_x{ 10.f };
    float move_offset_y{ 5.f };

    bool is_cursor{ true };
    bool is_fullscreen{ false };

    hi::AtlasId diversity{ -1 };
    hi::AtlasId cjk{ -1 };

    MainWindow() noexcept {
        this->setTitle(IO_U8("Hello World! Привіт Світе!"));
    }

    void onRender() noexcept override {
        using io::u64;
        static float red = 0.;
        red += .0001f;
        if (red > 1.f) red = 0.f;

        gl::ClearColor(red, 0.f, 0.f, 0.f);
        gl::Clear(gl::buffer_bit.Color | gl::buffer_bit.Depth);

        hi::TextStyle ts{};
        ts.r = red;
        ts.b = red;
        ts.softness_px = .0f;
        ts.outline = true;
        ts.outline_px = text_scale * 0.05f;
        hi::TextDraw td{};
        td.style = ts;
        td.atlas = diversity;
        td.scale = text_scale;
        td.x = move_offset_x;
        td.y = move_offset_y;
        
        td.space_between = -0.18f;
        td.tab_width = 4.f;
        td.text = IO_U8("AHello World!\nПривіт Світе!\n\tThis line has tab");
        this->DrawText(td);

        this->FlushText();
        this->SwapBuffers();
    }

    void onError(hi::Error err, hi::AboutError ae) noexcept override {
        setTitle(hi::what(ae));
    }

    void onKeyDown(hi::Key k) noexcept override {
        using K = hi::Key;
        switch (k) {
            case K::_1: is_cursor     =! is_cursor;     setCursorVisible(is_cursor);  break;
            case K::_2: is_fullscreen =! is_fullscreen; setFullscreen(is_fullscreen); break;

            case K::Right: move_offset_x += text_scale; break;
            case K::Left:  move_offset_x -= text_scale; break;
            case K::Down:  move_offset_y += text_scale; break;
            case K::Up:    move_offset_y -= text_scale; break;
        }
    }

    void onScroll(float deltaX, float deltaY) noexcept override {
        text_scale += deltaX;
        text_scale += deltaY;
        io::out.reset();
        io::out << "Current scale is " << text_scale << " times";
        setTitle(io::out.scrap_view());
    }

    bool LoadResources() noexcept {
        io::string cwd, font_path;
        if (!fs::current_directory(cwd)
            || !fs::path_join(cwd, DEFAULT_FONT_FILENAME, font_path))
            return false;

        io::out << "current dir is: " << cwd << '\n'
                << "searching for font: " << font_path << io::out.endl;
        hi::FontId font_id = this->LoadFont(DEFAULT_FONT_FILENAME);
        if (font_id < 0) return false;

        hi::FontAtlasDesc desc{};
        desc.mode = hi::FontAtlasMode::SDF;
        desc.pixel_height = 24;
        desc.spread_px = 4.f;

        diversity = this->GenerateFontAtlas(font_id, desc, stbtt_codepoints::Script::Latin, stbtt_codepoints::Script::Cyrillic);
        if (diversity < 0) return false;

        io::out << "diversity: " << getAtlasSide(diversity) << io::out.endl;
        return true;
    }
}; // struct MainWindow

int main() {
    MainWindow win{};
    if (!win.LoadResources()) io::exit_process(-1);

    while (win.PollEvents()) {
        win.Render();
        
    }
    io::exit_process(0);
}