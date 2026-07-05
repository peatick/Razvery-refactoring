#pragma once
#include "sdlutil.h"
#include "widget.h"
#include "sdl2/include/SDL.h"
#include <vector>
#include <memory>
#include <cmath>
#include <cstdio>
#include <algorithm>

class PaintApp {
public:
    int W = 1000, H = 800;
    static constexpr int UNDO_MAX = 64;
    SDL_Rect size{ 100, 100, 100, 100 };

    void setSize(int w, int h, SDL_Renderer* ren) {
        W = w; H = h;
        tex.reset(SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, W, H));

        canvas.assign(W * H, 0xFFFFFFFFu);
        pushUndo();
    }

    void handleEvent(const SDL_Event& ev) {
        switch (ev.type) {
        case SDL_KEYDOWN: handleKey(ev.key.keysym.sym); break;
        case SDL_KEYUP: handleKeyUp(ev.key.keysym.sym); break;
        case SDL_MOUSEBUTTONDOWN: mouseDown(ev.button); break;
        case SDL_MOUSEBUTTONUP: mouseUp(ev.button); break;
        case SDL_MOUSEMOTION: mouseMove(ev.motion); break;
        case SDL_MOUSEWHEEL: mouseWheel(ev.wheel); break;
        }
    }
    void skip_ev() {
        drawing = erasing = false;
        lastX = lastY = -1;
        panL = panR = panU = panD = false;
    }

    /* ---------- Render ---------- */
    Uint32 coltoUint32(SDL_Color col) {
        return (Uint32(col.a * 255) << 24) | (Uint32(col.r * 255) << 16) | (Uint32(col.g * 255) << 8) | Uint32(col.b * 255);
    }

    Uint32 getColor() {
		return canvas[lastY * W + lastX];
    }

    void render(SDL_Renderer* ren) {
        SDL_RenderSetClipRect(ren, &size);
        void* pix; int pitch;
        SDL_LockTexture(tex.get(), nullptr, &pix, &pitch);

        for (int y = 0; y < H; y++)
            memcpy((Uint8*)pix + y * pitch, canvas.data() + y * W, W * 4);

        SDL_UnlockTexture(tex.get());

        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);

        SDL_FRect dst{ -camX * zoom, -camY * zoom, W * zoom, H * zoom };
        SDL_RenderCopyF(ren, tex.get(), nullptr, &dst);
        SDL_RenderSetClipRect(ren, nullptr);
    }
