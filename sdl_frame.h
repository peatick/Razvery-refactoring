#pragma once
#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"
#include "Widget_util.h"
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

	std::unordered_map<std::string, std::unique_ptr<Widget_util>> Widget_s;
	std::vector<Widget_util*> Widget_Oders;

	//events
	bool mouseDown = false;
	int mx = 0;
	int my = 0;
	int mousex = 0, mousey = 0;
	SDL_Point now_mouse_P = { mousex, mousey };
	SDL_Point clicked_m = { mx, my };


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
		for (auto& w_u : Widget_Oders) {
			w_u->Destroyer(renderer);
		}
		renderer.destroy_all_buttons(w_mgr.ui_btns);
		renderer.destroy();
	}

	enum {
		Editor_UW,
		Explorer_UW
	};

	void addwidget(int type,const SDL_Rect& r,int layer,const std::string& name) {
		if(type == Editor_UW){
			if(!Widget_s.contains(name)){
				auto w_u = std::make_unique<Widget_Ed_u>();
				w_u->init(renderer,w_mgr,r,layer,"");
				Widget_s[name] = std::move(w_u);
			}
		}
		else if (type == Explorer_UW) {
			if (!Widget_s.contains(name)) {
				auto w_u = std::make_unique<Widget_explorer_u>();
				w_u->init(renderer, w_mgr, r, layer, "");
				Widget_s[name] = std::move(w_u);
			}
		}
	}
	void Widget_Call(const std::string& name) {
		if (Widget_s.contains(name)) {
			Widget_Oders.push_back(Widget_s[name].get());
		}
	}
	//add button wap
	void w_addbtn(const std::string& id, const std::string& group, const std::string& name, const SDL_Rect& b_r, bool tgr = false,bool radio = false) {
		w_mgr.ui_btns.add_btn(id,b_r,group,tgr,radio);
		w_mgr.ui_btns.btns[id].button.btn_name = name;
	}
	bool q_Btn(const std::string& id) {
		return w_mgr.ui_btns.imitate_btn(id);
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

			handler.Btnui_w_t(w_mgr);
			//Event Prosses
			for (auto& w_u : Widget_Oders) {
				w_u->Event(handler, w_mgr);
			}
		}
	}
	void render_obj() {
		renderer.draw_bg({ 250,250,250,255 });
		for (auto& w_u : Widget_Oders) {
			w_u->Render(renderer);
		}
		Widget_Oders.clear();
		renderer.drw_all_buttons(w_mgr.ui_btns);
		w_mgr.btn_order_cls();
		renderer.rend();
	}
};