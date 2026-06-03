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
	Widget w;
    File_explorer f_e;
    f_e.init(renderer.lineH);
    renderer.fs_texture_init(f_e);
	w.widget_editor.set_init({ 10, 20, 600, 400 }, "Hello, SDL2!", renderer.lineH);
	w.widget_rect = { 10, 20, 600, 400 };
    w.widget_layer = 1;
	WidgetManager w_mgr;
	w_mgr.addWidget(w);
	renderer.init_icon_tex();
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
            handler.textEditEvent_sh(f_e.path_box_ed,true);
            handler.File_explorer_Event(f_e);
        }
        renderer.draw_bg({250,250,250,255});
        renderer.TextBox(w.widget_editor);
        renderer.drw_file_explorer(f_e);
        f_e.tickupdate();
        renderer.rend();
    }
    renderer.destroy();
    return 0;
}