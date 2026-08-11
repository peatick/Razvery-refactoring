#pragma once
#include "sdlutil.h"
#include "widget.h"
#include <vector>
#include <memory>
#include <cmath>
#include <cstdio>
#include <algorithm>

class PaintApp {
public:
    int W = 1000, H = 800;
    static constexpr int UNDO_MAX = 64;
    int brush = 8;
    int brush_size_max = 80, burush_size_min = 1;

    void setSize(int w, int h, SDL_Renderer* ren) {
        W = w; H = h;
        tex.reset(SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_STREAMING, W, H));

        SDL_SetTextureBlendMode(tex.get(), SDL_BLENDMODE_BLEND);

        canvas.assign(W * H, 0xFFFFFFFFu);
        canvasDirty = true;
        pushUndo();
    }

    // ウィジェット（表示枠）の位置・サイズをまとめて設定する唯一の入口。
    // size を直接書き換えると render() 側の計算前提が崩れるので、
    // 必ずこの関数経由で更新すること。
    void setRect(int x, int y, int w, int h) {
        size = SDL_Rect{ x, y, w, h };
    }
    SDL_Rect getRect() const { return size; }

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
        drawing = erasing = picking = false;
        lastX = lastY = -1;
        panL = panR = panU = panD = false;
    }

    /* ---------- Render ---------- */
    Uint32 coltoUint32(SDL_Color col) {
        return (Uint32(col.a) << 24) | (Uint32(col.r) << 16) | (Uint32(col.g) << 8) | Uint32(col.b);
    }
    SDL_Color uint32ToCol(Uint32 v) {
        SDL_Color c;
        c.a = (v >> 24) & 0xFF;
        c.r = (v >> 16) & 0xFF;
        c.g = (v >> 8)  & 0xFF;
        c.b =  v        & 0xFF;
        return c;
    }
    void setColor(SDL_Color col) {
        color = coltoUint32(col);
    }
    SDL_Color getCurrentColor() {
        return uint32ToCol(color);
    }

    /* ---------- Pan ---------- */
    void pan() {
        float step = 2.0f / zoom;
        if (panL) camX -= step;
        if (panR) camX += step;
        if (panU) camY -= step;
        if (panD) camY += step;
    }

    void render(SDL_Renderer* ren) {
        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
		SDL_RenderFillRect(ren, &size);
        if (canvasDirty) {
            SDL_UpdateTexture(tex.get(), nullptr, canvas.data(), W * 4);
            canvasDirty = false;
        }

        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 200, 200, 200, 255);

        // ウィジェット位置(size.x, size.y)を基準点として、
        // そこからカメラ・ズームぶんだけずらした位置にキャンバスを描く。
        float dstFullX = size.x - camX * zoom;
        float dstFullY = size.y - camY * zoom;
        float dstFullW = W * zoom;
        float dstFullH = H * zoom;

        float visX0 = std::max(dstFullX, (float)size.x);
        float visY0 = std::max(dstFullY, (float)size.y);
        float visX1 = std::min(dstFullX + dstFullW, (float)(size.x + size.w));
        float visY1 = std::min(dstFullY + dstFullH, (float)(size.y + size.h));

        if (visX1 <= visX0 || visY1 <= visY0)
            return;

        float srcX0 = (visX0 - dstFullX) / zoom;
        float srcY0 = (visY0 - dstFullY) / zoom;
        float srcX1 = (visX1 - dstFullX) / zoom;
        float srcY1 = (visY1 - dstFullY) / zoom;

        SDL_Rect dst{
            (int)std::lround(visX0), (int)std::lround(visY0),
            (int)std::lround(visX1 - visX0), (int)std::lround(visY1 - visY0)
        };
        SDL_Rect src{
            (int)std::lround(srcX0), (int)std::lround(srcY0),
            (int)std::lround(srcX1 - srcX0), (int)std::lround(srcY1 - srcY0)
        };

        SDL_RenderSetClipRect(ren, &size);
        if (dst.w > 0 && dst.h > 0 && src.w > 0 && src.h > 0)
            SDL_RenderCopy(ren, tex.get(), &src, &dst);

        SDL_Rect canvasFullRect{
            (int)std::lround(dstFullX), (int)std::lround(dstFullY),
            (int)std::lround(dstFullW), (int)std::lround(dstFullH)
        };
        drawCanvasBorder(ren, canvasFullRect);

        SDL_RenderSetClipRect(ren, nullptr);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
    }
    bool col_d = false;
