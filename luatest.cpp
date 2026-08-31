#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "core.h"

int main(int argc, char* argv[]) {
    GameEngine ge;
	AssetLoader AL;
	if (!AL.init()) {
		return 0;
	}
	

    ge.init();

    window_Manager wm;
    wm.win_w = 1000;
    wm.win_h = 625;
    wm.logic_w = 1000;
    wm.logic_h = 625;
    if (!wm.init(argc, argv)) {
        return 1;
    }
    ge.scenes["main"] = scene();
    ge.Now_Scene = &ge.scenes["main"];
	scene& sc = *ge.Now_Scene;
	sc.init(ge.lua);
	ge.lua.script_file("test.lua");
	ge.ks.set_keybind('d', "move_up");

    std::vector<std::function<void()>> ev;
    std::vector<std::function<void()>> rd;

	ev.push_back([&]() {
		ge.eventH(wm.mf.handler);
		});
    rd.push_back([&]() {
		ge.update(wm.mf.renderer);
        });
    rd.push_back([&]() {
		ge.Now_Scene->update_Renderer(wm.mf.renderer);
		});
	dt_timer dt_t;
	dt_t.set_time = 1;

	
	ge.lasttime = SDL_GetTicks();
    while (wm.mf.running) {
		float deltaTime = ge.delta_time();
		
		//printf("Delta Time: %.6f seconds\n", deltaTime);
        ge.lua_update(deltaTime);
		wm.deb_f(ev, rd);
		ge.ks.reset();

		ge.flame_delay();
    }
    wm.exit();
	return 0;
}
