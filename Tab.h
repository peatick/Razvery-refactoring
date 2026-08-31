#pragma once
#include "Renderer.h"
#include "Events.h"
class Tab {
private:
	void all_noneselect() {
		for (auto& t : tabs) {
			t.selected = false;
		}
	}
	bool MD_T = false;
protected:
	
public:
	struct tab {
		std::string title = "No Title";
		bool selected = false;
		SDL_Texture* title_tex = nullptr;
		bool change_name = false;
		SDL_Rect r = { 0,0,0,0 };
		fs::path SavePath = "";
	};
	std::vector<tab> tabs;
	int max = 5;
	SDL_Rect size = {};
	int tab_wide = 0;
	int tab_hi = 0;

	void init(SDL_Rect r, int Tab_max = 5) {
		size = r;
		max = Tab_max;
		tab_wide = r.w / Tab_max;
		tab_hi = r.h;
	}


	int add(Renderer& ren, std::string title = "", fs::path p = "") {
		if (tabs.size() < max) {
			int ret = tabs.size();
			SDL_Rect rt = { size.x + (tab_wide * ret), size.y, tab_wide, tab_hi };
			tab Tmp;
			if (!title.empty()) {
				for (auto& t : tabs) {
					if (t.title == title) {
						return -1;
					}
					if (t.SavePath == p) {
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
			return ret;
		}
		else {
			return -1;
		}
	}

	void Event(EventHandler& ev) {
		if (SDL_PointInRect(ev.nmP, &size)) {
			if (ev.MD_click() && !MD_T) {
				bool erased = false;
				for (auto it = tabs.begin(); it != tabs.end(); ) {
					if (SDL_PointInRect(ev.nmP, &it->r)) {
						SDL_DestroyTexture(it->title_tex);
						it = tabs.erase(it);
						erased = true;
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

	void render(Renderer& ren) {
		SDL_SetRenderDrawColor(ren.ren, ren.colBg.r, ren.colBg.g, ren.colBg.b, 255);
		SDL_RenderFillRect(ren.ren, &size);
		for (const auto& t : tabs) {
			if (t.selected) {
				SDL_SetRenderDrawColor(ren.ren, 80, 80, 100, 255);
			}
			else {
				SDL_SetRenderDrawColor(ren.ren, ren.colBg.r, ren.colBg.g, ren.colBg.b, 255);
			}
			SDL_RenderFillRect(ren.ren, &t.r);
			ren.drawtexture(t.title_tex, t.r.x + 5, t.r.y + 5);
		}
	}

	void destroy(Renderer& ren) {
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
};