private:
    struct SDL_Deleter {
        void operator()(SDL_Texture* p) const { if (p) SDL_DestroyTexture(p); }
        void operator()(SDL_Surface* p) const { if (p) SDL_FreeSurface(p); }
    };

    std::unique_ptr<SDL_Texture, SDL_Deleter> tex;

    std::vector<Uint32> canvas;
    std::vector<std::vector<Uint32>> undo;
    int undoTop = -1, undoEnd = -1, undoBase = 0;

    // ウィジェットの位置・サイズ。直接触らず setRect() 経由で変更すること。
    SDL_Rect size{ 100, 100, 100, 100 };

    float zoom = 1.0f, camX = 0, camY = 0;

    Uint32 color = 0xFF000000u;
    Uint32 bg = 0xFFFFFFFFu;

    bool canvasDirty = true;

    bool drawing = false, erasing = false, picking = false;
    int lastX = -1, lastY = -1;

    bool panL = false, panR = false, panU = false, panD = false;

    /* ---------- 座標変換 ---------- */
    // ウィジェット位置(size.x, size.y)を原点オフセットとして加える。
    // これにより setRect() でウィジェットをどこに動かしても
    // 表示位置とクリック判定位置が一致する。
    int screenOriginX() const { return size.x + (int)std::floor(-camX * zoom); }
    int screenOriginY() const { return size.y + (int)std::floor(-camY * zoom); }

    int screenToCanvasX(int sx) const { return (int)std::floor((sx - screenOriginX()) / zoom); }
    int screenToCanvasY(int sy) const { return (int)std::floor((sy - screenOriginY()) / zoom); }

    // スクリーン座標がウィジェット枠(size)の内側かどうか。
    // マウス入力全般で「枠外は無視する」ためのガードに使う。
    bool inWidget(int sx, int sy) const {
        SDL_Point p{ sx, sy };
        return SDL_PointInRect(&p, &size);
    }

    /* ---------- 見た目：キャンバス境界線 ---------- */
    void drawCanvasBorder(SDL_Renderer* ren, const SDL_Rect& canvasScreenRect) {
        SDL_Rect inner = canvasScreenRect;
        SDL_Rect outer{ inner.x - 1, inner.y - 1, inner.w + 2, inner.h + 2 };

        SDL_SetRenderDrawColor(ren, 128, 128, 128, 255);
        SDL_RenderDrawRect(ren, &outer);

        SDL_SetRenderDrawColor(ren, 255, 255, 255, 255);
        SDL_RenderDrawRect(ren, &inner);
    }

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
        canvasDirty = true;
    }

    void redoAction() {
        if (undoTop == undoEnd) return;
        undoTop = (undoTop + 1) % UNDO_MAX;
        canvas = undo[undoTop];
        canvasDirty = true;
    }

    /* ---------- Drawing ---------- */
    void paintPixel(int x, int y, Uint32 col) {
        if (x < 0 || x >= W || y < 0 || y >= H) return;

        Uint32 dst = canvas[y * W + x];

        Uint8 sa = (col >> 24) & 0xFF;
        float a = sa / 255.0f;

        Uint8 sr = (col >> 16) & 0xFF;
        Uint8 sg = (col >> 8) & 0xFF;
        Uint8 sb = (col) & 0xFF;

        Uint8 dr = (dst >> 16) & 0xFF;
        Uint8 dg = (dst >> 8) & 0xFF;
        Uint8 db = (dst) & 0xFF;

        Uint8 r = sr * a + dr * (1 - a);
        Uint8 g = sg * a + dg * (1 - a);
        Uint8 b = sb * a + db * (1 - a);

        canvas[y * W + x] = (0xFF << 24) | (r << 16) | (g << 8) | b;
        canvasDirty = true;
    }

    void paintPixelHard(int x, int y, Uint32 col) {
        if (x < 0 || x >= W || y < 0 || y >= H) return;
        canvas[y * W + x] = col | 0xFF000000u;
        canvasDirty = true;
    }

    void paintCircle(int cx, int cy) {
        Uint32 col = drawing ? color : bg;

        if (brush <= 1) {
            paintPixelHard(cx, cy, col);
            return;
        }

        for (int y = -brush; y <= brush; y++)
            for (int x = -brush; x <= brush; x++)
                if (x * x + y * y <= brush * brush)
                    paintPixel(cx + x, cy + y, col);
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

    // 中央ボタン用スポイト：範囲外は無視
    void pickColor(int x, int y) {
        if (x < 0 || x >= W || y < 0 || y >= H) return;
        color = canvas[y * W + x];
        col_d = true;
    }

    /* ---------- Event ---------- */
    void handleKey(SDL_Keycode k) {
        const Uint8* st = SDL_GetKeyboardState(nullptr);
        bool ctrl = st[SDL_SCANCODE_LCTRL] || st[SDL_SCANCODE_RCTRL];
        bool shift = st[SDL_SCANCODE_LSHIFT] || st[SDL_SCANCODE_RSHIFT];

        if (k == SDLK_z && ctrl) shift ? redoAction() : undoAction();
        if (k == SDLK_y && ctrl) redoAction();

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
        // ウィジェット枠の外でのクリックはすべて無視する。
        if (!inWidget(b.x, b.y)) return;

        int cx = screenToCanvasX(b.x);
        int cy = screenToCanvasY(b.y);

        if (b.button == SDL_BUTTON_MIDDLE) {
            picking = true;
            pickColor(cx, cy);
            lastX = cx; lastY = cy;
            return; // Undoは積まない
        }

        pushUndo();
        drawing = (b.button == SDL_BUTTON_LEFT);
        erasing = (b.button == SDL_BUTTON_RIGHT);

        paintCircle(cx, cy);
        lastX = cx; lastY = cy;
    }

    void mouseUp(const SDL_MouseButtonEvent&) {
        drawing = erasing = picking = false;
        lastX = lastY = -1;
    }

    void mouseMove(const SDL_MouseMotionEvent& m) {
        if (!drawing && !erasing && !picking) return;

        int cx = screenToCanvasX(m.x);
        int cy = screenToCanvasY(m.y);

        if (picking) {
            // ドラッグ中はホバー先の色を連続で拾う
            pickColor(cx, cy);
            lastX = cx; lastY = cy;
            return;
        }

        if (lastX >= 0) paintLine(lastX, lastY, cx, cy);
        else paintCircle(cx, cy);

        lastX = cx; lastY = cy;
    }

    void mouseWheel(const SDL_MouseWheelEvent& w) {
        int mx, my;
        SDL_GetMouseState(&mx, &my);

        // ウィジェット枠の外でのホイール操作は無視する
        // （他のUI要素の上でズームしてしまうのを防ぐ）
        if (!inWidget(mx, my)) return;

        float cx = camX + mx / zoom;
        float cy = camY + my / zoom;

        float factor = (w.y > 0) ? 1.15f : 1.0f / 1.15f;
        zoom = std::clamp(zoom * factor, 0.05f, 64.0f);

        camX = cx - mx / zoom;
        camY = cy - my / zoom;
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