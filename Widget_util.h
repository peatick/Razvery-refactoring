#pragma once
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"
#include "PaintTool.h"
#include "sdl_frame.h"
#include "file_Explorer_tree.h"
#include "Paint_v2.h"
#include "File_Open_Save.h"
#include "Tab.h"
#include "map_canvas.hpp"
#include "palette_canvas.hpp"
#include "tileset.hpp"
#include <functional>
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

class Widget_util : public Widget {
public:
	bool save_req = false;
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
		pa.setSize( 8, 8, renderer.ren);
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
class Widget_TreeExplorer : public Widget_util {
public:
	SDL_Rect size = { 0,0,0,0 };
	File_explorer_tree fe_t;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		size = rec;
		fe_t.size = { rec.x, rec.y, rec.w, rec.h - 20 };
		fe_t.hsl.size = { rec.x, rec.y + rec.h - 20, rec.w, 20 };
		fe_t.init();
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		fe_t.eventH(ev_h);
		if (ev_h.Widget_ev(*this, w_mgr)) return;
	}
	void Render(Renderer& renderer) override {
		fe_t.render(renderer);
	}
	void Destroyer(Renderer& renderer) override {
		fe_t.destroy();
	}
};
class Widget_Paint_v2 : public Widget_util {
	SDL_Texture* NoSave_Tex = nullptr;
public:
	SDL_Rect size = { 0,0,0,0 };
	PaintWidget paint;
	

	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		size = rec;
		paint.init(renderer.ren, rec, 512, 512, renderer.font_sml);
		NoSave_Tex = renderer.text_texture_white("*");
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		paint.handleEvent(*ev_h.ev);
		save_req = ev_h.save_btn();
	}
	void Render(Renderer& renderer) override {
		if (paint.canvasDirty()) paint.save_dirt = true;
		paint.render();

		int nx = widget_rect.x + widget_rect.w - 200;
		int ny = widget_rect.y + widget_rect.h - 50;
		if (!paint.savepath.empty()) {
			std::string filename = path2string_s(str2path(paint.savepath).filename());
			renderer.drawText(filename, nx + 10, ny, {220, 220, 220, 255});
		}
		if (paint.save_dirt) renderer.drawtexture(NoSave_Tex, nx, ny);
	}
	void Destroyer(Renderer& renderer) override {
		paint.cleanup();
		PaintWidget::shutdownShared();
		paint.destroyCanvasTexture();
		SDL_DestroyTexture(NoSave_Tex);
	}
};
class Widget_File_Open_u : public Widget_util {
public:
	File_Open F;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		F.explorer.size = rec;
		F.explorer.init(renderer.lineH);
		renderer.fs_texture_init(F.explorer);
		F.init(renderer, "Open");
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		F.HandleEvent(ev_h);
	}
	void Render(Renderer& renderer) override {
		F.render(renderer);
	}
	void Destroyer(Renderer& renderer) override {
		renderer.fs_texture_destruct(F.explorer);
	}
};
class Widget_File_Save_u : public Widget_util {
public:
	File_Save_as F;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		F.explorer.size = rec;
		F.explorer.init(renderer.lineH);
		renderer.fs_texture_init(F.explorer);
		F.init(renderer, "Save as");
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		F.HandleEvent(ev_h);
	}
	void Render(Renderer& renderer) override {
		F.render(renderer);
	}
	void Destroyer(Renderer& renderer) override {
		renderer.fs_texture_destruct(F.explorer);
	}
};
class Widget_SynLua_u : public Widget_util {
public:
	Editor_syntaxed ed_u;
	Editor_Search es;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		ed_u.TextEditor.set_init(rec, "", renderer.lineH);
		es.init({ rec.x + rec.w - 200,rec.y,200,50 }, { rec.x + rec.w - 200,rec.y,200,50 }, renderer.lineH);
		es.EDS = &ed_u.TextEditor;
		es.Search_box.noLineNo = true;
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		ed_u.TextEditor.selected = ev_h.Widget_ev(*this, w_mgr);
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		ev_h.textEditEvent_u(ed_u.TextEditor);
		if (ed_u.TextEditor.searchMode) {
			ev_h.SearchBox(es, es.EDS);
		}
		save_req = ev_h.save_btn();
		if (ev_h.ev->type == SDL_KEYDOWN) {
			ed_u.no_save = true;
		}
	}
	void Render(Renderer& renderer) override {
		renderer.TextBox_Syn(ed_u);
		if (ed_u.TextEditor.selected) {
			ed_u.TextEditor.tickBlink();
		}
		if (ed_u.TextEditor.searchMode) {
			renderer.drw_Searchbox(es);
		}
	}
};
class Widget_Tab_u : public Widget_util {
public:
	Tab _tab;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		_tab.init(rec);
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		_tab.Event(ev_h);
	}
	void Render(Renderer& renderer) override {
		_tab.render(renderer);
	}
	void Destroyer(Renderer& renderer) override {
		_tab.destroy(renderer);
	}
};
class Widget_Lua_Tab_s : public Widget_util {
private:
	void all_noneselect() {
		for (auto& t : tabs) {
			t.selected = false;
		}
	}
	bool MD_T = false;
	bool vaid_path(std::string& s) {
		if (s.empty()) return false;
		fs::path p = str2path(s);
		return fs::exists(p);
	}
protected:

public:
	struct tab {
		std::string title = "No Title";
		bool selected = false;
		SDL_Texture* title_tex = nullptr;
		bool change_name = false;
		SDL_Rect r = { 0,0,0,0 };
		fs::path SavePath = "";
		bool no_saved = true;
	};

