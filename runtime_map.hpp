// runtime_map.hpp
// エディタ(map_canvas.hpp)で保存した .dat ファイルを、実際のゲーム側で
// 読み込んで「描画」「当たり判定」に使うための軽量クラス。
//
// エディタの MapCanvas と違い、パン・ズームやチャンクのテクスチャ焼き込みは
// 行わない。理由: ゲームでは「画面に映る範囲のタイルだけ毎フレーム描く」
// だけで十分軽く、チャンクをテクスチャに焼くコストの方が高くつくため。
//
// タイルの実データは std::unordered_map<座標, タイル番号> に持つだけ。
// (エディタと同じ .dat フォーマットをそのまま読み込める)

#pragma once

#include "sdlutil.h"
#include <cmath>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>

#include "tileset.hpp"

class RuntimeMap {
public:
    // map.dat (MapCanvas::saveToFile で保存した疎なタイル一覧形式) を読み込む
    bool loadFromFile(const std::string& path) {
        std::ifstream in(path, std::ios::binary);
        if (!in) return false;

        char magic[4] = {};
        in.read(magic, sizeof(magic));
        if (in.gcount() != 4 || magic[0] != 'T' || magic[1] != 'M' || magic[2] != 'A' || magic[3] != '2') {
            return false;
        }

        int32_t count = 0;
        in.read(reinterpret_cast<char*>(&count), sizeof(count));
        if (!in || count < 0) return false;

        tiles_.clear();
        for (int32_t i = 0; i < count; ++i) {
            int32_t col = 0, row = 0;
            int16_t tile = -1;
            in.read(reinterpret_cast<char*>(&col), sizeof(col));
            in.read(reinterpret_cast<char*>(&row), sizeof(row));
            in.read(reinterpret_cast<char*>(&tile), sizeof(tile));
            if (!in) return false;
            tiles_[packKey(col, row)] = tile;
        }
        return true;
    }

    // タイル座標(col,row)のタイル番号。何も置かれていなければ -1。
    [[nodiscard]] int tileAt(int col, int row) const {
        auto it = tiles_.find(packKey(col, row));
        return it != tiles_.end() ? it->second : -1;
    }

    // ワールドピクセル座標からタイル番号を引く(当たり判定で便利)
    [[nodiscard]] int tileAtWorldPixel(float worldX, float worldY) const {
        const int col = static_cast<int>(std::floor(worldX / Tileset::kTileSize));
        const int row = static_cast<int>(std::floor(worldY / Tileset::kTileSize));
        return tileAt(col, row);
    }

    // 指定タイル座標のタイルが特定フラグ(bit 0〜7)を持つか。
    // 例: bit0 を「通行不可」の意味で使う、といった規約は呼び出し側で決める。
    [[nodiscard]] bool hasFlagAt(const Tileset& tileset, int col, int row, int bit) const {
        const int idx = tileAt(col, row);
        return idx >= 0 && tileset.hasFlag(idx, bit);
    }
    [[nodiscard]] bool hasFlagAtWorldPixel(const Tileset& tileset, float worldX, float worldY, int bit) const {
        const int idx = tileAtWorldPixel(worldX, worldY);
        return idx >= 0 && tileset.hasFlag(idx, bit);
    }

    // cameraX, cameraY: ワールドピクセル座標で、画面左上に映すワールド座標。
    // viewport: 画面上でマップを描画する矩形。
    // ズーム機能は無し(必要ならSDL_RenderSetScale等で全体をスケールしてください)。
    void render(SDL_Renderer* renderer, const Tileset& tileset, float cameraX, float cameraY,
                const SDL_Rect& viewport) const {
        const int colMin = static_cast<int>(std::floor(cameraX / Tileset::kTileSize));
        const int rowMin = static_cast<int>(std::floor(cameraY / Tileset::kTileSize));
        const int colMax = static_cast<int>(std::floor((cameraX + viewport.w) / Tileset::kTileSize));
        const int rowMax = static_cast<int>(std::floor((cameraY + viewport.h) / Tileset::kTileSize));

        const bool hadClip = SDL_RenderIsClipEnabled(renderer) == SDL_TRUE;
        SDL_Rect prevClip{};
        if (hadClip) SDL_RenderGetClipRect(renderer, &prevClip);
        SDL_RenderSetClipRect(renderer, &viewport);

        for (int row = rowMin; row <= rowMax; ++row) {
            for (int col = colMin; col <= colMax; ++col) {
                const int idx = tileAt(col, row);
                if (idx < 0) continue;

                const SDL_Rect src = tileset.tileSrcRect(idx);
                const SDL_Rect dst{
                    viewport.x + static_cast<int>(std::lround(col * Tileset::kTileSize - cameraX)),
                    viewport.y + static_cast<int>(std::lround(row * Tileset::kTileSize - cameraY)),
                    Tileset::kTileSize, Tileset::kTileSize};
                SDL_RenderCopy(renderer, tileset.texture(), &src, &dst);
            }
        }

        SDL_RenderSetClipRect(renderer, hadClip ? &prevClip : nullptr);
    }

private:
    static int64_t packKey(int col, int row) {
        return (static_cast<int64_t>(static_cast<uint32_t>(col)) << 32) | static_cast<uint32_t>(row);
    }

    std::unordered_map<int64_t, int> tiles_;  // packKey(col,row) -> タイル番号
};
