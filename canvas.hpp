// canvas.hpp
// SDL2 + C++20 向け、テクスチャの拡大縮小・パンと、
// グリッド／セルのヒットテストができるキャンバス基底クラス。
//
// 設計方針:
//   - テクスチャそのものは拡大縮小せず、SDL_RenderCopy の src/dst 矩形だけで
//     表示範囲(ズーム・パン)を制御する。可視範囲外のピクセルは src で切り捨てる
//     ので、巨大なテクスチャでも軽量に描画できる。
//   - 「ワールド座標」= テクスチャのピクセル座標。パンは originX_/originY_
//     (画面左上に対応するワールド座標)、ズームは zoom_ (ワールド1pxが画面
//     何pxになるか) の2つの状態だけで管理する。
//   - グリッドは purely ワールド座標上の等間隔線として扱う。ヒットテストは
//     スクリーン座標 -> ワールド座標 -> セルインデックス(floor(w / cellSize))
//     という単純な計算だけで済む。
//
// 使い方:
//   1. Canvas を継承したクラスを作り、必要なら onCellClicked / onCanvasClicked /
//      onOverlayRender をオーバーライドする。
//   2. 毎フレーム handleEvent(event) と render() を呼ぶ。

#pragma once

#include "sdlutil.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <optional>

class Canvas {
public:
    // グリッド上のセル位置(列, 行)
    struct GridCell {
        int col = 0;
        int row = 0;

        friend bool operator==(const GridCell&, const GridCell&) = default;
    };

    // renderer: 描画先。texture: 表示するテクスチャ(nullptr可、後からsetTextureも可)。
    // viewport: このキャンバスが占める画面上の矩形(スクロール領域やウィンドウ全体など)。
    Canvas(SDL_Renderer* renderer, SDL_Texture* texture, SDL_Rect viewport)
        : renderer_(renderer), texture_(texture), viewport_(viewport) {
        queryTextureSize();
        fitToViewport();
    }

    virtual ~Canvas() = default;

    Canvas(const Canvas&) = delete;
    Canvas& operator=(const Canvas&) = delete;

    // ---------------------------------------------------------------
    // セットアップ
    // ---------------------------------------------------------------

    // 表示するテクスチャを差し替える。所有権はこのクラスでは持たない
    // (生成/破棄は呼び出し側の責任)。
    void setTexture(SDL_Texture* tex) {
        texture_ = tex;
        queryTextureSize();
    }

    void setViewport(const SDL_Rect& vp) {
        viewport_ = vp;
        clampOrigin();
    }
    [[nodiscard]] const SDL_Rect& viewport() const { return viewport_; }

    // cellWorldSize: グリッド1マスの大きさ(テクスチャのピクセル単位)。
    void setGrid(float cellWorldSize, SDL_Color lineColor = SDL_Color{ 255, 255, 255, 80 }) {
        gridSize_ = cellWorldSize;
        gridColor_ = lineColor;
    }
    void setGridVisible(bool visible) { gridVisible_ = visible; }
    [[nodiscard]] bool gridVisible() const { return gridVisible_; }

    void setZoomLimits(float minZoom, float maxZoom) {
        minZoom_ = minZoom;
        maxZoom_ = maxZoom;
        zoom_ = std::clamp(zoom_, minZoom_, maxZoom_);
        clampOrigin();
    }

    // ---------------------------------------------------------------
    // ビュー操作 (パン・ズーム)
    // ---------------------------------------------------------------

    // dxScreen, dyScreen: スクリーン座標系での移動量(ドラッグ量など)。
    void panBy(float dxScreen, float dyScreen) {
        originX_ -= dxScreen / zoom_;
        originY_ -= dyScreen / zoom_;
        clampOrigin();
    }

    // factor: ズーム倍率 (1.1 = 10%拡大, 1/1.1 = 10%縮小)。
    // screenX, screenY: ズームの中心にしたい画面座標 (通常はマウス位置)。
    void zoomAt(float factor, int screenX, int screenY) {
        if (factor <= 0.0f) return;

        float worldX = 0.0f, worldY = 0.0f;
        screenToWorld(screenX, screenY, worldX, worldY);

        zoom_ = std::clamp(zoom_ * factor, minZoom_, maxZoom_);

        // ズーム後も (screenX, screenY) が同じワールド座標を指すよう originを補正
        originX_ = worldX - static_cast<float>(screenX - viewport_.x) / zoom_;
        originY_ = worldY - static_cast<float>(screenY - viewport_.y) / zoom_;
        clampOrigin();
    }

