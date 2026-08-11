#pragma once
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"
#include "BTNS_V2.h"
 


class File_Ed {
private:
	SDL_Rect File_act_btn;
	std::string act = "Open";
	SDL_Texture* exten_W = nullptr;
	SDL_Point start_P = { 0,0 };
protected:
	Editor File_name_ed;
	Renderer* rendererD = nullptr;
	WD_Btn act_btn;
	bool equals_ignore_case(const std::string& a, const std::string& b) {
		if (a.length() != b.length()) return false;
		return std::equal(a.begin(), a.end(), b.begin(), [](unsigned char c1, unsigned char c2) {
			return std::tolower(c1) == std::tolower(c2);
			});
	}
	std::string path2string_s(const fs::path& p) {
		std::string str;
		std::u8string u8temp = p.u8string();
		str = std::string(reinterpret_cast<const char*>(u8temp.c_str()));
		return str;
	}
public:
    File_explorer explorer;
	std::string File_extension = "dir";
	bool File_extension_Lm = false;
	bool relese = false;
	std::string pStr;

	virtual void file_exits_True(fs::path FilePath) {

	}
	virtual void file_exits_False(fs::path FilePath) {

	}

	void init(Renderer& renderer, std::string action = "Open") {
		act = action;
		SDL_Rect rec = { 0, explorer.size.h - 40, explorer.size.w, 40 };
		File_act_btn = { rec.x + 50 + (rec.w / 3) * 2 + 10 , rec.y, 70, 20 };
		File_name_ed.set_init({ rec.x + 50, rec.y, (rec.w / 3) * 2 - 10, 20 }, "", renderer.lineH);
		File_name_ed.PADDING = 5;
		File_name_ed.noLineNo = true;
		act_btn.init(renderer, act, File_act_btn, false);
		start_P = { rec.x + 50 + (rec.w / 3) * 2 + 10, rec.y - 15};
		rendererD = &renderer;
	}

	virtual void init_e(std::string ex) {
		File_extension = ex;
		exten_W = rendererD->text_texture(ex);
	}

	virtual void HandleEvent(EventHandler& ev_h) {
		ev_h.File_explorer_Event(explorer);
		ev_h.textEditEvent_sh(File_name_ed, true);
		act_btn.handleEvent(ev_h);
	}
	virtual void update() {
		if (explorer.selected_file_c2) {
			if (File_extension != "dir" && File_extension != "img") {
				explorer.selected_file_c2 = false; // Reset the flag
				// Handle the case when a file is selected
				fs::path tmp = explorer.selected_file_path.extension();
				std::string ex;
				explorer.filename2string(tmp, ex);

				if ((equals_ignore_case(ex, File_extension) || File_extension_Lm) && !fs::is_directory(explorer.selected_file_path)) {
					std::string File_name;
					fs::path s_path = explorer.selected_file_path.filename();
					explorer.filename2string(s_path, File_name);
					File_name_ed.buf.setAllText(File_name);
				}
			}
			else if (File_extension == "dir") {
				if (fs::is_directory(explorer.selected_file_path)){
					std::string File_name;
					fs::path s_path = explorer.selected_file_path.filename();
					explorer.filename2string(s_path, File_name);
					File_name_ed.buf.setAllText(File_name);
				}
			}
			else if (File_extension == "img") {
				std::vector<std::string> exs = {
					".png",".jpg",".jpeg",".bmp",
					".webp",".gif",".tif",".tiff",
					".tgf",".tga",".pcx",".ppm",
					".pbm",".pgm",".ppm",".xpm",
					".xcf"};
				fs::path tmp = explorer.selected_file_path.extension();
				std::string ex;
				explorer.filename2string(tmp, ex);
				for (auto& s : exs) {
					if ((equals_ignore_case(ex, s) || File_extension_Lm) && !fs::is_directory(explorer.selected_file_path)) {
						std::string File_name;
						fs::path s_path = explorer.selected_file_path.filename();
						explorer.filename2string(s_path, File_name);
						File_name_ed.buf.setAllText(File_name);
						break;
					}
				}
			}
		}
		if (act_btn.Active) {
			std::string fn = File_name_ed.buf.line(0);
			if (!fn.empty()) {
				std::u8string file_u8 = reinterpret_cast<const char8_t*>(fn.c_str());
				fs::path file_n;
				explorer.u8string_to_path(file_u8, file_n);
				fs::path selected_path = explorer.path_box / file_n;
				if (fs::exists(selected_path)) {
					file_exits_True(selected_path);
				}
				else {
					// ファイルが存在しない場合の処理
					file_exits_False(selected_path);
				}
			}
		}
	}
	virtual void render(Renderer& renderer) {
		explorer.tickupdate();
		renderer.drw_file_explorer(explorer);
		renderer.TextBoxsh(File_name_ed);
		act_btn.render(renderer);
		renderer.drawtexture(exten_W, start_P.x, start_P.y);
		update();
	}
	virtual void destroy() {
		act_btn.destroy(*rendererD);
		SDL_DestroyTexture(exten_W);

	}
};

