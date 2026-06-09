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