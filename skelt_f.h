#pragma once
#include "sdl_frame.h"
#include <any>

class window_Manager {
private:
	struct dialog {
		std::string window_name = "";
		std::string call_widget = "";
		std::any data_p;
		std::function<void(std::string s)> f;
	};
public:

	int win_w = 800, win_h = 600;
	int logic_w = 800, logic_h = 600;
	bool Stop_MF = false;
	SKEL_Frame mf;
	std::unordered_map<std::string, std::unique_ptr<SKEL_Frame>> window_map;
	bool& running = mf.running;
	dialog now_Open_dialog;

	bool vaid_path(std::string& s) {
		if (s.empty()) return false;
		fs::path p = str2path(s);
		return fs::exists(p);
	}

	bool init(int& argc, char* argv[]) {
		if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
		if (TTF_Init() < 0) return false;
		int flags = IMG_INIT_PNG | IMG_INIT_JPG;
		if ((IMG_Init(flags) & flags) != flags) {
			// 必要なフォーマットの読み込みに失敗した場合のエラー処理
			SDL_Log("IMG_Init failed: %s", IMG_GetError());
		}
		return mf.init(argc, argv, win_w, win_h, logic_w, logic_h, "MDGW Editor");
	}

	void new_window(int w_w, int w_h, int log_w, int log_h, const std::string& title) {
		if (window_map.contains(title)) {
			// 既に同じタイトルのウィンドウが存在する場合は何もしない
			return;
		}
		auto new_frame = std::make_unique<SKEL_Frame>();
		if (new_frame->init(mf.argc, mf.argv, w_w, w_h, log_w, log_h, title)) {
			window_map[title] = std::move(new_frame);
		}
	}
	void close_window(const std::string& title) {
		if (window_map.contains(title)) {
			window_map[title]->running = false;
			window_map[title]->exit();
			window_map.erase(title);
		}
	}

	void events() {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) { running = false; continue; }

			if (SDL_GetWindowID(mf.win) == e.window.windowID) {
				if (Stop_MF) continue;
				mf.handler.ev = &e;
				mf.event_K(e);
				if (!mf.running) running = false;
				continue;
			}
			for (const auto& pair : window_map) {
				const auto& frame = pair.second;
				if (SDL_GetWindowID(frame->win) == e.window.windowID) {
					frame->handler.ev = &e;
					frame->event_K(e);
					break; // 一致したら他のフレームを見る必要はない
				}
			}
		}
		// ループの外でまとめて削除
		for (auto it = window_map.begin(); it != window_map.end(); ) {
			if (!it->second->running) {
				Stop_MF = false;
				it->second->exit();
				it = window_map.erase(it);  // erase が次のイテレータを返す
			}
			else {
				++it;
			}
		}
	}

	void render() {
		mf.render_obj();
		for (const auto& pair : window_map) {
			pair.second->render_obj();
		}
	}

	void exit() {
		mf.exit();
		IMG_Quit();
		TTF_Quit();
		SDL_Quit();
	}

	void deb_f(std::vector<std::function<void()>>& e_funcs, std::vector<std::function<void()>>& r_funcs) {
		SDL_Event e;
		while (SDL_PollEvent(&e)) {
			if (e.type == SDL_QUIT) { running = false; continue; }
			
			for (auto& func : e_funcs) {
				func();
			}
			
			if (SDL_GetWindowID(mf.win) == e.window.windowID) {
				mf.handler.ev = &e;
				mf.event_K(e);
				if (!mf.running) running = false;
				continue;
			}
			for (const auto& pair : window_map) {
				const auto& frame = pair.second;
				if (SDL_GetWindowID(frame->win) == e.window.windowID) {
					frame->handler.ev = &e;
					frame->event_K(e);
					break; // 一致したら他のフレームを見る必要はない
				}
			}
		}
		// ループの外でまとめて削除
		for (auto it = window_map.begin(); it != window_map.end(); ) {
			if (!it->second->running) {
				it->second->exit();
				it = window_map.erase(it);  // erase が次のイテレータを返す
			}
			else {
				++it;
			}
		}
		mf.renderer.draw_bg({255,255,255,255});
		for (auto& func : r_funcs) {
			func();
		}
		mf.renderer.rend();
	}
	
	void new_dialog_Open(const std::string& ext, std::function<void(std::string s)> f) {
		new_window(800, 600, 800, 600, "File Open");
		Widget_File_Open_u* F_O = window_map["File Open"]->addwidget_t<Widget_File_Open_u>({ 0, 0, 800, 600 }, 1, "File_Open");
		now_Open_dialog.call_widget = "File_Open";
		now_Open_dialog.window_name = "File Open";
		File_Ed* F_Ad = &F_O->F;
		now_Open_dialog.data_p = static_cast<File_Ed*>(F_Ad);
		now_Open_dialog.f = f;
		F_O->F.init_e(ext);
		Stop_MF = true;
	}

	void new_dialog_Save_as(const std::string& ext, std::function<void(std::string s)> f) {
		new_window(800, 600, 800, 600, "File Save");
		Widget_File_Save_u* F_O = window_map["File Save"]->addwidget_t<Widget_File_Save_u>({ 0, 0, 800, 600 }, 1, "File_Save");
		now_Open_dialog.call_widget = "File_Save";
		now_Open_dialog.window_name = "File Save";
		File_Ed* F_Ad = &F_O->F;
		F_Ad->File_extension_Lm = true;
		now_Open_dialog.data_p = static_cast<File_Ed*>(F_Ad);
		now_Open_dialog.f = f;
		F_O->F.init_e(ext);
		Stop_MF = true;
	}

	void new_dialog_Save_as_Simple(const std::string& ext, std::function<void(std::string s)> f, fs::path save_dir) {
		new_window(300, 100, 300, 100, "File Save");
		File_Save_Dialog* F_O = window_map["File Save"]->addwidget_t<File_Save_Dialog>({ 0, 0, 300, 100 }, 1, "File_Save");
		F_O->F.path_box = save_dir;
		now_Open_dialog.call_widget = "File_Save";
		now_Open_dialog.window_name = "File Save";
		File_Ed* F_Ad = &F_O->F;
		F_Ad->File_extension_Lm = true;
		now_Open_dialog.data_p = static_cast<File_Ed*>(F_Ad);
		now_Open_dialog.f = f;
		F_O->F.init_e(ext);
		Stop_MF = true;
	}

	void dialog_EV() {
		if (!now_Open_dialog.window_name.empty()){
			if (window_map.contains(now_Open_dialog.window_name)) {
				window_map[now_Open_dialog.window_name]->Widget_Call(now_Open_dialog.call_widget);
				File_Ed* F = std::any_cast<File_Ed*>(now_Open_dialog.data_p);
				if (F) {
					if (F->relese) {
						now_Open_dialog.f(F->pStr);
						close_window(now_Open_dialog.window_name);
						Stop_MF = false;
					}
				}
			}
		}
	}
};