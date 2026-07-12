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
#include <type_traits>
#include <functional>
#include <memory>

class SKEL_Frame {
public:
	bool running = false;
	std::string window_title = "new window 新しいウィンドウ";


	Renderer renderer;
	EventHandler handler;
	WidgetManager w_mgr;
	SDL_Window* win = nullptr;
	Uint32 win_id = 0;

	std::unordered_map<std::string, std::unique_ptr<Widget_util>> Widget_s;
	std::vector<Widget_util*> Widget_Oders;

	//events
	bool mouseDown = false;
	bool LmouseDown = false;
	int mx = 0;
	int my = 0;
	int mousex = 0, mousey = 0;
	SDL_Point now_mouse_P = { mousex, mousey };
	SDL_Point clicked_m = { mx, my };


	int argc;
	char** argv;
	bool init(int& ar, char* arg[],int w_w, int w_h, int log_w, int log_h, std::string title = "new window 新しいウィンドウ") {
		argc = ar;
		argv = arg;
		window_title = title;
		const char* fontPath = (argc > 1) ? argv[1] : "";
		win = SDL_CreateWindow(window_title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w_w, w_h, SDL_WINDOW_RESIZABLE);
		if (!renderer.init(fontPath, win, log_w, log_h)) {
			SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Init: %s | %s", SDL_GetError(), TTF_GetError());
			return false;
		}
		win_id = SDL_GetWindowID(win);
		std::cout << win_id << std::endl;
		renderer.init_icon_tex();
		handler.rend = &renderer;
		handler.mb = &mouseDown;
		handler.nmP = &now_mouse_P;
		handler.mP = &clicked_m;
		handler.L_MDown = &LmouseDown;
		running = true;
		return true;
	}
	void exit() {
		for (auto& w_us : Widget_s){
			w_us.second->Destroyer(renderer);
		}
		renderer.destroy_all_buttons(w_mgr.ui_btns);
		renderer.destroy(win);
	}

	template<class T>
	void addwidget_t(const SDL_Rect& r,int layer,const std::string& name){
		if(!Widget_s.contains(name)){
			auto w_u = std::make_unique<T>();
			w_u->init(renderer,w_mgr,r,layer,"");
			Widget_s[name] = std::move(w_u);
		}
	}

	void Widget_Call(const std::string& name) {
		if (Widget_s.contains(name)) {
			Widget_Oders.push_back(Widget_s[name].get());
		}
	}

	template<class T>
	T* get(const std::string& name) {
		auto it = Widget_s.find(name);
		if (it == Widget_s.end())
			return nullptr;
		return dynamic_cast<T*>(it->second.get());
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
			if (SDL_GetWindowID(win) == e.window.windowID){
				switch (e.type) {
				case SDL_QUIT: running = false; return;
				}
				if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
					running = false; return;
				}
				handler.ev = &e;
				if (e.type == SDL_MOUSEBUTTONDOWN) {
					mx = e.button.x;
					my = e.button.y;
					clicked_m = { mx, my };
				}
				handler.mP = &clicked_m;
				mouseDown = (e.type == SDL_MOUSEBUTTONDOWN) ? true : (e.type == SDL_MOUSEBUTTONUP) ? false : mouseDown;
				LmouseDown = (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ? true : (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ? false : LmouseDown;
				renderer.mouse_logical_pos(mousex, mousey);
				now_mouse_P = { mousex, mousey };

				handler.Btnui_w_t(w_mgr);
				//Event Prosses
				for (auto& w_u : Widget_Oders) {
					w_u->Event(handler, w_mgr);
				}
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

	struct idAndname {
		std::string id;
		std::string name;
	};
	void BtnAutoset_Beside(std::vector<idAndname> iAn, std::string grp, const SDL_Rect s_mpl){
		if(iAn.empty()) return;
		int tx = 0;
		for (int i = 0;i < iAn.size(); i++){
			tx = s_mpl.x + s_mpl.w * i;
			std::cout << tx << std::endl;
			w_addbtn(iAn[i].id, grp, iAn[i].name, {tx, s_mpl.y, s_mpl.w, s_mpl.h}, true, true);
		}
	}


	void event_test(std::vector<std::function<void()>> fncs) {
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
			LmouseDown = (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ? true : (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ? false : LmouseDown;
			renderer.mouse_logical_pos(mousex, mousey);
			now_mouse_P = { mousex, mousey };

			handler.Btnui_w_t(w_mgr);
			//Event Prosses
			for (auto& w_u : Widget_Oders) {
				w_u->Event(handler, w_mgr);
			}
			for (auto& fn : fncs) {
				fn();
			}
		}
	}

	void event_K(SDL_Event& e) {
		if (SDL_GetWindowID(win) == e.window.windowID) {
			switch (e.type) {
			case SDL_QUIT: running = false; return;
			}
			if (e.window.event == SDL_WINDOWEVENT_CLOSE) {
				running = false; return;
			}
			handler.ev = &e;
			if (e.type == SDL_MOUSEBUTTONDOWN) {
				mx = e.button.x;
				my = e.button.y;
				clicked_m = { mx, my };
			}
			handler.mP = &clicked_m;
			mouseDown = (e.type == SDL_MOUSEBUTTONDOWN) ? true : (e.type == SDL_MOUSEBUTTONUP) ? false : mouseDown;
			LmouseDown = (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT) ? true : (e.type == SDL_MOUSEBUTTONUP && e.button.button == SDL_BUTTON_LEFT) ? false : LmouseDown;
			renderer.mouse_logical_pos(mousex, mousey);
			now_mouse_P = { mousex, mousey };

			handler.Btnui_w_t(w_mgr);
			//Event Prosses
			for (auto& w_u : Widget_Oders) {
				w_u->Event(handler, w_mgr);
			}
		}
	}
};