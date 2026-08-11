#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "core.h"

int main(int argc, char* argv[]) {
    GameEngine ge;

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
    ge.s = &ge.scenes["main"];
	scene& sc = *ge.s;
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
		ge.s->update_Renderer(wm.mf.renderer);
		});
	dt_timer dt_t;
	dt_t.set_time = 1; // 60 FPS

	Uint32 frameStart, frameTime,lasttime;
	int fps = 60;
	int frameDelay = 1000 / fps;
	lasttime = SDL_GetTicks();
    while (wm.mf.running) {
		frameStart = SDL_GetTicks();
		float deltaTime = (frameStart - lasttime) / 1000.0f; // 秒単位のデルタタイム
		//printf("Delta Time: %.6f seconds\n", deltaTime);
        ge.lua_update(deltaTime);
		wm.deb_f(ev, rd);
		ge.ks.reset();
        frameTime = SDL_GetTicks() - frameStart;
		lasttime = SDL_GetTicks();
		if (frameTime < frameDelay) {
			SDL_Delay(frameDelay - frameTime);
			//std::cout << "Frame Time: " << frameTime << " ms, Delayed for: " << (frameDelay - frameTime) << " ms" << std::endl;
		}
    }
    wm.exit();
}
