#include "sdl2/include/SDL.h"
#include "mini_phys2d.hpp"
#include "sdl_render_utils.h"
#include <vector>
#include <memory>

using namespace mini2d;

struct Demo4Scene {
    std::unique_ptr<World2D> world;
    std::vector<Body2D> planks;
    Body2D wallL{}, wallR{};
    std::vector<Body2D> boulders;
    bool boulderSpawned = false;

    static constexpr int PLANKS = 8;
    static constexpr float SEG = 0.8f, HW = SEG * 0.45f, HH = 0.1f, Y0 = 4.f;
    static constexpr float K = 60.f, D = 12.f;

    void reset() {
        world = std::make_unique<World2D>();
        world->raw().config().enableSleeping = false;
        world->addStatic(shape2DEdge({ 0,1 }, 0.f), { 0,0 }); // ground

        wallL = world->addStatic(shape2DBox(0.15f, 1.5f), { -3.6f,Y0 });
        wallR = world->addStatic(shape2DBox(0.15f, 1.5f), { 3.6f,Y0 });

        planks.clear();
        for (int i = 0; i < PLANKS; ++i)
            planks.push_back(world->addDynamic(
                shape2DBox(HW, HH), { -2.8f + i * SEG, Y0 }, 0.f, 0.4f, 0.02f, 0.6f));

        world->addSpring(wallL, planks.front(), { 0,0 }, { -HW,0 }, 0.4f, K, D);
        for (int i = 0; i + 1 < PLANKS; ++i)
            world->addSpring(planks[i], planks[i + 1], { HW,0 }, { -HW,0 }, 0.05f, K, D);
        world->addSpring(planks.back(), wallR, { HW,0 }, { 0,0 }, 0.4f, K, D);

        boulders.clear();
        boulderSpawned = false;
    }

    void spawnBoulder() {
        boulders.push_back(world->addDynamic(shape2DCircle(0.4f), { 0.f,6.5f }, 0.f, 1.f, 0.3f, 0.5f));
    }

    void step(float dt, int substeps) {
        for (int i = 0; i < substeps; ++i) {
            world->step(dt);
        }
    }
};

int main(int, char**) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        SDL_Log("SDL_Init failed: %s", SDL_GetError());
        return 1;
    }
    const int W = 960, H = 640;
    SDL_Window* win = SDL_CreateWindow("phys3d demo4: spring bridge  [R=reset, SPACE=drop boulder]",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, W, H, SDL_WINDOW_SHOWN);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    sdlrender::Camera cam{ -5.f, 5.f, -1.f, 9.f, W, H };

    Demo4Scene scene;
    scene.reset();

    bool running = true;
    Uint64 last = SDL_GetPerformanceCounter();
    const float PHYS_DT = 1.f / 60.f;
    float accumulator = 0.f;

    while (running) {
        SDL_Event ev;
        while (SDL_PollEvent(&ev)) {
            if (ev.type == SDL_QUIT) running = false;
            if (ev.type == SDL_KEYDOWN) {
                if (ev.key.keysym.sym == SDLK_ESCAPE) running = false;
                if (ev.key.keysym.sym == SDLK_r) scene.reset();
                if (ev.key.keysym.sym == SDLK_SPACE) scene.spawnBoulder();
            }
        }

        Uint64 now = SDL_GetPerformanceCounter();
        float frameDt = (float)((now - last) / (double)SDL_GetPerformanceFrequency());
        last = now;
        frameDt = std::min(frameDt, 0.05f);
        accumulator += frameDt;
        int steps = 0;
        while (accumulator >= PHYS_DT && steps < 8) {
            scene.step(PHYS_DT, 1);
            accumulator -= PHYS_DT;
            ++steps;
        }

        SDL_SetRenderDrawColor(ren, 18, 18, 24, 255);
        SDL_RenderClear(ren);

        sdlrender::drawEdge(ren, cam, { 0,1 }, 0.f, 90, 90, 100);

        // walls
        sdlrender::drawFilledBox(ren, cam, scene.world->getPosition(scene.wallL), 0.15f, 1.5f, 0, 130, 130, 140);
        sdlrender::drawFilledBox(ren, cam, scene.world->getPosition(scene.wallR), 0.15f, 1.5f, 0, 130, 130, 140);

        // spring lines between planks/walls (visualize tension)
        auto plankPos = [&](int idx) { return scene.world->getPosition(scene.planks[idx]); };
        Vec2 wl = scene.world->getPosition(scene.wallL) + Vec2{ 0,0 };
        Vec2 wr = scene.world->getPosition(scene.wallR) + Vec2{ 0,0 };
        sdlrender::drawLineWorld(ren, cam, wl, plankPos(0) + Vec2{ -Demo4Scene::HW,0 }, 200, 200, 90);
        for (int i = 0; i + 1 < Demo4Scene::PLANKS; ++i) {
            Vec2 a = plankPos(i) + Vec2{ Demo4Scene::HW,0 };
            Vec2 b = plankPos(i + 1) + Vec2{ -Demo4Scene::HW,0 };
            sdlrender::drawLineWorld(ren, cam, a, b, 200, 200, 90);
        }
        sdlrender::drawLineWorld(ren, cam, plankPos(Demo4Scene::PLANKS - 1) + Vec2{ Demo4Scene::HW,0 }, wr, 200, 200, 90);

        // planks
        for (auto& p : scene.planks) {
            Vec2 pos = scene.world->getPosition(p);
            float ang = scene.world->getAngle(p);
            sdlrender::drawFilledBox(ren, cam, pos, Demo4Scene::HW, Demo4Scene::HH, ang, 150, 100, 60);
        }

        // boulders
        for (auto& b : scene.boulders) {
            Vec2 pos = scene.world->getPosition(b);
            float ang = scene.world->getAngle(b);
            sdlrender::drawCircle(ren, cam, pos, 0.3f, ang, 200, 200, 210);
        }

        SDL_RenderPresent(ren);
    }

    SDL_DestroyRenderer(ren);
    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}