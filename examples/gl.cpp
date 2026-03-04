#define IO_IMPLEMENTATION
#include "../hi/hi.hpp"
#include "../3rd_party/stb_truetype_stream/codepoints/stbtt_codepoints_stream.hpp"

static const char* FONT_DIR{ "../resources/noto/" };
static const char* FONT_FILENAME_WORLD{ "NotoSans-Regular.ttf" };
static const char* FONT_FILENAME_JAPANESE{ "NotoSansJP-Regular.ttf" };
static const char* FONT_FILENAME_KOREAN{ "NotoSansKR-Regular.ttf" };
static const char* FONT_FILENAME_TRADITIONAL_CHINESE{ "NotoSansTC-Regular.ttf" };
static const char* FONT_FILENAME_SIMPLIFIED_CHINESE{ "NotoSansSC-Regular.ttf" };

struct MainWindow : public hi::Window<MainWindow> {
    float text_scale{ 1.f };
    float move_offset_x{ 10.f };
    float move_offset_y{ 5.f };

    // mouse
    float mouse_x = 0.f;
    float mouse_y = 0.f;
    bool  mouse_down = false;
    bool  prev_mouse_down = false;

    bool is_cursor{ true };
    bool is_fullscreen{ false };

    hi::AtlasId world_atlas{ -1 };
    hi::AtlasId jp_atlas{ -1 };
    hi::AtlasId kr_atlas{ -1 };
    hi::AtlasId tc_atlas{ -1 };
    hi::AtlasId sc_atlas{ -1 };

    MainWindow() noexcept {
        this->setTitle(IO_U8("Hello World! Привіт Світе!"));
    }

    void onError(hi::Error err, hi::AboutError ae) noexcept override {
        setTitle(hi::what(ae));
    }

    void onKeyDown(hi::Key k) noexcept override {
        using K = hi::Key;
        switch (k) {
        case K::_1: is_cursor = !is_cursor;         setCursorVisible(is_cursor);  break;
        case K::_2: is_fullscreen = !is_fullscreen; setFullscreen(is_fullscreen); break;

        case K::Right: move_offset_x += text_scale; break;
        case K::Left:  move_offset_x -= text_scale; break;
        case K::Down:  move_offset_y += text_scale; break;
        case K::Up:    move_offset_y -= text_scale; break;

        case K::MouseRight:
        case K::MouseLeft: mouse_down = true; break;
        }
    }

    void onKeyUp(hi::Key k) noexcept override {
        using K = hi::Key;
        switch (k) {
        case K::_1: is_cursor = !is_cursor;         setCursorVisible(is_cursor);  break;
        case K::_2: is_fullscreen = !is_fullscreen; setFullscreen(is_fullscreen); break;

        case K::Right: move_offset_x += text_scale; break;
        case K::Left:  move_offset_x -= text_scale; break;
        case K::Down:  move_offset_y += text_scale; break;
        case K::Up:    move_offset_y -= text_scale; break;

        case K::MouseRight:
        case K::MouseLeft: mouse_down = false; break;
        }
    }

    void onMouseMove(int x, int y) noexcept override {
        mouse_x = (float)x;
        mouse_y = (float)y;
    }

    void onScroll(float deltaX, float deltaY) noexcept override {
        text_scale += deltaX;
        text_scale += deltaY;
        io::out.reset();
        io::out << "Current scale is " << text_scale << " times";
        setTitle(io::out.scrap_view());
    }