private:
    struct SDL_Deleter {
        void operator()(SDL_Texture* p) const { if (p) SDL_DestroyTexture(p); }
        void operator()(SDL_Surface* p) const { if (p) SDL_FreeSurface(p); }
    };

    std::unique_ptr<SDL_Texture, SDL_Deleter> tex;

    std::vector<Uint32> canvas;
    std::vector<std::vector<Uint32>> undo;
    int undoTop = -1, undoEnd = -1, undoBase = 0;

    float zoom = 1.0f, camX = 0, camY = 0;
    int brush = 8;
    Uint32 color = 0xFF000000u;
    Uint32 bg = 0xFFFFFFFFu;

    bool drawing = false, erasing = false;
    int lastX = -1, lastY = -1;

    bool panL = false, panR = false, panU = false, panD = false;

    /* ---------- Undo ---------- */
    void pushUndo() {
        undoTop = (undoTop + 1) % UNDO_MAX;
        if (undo.size() < UNDO_MAX) undo.resize(UNDO_MAX);
        undo[undoTop] = canvas;
        undoEnd = undoTop;
        if (undoTop == undoBase && undoEnd != undoBase)
            undoBase = (undoBase + 1) % UNDO_MAX;
    }

    void undoAction() {
        if (undoTop == undoBase) return;
        canvas = undo[undoTop];
        undoTop = (undoTop - 1 + UNDO_MAX) % UNDO_MAX;
    }

    void redoAction() {
        if (undoTop == undoEnd) return;
        undoTop = (undoTop + 1) % UNDO_MAX;
        canvas = undo[undoTop];
    }

    /* ---------- Drawing ---------- */
    void paintPixel(int x, int y, Uint32 col) {
        if (x >= 0 && x < W && y >= 0 && y < H)
            canvas[y * W + x] = col;
    }

    void paintCircle(int cx, int cy) {
        for (int y = -brush; y <= brush; y++)
            for (int x = -brush; x <= brush; x++)
                if (x * x + y * y <= brush * brush)
                    paintPixel(cx + x, cy + y, drawing ? color : bg);
    }

    void paintLine(int x0, int y0, int x1, int y1) {
        int dx = abs(x1 - x0), dy = abs(y1 - y0);
        int sx = x0 < x1 ? 1 : -1;
        int sy = y0 < y1 ? 1 : -1;
        int err = dx - dy;

        while (true) {
            paintCircle(x0, y0);
            if (x0 == x1 && y0 == y1) break;
            int e2 = err * 2;
            if (e2 > -dy) { err -= dy; x0 += sx; }
            if (e2 < dx) { err += dx; y0 += sy; }
        }
    }

    /* ---------- Event ---------- */

    void handleKey(SDL_Keycode k) {
        const Uint8* st = SDL_GetKeyboardState(nullptr);
        bool ctrl = st[SDL_SCANCODE_LCTRL] || st[SDL_SCANCODE_RCTRL];
        bool shift = st[SDL_SCANCODE_LSHIFT] || st[SDL_SCANCODE_RSHIFT];

        if (k == SDLK_ESCAPE || k == SDLK_q) SDL_Event ev{ SDL_QUIT };

        if (k == SDLK_z && ctrl) shift ? redoAction() : undoAction();
        if (k == SDLK_y && ctrl) redoAction();

        if (k == SDLK_LEFTBRACKET) brush = std::max(1, brush - 2);
        if (k == SDLK_RIGHTBRACKET) brush = std::min(80, brush + 2);

        if (k == SDLK_LEFT) panL = true;
        if (k == SDLK_RIGHT) panR = true;
        if (k == SDLK_UP) panU = true;
        if (k == SDLK_DOWN) panD = true;
    }
    void handleKeyUp(SDL_Keycode k) {
        if (k == SDLK_LEFT) panL = false;
        if (k == SDLK_RIGHT) panR = false;
        if (k == SDLK_UP) panU = false;
        if (k == SDLK_DOWN) panD = false;
    }
    void mouseDown(const SDL_MouseButtonEvent& b) {
        int cx = camX + b.x / zoom;
        int cy = camY + b.y / zoom;

        pushUndo();
        drawing = (b.button == SDL_BUTTON_LEFT);
        erasing = (b.button == SDL_BUTTON_RIGHT);

        paintCircle(cx, cy);
        lastX = cx; lastY = cy;
    }
    void mouseUp(const SDL_MouseButtonEvent&) {
        drawing = erasing = false;
        lastX = lastY = -1;
    }
    void mouseMove(const SDL_MouseMotionEvent& m) {
        if (!drawing && !erasing) return;
        int cx = camX + m.x / zoom;
        int cy = camY + m.y / zoom;

        if (lastX >= 0) paintLine(lastX, lastY, cx, cy);
        else paintCircle(cx, cy);

        lastX = cx; lastY = cy;
    }
    void mouseWheel(const SDL_MouseWheelEvent& w) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        float cx = camX + mx / zoom;
        float cy = camY + my / zoom;

        float factor = (w.y > 0) ? 1.15f : 1.0f / 1.15f;
        zoom = std::clamp(zoom * factor, 0.05f, 64.0f);

        camX = cx - mx / zoom;
        camY = cy - my / zoom;
    }

    /* ---------- Pan ---------- */
    void pan() {
        float step = 10.0f / zoom;
        if (panL) camX -= step;
        if (panR) camX += step;
        if (panU) camY -= step;
        if (panD) camY += step;
    }

    /* ---------- Save ---------- */
    void saveBMP() {
        std::unique_ptr<SDL_Surface, SDL_Deleter> surf(
            SDL_CreateRGBSurfaceWithFormat(0, W, H, 32, SDL_PIXELFORMAT_ARGB8888));
        memcpy(surf->pixels, canvas.data(), canvas.size() * 4);
        SDL_SaveBMP(surf.get(), "output.bmp");
        printf("Saved output.bmp\n");
    }

};