#pragma once
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"

// This Header file is Button on UI

class WD_Btn {
private:
	SDL_Texture* text_texture = nullptr;
	std::string btn_name = "new Button";
	bool tgr = false;
	bool hovered = false;
public:
	SDL_Rect size = { 0,0,0,0 };
	bool Active = false;
	void init(Renderer& renderer, std::string name, SDL_Rect rect, bool triggered = false) {
		text_texture = nullptr;
		btn_name = name;
		size = rect;
		tgr = triggered;
		text_texture = renderer.text_texture(btn_name);
	}

	void handleEvent(EventHandler& ev_h) {
		hovered = SDL_PointInRect(ev_h.nmP, &size);
		if (hovered && ev_h.L_click()) {
			if (tgr) {
				Active = !Active;
			}
			else {
				Active = true;
			}
		}
		else if (!tgr) {
			Active = false;
		}
	}

	void render(Renderer& renderer) {
		if (Active) {
			SDL_SetRenderDrawColor(renderer.ren, 180, 180, 180, 255);
		}
		else if (hovered){
			SDL_SetRenderDrawColor(renderer.ren, 210, 210, 210, 255);
		}
		else {
			SDL_SetRenderDrawColor(renderer.ren, 220, 220, 220, 255);
		}
		SDL_RenderFillRect(renderer.ren, &size);
		renderer.drawtexture_center(text_texture, size.x + size.w / 2, size.y + size.h / 2);
	}
	void destroy(Renderer& renderer) {
		SDL_DestroyTexture(text_texture);
	}
};