#pragma once
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

class S_Frame {
public:

	bool running = false;

	Renderer renderer;
	EventHandler handler;
	WidgetManager w_mgr;

	std::unordered_map<std::string, std::unique_ptr<Widget_Editor>> w_editors;
	std::unordered_map<std::string, std::unique_ptr<Widget_File_explorer>> w_explorers;

	//Render & Events, Render to clear
	std::vector<Widget_Editor*> w_editor_calls;
	std::vector<Widget_File_explorer*> w_explorer_calls;
	
	//events
	bool mouseDown = false;
	int mx = 0;
	int my = 0;
	int mousex = 0, mousey = 0;
	SDL_Point now_mouse_P = { mousex, mousey };
	SDL_Point clicked_m = { mx, my };


	enum widget_type {
		w_Editor,
		w_explorer
	};

	int argc;
	char** argv;
	bool init(int& ar, char* arg[]) {
		argc = ar;
		argv = arg;
		const char* fontPath = (argc > 1) ? argv[1] : "";
		if (!renderer.init(fontPath)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Init: %s | %s", SDL_GetError(), TTF_GetError());
			return false;
		}
		renderer.init_icon_tex();
		handler.rend = &renderer;
		handler.mb = &mouseDown;
		handler.nmP = &now_mouse_P;
		handler.mP = &clicked_m;
		running = true;
		return true;
	}
	void exit() {
		renderer.destroy_all_buttons(w_mgr.ui_btns);
		renderer.destroy();
	}

	void addwidget(int type, SDL_Rect r,int layer,const std::string& name) {
		if (type == w_Editor) {
			if (!w_editors.contains(name)) {
				auto w_ed = std::make_unique<Widget_Editor>();
				w_ed->widget_editor.set_init(r, "", renderer.lineH);
				w_ed->widget_rect = r;
				w_ed->widget_layer = layer;
				w_mgr.addWidget(*w_ed);
				w_editors[name] = std::move(w_ed);
			}
		}
		else if (type == w_explorer) {
			if (!w_explorers.contains(name)) {
				auto w_fe = std::make_unique<Widget_File_explorer>();
				w_fe->explorer.size = r;
				w_fe->explorer.init(renderer.lineH);
				renderer.fs_texture_init(w_fe->explorer);
				w_fe->widget_rect = r;
				w_fe->widget_layer = layer;
				w_mgr.addWidget(*w_fe);
				w_explorers[name] = std::move(w_fe);
			}
		}
		else {
			return;
		}
	}
	
	void ww_editor(std::string name) {
		if (w_editors.contains(name)) {
			w_editor_calls.push_back(w_editors[name].get());
		}
	}
	void ww_explorer(std::string name) {
		if (w_explorers.contains(name)) {
			w_explorer_calls.push_back(w_explorers[name].get());
		}
	}

	void events() {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			switch (e.type) {
				case SDL_QUIT: running = false; break;
			}
			handler.ev = &e;
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				mx = e.button.x;
				my = e.button.y;
				clicked_m = { mx, my };
			}
			handler.mP = &clicked_m;
			mouseDown = (e.type == SDL_MOUSEBUTTONDOWN) ? true : (e.type == SDL_MOUSEBUTTONUP) ? false : mouseDown;
			renderer.mouse_logical_pos(mousex, mousey);
			now_mouse_P = { mousex, mousey };

			//Event Prosses

			if (!w_editor_calls.empty()) {
				for (auto& w : w_editor_calls) {
					handler.textEditEvent_w_t(*w, w_mgr);
				}
			}
			if (!w_explorer_calls.empty()) {
				for (auto& w : w_explorer_calls) {
					handler.FileExplorer_w_t(*w, w_mgr);
				}
			}
		}
	}
	void render_obj() {
		renderer.draw_bg({ 250,250,250,255 });
		if (!w_editor_calls.empty()) {
			for (auto& w : w_editor_calls) {
				renderer.TextBox(w->widget_editor);
			}
		}
		if (!w_explorer_calls.empty()) {
			for (auto& w : w_explorer_calls) {
				renderer.drw_file_explorer(w->explorer);
				w->explorer.tickupdate();
			}
		}
		w_editor_calls.clear();
		w_explorer_calls.clear();
		renderer.rend();
	}
};