#pragma once
#include "sdlutil.h"
#include "Renderer.h"
class EventHandler {
public:
    SDL_Event* ev = nullptr;
    Renderer* rend = nullptr;
    bool* mb = nullptr;
    SDL_Point* mP = nullptr;
    SDL_Point* nmP = nullptr;
    bool nl_check(){
        if(ev == nullptr || rend == nullptr || mb == nullptr || mP == nullptr || nmP == nullptr){
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
    bool L_clicks(int c) {
        if (!nl_check()) {
            return false;
        }
        SDL_Event& e = *ev;
        return e.type == SDL_MOUSEBUTTONDOWN && e.button.clicks == c && e.button.button == SDL_BUTTON_LEFT;
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
        if (select) {
            ed.tickBlink();
        }
    }
    void textEditEvent_sh(Editor& ed, bool handler)
    {
        if(!nl_check()){
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
            f.path_set("");
            break;
        }
        int start_y = f.size.y + 30;
        int viewRow = (f.size.h - f.under_box.h - 30) / 20;
        SDL_Rect view_rect = { f.size.x, start_y, f.size.w, viewRow * 20 };
        if (SDL_PointInRect(&nm_P, &view_rect)) {
            SDL_Rect click_rect;
            if (L_click()) {
                for (int i = 0; i < f.file_list.size(); i++) {
                    if (viewRow - 1 < i) break;
                    if (!(i + f.scrollrow < f.file_list.size())) break;
                    click_rect = { f.size.x, start_y + i * 20, f.size.w, 20 };
                    if(SDL_PointInRect(&nm_P, &click_rect)){
						for (auto& file : f.file_list) {
							file.selected = false;
						}
                        f.file_list[i + f.scrollrow].selected = true;
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
            }
        }
    }
    void textEditEvent_w(SDL_Event& e, Widget& w, Renderer& renderer, bool& mouseDown, SDL_Point mouse_P, WidgetManager& w_mgr)
    {
        bool select = w_mgr.Widget_event(mouse_P,true) == w.widget_layer;
        if (!select) return;
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
    void textEditEvent_w_t(Widget& w, WidgetManager& w_mgr)
    {
        if(!nl_check()){
           return; 
        }
        SDL_Event& e = *ev;
        Renderer& renderer = *rend;
        bool& mouseDown = *mb;
        SDL_Point& mouse_P = *mP;
        bool select = w_mgr.Widget_event(mouse_P,true) == w.widget_layer;
        if (!select) return;
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
};