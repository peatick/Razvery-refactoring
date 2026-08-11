// sdl_render_utils.hpp
// Small helpers to draw mini2d shapes with SDL2's software renderer.
#pragma once
#include "sdl2/include/SDL.h"
#include <cmath>
#include "mini_phys2d.hpp"

namespace sdlrender {

    // World<->screen transform: world (x right, y up) -> screen (x right, y down)
    struct Camera {
        float worldLeft, worldRight, worldBottom, worldTop;
        int screenW, screenH;

        SDL_FPoint toScreen(mini2d::Vec2 p) const {
            float sx = (p.x - worldLeft) / (worldRight - worldLeft) * screenW;
            float sy = screenH - (p.y - worldBottom) / (worldTop - worldBottom) * screenH;
            return { sx, sy };
        }
    };

    inline void drawCircle(SDL_Renderer* r, const Camera& cam, mini2d::Vec2 center, float radius, float angle,
        Uint8 red, Uint8 g, Uint8 b) {
        SDL_FPoint c = cam.toScreen(center);
        float pxRadius = radius / (cam.worldRight - cam.worldLeft) * cam.screenW;
        const int SEG = 28;
        SDL_SetRenderDrawColor(r, red, g, b, 255);
        SDL_FPoint pts[SEG + 1];
        for (int i = 0; i <= SEG; ++i) {
            float t = (float)i / SEG * 2.f * (float)M_PI;
            pts[i] = { c.x + std::cos(t) * pxRadius, c.y + std::sin(t) * pxRadius };
        }
        SDL_RenderDrawLinesF(r, pts, SEG + 1);
        // orientation tick so spin is visible
        SDL_FPoint tick[2] = { c, { c.x + std::cos(-angle) * pxRadius, c.y + std::sin(-angle) * pxRadius } };
        SDL_RenderDrawLinesF(r, tick, 2);
    }

    inline void drawBox(SDL_Renderer* r, const Camera& cam, mini2d::Vec2 center, float hw, float hh, float angle,
        Uint8 red, Uint8 g, Uint8 b) {
        mini2d::Vec2 local[5] = { {hw,hh},{-hw,hh},{-hw,-hh},{hw,-hh},{hw,hh} };
        SDL_FPoint pts[5];
        for (int i = 0; i < 5; ++i) {
            mini2d::Vec2 wp = center + mini2d::rotate(local[i], angle);
            pts[i] = cam.toScreen(wp);
        }
        SDL_SetRenderDrawColor(r, red, g, b, 255);
        SDL_RenderDrawLinesF(r, pts, 5);
    }

    inline void drawFilledBox(SDL_Renderer* r, const Camera& cam, mini2d::Vec2 center, float hw, float hh, float angle,
        Uint8 red, Uint8 g, Uint8 b, Uint8 a = 255) {
        mini2d::Vec2 local[4] = { {hw,hh},{-hw,hh},{-hw,-hh},{hw,-hh} };
        SDL_Vertex verts[4];
        for (int i = 0; i < 4; ++i) {
            mini2d::Vec2 wp = center + mini2d::rotate(local[i], angle);
            SDL_FPoint sp = cam.toScreen(wp);
            verts[i].position = sp;
            verts[i].color = { red, g, b, a };
            verts[i].tex_coord = { 0,0 };
        }
        int idx[6] = { 0,1,2, 0,2,3 };
        SDL_RenderGeometry(r, nullptr, verts, 4, idx, 6);
        drawBox(r, cam, center, hw, hh, angle, 20, 20, 20);
    }

    inline void drawLineWorld(SDL_Renderer* r, const Camera& cam, mini2d::Vec2 a, mini2d::Vec2 b,
        Uint8 red, Uint8 g, Uint8 bl) {
        SDL_FPoint pa = cam.toScreen(a), pb = cam.toScreen(b);
        SDL_SetRenderDrawColor(r, red, g, bl, 255);
        SDL_RenderDrawLineF(r, pa.x, pa.y, pb.x, pb.y);
    }

    // Ground/wall edge, drawn as a long segment along the plane through worldLeft..worldRight (for horizontal-ish
    // normals we sweep the visible box edge instead so vertical walls also render correctly).
    inline void drawEdge(SDL_Renderer* r, const Camera& cam, mini2d::Vec2 normal, float offset,
        Uint8 red, Uint8 g, Uint8 b) {
        mini2d::Vec2 tangent{ -normal.y, normal.x };
        mini2d::Vec2 p0 = normal * offset;
        float span = 20.f;
        drawLineWorld(r, cam, p0 - tangent * span, p0 + tangent * span, red, g, b);
    }

} // namespace sdlrender