    // テクスチャ全体がビューポートに収まるようにズーム・パンをリセットする。
    void fitToViewport() {
        if (texW_ <= 0 || texH_ <= 0 || viewport_.w <= 0 || viewport_.h <= 0) {
            zoom_ = 1.0f;
            originX_ = originY_ = 0.0f;
            return;
        }
        const float zx = static_cast<float>(viewport_.w) / static_cast<float>(texW_);
        const float zy = static_cast<float>(viewport_.h) / static_cast<float>(texH_);
        zoom_ = std::clamp(std::min(zx, zy), minZoom_, maxZoom_);

        originX_ = (static_cast<float>(texW_) - viewport_.w / zoom_) * 0.5f;
        originY_ = (static_cast<float>(texH_) - viewport_.h / zoom_) * 0.5f;
        clampOrigin();
    }

    [[nodiscard]] float zoom() const { return zoom_; }
    [[nodiscard]] float originX() const { return originX_; }
    [[nodiscard]] float originY() const { return originY_; }

    // ---------------------------------------------------------------
    // 座標変換
    // ---------------------------------------------------------------

    void screenToWorld(int sx, int sy, float& wx, float& wy) const {
        wx = originX_ + static_cast<float>(sx - viewport_.x) / zoom_;
        wy = originY_ + static_cast<float>(sy - viewport_.y) / zoom_;
    }

    void worldToScreen(float wx, float wy, int& sx, int& sy) const {
        sx = viewport_.x + static_cast<int>(std::lround((wx - originX_) * zoom_));
        sy = viewport_.y + static_cast<int>(std::lround((wy - originY_) * zoom_));
    }

    // ---------------------------------------------------------------
    // グリッド・ヒットテスト
    // ---------------------------------------------------------------

    // 画面座標からグリッドセルを求める。ビューポート外/グリッド範囲外は nullopt。
    [[nodiscard]] std::optional<GridCell> hitTestGrid(int screenX, int screenY) const {
        if (gridSize_ <= 0.0f) return std::nullopt;
        if (!inViewport(screenX, screenY)) return std::nullopt;

        float wx = 0.0f, wy = 0.0f;
        screenToWorld(screenX, screenY, wx, wy);

        if (texW_ > 0 && texH_ > 0) {
            if (wx < 0.0f || wy < 0.0f || wx >= static_cast<float>(texW_) ||
                wy >= static_cast<float>(texH_)) {
                return std::nullopt;
            }
        }

        GridCell cell;
        cell.col = static_cast<int>(std::floor(wx / gridSize_));
        cell.row = static_cast<int>(std::floor(wy / gridSize_));
        return cell;
    }

    // 指定セルのワールド座標上の矩形(左上x, 左上y, 幅, 高さ)を返す。
    [[nodiscard]] SDL_FRect gridCellWorldRect(const GridCell& cell) const {
        return SDL_FRect{ static_cast<float>(cell.col) * gridSize_,
                          static_cast<float>(cell.row) * gridSize_, gridSize_, gridSize_ };
    }

