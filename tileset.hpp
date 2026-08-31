// tileset.hpp
// img.png のようなタイルシート画像を読み込み、8x8px単位に分割して扱うクラス。
// TIC-80のスプライトフラグ同様、タイル(チップ)ごとに8bit(0〜7)のON/OFFフラグを持てる。
//
// インデックスの割り当て規則(タイルシート上の座標 -> インデックス):
//   index = row * columns + col
//   例: (col=0,row=0) -> index 0, (col=1,row=0) -> index 1, ...
//       (col=0,row=1) -> index = columns
//   このインデックスごとに flags[index] という 8bit の値を持つ
//   (「フラグ0」がbit0、「フラグ1」がbit1、…「フラグ7」がbit7)。
//
// フラグは img.png と同じディレクトリに "<画像名>.flags" というテキストファイルで
// 保存・復元される (1行 = 1タイル分の8bit値を10進数で記述)。

#pragma once

#include "sdlutil.h"

#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

class Tileset {
public:
    static constexpr int kTileSize = 8;   // 1タイルの一辺のピクセル数
    static constexpr int kFlagCount = 8;  // タイル1枚あたりのフラグ数(bit0〜bit7)

    Tileset(SDL_Renderer* renderer, const std::string& imagePath) : imagePath_(imagePath) {
        SDL_Surface* surface = IMG_Load(imagePath.c_str());
        if (!surface) {
            throw std::runtime_error("画像の読み込みに失敗しました: " + imagePath + " (" + IMG_GetError() + ")");
        }

        texW_ = surface->w;
        texH_ = surface->h;

        texture_ = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_FreeSurface(surface);

        if (!texture_) {
            throw std::runtime_error(std::string("テクスチャの作成に失敗しました: ") + SDL_GetError());
        }
        SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_BLEND);
        // タイルの境目でにじまないよう、拡大時も最近傍補間にする
        SDL_SetTextureScaleMode(texture_, SDL_ScaleModeNearest);

        columns_ = std::max(1, texW_ / kTileSize);
        rows_ = std::max(1, texH_ / kTileSize);
        flags_.assign(static_cast<size_t>(columns_) * static_cast<size_t>(rows_), 0);

        loadFlags();  // ファイルが無ければ何もしない(全フラグ0のまま)
    }

    ~Tileset() {
        if (texture_) SDL_DestroyTexture(texture_);
    }

    Tileset(const Tileset&) = delete;
    Tileset& operator=(const Tileset&) = delete;

    [[nodiscard]] SDL_Texture* texture() const { return texture_; }
    [[nodiscard]] int columns() const { return columns_; }
    [[nodiscard]] int rows() const { return rows_; }
    [[nodiscard]] int tileCount() const { return columns_ * rows_; }
    [[nodiscard]] int textureWidth() const { return texW_; }
    [[nodiscard]] int textureHeight() const { return texH_; }

    // タイルシート上の (col, row) からタイルインデックスを求める。
    [[nodiscard]] int indexFromColRow(int col, int row) const { return row * columns_ + col; }
    [[nodiscard]] int colOf(int index) const { return (index >= 0 && index < tileCount()) ? index % columns_ : 0; }
    [[nodiscard]] int rowOf(int index) const { return (index >= 0 && index < tileCount()) ? index / columns_ : 0; }

    // タイルインデックスに対応するタイルシート上の切り出し矩形(src rect)を返す。
    [[nodiscard]] SDL_Rect tileSrcRect(int index) const {
        if (index < 0 || index >= tileCount()) return SDL_Rect{ 0, 0, 0, 0 };
        return SDL_Rect{ colOf(index) * kTileSize, rowOf(index) * kTileSize, kTileSize, kTileSize };
    }

    // ---- フラグ操作 (bit: 0〜7) ----
    [[nodiscard]] bool hasFlag(int tileIndex, int bit) const {
        if (!validIndex(tileIndex) || bit < 0 || bit >= kFlagCount) return false;
        return ((flags_[static_cast<size_t>(tileIndex)] >> bit) & 1u) != 0;
    }

    void setFlag(int tileIndex, int bit, bool value) {
        if (!validIndex(tileIndex) || bit < 0 || bit >= kFlagCount) return;
        auto& f = flags_[static_cast<size_t>(tileIndex)];
        if (value) {
            f = static_cast<uint8_t>(f | (1u << bit));
        }
        else {
            f = static_cast<uint8_t>(f & ~(1u << bit));
        }
    }

    void toggleFlag(int tileIndex, int bit) {
        if (!validIndex(tileIndex) || bit < 0 || bit >= kFlagCount) return;
        flags_[static_cast<size_t>(tileIndex)] ^= static_cast<uint8_t>(1u << bit);
    }

    [[nodiscard]] uint8_t flagBits(int tileIndex) const {
        return validIndex(tileIndex) ? flags_[static_cast<size_t>(tileIndex)] : 0;
    }

    // ---- 永続化 ----
    void saveFlags() const {
        std::ofstream out(flagsPath());
        if (!out) return;
        out << "# tic_map_editor tile flags (1 line = 1 tile, bit0..bit7)\n";
        for (int i = 0; i < tileCount(); ++i) {
            out << static_cast<int>(flags_[static_cast<size_t>(i)]) << '\n';
        }
    }

    void loadFlags() {
        std::ifstream in(flagsPath());
        if (!in) return;
        std::string line;
        int i = 0;
        while (std::getline(in, line) && i < tileCount()) {
            if (line.empty() || line[0] == '#') continue;
            std::istringstream iss(line);
            int v = 0;
            if (iss >> v) {
                flags_[static_cast<size_t>(i)] = static_cast<uint8_t>(v & 0xFF);
                ++i;
            }
        }
    }

    [[nodiscard]] const std::string& imagePath() const { return imagePath_; }

private:
    [[nodiscard]] bool validIndex(int index) const { return index >= 0 && index < tileCount(); }
    [[nodiscard]] std::string flagsPath() const { return imagePath_ + ".flags"; }

    std::string imagePath_;
    SDL_Texture* texture_ = nullptr;
    int texW_ = 0;
    int texH_ = 0;
    int columns_ = 0;
    int rows_ = 0;

    std::vector<uint8_t> flags_;  // タイルインデックス -> 8bitフラグ
};