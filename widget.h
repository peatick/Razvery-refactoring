#pragma once
#include "sdlutil.h"
class UI_Btn {
public:
	std::string btn_name = "new Button";
	SDL_Texture* text_texture = nullptr;
	bool hovered = false;
	bool clicked = false;
	bool tgr = false;
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
};
class WidgetManager {
public:
	std::vector<Widget> widgets;
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
		return max_layer;
    }
};