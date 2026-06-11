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

class Widget_util : public Widget{
public:
	virtual ~Widget_util(){}
	virtual void init(Renderer& renderer,WidgetManager& w_mgr,const SDL_Rect& rec, int layer,const std::string& name){}
	virtual void Event(EventHandler& ev_h,WidgetManager& w_mgr){}
	virtual void Render(Renderer& renderer){}
	virtual void Destroyer(Renderer& renderer){}
};
class Widget_Ed_u : public Widget_util{
public:
	Editor ed_u;
	void init(Renderer& renderer,WidgetManager& w_mgr,const SDL_Rect& rec, int layer,const std::string& name) override{
		ed_u.set_init(rec,"",renderer.lineH);
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h,WidgetManager& w_mgr) override{
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		ev_h.textEditEvent_u(ed_u);
	}
	void Render(Renderer& renderer) override{
		renderer.TextBox(ed_u);
	} 
};

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
		}
	}
	void render_obj() {
		renderer.draw_bg({ 250,250,250,255 });
		renderer.rend();
	}
};