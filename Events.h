#pragma once
#include "sdlutil.h"
#include "Renderer.h"
class EventHandler {
public:
    SDL_Event* ev = nullptr;
    Renderer* rend = nullptr;
    bool* mb = nullptr;
    bool* L_MDown = nullptr;
    SDL_Point* mP = nullptr;
    SDL_Point* nmP = nullptr;
    bool nl_check() {
        if (ev == nullptr || rend == nullptr || mb == nullptr || mP == nullptr || nmP == nullptr || L_MDown == nullptr) {
            return false;
        }
        return true;
    }
    bool L_click() {
        if (!nl_check()) {
            return false;
        }
        SDL_Event& e = *ev;
        return e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;
    }
    bool MD_click() {
        if (!nl_check()) {
            return false;
        }
        SDL_Event& e = *ev;
        return e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_MIDDLE;
    }
    bool L_clicks(int c) {
        if (!nl_check()) {
            return false;
        }
        SDL_Event& e = *ev;
        return e.type == SDL_MOUSEBUTTONDOWN && e.button.clicks == c && e.button.button == SDL_BUTTON_LEFT;
    }
    bool R_click() {
        if (!nl_check()) {
            return false;
        }
        SDL_Event& e = *ev;
        return e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT;
    }
    bool save_btn() {
        if (ev->type == SDL_KEYDOWN) {
            if (SDL_GetModState() && KMOD_CTRL) {
                if (ev->key.keysym.sym == SDLK_s) {
                    return true;
                }
            }
        }
        return false;
    }
    bool Widget_ev(Widget w, WidgetManager w_mgr) {
        if (!nl_check()) {
            return false;
        }
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        bool select = w_mgr.Widget_event(mouse_P, true) == w.widget_layer;
        return select;
    }
    void textEditEvent(SDL_Event& e, Editor& ed, Renderer& renderer, bool& mouseDown, SDL_Point mouse_P, bool handler)
    {
        bool select = SDL_PointInRect(&mouse_P, &ed.TX_Rect) && handler;
        int nm_x = e.button.x;
        int nm_y = e.button.y;
        switch (e.type) {
        case SDL_KEYDOWN: {
            if (!select) {
                break;
            }
            int key = e.key.keysym.sym, mod = e.key.keysym.mod;
            bool ctrl = (mod & KMOD_CTRL) != 0;
            bool shift = (mod & KMOD_SHIFT) != 0;
            ed.resetBlink();

            if (ctrl) {
                switch (key) {
                case SDLK_a: ed.selectAll(); break;
                case SDLK_c: ed.copy(); break;
                case SDLK_x: ed.cut(); break;
                case SDLK_v: ed.paste(); break;
                case SDLK_z: shift ? ed.doRedo() : ed.doUndo(); break;
                case SDLK_y: ed.doRedo(); break;
                case SDLK_LEFT:  ed.moveLeft(shift, true); break;
                case SDLK_RIGHT: ed.moveRight(shift, true); break;
                case SDLK_HOME:  ed.moveHome(shift, true); break;
                case SDLK_END:   ed.moveEnd(shift, true); break;
                }
            }
            else {
                switch (key) {
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (ed.imeComposing.empty()) ed.insertNewline();
                    break;
                case SDLK_BACKSPACE:
                    if (ed.imeComposing.empty()) ed.backspace();
                    break;
                case SDLK_DELETE:
                    if (ed.imeComposing.empty()) ed.deleteForward();
                    break;
                case SDLK_LEFT:  ed.moveLeft(shift, false); break;
                case SDLK_RIGHT: ed.moveRight(shift, false); break;
                case SDLK_UP:    ed.moveUp(shift); break;
                case SDLK_DOWN:  ed.moveDown(shift); break;
                case SDLK_HOME:  ed.moveHome(shift, false); break;
                case SDLK_END:   ed.moveEnd(shift, false); break;
                }
            }
            break;
        }
        case SDL_TEXTEDITING:
            if (!select) {
                break;
            }
            ed.imeComposing = e.edit.text;
            ed.imeCursor = e.edit.start;
            break;
        case SDL_TEXTINPUT:
            if (!select) {
                break;
            }
            ed.imeComposing.clear();
            ed.imeCursor = 0;
            ed.insertText(e.text.text);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (!select) {
                break;
            }
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouseDown = true;
                auto p = ed.hitTest(e.button.x, e.button.y, renderer.getFont());
                if (e.button.clicks == 2) {
                    ed.cursor = p;
                    ed.hasSelection = true;
                    ed.selAnchor = p;
                    ed.selAnchor.col = utf8::prevWord(ed.buf.line(p.row), p.col);
                    ed.cursor.col = utf8::nextWord(ed.buf.line(p.row), p.col);
                }
                else {
                    bool sh = (SDL_GetModState() & KMOD_SHIFT) != 0;
                    if (sh) ed.startSelection(); else ed.clearSelection();
                    ed.cursor = p;
                    if (!sh) ed.selAnchor = p;
                    ed.resetBlink();
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (!select) {
                break;
            }
            if (e.button.button == SDL_BUTTON_LEFT)
                mouseDown = false;
            break;

        case SDL_MOUSEMOTION:
            if (!select) {
                break;
            }
            if (mouseDown) {
                ed.hasSelection = true;
                ed.cursor = ed.hitTest(e.motion.x, e.motion.y, renderer.getFont());
            }
            break;

        case SDL_MOUSEWHEEL:
            if (!select) {
                break;
            }
            ed.scrollRow = std::clamp(ed.scrollRow - e.wheel.y, 0, ed.buf.numLines() - 1);
            break;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                ed.viewRows = (e.window.data2 - ed.PADDING * 2) / renderer.lineH;
            break;
        }
    }
    void textEditEvent_sh(Editor& ed, bool handler)
    {
        if (!nl_check()) {
            return;
        }
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        bool select = SDL_PointInRect(&mouse_P, &ed.TX_Rect) && handler;
        int nm_x = e.button.x;
        int nm_y = e.button.y;

        switch (e.type) {
        case SDL_KEYDOWN: {
            if (!select) {
                break;
            }
            int key = e.key.keysym.sym, mod = e.key.keysym.mod;
            bool ctrl = (mod & KMOD_CTRL) != 0;
            bool shift = (mod & KMOD_SHIFT) != 0;
            ed.resetBlink();

            if (ctrl) {
                switch (key) {
                case SDLK_a: ed.selectAll(); break;
                case SDLK_c: ed.copy(); break;
                case SDLK_x: ed.cut(); break;
                case SDLK_v: ed.paste(); break;
                case SDLK_LEFT:  ed.moveLeft(shift, true); break;
                case SDLK_RIGHT: ed.moveRight(shift, true); break;
                case SDLK_HOME:  ed.moveHome(shift, true); break;
                case SDLK_END:   ed.moveEnd(shift, true); break;
                }
            }
            else {
                switch (key) {
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    break;
                case SDLK_BACKSPACE:
                    if (ed.imeComposing.empty()) ed.backspace();
                    break;
                case SDLK_DELETE:
                    if (ed.imeComposing.empty()) ed.deleteForward();
                    break;
                case SDLK_LEFT:  ed.moveLeft(shift, false); break;
                case SDLK_RIGHT: ed.moveRight(shift, false); break;
                case SDLK_UP:    ed.moveUp(shift); break;
                case SDLK_DOWN:  ed.moveDown(shift); break;
                case SDLK_HOME:  ed.moveHome(shift, false); break;
                case SDLK_END:   ed.moveEnd(shift, false); break;
                }
            }
            break;
        }
        case SDL_TEXTEDITING:
            if (!select) {
                break;
            }
            ed.imeComposing = e.edit.text;
            ed.imeCursor = e.edit.start;
            break;
        case SDL_TEXTINPUT:
            if (!select) {
                break;
            }
            ed.imeComposing.clear();
            ed.imeCursor = 0;
            ed.insertText(e.text.text);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (!select) {
                break;
            }
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouseDown = true;
                auto p = ed.hitTest(e.button.x, e.button.y, renderer.getFont_sml());
                if (e.button.clicks == 2) {
                    ed.cursor = p;
                    ed.hasSelection = true;
                    ed.selAnchor = p;
                    ed.selAnchor.col = utf8::prevWord(ed.buf.line(p.row), p.col);
                    ed.cursor.col = utf8::nextWord(ed.buf.line(p.row), p.col);
                }
                else {
                    bool sh = (SDL_GetModState() & KMOD_SHIFT) != 0;
                    if (sh) ed.startSelection(); else ed.clearSelection();
                    ed.cursor = p;
                    if (!sh) ed.selAnchor = p;
                    ed.resetBlink();
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (!select) {
                break;
            }
            if (e.button.button == SDL_BUTTON_LEFT)
                mouseDown = false;
            break;

        case SDL_MOUSEMOTION:
            if (!select) {
                break;
            }
            if (mouseDown) {
                ed.hasSelection = true;
                ed.cursor = ed.hitTest(e.motion.x, e.motion.y, renderer.getFont_sml());
            }
            break;

        case SDL_MOUSEWHEEL:
            if (!select) {
                break;
            }
            ed.scrollRow = std::clamp(ed.scrollRow - e.wheel.y, 0, ed.buf.numLines() - 1);
            break;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                ed.viewRows = (e.window.data2 - ed.PADDING * 2) / renderer.lineH;
            break;
        }
        if (select) {
            ed.tickBlink();
        }
    }
    void File_explorer_Event(File_explorer& f) {
        if (!nl_check()) return;
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        SDL_Point& nm_P = *nmP;
        bool hover = SDL_PointInRect(&nm_P, &f.size);
        textEditEvent_sh(f.path_box_ed, true);
        if (!hover) {
            if (L_click()) {
                for (auto& file : f.file_list) {
                    file.selected = false;
                }
            }
            return;
        }
        int nm_x = e.button.x;
        int nm_y = e.button.y;
        int key = e.key.keysym.sym, mod = e.key.keysym.mod;
        switch (e.type) {
        case SDL_MOUSEWHEEL:
            f.scrollrow = std::clamp(f.scrollrow - e.wheel.y, 0, int(f.file_list.size() - 1));
            break;
        }
        switch (key) {
        case SDLK_RETURN:
        case SDLK_KP_ENTER:
            if (!SDL_PointInRect(&mouse_P, &f.path_box_ed.TX_Rect)) break;
            f.path_set("");
            break;
        }
        int start_y = f.size.y + 30;
        int viewRow = (f.size.h - f.under_box.h - 30) / 20;
        SDL_Rect view_rect = { f.size.x, start_y, f.size.w, viewRow * 20 };
        SDL_Rect back_rect = { f.size.x, f.size.y, 40,f.path_box_ed.TX_Rect.h };
        if (SDL_PointInRect(&nm_P, &view_rect)) {
            SDL_Rect click_rect;

            if (L_click()) {
                for (int i = 0; i < f.file_list.size(); i++) {
                    if (viewRow - 1 < i) break;
                    if (!(i + f.scrollrow < f.file_list.size())) break;
                    click_rect = { f.size.x, start_y + i * 20, f.size.w, 20 };
                    if (SDL_PointInRect(&nm_P, &click_rect)) {
                        for (auto& file : f.file_list) {
                            file.selected = false;
                        }
                        f.file_list[i + f.scrollrow].selected = true;
                        if (L_clicks(2)) {
							f.selected_file_path = f.file_list[i + f.scrollrow].file_path;
                            f.selected_file_c2 = true;
                            if (fs::is_directory(f.file_list[i + f.scrollrow].file_path)){
                                std::u8string u8temp = f.file_list[i + f.scrollrow].file_path.u8string();
                                std::string str = std::string(reinterpret_cast<const char*>(u8temp.c_str()));
                                f.path_set(str);
                            }
                        }
                        break;
                    }
                }
            }
        }
        else {
            if (L_click()) {
                for (auto& file : f.file_list) {
                    file.selected = false;
                }
                if (SDL_PointInRect(&nm_P, &back_rect)) {
                    f.back_path();
                }
            }
        }
    }
    void textEditEvent_w_t(Widget_Editor& w, WidgetManager& w_mgr)
    {
        if (!nl_check()) {
            return;
        }
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        bool select = w_mgr.Widget_event(mouse_P, true) == w.widget_layer;
        if (!select) return;
        if (!SDL_PointInRect(&mouse_P, &w.widget_editor.TX_Rect)) return;
        Editor& ed = w.widget_editor;
        int nm_x = e.button.x;
        int nm_y = e.button.y;
        switch (e.type) {
        case SDL_KEYDOWN: {
            if (!select) {
                break;
            }
            int key = e.key.keysym.sym, mod = e.key.keysym.mod;
            bool ctrl = (mod & KMOD_CTRL) != 0;
            bool shift = (mod & KMOD_SHIFT) != 0;
            ed.resetBlink();

            if (ctrl) {
                switch (key) {
                case SDLK_a: ed.selectAll(); break;
                case SDLK_c: ed.copy(); break;
                case SDLK_x: ed.cut(); break;
                case SDLK_v: ed.paste(); break;
                case SDLK_z: shift ? ed.doRedo() : ed.doUndo(); break;
                case SDLK_y: ed.doRedo(); break;
                case SDLK_LEFT:  ed.moveLeft(shift, true); break;
                case SDLK_RIGHT: ed.moveRight(shift, true); break;
                case SDLK_HOME:  ed.moveHome(shift, true); break;
                case SDLK_END:   ed.moveEnd(shift, true); break;
                }
            }
            else {
                switch (key) {
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (ed.imeComposing.empty()) ed.insertNewline();
                    break;
                case SDLK_BACKSPACE:
                    if (ed.imeComposing.empty()) ed.backspace();
                    break;
                case SDLK_DELETE:
                    if (ed.imeComposing.empty()) ed.deleteForward();
                    break;
                case SDLK_LEFT:  ed.moveLeft(shift, false); break;
                case SDLK_RIGHT: ed.moveRight(shift, false); break;
                case SDLK_UP:    ed.moveUp(shift); break;
                case SDLK_DOWN:  ed.moveDown(shift); break;
                case SDLK_HOME:  ed.moveHome(shift, false); break;
                case SDLK_END:   ed.moveEnd(shift, false); break;
                }
            }
            break;
        }
        case SDL_TEXTEDITING:
            if (!select) {
                break;
            }
            ed.imeComposing = e.edit.text;
            ed.imeCursor = e.edit.start;
            break;
        case SDL_TEXTINPUT:
            if (!select) {
                break;
            }
            ed.imeComposing.clear();
            ed.imeCursor = 0;
            ed.insertText(e.text.text);
            break;
        case SDL_MOUSEBUTTONDOWN:
            if (!select) {
                break;
            }
            if (e.button.button == SDL_BUTTON_LEFT) {
                mouseDown = true;
                auto p = ed.hitTest(e.button.x, e.button.y, renderer.getFont());
                if (e.button.clicks == 2) {
                    ed.cursor = p;
                    ed.hasSelection = true;
                    ed.selAnchor = p;
                    ed.selAnchor.col = utf8::prevWord(ed.buf.line(p.row), p.col);
                    ed.cursor.col = utf8::nextWord(ed.buf.line(p.row), p.col);
                }
                else {
                    bool sh = (SDL_GetModState() & KMOD_SHIFT) != 0;
                    if (sh) ed.startSelection(); else ed.clearSelection();
                    ed.cursor = p;
                    if (!sh) ed.selAnchor = p;
                    ed.resetBlink();
                }
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if (!select) {
                break;
            }
            if (e.button.button == SDL_BUTTON_LEFT)
                mouseDown = false;
            break;

        case SDL_MOUSEMOTION:
            if (!select) {
                break;
            }
            if (mouseDown) {
                ed.hasSelection = true;
                ed.cursor = ed.hitTest(e.motion.x, e.motion.y, renderer.getFont());
            }
            break;

        case SDL_MOUSEWHEEL:
            if (!select) {
                break;
            }
            ed.scrollRow = std::clamp(ed.scrollRow - e.wheel.y, 0, ed.buf.numLines() - 1);
            break;

        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_RESIZED)
                ed.viewRows = (e.window.data2 - ed.PADDING * 2) / renderer.lineH;
            break;
        }
        if (select) {
            ed.tickBlink();
        }
    }
    void FileExplorer_w_t(Widget_File_explorer& w, WidgetManager& w_mgr) {
        if (!nl_check()) {
            return;
        }
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        bool select = w_mgr.Widget_event(mouse_P, true) == w.widget_layer;
        if (!select) return;
        if (!SDL_PointInRect(&mouse_P, &w.explorer.size)) return;
        File_explorer& f = w.explorer;
        textEditEvent_sh(f.path_box_ed, select);
        File_explorer_Event(f);
    }
    bool Btnui_w_t(WidgetManager& w_mgr) {
        if (!nl_check()) {
            return false;
        }
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        SDL_Point& nm_P = *nmP;
        btn_mgr& bm = w_mgr.ui_btns;

        bool btn_clicked = false;
        for (auto& btn_name : w_mgr.ui_btns.btn_order){
            Widget_button& b = bm.btns[btn_name];
            if (!SDL_PointInRect(&nm_P, &b.widget_rect)) {
                b.button.hovered = false;
                continue;
            }
            else {
                b.button.hovered = true;
            }
            if (L_click()) btn_clicked = true;
            if (!b.button.tgr) {
                b.button.clicked = L_click();
            }
            else {
                if (L_click()) {
                    b.button.clicked = !b.button.clicked;
                    if (b.button.radio && b.button.clicked) {
                        bm.group_off(b.button.group);
                        b.button.clicked = true;
                    }
                }
            }
        }
        if (L_click() && !btn_clicked) {
            for (auto& btn : bm.btns) {
                Widget_button& b = btn.second;
                b.button.clicked = false;
            }
        }
        return btn_clicked;
    }
    void textEditEvent_u(Editor& ed)
    {
        if (!nl_check()) {
            return;
        }
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        int nm_x = e.button.x;
        int nm_y = e.button.y;
        if (ed.searchMode) {
            SDL_Rect r = { ed.TX_Rect.x + ed.TX_Rect.w - 200,ed.TX_Rect.y, 200, 50 };
            if (SDL_PointInRect(nmP, &r)) return;
        }
        textEditEvent(e, ed, renderer, mouseDown, mouse_P, true);
        if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            int key = e.key.keysym.sym, mod = e.key.keysym.mod;
            bool ctrl = (mod & KMOD_CTRL) != 0;
            if (key == SDLK_f && ctrl) {
                ed.searchMode = !ed.searchMode;
                std::cout << ed.searchMode;
            }
        }
    }
    void SearchBox(Editor_Search& es, Editor* ed) {
        if (!nl_check()) {
            return;
        }
        if (!ed) return;
        Editor& edit = *ed;
        SDL_Point& mouse_P = *mP;
        SDL_Event e = *ev;
        if (!SDL_PointInRect(mP, &es.size)) return;
        textEditEvent_sh(es.Search_box, true);
        if (!SDL_PointInRect(mP, &es.Search_box.TX_Rect)) return;
        if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
            int key = e.key.keysym.sym;
            if (key == SDLK_RETURN) {
                if (es.search_str != es.Search_box.buf.line(0)) {
                    es.search_str = es.Search_box.buf.line(0);
                    es.search();
                }
                else {
                    es.cursor_move();
                }
            }
        }

    }
    void Slider_ev(Slider& s) {
        if (!nl_check()) return;
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        SDL_Point& mouse_nP = *nmP;

        int left_l = s.bar.x;
        int right_l = s.bar.x + s.bar.w - s.handL.w;
        s.handL.h = s.bar.h * 1.5;
        s.handL.w = 10;
        s.handL.y = int(s.bar.y + double(s.bar.h / 2)) - int(double(s.handL.h / 2));
        s.hover = SDL_PointInRect(nmP, &s.handL);
        switch (e.type) {
        case SDL_MOUSEBUTTONDOWN:
            if (s.hover) {
                s.clicked = true;
                s.Clickedx = mouse_nP.x - s.handL.x;
            }
            else {
                s.clicked = false;
            }
            break;
        case SDL_MOUSEMOTION:
            if (s.clicked) {
                s.handL.x = e.motion.x - s.Clickedx;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            s.clicked = false;
            break;

        }

        s.handL.x = std::clamp(s.handL.x, left_l, right_l);
        double vx = s.handL.x - s.bar.x;
        float val = s.v_min + (vx / (s.bar.w - s.handL.w)) * (s.v_max - s.v_min);
        s.now_val = int(val);

        if (s.clicked) s.box.buf.setAllText(std::to_string(s.now_val));

        textEditEvent_sh(s.box, true);
        if (SDL_PointInRect(&mouse_P, &s.box.TX_Rect)) {
            if (e.type == SDL_KEYDOWN && e.key.repeat == 0) {
                int key = e.key.keysym.sym;
                if (key == SDLK_RETURN) {
                    s.box.cursor = { 0, 0 };
                    try {
                        std::string t = s.box.buf.allText();
                        if (!t.empty())
                        {
                            int tmp = std::stoi(s.box.buf.allText());
                            tmp = std::clamp(tmp, s.v_min, s.v_max);
                            std::cout << tmp << std::endl;
                            s.now_val = tmp;
                            s.set(tmp);
                            s.box.buf.setAllText(std::to_string(tmp));
                        }
                        else {
                            s.box.buf.setAllText(std::to_string(s.v_min));
                        }
                    }
                    catch (const std::invalid_argument& evs) {
                        s.box.buf.setAllText(std::to_string(s.now_val));
                    }
                    catch (const std::out_of_range& evs) {
                        s.box.buf.setAllText(std::to_string(s.now_val));
                    }
                }
            }
        }

    }
    void drw_toolbar_ev(Drws_Toolbar& tb) {
        if (!nl_check()) return;
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_nP = *nmP;
        Slider_ev(tb.sl_r);
        Slider_ev(tb.sl_g);
        Slider_ev(tb.sl_b);
        Slider_ev(tb.sl_a);
        Slider_ev(tb.sl_size);
        tb.setColor();
        for (int i = 0; i < 8; i++) {
            if (SDL_PointInRect(nmP, &tb.pal[i].rect)) {
                if (L_click()) {
                    tb.now_color = tb.pal[i].color;
                    tb.sl_r.set(tb.now_color.r);
                    tb.sl_g.set(tb.now_color.g);
                    tb.sl_b.set(tb.now_color.b);
                    tb.sl_a.set(tb.now_color.a);
                }
                if (R_click()) {
                    tb.pal[i].color = tb.now_color;
                }
            }
        }
    }
    void paintApp_ev(PaintApp& app) {
        if (!nl_check()) return;
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        SDL_Point& nm_P = *nmP;
        app.handleEvent(e);
    }
};