    // ---------------------------------------------------------------
    // イベント処理
    // ---------------------------------------------------------------
    // true を返した場合、このキャンバスがイベントを消費したことを示す。
    virtual bool handleEvent(const SDL_Event& e) {
        switch (e.type) {
        case SDL_MOUSEWHEEL: {
            int mx = 0, my = 0;
            SDL_GetMouseState(&mx, &my);
            float l_x = 0;
            float l_y = 0;
            SDL_RenderWindowToLogical(renderer(), mx, my, &l_x, &l_y);
            int lmx, lmy;
            lmx = int(l_x);
            lmy = int(l_y);
            if (!inViewport(lmx, lmy)) return false;
            const float factor = (e.wheel.y > 0) ? kZoomStep : (1.0f / kZoomStep);
            zoomAt(factor, lmx, lmy);
            return true;
        }
        case SDL_MOUSEBUTTONDOWN: {
            if (!inViewport(e.button.x, e.button.y)) return false;
            if (e.button.button == SDL_BUTTON_MIDDLE || e.button.button == SDL_BUTTON_RIGHT) {
                dragging_ = true;
                lastMouseX_ = e.button.x;
                lastMouseY_ = e.button.y;
                return true;
            }
            if (e.button.button == SDL_BUTTON_LEFT) {
                if (auto cell = hitTestGrid(e.button.x, e.button.y)) {
                    onCellClicked(*cell);
                }
                onCanvasClicked(e.button.x, e.button.y);
                return true;
            }
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            if (e.button.button == SDL_BUTTON_MIDDLE || e.button.button == SDL_BUTTON_RIGHT) {
                dragging_ = false;
                return true;
            }
            break;
        }
        case SDL_MOUSEMOTION: {
            if (dragging_) {
                const int dx = e.motion.x - lastMouseX_;
                const int dy = e.motion.y - lastMouseY_;
                lastMouseX_ = e.motion.x;
                lastMouseY_ = e.motion.y;
                panBy(static_cast<float>(dx), static_cast<float>(dy));
                return true;
            }
            break;
        }
        default:
            break;
        }
        return false;
    }

    // ---------------------------------------------------------------
    // 描画
    // ---------------------------------------------------------------
    void render() {
        if (!renderer_) return;

        const bool hadClip = SDL_RenderIsClipEnabled(renderer_) == SDL_TRUE;
        SDL_Rect prevClip{};
        if (hadClip) SDL_RenderGetClipRect(renderer_, &prevClip);

        SDL_RenderSetClipRect(renderer_, &viewport_);

        if (texture_) drawTexture();
        if (gridVisible_ && gridSize_ > 0.0f) drawGrid();

        onOverlayRender();

        SDL_RenderSetClipRect(renderer_, hadClip ? &prevClip : nullptr);
    }

protected:
    // 派生クラスが renderer に直接描画したい場合(例: render targetへの事前焼き込み)に使う。
    [[nodiscard]] SDL_Renderer* renderer() const { return renderer_; }

    // --- 派生クラスでオーバーライドするフック ---

    // グリッドが有効な状態で、いずれかのセルがクリックされたときに呼ばれる。
    virtual void onCellClicked(const GridCell& /*cell*/) {}

    // 左クリックされたとき常に呼ばれる(グリッドの有無に関わらず)。
    virtual void onCanvasClicked(int /*screenX*/, int /*screenY*/) {}

    // テクスチャ・グリッドを描画し終えた後に呼ばれる。選択枠のハイライトなど
    // 追加のオーバーレイ描画に使う。
    virtual void onOverlayRender() {}

    // origin(ビューポート左上に対応するワールド座標)を直接書き換える。
    // clampOrigin() のオーバーライドから、パン可能範囲を派生クラス側で
    // 制御したい場合に使う(例: 座標0を境界にして負の方向へパンできなくする)。
    void setOrigin(float x, float y) {
        originX_ = x;
        originY_ = y;
    }

    // テクスチャの外側が大きく映り込みすぎないよう origin をクランプする。
    // 既定ではテクスチャの矩形範囲内に収める(texture_ が null の場合は無制限)。
    // 派生クラスでオーバーライドすると、パン可能範囲を自由に決められる。
    virtual void clampOrigin() {
        if (texW_ <= 0 || texH_ <= 0 || zoom_ <= 0.0f) return;

        const float viewWorldW = static_cast<float>(viewport_.w) / zoom_;
        const float viewWorldH = static_cast<float>(viewport_.h) / zoom_;

        const float maxX = std::max(0.0f, static_cast<float>(texW_) - viewWorldW);
        const float maxY = std::max(0.0f, static_cast<float>(texH_) - viewWorldH);

        originX_ = std::clamp(originX_, 0.0f, maxX);
        originY_ = std::clamp(originY_, 0.0f, maxY);
    }

private:
    static constexpr float kZoomStep = 1.1f;

    [[nodiscard]] bool inViewport(int x, int y) const {
        const SDL_Point p{ x, y };
        return SDL_PointInRect(&p, &viewport_) == SDL_TRUE;
    }

