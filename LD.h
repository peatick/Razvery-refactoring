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

	bool init() {
		if (Cfg.load("Project.MDGW")) {
			projectpath = str2path(Cfg.get("Path", "Project_Current_Path"));
			if (!fs::exists(projectpath)) return false;
			script_ITR();
			fs::create_directory(projectpath / "img");
			fs::create_directory(projectpath / "script");
			fs::create_directory(projectpath / "maps");
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