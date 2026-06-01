#include "sdlutil.h"
class Widget {
public:
    SDL_Rect widget_rect = {0,0,0,0};
	std::string widget_name = "new Widget";
	int widget_layer = 0;
	Editor widget_editor;
    Editors* widget_editors;
    File_explorer* file_ex;
    int ed_index = 0;
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