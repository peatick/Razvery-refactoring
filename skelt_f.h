#pragma once
#include "sdl_frame.h"

class window_Manager {
public:
	int win_w = 800, win_h = 600;
	int logic_w = 800, logic_h = 600;

	SKEL_Frame main_frame;
	std::vector<std::unique_ptr<SKEL_Frame>> frames;
	bool& running = main_frame.running;

	bool init(int& argc, char* argv[]) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
		if (TTF_Init() < 0) return false;
		
		return main_frame.init(argc, argv, win_w, win_h, logic_w, logic_h, "Main Window");
	}

	void new_window(int w_w, int w_h, int log_w, int log_h, const std::string& title) {
		auto new_frame = std::make_unique<SKEL_Frame>();
		if (new_frame->init(main_frame.argc, main_frame.argv, w_w, w_h, log_w, log_h, title)) {
			frames.push_back(std::move(new_frame));
		}
	}
	void close_window(int index) {
		if (index >= 0 && index < frames.size()) {
			frames[index]->exit();
			frames.erase(frames.begin() + index);
		}
	}

	void events() {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) { running = false; continue; }

			if (SDL_GetWindowID(main_frame.win) == e.window.windowID) {
				main_frame.handler.ev = &e;
				main_frame.event_K(e);
				if (!main_frame.running) running = false;
				continue;
			}
			for (auto& frame : frames) {
				if (SDL_GetWindowID(frame->win) == e.window.windowID) {
					frame->handler.ev = &e;
					frame->event_K(e);
					break; // 一致したら他のフレームを見る必要はない
				}
			}
		}
		// ループの外でまとめて削除
		frames.erase(std::remove_if(frames.begin(), frames.end(),
			[](std::unique_ptr<SKEL_Frame>& f) {
				if (!f->running) { f->exit(); return true; }
				return false;
			}), frames.end());
	}

	void render() {
		main_frame.render_obj();
		for (auto& frame : frames) {
			frame->render_obj();
		}
	}

	void exit() {
		main_frame.exit();
		TTF_Quit();
		SDL_Quit();
	}


};