	std::function<void()> close_ev = nullptr;

	Widget_SynLua_u Lua_Editor[5];
	SDL_Texture* Nosave_mark = nullptr;

	std::vector<tab> tabs;
	int max = 5;
	SDL_Rect size = { 0,0,0,0 };
	int tab_wide = 0;
	int tab_hi = 0;

	void INITAL(SDL_Rect r, int Tab_max = 5) {
		size = r;
		max = Tab_max;
		tab_wide = r.w / Tab_max;
		tab_hi = r.h;
	}
	int add(Renderer& ren, fs::path p = "", LuaKey_Lex src = {nullptr, nullptr, "", nullptr}) {
		if (tabs.size() < max) {
			int ret = tabs.size();
			SDL_Rect rt = { size.x + (tab_wide * ret), size.y, tab_wide, tab_hi };
			tab Tmp;
			std::string title = path2string_s(p.filename());
			if (!title.empty()) {
				for (auto& t : tabs) {
					if (t.title == title) {
						return -1;
					}
					if (t.SavePath == p) {
						return -1;
					}
				}
				Tmp.title = title;
			}
			Tmp.title_tex = ren.text_texture_white(Tmp.title);
			all_noneselect();
			Tmp.selected = true;
			Tmp.r = rt;
			Tmp.SavePath = p;
			tabs.push_back(Tmp);
			Lua_Editor[ret].ed_u.Open_File(path2string_s(p), src);
			return ret;
		}
		else {
			return -1;
		}
	}
	void Event_T(EventHandler& ev) {
		if (SDL_PointInRect(ev.nmP, &size)) {
			if (ev.MD_click() && !MD_T) {
				bool erased = false;
				for (auto it = tabs.begin(); it != tabs.end(); ) {
					if (SDL_PointInRect(ev.nmP, &it->r)) {
						SDL_DestroyTexture(it->title_tex);
						int d = std::distance(tabs.begin(), it);
						Lua_Editor[d].ed_u.reset();
						it = tabs.erase(it);
						erased = true;
						if (close_ev) close_ev();
						continue;  // erase 後は必ず continue
					}
					if (erased) {
						it->r.x -= tab_wide;
					}
					++it;
				}
				MD_T = true;
			}
			else {
				MD_T = false;
			}
			if (ev.L_click()) {
				for (auto& t : tabs) {
					if (SDL_PointInRect(ev.nmP, &t.r)) {
						all_noneselect();
						t.selected = true;
						break;
					}
				}
			}
		}
	}
	void render_T(Renderer& ren) {
		SDL_SetRenderDrawColor(ren.ren, ren.colBg.r, ren.colBg.g, ren.colBg.b, 255);
		SDL_RenderFillRect(ren.ren, &size);
		for (size_t i = 0; i < tabs.size(); i++) {
			tabs[i].no_saved = Lua_Editor[i].ed_u.no_save;
		}
		for (const auto& t : tabs) {
			if (t.selected) {
				SDL_SetRenderDrawColor(ren.ren, 80, 80, 100, 255);
			}
			else {
				SDL_SetRenderDrawColor(ren.ren, ren.colBg.r, ren.colBg.g, ren.colBg.b, 255);
			}
			SDL_RenderFillRect(ren.ren, &t.r);
			ren.drawtexture(t.title_tex, t.r.x + 5, t.r.y + 5);
			if (t.no_saved) {
				ren.drawtexture(Nosave_mark, t.r.x + t.r.w - 8, t.r.y + 2);
			}
			if (t.change_name) {

			}
		}
	}
	void destroy_T() {
		for (auto& t : tabs) {
			SDL_DestroyTexture(t.title_tex);
		}
	}
	int selected_num() {
		for (int i = 0; i < tabs.size(); i++) {
			if (tabs[i].selected) {
				return i;
			}
		}
		return -1;
	}
	tab* selected_tab() {
		for (int i = 0; i < tabs.size(); i++) {
			if (tabs[i].selected) {
				return &tabs[i];
			}
		}
		return nullptr;
	}
	
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		INITAL({ 200, 20, 800, 20 });
		for (int i = 0; i < 5; i++) {
			Lua_Editor[i].init(renderer, w_mgr, rec, layer, name + std::to_string(i));
		}
		Nosave_mark = renderer.text_texture_white("*");
		widget_rect = {rec.x, rec.y - size.y, rec.w, rec.h + size.h};
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		Event_T(ev_h);
		if (selected_num() >= 0) {
			Lua_Editor[selected_num()].Event(ev_h, w_mgr);
		}
	}
	void Render(Renderer& renderer) override {
		SDL_SetRenderDrawColor(renderer.ren, 30, 30, 35, 255);
		SDL_RenderFillRect(renderer.ren, &widget_rect);
		render_T(renderer);
		if (selected_num() >= 0) {
			Lua_Editor[selected_num()].Render(renderer);
			Lua_Editor[selected_num()].ed_u.Src_sync();
		}
		for (auto& t : tabs) {
			if (t.change_name) {
				t.change_name = false;
				if (t.title_tex) {
					SDL_DestroyTexture(t.title_tex);
				}
				t.title_tex = renderer.text_texture_white(t.title);
			}
		}
	}
	void Destroyer(Renderer& renderer) override {
		destroy_T();
		SDL_DestroyTexture(Nosave_mark);
	}

	Widget_SynLua_u* act_Editor() {
		if (selected_num() >= 0) {
			return &Lua_Editor[selected_num()];
		}
		else {
			return nullptr;
		}
	}

	bool save_Text() {
		if (act_Editor()) {
			if (vaid_path(act_Editor()->ed_u.savepath)) {
				act_Editor()->ed_u.Save_File(act_Editor()->ed_u.savepath);
				return true;
			}
		}
		return false;
	}

	void save_as(std::string file_path) {
		if (!act_Editor()) return;
		act_Editor()->ed_u.Save_File(file_path);
		if (selected_tab()) {
			selected_tab()->change_name = true;
			std::string filename = path2string_s(str2path(file_path).filename());
			selected_tab()->title = filename;
		}
	}
};
class File_Save_Dialog : public Widget_util {
public:
	File_Simple F;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		F.inits(renderer, rec,"Save as");
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		F.HandleEvent(ev_h);
	}
	void Render(Renderer& renderer) override {
		F.render(renderer);
	}
	void Destroyer(Renderer& renderer) override {
		F.destroy();
	}
};
class Widget_Tab_System_u : public Widget_util {
public:
	Tab tab;
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		tab.init(rec, 3);
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		w_mgr.addWidget(*this);
		tab.add(renderer, "ScriptEditor");
		tab.add(renderer, "Paint");
		tab.add(renderer, "MapEditor");
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (!ev_h.Widget_ev(*this, w_mgr)) return;
		tab.Event(ev_h);
	}
	void Render(Renderer& renderer) override {
		tab.render(renderer);
	}
	void Destroyer(Renderer& renderer) override {
		tab.destroy(renderer);
	}
	int selected() {
		return tab.selected_num();
	}
};
class Widget_Map_Ed_u : public Widget_util {
	std::unique_ptr<Tileset> tileset;
	SDL_Rect paletteViewport;
	SDL_Rect mapViewport;
	bool active = false;
	bool active_m = false;
	SDL_Renderer* render_ = nullptr;
	std::unique_ptr<PaletteCanvas> palette_p = nullptr;
	std::unique_ptr<MapCanvas> mapCanvas_p = nullptr;
	SDL_Texture* Nosave_M = nullptr;
public:
	std::string mapName;
	bool noSave = false;
	void saveAll(Tileset& tileset, MapCanvas& map) {
		tileset.saveFlags();
		map.saveToFile(mapName);
	}
	void init(Renderer& renderer, WidgetManager& w_mgr, const SDL_Rect& rec, int layer, const std::string& name) override {
		widget_rect = rec;
		widget_name = name;
		widget_layer = layer;
		int wid = rec.w / 5;
		paletteViewport = {rec.x, rec.y + 20, wid, rec.h - 20};
		mapViewport = { rec.x + wid, rec.y + 20 , wid * 4, rec.h - 20};
		render_ = renderer.ren;
		Nosave_M = renderer.text_texture_white("*");
		w_mgr.addWidget(*this);
	}
	void Event(EventHandler& ev_h, WidgetManager& w_mgr) override {
		if (active) {
			palette_p->handleEvent(*ev_h.ev);
			if (mapCanvas_p->handleEvent(*ev_h.ev)) {
				noSave = true;
			}
		}
		save_req = ev_h.save_btn();
	}
	void Render(Renderer& renderer) override {
		SDL_SetRenderDrawColor(renderer.ren, 18, 18, 22, 255);
		SDL_RenderFillRect(renderer.ren, &widget_rect);

		SDL_SetRenderDrawColor(renderer.ren, 40, 40, 46, 255);
		SDL_RenderFillRect(renderer.ren, &paletteViewport);
		// マップ背景
		SDL_SetRenderDrawColor(renderer.ren, 18, 18, 22, 255);
		SDL_RenderFillRect(renderer.ren, &mapViewport);

		if (noSave) renderer.drawtexture(Nosave_M, widget_rect.x + 10, widget_rect.y);
		if (!mapName.empty()) renderer.drawText(mapName, widget_rect.x + 25, widget_rect.y, {220, 220, 220, 255});

		if (active) {
			palette_p->render();
		}
		if (active_m) {
			mapCanvas_p->render();
		}
	}
	void init_Tileset(const std::string& IMGpath) {
		try {
			tileset = std::make_unique<Tileset>(render_, IMGpath);
		}
		catch (const std::exception& ex) {
			std::cerr << ex.what() << std::endl;
			return;
		}
		palette_p = std::make_unique<PaletteCanvas>(render_, *tileset, paletteViewport);
		mapCanvas_p = std::make_unique<MapCanvas>(render_, *tileset, mapViewport);
		palette_p->setOnTileSelected([&](int index) {mapCanvas_p->setPaintTile(index); });
		mapCanvas_p->setPaintTile(palette_p->selectedTile());

		active = true;
	}
	void read_map(std::string dat_path) {
		if (mapCanvas_p) {
			mapCanvas_p->loadFromFile(dat_path);
			active_m = true;
		}
	}
	void clear() {
		if (mapCanvas_p) mapCanvas_p->clearAll();
	}
	bool save_map() {
		if (mapName.empty()) {
			return false;
		}
		saveAll(*tileset, *mapCanvas_p);
		mapCanvas_p->loadFromFile(mapName);
		read_map(mapName);
		noSave = false;
		return true;
	}
	bool arw_save() {
		if (palette_p) {
			if (mapCanvas_p) {
				return true;
			}
		}
		return false;
	}
	void Destroyer(Renderer& renderer) override {
		SDL_DestroyTexture(Nosave_M);
	}
};