    void onRender() noexcept override {
        static float red = 0.f;
        red += .0001f;
        if (red > 1.f) red = 0.f;
        gl::ClearColor(red, 0.f, 0.f, 0.f);
        gl::Clear(gl::buffer_bit.Color | gl::buffer_bit.Depth);


        const bool mouse_released = (prev_mouse_down && !mouse_down);

        hi::ButtonDraw btn{};
        btn.atlas = world_atlas;
        btn.dock = hi::TextDock::TopC;
        btn.x = 0.f; btn.y = 150.f;
        btn.scale = text_scale;
        btn.text = IO_U8("Click me");
        btn.style.normal = hi::TextStyle{1.f,  1.f, 1.f,  1.f, false };
        btn.style.hover = hi::TextStyle{ 1.f,  1.f, 0.7f, 1.f, true, 0.f, 0.f, 0.f, 1.f, /*.outline_px*/1.2f, /*.softness_px*/0.9f };
        btn.style.active = hi::TextStyle{0.7f, 1.f, 0.7f, 1.f, true, 0.f, 0.f, 0.f, 1.f, /*.outline_px*/2.0f, /*.softness_px*/0.9f };

        auto st = Button(btn, mouse_x, mouse_y, mouse_down, mouse_released);
        if (st.clicked) {
            setTitle(IO_U8("Clicked!"));
        }
        

        hi::TextStyle ts{};
        ts.r = red;
        ts.b = red;
        ts.softness_px = 0.f;
        ts.outline = true;
        ts.outline_px = text_scale * 0.05f;

        hi::TextDraw td{};
        td.style = ts;

        td.scale = text_scale;
        td.space_between = -0.18f;

        const float x0 = move_offset_x;
        float y = move_offset_y;

        // 1) WORLD: Latin + Cyrillic (UI / mixed text)
        td.atlas = world_atlas;

        /*td.x = x0; td.y = y;
        td.text = IO_U8("[WORLD] Hello World!  Привіт Світе!  Greek? (won't)  عربى? (won't)");
        this->DrawText(td);*/

        td.x = td.y = 0.f;
        td.dock = hi::TextDock::TopL;    td.text = "[top-left]"; this->DrawText(td);
        td.dock = hi::TextDock::TopC;    td.text = "[top-center]"; this->DrawText(td);
        td.dock = hi::TextDock::TopR;    td.text = "[top-right]"; this->DrawText(td);
        td.dock = hi::TextDock::LeftC;   td.text = "[left-center]"; this->DrawText(td);
        td.dock = hi::TextDock::RightC;  td.text = "[right-center]"; this->DrawText(td);
        td.dock = hi::TextDock::BottomL; td.text = "[bottom-left]"; this->DrawText(td);
        td.dock = hi::TextDock::BottomC; td.text = "[bottom-center]"; this->DrawText(td);
        td.dock = hi::TextDock::BottomR; td.text = "[bottom-right]"; this->DrawText(td);

        td.dock = hi::TextDock::TopL;

        // td.x = x0; td.y = y;
        //y += 90.f;

        //// 2) Han-string with the same glyphs, but with different atlases JP/SC/TC/KR.
        //io::char_view han_line = IO_U8("漢字對照: 直 骨 令 青 海 國 龍 風 體");

        //td.x = x0; td.y = y;

        //io::out.reset();

        //td.atlas = jp_atlas;
        //io::out << "[JP atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 70.f;

        //io::out.reset();

        //td.atlas = sc_atlas;
        //td.x = x0; td.y = y;
        //io::out << "[SC atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 70.f;

        //io::out.reset();

        //td.atlas = tc_atlas;
        //td.x = x0; td.y = y;
        //io::out << "[TC atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 70.f;

        //io::out.reset();

        //td.atlas = kr_atlas;
        //td.x = x0; td.y = y;
        //io::out << "[KR atlas]" << han_line;
        //td.text = io::out.scrap_view();
        //this->DrawText(td);
        //y += 90.f;

        //// 3) Lang examples (to ensure Kana/Hangul works)
        //td.atlas = jp_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[JP] 日本語テスト: こんにちは世界  カタカナ: アイウエオ  漢字: 東京 大学 日本");
        //this->DrawText(td);
        //y += 70.f;

        //td.atlas = kr_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[KR] 한국어 테스트: 안녕하세요 세계  한글 + 漢字 혼용: 國語 漢字");
        //this->DrawText(td);
        //y += 70.f;

        //td.atlas = sc_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[SC] 你好，世界  简体中文示例: 汉字 语言 共和国 龙 风 体");
        //this->DrawText(td);
        //y += 70.f;

        //td.atlas = tc_atlas;
        //td.x = x0; td.y = y;
        //td.text = IO_U8("[TC] 你好，世界  繁體中文示例: 漢字 語言 共和國 龍 風 體");
        //this->DrawText(td);

        this->FlushText();

        prev_mouse_down = mouse_down;
    }

    bool LoadResources() noexcept {
        hi::FontAtlasDesc desc{};
        desc.mode = hi::FontAtlasMode::SDF;
        desc.pixel_height = 48;
        desc.spread_px = 4.f;

        io::string full_font_path;
        {
            io::string cwd;
            if (!fs::current_directory(cwd) || !fs::path_join(cwd, FONT_DIR, full_font_path)) return false;
            io::out << "current dir is: " << cwd << '\n'
                << "searching in directory: " << full_font_path << io::out.endl;
        }

        io::string current_font;

        auto load_atlas = [&](const char* filename, hi::AtlasId& out_atlas, auto... scripts) -> bool {
            fs::path_join(full_font_path, filename, current_font);
            hi::FontId font_id = this->LoadFont(current_font.as_view());
            if (font_id < 0) return false;
            out_atlas = this->GenerateFontAtlas(font_id, desc, scripts...);
            return out_atlas >= 0;
        };

        // WORLD: Latin/Cyrillic/Greek/Arabic/...
        if (!load_atlas(FONT_FILENAME_WORLD, world_atlas,
            stbtt_codepoints::Script::Latin,
            stbtt_codepoints::Script::Cyrillic)) return false;
        io::out << "WORLD atlas side: " << getAtlasSide(world_atlas) << io::out.endl;

        //// JP
        //if (!load_atlas(FONT_FILENAME_JAPANESE, jp_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::Kana,
        //    stbtt_codepoints::Script::JouyouKanji,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "JP atlas side: " << getAtlasSide(jp_atlas) << io::out.endl;

        //// KR
        //// TODO: Script::Hangul
        //if (!load_atlas(FONT_FILENAME_KOREAN, kr_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "KR atlas side: " << getAtlasSide(kr_atlas) << io::out.endl;

        //// TC
        //if (!load_atlas(FONT_FILENAME_TRADITIONAL_CHINESE, tc_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "TC atlas side: " << getAtlasSide(tc_atlas) << io::out.endl;

        //// SC
        //if (!load_atlas(FONT_FILENAME_SIMPLIFIED_CHINESE, sc_atlas,
        //    stbtt_codepoints::Script::Latin,
        //    stbtt_codepoints::Script::CJK)) return false;
        //io::out << "SC atlas side: " << getAtlasSide(sc_atlas) << io::out.endl;

        return true;
    }
}; // struct MainWindow

int main() {
    MainWindow win{};
    if (!win.LoadResources()) io::exit_process(-1);

    while (win.PollEvents()) {
        win.Render();
        win.SwapBuffers();
    }
    io::exit_process(0);
}