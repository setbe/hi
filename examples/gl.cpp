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

    MainWindow() noexcept {
        this->setTitle(IO_U8("Hello World! Привіт Світе!"));
        this->setGlCore(3, 3);
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
        td.atlas = 0;
        td.scale = text_scale;
        td.x = move_offset_x;
        td.y = move_offset_y;
        td.text = IO_U8("AHello World!\nПривіт Світе!");
        this->DrawText(td);
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
        io::out.reset();
        io::out << "Current scale is " << text_scale << " times";
        setTitle(io::out.scrap_view());
    }

    hi::AtlasId create_atlas() noexcept {
        io::string cwd, font_path;
        if (!fs::current_directory(cwd)
            || !fs::path_join(cwd, DEFAULT_FONT_FILENAME, font_path))
            return -1;

        io::out << "current dir is: " << cwd << '\n'
                << "searching for font: " << font_path << io::out.endl;
        hi::FontId font_id = this->LoadFont(DEFAULT_FONT_FILENAME);
        if (font_id < 0) return -1;

        hi::FontAtlasDesc desc{};
        desc.mode = hi::FontAtlasMode::SDF;
        desc.pixel_height = 64;
        desc.spread_px = 4.f;
        io::u32 cps[]{ 'A', 'H', 'e', 'l', 'o', 'W', 'r', 'd', '!', L'П', L'р', L'и', L'в', L'і', L'т', L'С', L'е'};
        desc.codepoints = cps;
        desc.codepoint_count = sizeof(cps);
        hi::AtlasId atlas_id = this->GenerateFontAtlas(font_id, desc);
        io::u16 atlas_side = this->getAtlasSide(atlas_id);
        io::out << "Generated atlas has " << atlas_side << 'x' << atlas_side << " image resolution" << io::out.endl;
        return atlas_id;
    }
}; // struct MainWindow

int main() {
    MainWindow win{};
    if (win.create_atlas() < 0) io::exit_process(-1);

    while (win.PollEvents()) {
        win.Render();
        win.FlushText();
        win.SwapBuffers();
    }
    io::exit_process(0);
}