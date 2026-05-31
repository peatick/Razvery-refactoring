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
	Widget w,w2;
	w.widget_editor.set_init({ 10, 20, 600, 400 }, "Hello, SDL2!", renderer.lineH);
	w.widget_rect = { 10, 20, 600, 400 };
    w.widget_layer = 1;
	w2.widget_editor.set_init({ 10, 120, 600, 400 }, "Hello, SDL2! (Second)", renderer.lineH);
	w2.widget_rect = { 10, 120, 600, 400 };
    w2.widget_layer = 2;
	WidgetManager w_mgr;
	w_mgr.addWidget(w);
	w_mgr.addWidget(w2);
	renderer.init_icon_tex();
    int mx = 0;
    int my = 0;
	int mousex = 0, mousey = 0;
    bool running = true, mouseDown = false;
    
    while (running) {
        SDL_Event e;
 
        while (SDL_PollEvent(&e)) {
            mousex = e.button.x;
            mousey = e.button.y;
            switch (e.type) {
            case SDL_QUIT: running = false; break;
            }
			SDL_Point mouse_P = { mx, my };
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                mx = e.button.x;
                my = e.button.y;
            }
			mouseDown = (e.type == SDL_MOUSEBUTTONDOWN) ? true : (e.type == SDL_MOUSEBUTTONUP) ? false : mouseDown;
			renderer.mouse_logical_pos(mousex, mousey);
			handler.textEditEvent_w(e, w, renderer, mouseDown, mouse_P, w_mgr);
            handler.textEditEvent_w(e, w2, renderer, mouseDown, mouse_P, w_mgr);
        }
        renderer.draw_bg({250,250,250,255});
        renderer.TextBox(w.widget_editor);
		renderer.TextBox(w2.widget_editor);

        SDL_Rect dst = {20,20,20,20};
        SDL_RenderCopy(renderer.ren, renderer.folderIcon, nullptr, &dst);
        dst.x = 50;
		SDL_RenderCopy(renderer.ren, renderer.fileIcon, nullptr, &dst);
        renderer.rend();
    }
    renderer.destroy();
    return 0;
}