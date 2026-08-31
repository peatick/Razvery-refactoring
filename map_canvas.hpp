// map_canvas.hpp
// Canvas を継承した「右方向・下方向へは実質無制限」なマップ編集用キャンバス。
// (0, 0) を左上の境界として、そこより外(負の座標)へはスクロールできない。
//
// 仕組み(チャンク方式):
//   SDLのテクスチャには最大サイズの制限があるため、マップ全体を1枚のテクスチャに
//   することはできない。そこでマップを chunkSize x chunkSize タイルの「チャンク」に
//   分割し、実際にタイルが置かれたチャンクだけを std::unordered_map で動的に生成する。
//   各チャンクは chunkSize*8 x chunkSize*8 px の render-target テクスチャを1枚持ち、
//   タイルが置かれる/消されるたびにそのチャンク内の該当8x8pxだけ焼き直す(bake)。
//
//   描画時は現在の表示範囲(パン・ズーム)から「画面に映っているチャンク座標の範囲」を
//   逆算し、存在するチャンクのテクスチャだけを1個ずつ SDL_RenderCopy する。
//   => タイルを置いた範囲がどれだけ広がっても、メモリ・描画コストは「実際に使われた
//      チャンクの数」にしか比例しない。
//
//   基底 Canvas にはテクスチャを渡さない(texture_ が null のまま)ため、
//   Canvas側のパン範囲クランプ(clampOrigin)が働かず、上下左右どこまでもパンできる。
//
// 操作:
//   - 左クリック / 左ドラッグ : 選択中のタイルを配置
//   - 右クリック / 右ドラッグ : タイルを消去
//   - 中ドラッグ              : パン(基底Canvasの機能、無制限)
//   - ホイール                : ズーム(基底Canvasの機能)

#pragma once

#include "canvas.hpp"
#include "tileset.hpp"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <unordered_map>
#include <vector>

class MapCanvas : public Canvas {
public:
    // chunkSize: 1チャンクの一辺のタイル数。マップの上限には影響しない、
    //            メモリ/描画コストの粒度を決めるチューニング値(既定16 = 128x128px/チャンク)。
    MapCanvas(SDL_Renderer* renderer, Tileset& tileset, SDL_Rect viewport, int chunkSize = 16)
        : Canvas(renderer, nullptr, viewport), tileset_(tileset), chunkSize_(chunkSize) {
        if (chunkSize_ < 1) chunkSize_ = 1;
        setGrid(static_cast<float>(Tileset::kTileSize), SDL_Color{ 255, 255, 255, 60 });
        setGridVisible(true);
        setZoomLimits(0.1f, 16.0f);
        fitToViewport();  // texture_ が null なので実質 zoom=1, origin=(0,0) にリセットするだけ
    }

    ~MapCanvas() override {
        for (auto& [key, chunk] : chunks_) {
            if (chunk.texture) SDL_DestroyTexture(chunk.texture);
        }
    }

    void setPaintTile(int tileIndex) { paintTile_ = tileIndex; }
    [[nodiscard]] int paintTile() const { return paintTile_; }

    [[nodiscard]] int chunkSize() const { return chunkSize_; }
    [[nodiscard]] size_t loadedChunkCount() const { return chunks_.size(); }

    // col, row は範囲制限なし(負の値も含めどこまでも指定できる)
    [[nodiscard]] int tileAt(int col, int row) const {
        if (col < 0 || row < 0) return -1;
        const int cx = floorDiv(col, chunkSize_);
        const int cy = floorDiv(row, chunkSize_);
        const Chunk* chunk = findChunk(cx, cy);
        if (!chunk) return -1;
        const int lx = floorMod(col, chunkSize_);
        const int ly = floorMod(row, chunkSize_);
        return chunk->tiles[static_cast<size_t>(ly) * chunkSize_ + lx];
    }

    void setTile(int col, int row, int tileIndex) {
        if (col < 0 || row < 0) return;  // (0,0)が左上の境界なので負の座標は扱わない
        const int cx = floorDiv(col, chunkSize_);
        const int cy = floorDiv(row, chunkSize_);

        if (tileIndex < 0) {
            // 消去: そもそもチャンクが存在しない(=既に空)なら何もしない
            Chunk* existing = findChunkMutable(cx, cy);
            if (!existing) return;
            const int lx = floorMod(col, chunkSize_);
            const int ly = floorMod(row, chunkSize_);
            auto& slot = existing->tiles[static_cast<size_t>(ly) * chunkSize_ + lx];
            if (slot == tileIndex) return;
            slot = tileIndex;
            bakeLocalCell(*existing, lx, ly);
            return;
        }

        Chunk& chunk = getOrCreateChunk(cx, cy);
        const int lx = floorMod(col, chunkSize_);
        const int ly = floorMod(row, chunkSize_);
        auto& slot = chunk.tiles[static_cast<size_t>(ly) * chunkSize_ + lx];
        if (slot == tileIndex) return;
        slot = tileIndex;
        bakeLocalCell(chunk, lx, ly);
    }

