#pragma once
#include "skelt_f.h"
#include "comp.h"
#include <cstdlib>

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

    int now_key_code = 0;
	void set_keybinds(std::unordered_map<std::string, SDL_Keycode> binds) {
		for (const auto& [fn_name, e_key] : binds) {
			set_keybind(e_key, fn_name);
		}
	}

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
            if (e.type == SDL_KEYDOWN) {
                now_key_code = e.key.keysym.sym;
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
	SDL_Renderer* renderer = nullptr;
    Uint32 frameStart, frameTime, lasttime;
    int fps = 60;
    int frameDelay = 1000 / fps;
	bool crash = false;
	std::string error_msg = "";
	std::string script_path = "script/";
	std::string map_path = "map/";
	std::string img_path = "img/";
    static int my_panic(lua_State* L) {
        auto msg = sol::stack::unqualified_check_get<std::string>(L, -1);
        std::cerr << "Lua がパニックしました。エラーメッセージ: "
            << (msg ? *msg : "不明なエラー")
            << std::endl;
        return 0;
    }

    float delta_time() {
        frameStart = SDL_GetTicks();
        float deltaTime = (frameStart - lasttime) / 1000.0f; // 秒単位のデルタタイム
        return deltaTime;
    }
    void flame_delay() {
        frameTime = SDL_GetTicks() - frameStart;
        lasttime = SDL_GetTicks();
        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);
            //std::cout << "Frame Time: " << frameTime << " ms, Delayed for: " << (frameDelay - frameTime) << " ms" << std::endl;
        }
    }

    SDL_Rect size = { 0, 0, 0, 0 };
    sol::state lua{ sol::c_call<decltype(&my_panic), &my_panic> };
    keybord_states ks;
    std::unordered_map<std::string, scene> scenes;
    scene* Now_Scene = nullptr;


	void new_scene(std::string name, std::string lua_sc, sol::table payload = sol::nil) {
        std::cout << "Creating new scene: " << name << std::endl;
        std::cout << "img_path: " << img_path << std::endl;
		sol::environment env(lua, sol::create, lua.globals());
		scenes[name] = scene{};
		Now_Scene = &scenes[name];
        Now_Scene->img_path = img_path;
        Now_Scene->map_path = map_path;
        Now_Scene->renderer = renderer;
        Now_Scene->init(lua, env);
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
            )", env);
        try {
            lua.script_file(lua_sc, env);
		}
		catch (const sol::error& e) {
			std::cout << "Lua error : " << e.what() << std::endl;
			crash = true;
			error_msg = e.what();
		}
        Now_Scene->lua_update_func = env["Update"];
        Now_Scene->lua_env = env;
        if (payload.valid()) {
            Now_Scene->lua_env["payload"] = payload;
        }


	}
	void update_scene(float dt) {
        sol::protected_function_result result;
		if (Now_Scene != nullptr) {
			try {
				result = Now_Scene->lua_update_func(dt);
				if (!result.valid()) {
					sol::error err = result;
                    error_msg = err.what();
					crash = true;
				}
			}
			catch (const sol::error& e) {
				std::cout << "Lua error : " << e.what() << std::endl;
				crash = true;
                error_msg = e.what();
			}
		}
	}
    
    void init() {
		lua = sol::state{};
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
            lua["new_scene"] = [this](std::string name, std::string lua_sc, sol::table payload) {
				std::cout << "Creating new scene from Lua: " << name << std::endl;
				new_scene(name, script_path + "/" + lua_sc, payload);
				};
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

