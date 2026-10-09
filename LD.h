#pragma once
#include "Events.h"
#include "LuaLexerSet.h"
#include "config_reader.h"
#include <sstream>
#include <fstream>

class AssetLoader {
private:
	bool path_eq(const std::string& a, const std::string& b) {
		fs::path p_a = str2path(a);
		fs::path p_b = str2path(b);
		return fs::weakly_canonical(p_a) == fs::weakly_canonical(p_b);
	}
public:
	fs::path projectpath = "project";
	fs::path scriptpath = "script";
	fs::path mapspath = "maps";
	fs::path imgpath = "img";

	std::unordered_map<std::string, std::string> scripts;
	
	lualex::LuaLexerSet Lua_src_set;

	Config Cfg;
	std::unordered_map<std::string, SDL_Keycode> binds;

	SDL_Keycode toKeycode(const std::string& s) {
		// 1文字なら ASCII → SDL_Keycode（SDLK_a など）にそのまま使える
		if (s.size() == 1) {
			char c = s[0];

			// 大文字なら小文字に変換（SDL は a〜z の ASCII を使う）
			if ('A' <= c && c <= 'Z') {
				c = c - 'A' + 'a';
			}

			// 英字ならそのまま SDL_Keycode として使える
			if ('a' <= c && c <= 'z') {
				return c;  // SDLK_a〜SDLK_z と同じ値
			}
		}

		// 特殊キーの分岐
		if (s == "space") return SDLK_SPACE;
		if (s == "tab")   return SDLK_TAB;
		if (s == "alt")   return SDLK_LALT;   // 右ALTなら SDLK_RALT
		if (s == "ctrl")  return SDLK_LCTRL;  // 右CTRLなら SDLK_RCTRL
		if (s == "shift") return SDLK_LSHIFT; // 右SHIFTなら SDLK_RSHIFT

		return SDLK_UNKNOWN;
	}


	void load_binds() {
		std::unordered_map<std::string, std::string> tm = Cfg.get_section("Key_Binds");
		for (const auto& b : tm) {
			binds[b.first] = toKeycode(b.second);
		}
	}

	bool init() {
		if (Cfg.load("Project.MDGW")) {
			projectpath = str2path(Cfg.get("Path", "Project_Current_Path"));
			if (!fs::exists(projectpath)) return false;
			script_ITR();
			fs::create_directory(projectpath / "img");
			fs::create_directory(projectpath / "script");
			fs::create_directory(projectpath / "maps");

			load_binds();

			return true;
		}

		return false;
	}

	void NewProject(std::string& strpath) {
		std::ofstream ofs("Project.MDGW", std::ios::trunc);
		if (!ofs) return;
		ofs << "[Path]" << std::endl;
		ofs << "Project_Current_Path = " << strpath << std::endl;
		ofs.close();

		init();
	}

	bool script_ITR() {
		if (!fs::exists(projectpath / scriptpath)) return false;
		scripts.clear();
		Lua_src_set.reset();
		for (auto& e : fs::recursive_directory_iterator(projectpath / scriptpath)) {
			if (e.is_regular_file() && e.path().extension() == ".lua") {
				std::ifstream ifs(e.path());
				if (!ifs) continue;
				std::stringstream ss;
				ss << ifs.rdbuf();
				std::string filestr = ss.str();
				std::string path_str_key = e.path().string();
				scripts[path_str_key] = filestr;
				Lua_src_set.set(path_str_key, scripts[path_str_key]);
			}
		}
		std::string template_script = R"(
		function keyPressed(str)		
		function keyPressDown(str)
		function keyPressUp(str)
		function Sys_Reg(func)
		function create_entity()
		function add_newCom(entity_id, data, com_type)
		function add_newLuaCom(entity_id, data, com_type)
		function for_each(com_name_table, callback_func)
		function get_entity_with(...)
		function move_cam(x, y)
		function camera_pos()
		function new_scene(scene_name, lua_script, payload)
		function load_map(map_name, img_name)
		function resove_moveX(new_x, dx, dy, size)
		function resove_moveY(new_y, dx, dy, size)
		function get_lua_com(entity_id, com_type)
		function load_sprite(sprite_name, img_name)
		function draw_sprite(sprite_name, x, y, w, h)
		function remove_entity(entity_id)
		)";
		Lua_src_set.set("template_script", template_script);
		return true;
	}

	void debug_sc() {
		for (auto& txta : scripts) {
			std::cout << txta.first << std::endl;
			std::cout << txta.second << std::endl;
		}
	}
	std::string* script_str(const std::string& s) {
		for (auto& [path, content] : scripts) { // C++17 構造化束縛
			if (path_eq(s, path)) {
				return &content;
			}
		}
		return nullptr;
	}
	LuaKey_Lex LuaLex_Scr_shr(const std::string& s) {
		for (auto& [path, content] : scripts) { // C++17 構造化束縛
			if (path_eq(s, path)) {
				LuaKey_Lex Lx;
				Lx.LLSet = &Lua_src_set;
				Lx.LL = Lua_src_set.find(path);
				Lx.srcs = &scripts;
				Lx.key = path;

				return Lx;
			}
		}
		LuaKey_Lex Lx;
		return Lx;
	}

	void LuaLex_update() {
		for (const auto& f : scripts) {
			Lua_src_set.set(f.first, f.second);
		}
		
	}
};