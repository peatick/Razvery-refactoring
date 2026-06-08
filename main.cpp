#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    const char* fontPath = (argc > 1) ? argv[1] : "";
	EventHandler handler;
    Renderer renderer;
    if (!renderer.init(fontPath)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Init: %s | %s", SDL_GetError(), TTF_GetError());
        return 1;
    }
    Widget_Editor w_ed;
    w_ed.widget_editor.set_init({ 200, 20, 600, 780 }, "Hello, SDL2!", renderer.lineH);
    w_ed.widget_rect = w_ed.widget_editor.TX_Rect;
    w_ed.widget_layer = 1;
    Widget_File_explorer w_explorer;
    w_explorer.explorer.init(renderer.lineH);
    renderer.fs_texture_init(w_explorer.explorer);
    w_explorer.widget_rect = w_explorer.explorer.size;
    w_explorer.widget_layer = 1;
	WidgetManager w_mgr;
	w_mgr.addWidget(w_ed);
	w_mgr.addWidget(w_explorer);
	renderer.init_icon_tex();

	w_mgr.ui_btns.add_btn("File", { 0, 0, 70, 20 },"menu_group", true, true);
    w_mgr.ui_btns.add_btn("Edit", { 70, 0, 70, 20 }, "menu_group", true, true);
    w_mgr.ui_btns.add_btn("View", { 140, 0, 70, 20 }, "menu_group", true, true);
    w_mgr.ui_btns.add_btn("TextEditor", { 140, 20, 70, 20 }, "View", true, true);

    int mx = 0;
    int my = 0;
	int mousex = 0, mousey = 0;
    bool running = true, mouseDown = false;
    handler.rend = &renderer;
    handler.mb = &mouseDown;
	SDL_Point now_mouse_P = { mousex, mousey };
	handler.nmP = &now_mouse_P;
    while (running) {
        SDL_Event e;
        handler.ev = &e;
        while (SDL_PollEvent(&e)) {
            mousex = e.button.x;
            mousey = e.button.y;
            switch (e.type) {
            case SDL_QUIT: running = false; break;
            }
			SDL_Point mouse_P = { mx, my };
            handler.mP = &mouse_P;
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                mx = e.button.x;
                my = e.button.y;
            }
			mouseDown = (e.type == SDL_MOUSEBUTTONDOWN) ? true : (e.type == SDL_MOUSEBUTTONUP) ? false : mouseDown;
			renderer.mouse_logical_pos(mousex, mousey);
            now_mouse_P = { mousex, mousey };
			handler.Btnui_w_t(w_mgr);
			handler.FileExplorer_w_t(w_explorer, w_mgr);
			handler.textEditEvent_w_t(w_ed, w_mgr);
        }
        w_mgr.btn_order_cls();
        renderer.draw_bg({250,250,250,255});
        renderer.TextBox(w_ed.widget_editor);
        renderer.drw_file_explorer(w_explorer.explorer);

		w_mgr.ui_btns.imitate_btn("File");
		w_mgr.ui_btns.imitate_btn("Edit");
        if (w_mgr.ui_btns.imitate_btn("View")) {
			// "View" ボタンがクリックされたときの処理をここに追加
			w_mgr.ui_btns.imitate_btn("TextEditor");
        }

        renderer.drw_all_buttons(w_mgr.ui_btns);
        w_explorer.explorer.tickupdate();
        renderer.rend();
    }
	renderer.destroy_all_buttons(w_mgr.ui_btns);
    renderer.destroy();
    return 0;
}