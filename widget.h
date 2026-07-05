#pragma once
#include "sdlutil.h"
class UI_Btn {
public:
	std::string btn_name = "new Button";
	SDL_Texture* text_texture = nullptr;
	bool hovered = false;
	bool clicked = false;
	bool tgr = false;
	std::string group = "";
	bool radio = false;
	bool one_sht = true;
	bool bef_btn = false;
};
class Widget {
public:
    SDL_Rect widget_rect = {0,0,0,0};
	std::string widget_name = "new Widget";
	int widget_layer = 0;
};
class Widget_Editor : public Widget {
public:
	Editor widget_editor;
};
class Widget_File_explorer : public Widget {
public:
	File_explorer explorer;
};
class Widget_button : public Widget {
public:
	UI_Btn button;
	void btn_set(SDL_Rect r,std::string name, const std::string& group) {
		button.btn_name = name;
		button.group = group;
		widget_rect = r;
		widget_layer = 100;
	}
};
class btn_mgr {
public:
	std::unordered_map<std::string, Widget_button> btns;
	std::vector<std::string> btn_order;
	void add_btn(const std::string& name, SDL_Rect r, const std::string& group = "", bool toggle = false, bool radio = false) {
		if (btns.find(name) != btns.end()) {
			btns[name].btn_set(r, name, group);
		}
		else {
			Widget_button b;
			b.btn_set(r, name, group);
			b.button.tgr = toggle;
			b.button.radio = radio;
			btns[name] = b;
		}
	}
	bool imitate_btn(const std::string& name) {
		if (btns.contains(name)) {
			btn_order.push_back(name);
			
			if (btns[name].button.tgr) return btns[name].button.clicked;
			bool rt = btns[name].button.clicked && !btns[name].button.bef_btn;
			btns[name].button.bef_btn = btns[name].button.clicked;
			return rt;
		}
		return false;
	}
	void group_off(const std::string& group_name) {
		for (auto& [name, btn] : btns) {
			if (btn.button.group == group_name) {
				btn.button.clicked = false;
			}
		}
	}
};
class WidgetManager {
public:
	std::vector<Widget> widgets;
	btn_mgr ui_btns;
	void addWidget(const Widget& widget) {
		widgets.push_back(widget);
	}
    int Widget_event(SDL_Point mouse_P,bool click) {
		if (!click) return 99;
		if (widgets.empty()) return 98;
		int max_layer = -1;
        int index = -1;
		for (int i = 0; i < (int)widgets.size(); i++) {
			if (SDL_PointInRect(&mouse_P, &widgets[i].widget_rect)) {
				if (widgets[i].widget_layer > max_layer) {
					max_layer = widgets[i].widget_layer;

					index = i;
				}
			}
		}
		for (auto& name : ui_btns.btn_order) {
			if (SDL_PointInRect(&mouse_P, &ui_btns.btns[name].widget_rect)) {
				max_layer = 100;
				break;
			}
		}
		return max_layer;
    }
	void btn_order_cls() {
		ui_btns.btn_order.clear();
	}
};

class Slider{
public:
	SDL_Rect bar = {0,0,0,0};
	SDL_Rect handL = {0,0,0,0};
	bool hover = false;
	SDL_Color handle_col = {66, 135, 245, 255};
	SDL_Color handle_col_hv = { 45, 115, 225, 255 };
	int Clickedx = 0;
	bool clicked = false;
	std::string Label = "this";
	SDL_Texture* Label_tex = nullptr;
	int v_max = 255;
	int v_min = 0;

	int now_val = 0;

	void set(int value) {
		value = std::clamp(value, v_min, v_max);
		double t = double((value - v_min)) / double((v_max - v_min));
		int left_l = bar.x;
		int right_l = bar.x + bar.w - handL.w;

		float ax = t * (bar.w - handL.w) + bar.x;
		handL.x = int(ax);
	}
	Editor box;
	void init(int lH, SDL_Rect r) {
		bar = r;
		set(0);
		box.set_init({bar.x + 30, bar.y - 30, 40, 20}, "0", lH);
		box.noLineNo = true;
		box.PADDING = 5;
	}
	void destroy() {
		SDL_DestroyTexture(Label_tex);
	}
};


class Drws_Toolbar {
public:
	SDL_Rect size = {0,0,0,0};
	SDL_Rect color_preview = { 0,0,0,0 };
	Slider sl_r, sl_g, sl_b, sl_a;
	SDL_Color now_color = { 0,0,0,255 };
	void setColor() {
		now_color = { (Uint8)sl_r.now_val, (Uint8)sl_g.now_val, (Uint8)sl_b.now_val, (Uint8)sl_a.now_val };
	}
	struct palette {
		SDL_Color color;
		SDL_Rect rect;
	};
	palette pal[8] = {
		{ {255,0,0,255}, {0,0,0,255} },
		{ {0,255,0,255}, {0,0,0,255} },
		{ {0,0,255,255}, {0,0,0,255} },
		{ {255,255,0,255}, {0,0,0,255} },
		{ {255,165,0,255}, {0,0,0,255} },
		{ {128,0,128,255}, {0,0,0,255} },
		{ {255,192,203,255}, {0,0,0,255} },
		{ {128,128,128,255}, {0,0,0,255} }
	};
	void init(SDL_Rect r,int lH){
		size = r;
		int div = r.h / 8;
		int w = r.w - 20;
		sl_r.init(lH, { r.x + 10, r.y + div * 4, w, 10 });
		sl_g.init(lH, { r.x + 10, r.y + div * 5, w, 10 });
		sl_b.init(lH, { r.x + 10, r.y + div * 6, w, 10 });
		sl_a.init(lH, { r.x + 10, r.y + div * 7, w, 10 });
		sl_r.Label = "R:";
		sl_g.Label = "G:";
		sl_b.Label = "B:";
		sl_a.Label = "A:";
		color_preview = { r.x + 10, r.y + 10, w, div * 2 };
		SDL_Rect pal_rect;
		for (int i = 0; i < 8; i++) {
			pal_rect = { r.x + i * (w / 8 + 5), r.y + div * 2 + 15, w / 8 - 15, w / 8 - 15 };
			pal[i].rect = pal_rect;
		}
	}
	void destroy_dt() {
		sl_r.destroy();
		sl_g.destroy();
		sl_b.destroy();
		sl_a.destroy();
	}

};