    bool handleEvent(const SDL_Event& e) override {
        if (e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_RIGHT) {
            eraseAtScreen(e.button.x, e.button.y);
            return true;
        }
        if (e.type == SDL_MOUSEMOTION) {
            if (e.motion.state & SDL_BUTTON_LMASK) {
                paintAtScreen(e.motion.x, e.motion.y);
            }
            else if (e.motion.state & SDL_BUTTON_RMASK) {
                eraseAtScreen(e.motion.x, e.motion.y);
            }
        }
        return Canvas::handleEvent(e);
    }

    // ---- 保存 / 読み込み ----
    // 疎(スパース)なタイルだけを保存する簡易バイナリ形式。
    // マップサイズという概念が無いので、「置かれているタイルの一覧」を記録する。
    bool saveToFile(const std::string& path) const {
        std::ofstream out(path, std::ios::binary);
        if (!out) return false;
        struct Entry {
            int32_t col, row;
            int16_t tile;
        };
        std::vector<Entry> entries;
        for (const auto& [key, chunk] : chunks_) {
            int cx = 0, cy = 0;
            unpackKey(key, cx, cy);
            for (int ly = 0; ly < chunkSize_; ++ly) {
                for (int lx = 0; lx < chunkSize_; ++lx) {
                    const int idx = chunk.tiles[static_cast<size_t>(ly) * chunkSize_ + lx];
                    if (idx < 0) continue;
                    entries.push_back(Entry{ cx * chunkSize_ + lx, cy * chunkSize_ + ly, static_cast<int16_t>(idx) });
                }
            }
        }

        const char magic[4] = { 'T', 'M', 'A', '2' };  // フォーマット2: 疎なタイル一覧
        out.write(magic, sizeof(magic));
        const int32_t count = static_cast<int32_t>(entries.size());
        out.write(reinterpret_cast<const char*>(&count), sizeof(count));
        for (const auto& en : entries) {
            out.write(reinterpret_cast<const char*>(&en.col), sizeof(en.col));
            out.write(reinterpret_cast<const char*>(&en.row), sizeof(en.row));
            out.write(reinterpret_cast<const char*>(&en.tile), sizeof(en.tile));
        }
        return static_cast<bool>(out);
    }

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

        clearAll();
        for (int32_t i = 0; i < count; ++i) {
            int32_t col = 0, row = 0;
            int16_t tile = -1;
            in.read(reinterpret_cast<char*>(&col), sizeof(col));
            in.read(reinterpret_cast<char*>(&row), sizeof(row));
            in.read(reinterpret_cast<char*>(&tile), sizeof(tile));
            if (!in) return false;
            setTile(col, row, tile);
        }
        return true;
    }

    void clearAll() {
        for (auto& [key, chunk] : chunks_) {
            if (chunk.texture) SDL_DestroyTexture(chunk.texture);
        }
        chunks_.clear();
    }

protected:
    void onCellClicked(const GridCell& cell) override { setTile(cell.col, cell.row, paintTile_); }

    // (0,0) を左上の境界として、そこより負の方向へはパンできないようにする。
    // 右・下方向は無制限のまま。
    void clampOrigin() override {
        const float x = std::max(0.0f, originX());
        const float y = std::max(0.0f, originY());
        if (x != originX() || y != originY()) setOrigin(x, y);
    }

    // 基底Canvasは texture_ が null だと何も描かないので、可視チャンクの描画は
    // ここ(オーバーレイフック)で自前に行う。
    void onOverlayRender() override {
        const SDL_Rect vp = viewport();
        const float z = zoom();
        if (z <= 0.0f) return;

        float wLeft = 0.0f, wTop = 0.0f, wRight = 0.0f, wBottom = 0.0f;
        screenToWorld(vp.x, vp.y, wLeft, wTop);
        screenToWorld(vp.x + vp.w, vp.y + vp.h, wRight, wBottom);

        const int chunkPx = chunkSize_ * Tileset::kTileSize;
        const int cxMin = floorDiv(static_cast<int>(std::floor(wLeft)) - 1, chunkPx);
        const int cxMax = floorDiv(static_cast<int>(std::ceil(wRight)) + 1, chunkPx);
        const int cyMin = floorDiv(static_cast<int>(std::floor(wTop)) - 1, chunkPx);
        const int cyMax = floorDiv(static_cast<int>(std::ceil(wBottom)) + 1, chunkPx);

        SDL_Renderer* r = renderer();
        for (int cy = cyMin; cy <= cyMax; ++cy) {
            for (int cx = cxMin; cx <= cxMax; ++cx) {
                const Chunk* chunk = findChunk(cx, cy);
                if (!chunk) continue;

                int sx = 0, sy = 0;
                worldToScreen(static_cast<float>(cx * chunkPx), static_cast<float>(cy * chunkPx), sx, sy);
                const int size = static_cast<int>(std::lround(chunkPx * z));
                const SDL_Rect dst{ sx, sy, size, size };
                SDL_RenderCopy(r, chunk->texture, nullptr, &dst);
            }
        }
    }