class File_Open : public File_Ed{
public:
	void file_exits_True(fs::path FilePath) override {
		relese = true;
		pStr = path2string_s(FilePath);
	}
};

class File_Save_as : public File_Ed {
private:
	bool EX_eq(fs::path p) {
		std::string ps = path2string_s(p.extension());
		if (!ps.empty() && !File_extension.empty()) {
			return equals_ignore_case(ps, File_extension);
		}
		return false;
	}
	bool isValidFilename(const fs::path& test_path) {
		std::error_code ec;

		if (fs::exists(test_path)) return true;

		// 1. テスト用の出力ストリームを開く（作成を試みる）
		std::ofstream ofs(test_path, std::ios::out | std::ios::trunc);

		// 開けなかった場合は不正文字、権限エラー、パス長超過など
		if (!ofs.is_open()) {
			return false;
		}

		// 2. 作成に成功したらストリームを閉じる
		ofs.close();

		// 3. テストで作ったゴミファイルを確実に削除する
		fs::remove(test_path, ec);

		return true;
	}

	std::string warm_msg = "Do you want to overwrite the existing file?";
	SDL_Texture* warm_tex;
	bool overwrite_arrw = false;
	std::string temp_path_OW;
	bool ov_fal = false;
	WD_Btn MkDir_B;
public:
	
	void init_e(std::string ex) override {
		File_Ed::init_e(ex);
		warm_tex = rendererD->text_texture(warm_msg);
		SDL_Rect rec = { 0, explorer.size.h - 40, explorer.size.w, 40 };
		MkDir_B.init(*rendererD, "+Dir", { rec.x + 50 + (rec.w / 3) * 2 + 90 , rec.y, 70, 20 }, false);
	}
	void file_exits_True(fs::path FilePath) override {
		pStr = path2string_s(FilePath);
		if (!EX_eq(FilePath)) {
			pStr += File_extension;
		}
		file_save_p();
	}
	void file_exits_False(fs::path FilePath) override {
		pStr = path2string_s(FilePath);
		if (!EX_eq(FilePath)) {
			pStr += File_extension;
		}
		file_save_p();
	}
	void file_save_p() {
		std::u8string u8pStr = reinterpret_cast<const char8_t*>(pStr.c_str());
		fs::path p;

		if (pStr != temp_path_OW) {
			overwrite_arrw = false;
			ov_fal = false;
		}

		explorer.u8string_to_path(u8pStr, p);
		if (!isValidFilename(p)) {
			return;
		}
		if (!fs::exists(p)) {
			relese = true;
			return;
		}
		else {
			if (ov_fal && act_btn.Active) {
				relese = true;
				return;
			}
			overwrite_arrw = true;
			temp_path_OW = pStr;
		}
	}


	void HandleEvent(EventHandler& ev_h) override {
		File_Ed::HandleEvent(ev_h);
		MkDir_B.handleEvent(ev_h);
	}
	void update() override {
		File_Ed::update();
		if (overwrite_arrw && !act_btn.Active) {
			ov_fal = true;
		}
		if (MkDir_B.Active) {
			std::string name = File_name_ed.buf.line(0);
			if (!name.empty()) {
				fs::path name_p = str2path(name);
				fs::path make_dir_path = explorer.path_box / name_p;
				if (fs::create_directory(make_dir_path)) {
					
				}
			}
		}
	}
	void render(Renderer& renderer) override {
		File_Ed::render(renderer);
		if (overwrite_arrw){
			renderer.drawtexture(warm_tex, 50, explorer.size.h - 20);
		}
		MkDir_B.render(renderer);
	}
	void destroy() override {
		File_Ed::destroy();
		MkDir_B.destroy(*rendererD);
		SDL_DestroyTexture(warm_tex);
	}
};