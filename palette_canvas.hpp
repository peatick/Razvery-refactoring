// palette_canvas.hpp
// Canvas を継承し、タイルセット画像をそのまま表示してタイル選択・フラグ設定を行う
// パレット用キャンバス。
//
// 操作:
//   - 左クリック           : クリックしたタイルを「描画に使うタイル」として選択
//   - マウスホバー中に 0〜7キー : ホバー中のタイルの、対応するフラグ(bit)をON/OFF切替
//   - ホイール             : ズーム(基底Canvasの機能)
//   - 中ドラッグ           : パン(基底Canvasの機能)
//
// 各タイルの左上には、ONになっているフラグの数だけ小さな色付きドットを表示する
// (ズームインすると見やすくなる)。選択中のタイルには枠を表示する。

#pragma once

#include "canvas.hpp"
#include "tileset.hpp"

#include <functional>

class PaletteCanvas : public Canvas {
public:
    PaletteCanvas(SDL_Renderer* renderer, Tileset& tileset, SDL_Rect viewport)
        : Canvas(renderer, tileset.texture(), viewport), tileset_(tileset) {
        setGrid(static_cast<float>(Tileset::kTileSize), SDL_Color{ 80, 80, 80, 160 });
        setGridVisible(true);
        setZoomLimits(1.0f, 32.0f);  // パレットは縮小しすぎると選びにくいので最小1倍
        fitToViewport();
    }

    // タイルが選択されたときに呼ばれるコールバック (index を渡す)
    void setOnTileSelected(std::function<void(int)> cb) { onTileSelected_ = std::move(cb); }

    [[nodiscard]] int selectedTile() const { return selectedTile_; }
    void setSelectedTile(int index) { selectedTile_ = index; }

    bool handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_MOUSEMOTION) {
            if (auto cell = hitTestGrid(e.motion.x, e.motion.y)) {
                hoveredTile_ = tileset_.indexFromColRow(cell->col, cell->row);
            }
            else {
                hoveredTile_ = -1;
            }
        }
        else if (e.type == SDL_KEYDOWN && !e.key.repeat) {
            if (hoveredTile_ >= 0) {
                const int bit = numberKeyToBit(e.key.keysym.scancode);
                if (bit >= 0) {
                    tileset_.toggleFlag(hoveredTile_, bit);
                    SDL_Log("tile %d : flag%d -> %s", hoveredTile_, bit,
                        tileset_.hasFlag(hoveredTile_, bit) ? "ON" : "OFF");
                    return true;
                }
            }
        }
        return Canvas::handleEvent(e);
    }

protected:
    void onCellClicked(const GridCell& cell) override {
        const int index = tileset_.indexFromColRow(cell.col, cell.row);
        if (index < 0 || index >= tileset_.tileCount()) return;
        selectedTile_ = index;
        if (onTileSelected_) onTileSelected_(index);
    }

    void onOverlayRender() override {
        SDL_Renderer* r = renderer();
        int selected_flags[8] = {0, 0, 0, 0, 0, 0, 0, 0};
        // 各タイルのフラグ状況を小さなドットで表示
        for (int index = 0; index < tileset_.tileCount(); ++index) {
            const uint8_t bits = tileset_.flagBits(index);
            if (bits == 0) continue;

            const int col = tileset_.colOf(index);
            const int row = tileset_.rowOf(index);
            int sx = 0, sy = 0;
            worldToScreen(static_cast<float>(col * Tileset::kTileSize),
                static_cast<float>(row * Tileset::kTileSize), sx, sy);

            const float z = zoom();
            const float dotSize = std::max(1.0f, z * 0.16f);
            for (int bit = 0; bit < Tileset::kFlagCount; ++bit) {
                if (!((bits >> bit) & 1u)) continue;
                const int gx = bit % 4;
                const int gy = bit / 4;
                SDL_FRect dot{ sx + gx * dotSize + 1.0f, sy + gy * dotSize + 1.0f, dotSize - 0.5f, dotSize - 0.5f };
                const SDL_Color c = flagColor(bit);
                SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(r, c.r, c.g, c.b, 230);
                SDL_RenderFillRectF(r, &dot);

                if (selectedTile_ == index) {
                    selected_flags[bit] = 1;
                    std::cout << index << "=" << bit << std::endl;
                }
            }
        }

        // 選択中タイルの枠
        if (selectedTile_ >= 0) {
            drawTileOutline(selectedTile_, SDL_Color{ 255, 255, 0, 255 });
            
        }
        // ホバー中タイルの枠(薄め)
        if (hoveredTile_ >= 0 && hoveredTile_ != selectedTile_) {
            drawTileOutline(hoveredTile_, SDL_Color{ 255, 255, 255, 140 });
        }
    }

private:
    static int numberKeyToBit(SDL_Scancode sc) {
        switch (sc) {
        case SDL_SCANCODE_0: return 0;
        case SDL_SCANCODE_1: return 1;
        case SDL_SCANCODE_2: return 2;
        case SDL_SCANCODE_3: return 3;
        case SDL_SCANCODE_4: return 4;
        case SDL_SCANCODE_5: return 5;
        case SDL_SCANCODE_6: return 6;
        case SDL_SCANCODE_7: return 7;
        default: return -1;
        }
    }

    static SDL_Color flagColor(int bit) {
        static constexpr SDL_Color palette[Tileset::kFlagCount] = {
            {231, 76, 60, 255},   {230, 126, 34, 255}, {241, 196, 15, 255}, {46, 204, 113, 255},
            {26, 188, 156, 255},  {52, 152, 219, 255}, {155, 89, 182, 255}, {236, 240, 241, 255},
        };
        return palette[bit % Tileset::kFlagCount];
    }

    void drawTileOutline(int index, SDL_Color color) {
        const int col = tileset_.colOf(index);
        const int row = tileset_.rowOf(index);
        int sx0 = 0, sy0 = 0, sx1 = 0, sy1 = 0;
        worldToScreen(static_cast<float>(col * Tileset::kTileSize), static_cast<float>(row * Tileset::kTileSize),
            sx0, sy0);
        worldToScreen(static_cast<float>((col + 1) * Tileset::kTileSize),
            static_cast<float>((row + 1) * Tileset::kTileSize), sx1, sy1);
        SDL_Renderer* r = renderer();
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        const SDL_Rect rect{ sx0, sy0, sx1 - sx0, sy1 - sy0 };
        SDL_RenderDrawRect(r, &rect);
    }

    Tileset& tileset_;
    int selectedTile_ = 0;
    int hoveredTile_ = -1;
    std::function<void(int)> onTileSelected_;
};