    void queryTextureSize() {
        texW_ = texH_ = 0;
        if (texture_) {
            SDL_QueryTexture(texture_, nullptr, nullptr, &texW_, &texH_);
        }
    }


    // 可視範囲だけを src として切り出して描画する。テクスチャ全体を毎フレーム
    // スケーリングするのではなく、画面に映る分だけを GPU に渡すのがポイント。
    void drawTexture() {
        const float viewWorldW = static_cast<float>(viewport_.w) / zoom_;
        const float viewWorldH = static_cast<float>(viewport_.h) / zoom_;

        const float srcXf = std::clamp(originX_, 0.0f, static_cast<float>(texW_));
        const float srcYf = std::clamp(originY_, 0.0f, static_cast<float>(texH_));
        const float srcWf = std::clamp(viewWorldW, 0.0f, static_cast<float>(texW_) - srcXf);
        const float srcHf = std::clamp(viewWorldH, 0.0f, static_cast<float>(texH_) - srcYf);

        if (srcWf <= 0.0f || srcHf <= 0.0f) return;

        const SDL_Rect src{ static_cast<int>(std::floor(srcXf)), static_cast<int>(std::floor(srcYf)),
                            std::max(1, static_cast<int>(std::ceil(srcWf))),
                            std::max(1, static_cast<int>(std::ceil(srcHf))) };

        // src を整数に丸めた誤差分だけ dst の位置もずらして辻褄を合わせる。
        const SDL_Rect dst{
            viewport_.x + static_cast<int>(std::lround((static_cast<float>(src.x) - originX_) * zoom_)),
            viewport_.y + static_cast<int>(std::lround((static_cast<float>(src.y) - originY_) * zoom_)),
            static_cast<int>(std::lround(src.w * zoom_)), static_cast<int>(std::lround(src.h * zoom_)) };

        SDL_RenderCopy(renderer_, texture_, &src, &dst);
    }

    void drawGrid() {
        Uint8 pr, pg, pb, pa;
        SDL_GetRenderDrawColor(renderer_, &pr, &pg, &pb, &pa);
        SDL_BlendMode prevBlend;
        SDL_GetRenderDrawBlendMode(renderer_, &prevBlend);

        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, gridColor_.r, gridColor_.g, gridColor_.b, gridColor_.a);

        const float viewWorldW = static_cast<float>(viewport_.w) / zoom_;
        const float viewWorldH = static_cast<float>(viewport_.h) / zoom_;
        const float worldRight = originX_ + viewWorldW;
        const float worldBottom = originY_ + viewWorldH;

        const float startX = std::floor(originX_ / gridSize_) * gridSize_;
        const float startY = std::floor(originY_ / gridSize_) * gridSize_;

        for (float wx = startX; wx <= worldRight; wx += gridSize_) {
            int sx = 0, syDummy = 0;
            worldToScreen(wx, originY_, sx, syDummy);
            SDL_RenderDrawLine(renderer_, sx, viewport_.y, sx, viewport_.y + viewport_.h);
        }
        for (float wy = startY; wy <= worldBottom; wy += gridSize_) {
            int sxDummy = 0, sy = 0;
            worldToScreen(originX_, wy, sxDummy, sy);
            SDL_RenderDrawLine(renderer_, viewport_.x, sy, viewport_.x + viewport_.w, sy);
        }

        SDL_SetRenderDrawColor(renderer_, pr, pg, pb, pa);
        SDL_SetRenderDrawBlendMode(renderer_, prevBlend);
    }

    SDL_Renderer* renderer_ = nullptr;
    SDL_Texture* texture_ = nullptr;
    int texW_ = 0;
    int texH_ = 0;

    SDL_Rect viewport_{ 0, 0, 0, 0 };

    float zoom_ = 1.0f;
    float minZoom_ = 0.05f;
    float maxZoom_ = 32.0f;
    float originX_ = 0.0f;  // ビューポート左上に対応するワールド(テクスチャ)座標
    float originY_ = 0.0f;

    bool gridVisible_ = false;
    float gridSize_ = 0.0f;  // グリッド1マスのワールド単位サイズ
    SDL_Color gridColor_{ 255, 255, 255, 80 };

    bool dragging_ = false;
    int lastMouseX_ = 0;
    int lastMouseY_ = 0;
};