private:
    struct Chunk {
        SDL_Texture* texture = nullptr;
        std::vector<int> tiles;  // -1 = 空
    };

    // 負数にも対応した床(floor)方向の除算・剰余
    static int floorDiv(int a, int b) {
        int q = a / b;
        const int r = a % b;
        if (r != 0 && ((r < 0) != (b < 0))) --q;
        return q;
    }
    static int floorMod(int a, int b) {
        int r = a % b;
        if (r != 0 && ((r < 0) != (b < 0))) r += b;
        return r;
    }

    static int64_t packKey(int cx, int cy) {
        return (static_cast<int64_t>(static_cast<uint32_t>(cx)) << 32) | static_cast<uint32_t>(cy);
    }
    static void unpackKey(int64_t key, int& cx, int& cy) {
        cx = static_cast<int32_t>(key >> 32);
        cy = static_cast<int32_t>(static_cast<uint32_t>(key & 0xffffffffLL));
    }

    [[nodiscard]] const Chunk* findChunk(int cx, int cy) const {
        auto it = chunks_.find(packKey(cx, cy));
        return it != chunks_.end() ? &it->second : nullptr;
    }
    [[nodiscard]] Chunk* findChunkMutable(int cx, int cy) {
        auto it = chunks_.find(packKey(cx, cy));
        return it != chunks_.end() ? &it->second : nullptr;
    }

    Chunk& getOrCreateChunk(int cx, int cy) {
        const int64_t key = packKey(cx, cy);
        auto it = chunks_.find(key);
        if (it != chunks_.end()) return it->second;

        SDL_Renderer* r = renderer();
        const int px = chunkSize_ * Tileset::kTileSize;

        Chunk chunk;
        chunk.texture = SDL_CreateTexture(r, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, px, px);
        if (!chunk.texture) {
            throw std::runtime_error(std::string("チャンク用テクスチャの作成に失敗しました: ") + SDL_GetError());
        }
        SDL_SetTextureBlendMode(chunk.texture, SDL_BLENDMODE_BLEND);
        SDL_SetTextureScaleMode(chunk.texture, SDL_ScaleModeNearest);

        SDL_Texture* prevTarget = SDL_GetRenderTarget(r);
        SDL_SetRenderTarget(r, chunk.texture);
        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderClear(r);  // 透明で初期化
        SDL_SetRenderTarget(r, prevTarget);

        chunk.tiles.assign(static_cast<size_t>(chunkSize_) * static_cast<size_t>(chunkSize_), -1);

        auto [insertedIt, ok] = chunks_.emplace(key, std::move(chunk));
        return insertedIt->second;
    }

    // チャンク内のローカル座標(lx,ly)の1マス分だけタイルセットから焼き直す
    void bakeLocalCell(Chunk& chunk, int lx, int ly) {
        SDL_Renderer* r = renderer();
        SDL_Texture* prevTarget = SDL_GetRenderTarget(r);

        SDL_SetRenderTarget(r, chunk.texture);
        const SDL_Rect dst{ lx * Tileset::kTileSize, ly * Tileset::kTileSize, Tileset::kTileSize, Tileset::kTileSize };

        SDL_SetRenderDrawBlendMode(r, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(r, 0, 0, 0, 0);
        SDL_RenderFillRect(r, &dst);  // 透明でクリア

        const int idx = chunk.tiles[static_cast<size_t>(ly) * chunkSize_ + lx];
        if (idx >= 0) {
            SDL_Rect src = tileset_.tileSrcRect(idx);
            SDL_RenderCopy(r, tileset_.texture(), &src, &dst);
        }

        SDL_SetRenderTarget(r, prevTarget);
    }

    void paintAtScreen(int sx, int sy) {
        if (auto cell = hitTestGrid(sx, sy)) setTile(cell->col, cell->row, paintTile_);
    }
    void eraseAtScreen(int sx, int sy) {
        if (auto cell = hitTestGrid(sx, sy)) setTile(cell->col, cell->row, -1);
    }

    Tileset& tileset_;
    int chunkSize_;
    std::unordered_map<int64_t, Chunk> chunks_;  // packKey(チャンクcol,row) -> Chunk
    int paintTile_ = 0;
};