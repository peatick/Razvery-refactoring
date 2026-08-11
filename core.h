#pragma once
#include "skelt_f.h"
#include "sol2/sol.hpp"
#include "comp.h"

class keybord_states {
private:
    struct key_b {
        SDL_Keycode ev;      // event 用（SDL_KEYDOWN）
        SDL_Scancode p;     // polling 用（SDL_GetKeyboardState）
		bool press_U = false;
		bool press_D = false;
		bool pressed = false;
    };
public:

    void set_keybind(SDL_Keycode e_key, std::string fn_name) {
        SDL_Scancode p_key = SDL_GetScancodeFromKey(e_key);
        key_bind[fn_name] = { e_key, p_key };
    }
    std::unordered_map<std::string, key_b> key_bind;
    void eventH(EventHandler& evh) {
		if (evh.ev != nullptr) {
            SDL_Event& e = *evh.ev;
            for (auto& [name, kb] : key_bind) {
                if (e.type == SDL_KEYDOWN && e.key.keysym.sym == kb.ev && e.key.repeat == 0) {
                    kb.press_D = true;
                }
                else if (e.type == SDL_KEYUP && e.key.keysym.sym == kb.ev) {
                    kb.press_U = true;
                }
            }
        }
    }
    void update() {
        const Uint8* state = SDL_GetKeyboardState(NULL);
        for (auto& [name, kb] : key_bind) {
            if (state[kb.p]) {
                kb.pressed = true;
            } else {
                kb.pressed = false;
            }
        }
    }
	void reset() {
		for (auto& [name, kb] : key_bind) {
			kb.press_U = false;
			kb.press_D = false;
		}
	}
};

class dt_timer {
private:

public:
	float set_time = 0.0f;
	bool update(float& delta_time) {
		set_time = set_time - delta_time;
		if (set_time < 0.0f) {
			set_time = 0.0f;
			return true;
        }
		return false;
	}
};

class GameEngine {
private:
    
public:
    SDL_Rect size = { 0, 0, 0, 0 };
    sol::state lua;
    keybord_states ks;

    std::unordered_map<std::string, scene> scenes;
    scene* s;
	sol::function lua_update;

    void init() {
        lua.open_libraries(
			sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table, sol::lib::os,
			sol::lib::bit32, sol::lib::io, sol::lib::coroutine, sol::lib::utf8
        );
        try {
            
            lua.script("print('GameEngine Init!')");
			lua["keyBind"] = [this](SDL_Keycode e_key, std::string fn_name) {
				ks.set_keybind(e_key, fn_name);
			};
			lua["keyPressed"] = [this](std::string fn_name) {
				auto it = ks.key_bind.find(fn_name);
				if (it != ks.key_bind.end()) {
					return it->second.pressed;
				}
				return false;
				};
			lua["keyPressDown"] = [this](std::string fn_name) {
				auto it = ks.key_bind.find(fn_name);
				if (it != ks.key_bind.end()) {
					return it->second.press_D;
				}
				return false;
				};
			lua["keyPressUp"] = [this](std::string fn_name) {
				auto it = ks.key_bind.find(fn_name);
				if (it != ks.key_bind.end()) {
					return it->second.press_U;
				}
				return false;
				};
			lua["str2char"] = [](const std::string& str) {
				return str.c_str();
				};
            lua.script(R"(
                local System = {}
                function Sys_Reg(sys_cync)
                    table.insert(System,sys_cync)
                    print("System Registered!")
                end
                function Update(dt)
                    for i, sys in ipairs(System) do
                        sys(dt)
                    end
                end
            )");
			lua_update = lua["Update"];
        }
        catch (const sol::error& e) {
            std::cout << "Lua error : " << e.what() << std::endl;
        }
    }

    void eventH(EventHandler& evh) {
        SDL_Event& e = *evh.ev;
		ks.eventH(evh);
    }

    void render(Renderer& rend) {
        SDL_SetRenderDrawColor(rend.ren, 50, 50, 50, 255);
        SDL_RenderFillRect(rend.ren, &size);
    }

    void update(Renderer& rend) {
        
        render(rend);
		ks.update();
    }
};
