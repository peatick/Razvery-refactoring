#pragma once
#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"
#include "PaintTool.h"
#include "sdl_frame.h"
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

class Widget_util : public Widget {
public:
	virtual ~Widget_util() {}
	virtual void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) {}
	virtual void Event(EventHandler& ev_h, WidgetManager& w_mgr) {}
	virtual void Render(Renderer& renderer) {}
	virtual void Destroyer(Renderer& renderer) {}
};
class Widget_Ed_u : public Widget_util {
public:
	Editor ed_u;
	Editor_Search es;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		ed_u.set_init(rec, "", renderer.lineH);
		es.init({rec.x + rec.w - 200,rec.y,200,50},{rec.x + rec.w - 200,rec.y,200,50},renderer.lineH);
		es.EDS = &ed_u;
		es.Search_box.noLineNo = true;
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		ed_u.selected = ev_h.Widget_ev(*this, w_mgr);
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		ev_h.textEditEvent_u(ed_u);
		if (ed_u.searchMode){
			ev_h.SearchBox(es, es.EDS);
		}
	}
	void Render(Renderer& renderer) override {
		renderer.TextBox(ed_u);
		if (ed_u.selected) {
			ed_u.tickBlink();
		}
		if (ed_u.searchMode){
			renderer.drw_Searchbox(es);
		}
	}
};
class Widget_explorer_u : public Widget_util {
public:
	File_explorer explorer;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		explorer.size = rec;
		explorer.init(renderer.lineH);
		renderer.fs_texture_init(explorer);
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		ev_h.File_explorer_Event(explorer);
	}
	void Render(Renderer& renderer) override {
		explorer.tickupdate();
		renderer.drw_file_explorer(explorer);
	}
	void Destroyer(Renderer& renderer) override {
		renderer.fs_texture_destruct(explorer);
	}
};
class Widget_d_Toolbar_u : public Widget_util{
public:
	Drws_Toolbar d_tb;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		d_tb.init(rec, renderer.lineH);
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		ev_h.drw_toolbar_ev(d_tb);
	}
	void Render(Renderer& renderer) override {
		renderer.drw_toolbar(d_tb);
	}
	void Destroyer(Renderer& renderer) override {
		d_tb.destroy_dt();
	}
};
class Widget_drw_tools : public Widget_util {
public:
	SDL_Rect size = {0,0,0,0};
	PaintApp pa;
	Drws_Toolbar DrT;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		size = rec;
		int dp = size.w / 3;
		pa.setRect(size.x, size.y, dp * 2, size.h);
		pa.setSize( 256, 256, renderer.ren);
		DrT.init({size.x + dp * 2, size.y, dp, size.h}, renderer.lineH);
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		ev_h.paintApp_ev(pa);
		ev_h.drw_toolbar_ev(DrT);
		if (ev_h.Widget_ev(*this, w_mgr)) return;
	}
	void Render(Renderer& renderer) override {
		pa.pan();
		if(pa.col_d){
            DrT.cl_set(pa.getCurrentColor());
            pa.col_d = false;
        }
		pa.setColor(DrT.now_color);
        pa.brush = std::clamp(DrT.sl_size.now_val,pa.burush_size_min,pa.brush_size_max);
		renderer.drw_PaintTool(pa);
		renderer.drw_toolbar(DrT);
	}
	void Destroyer(Renderer& renderer) override {
		DrT.destroy_dt();
	}
};