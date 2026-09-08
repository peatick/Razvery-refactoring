// game_demo.cpp
// RuntimeMap を使った最小のゲームループ例。
// 「フラグ0 = 壁」という規約で、プレイヤー(赤い四角)が壁をすり抜けないようにする。
//
// 操作: 矢印キー / WASD で移動。Escで終了。
//
// ビルド:
//   g++ -std=c++20 $(sdl2-config --cflags) game_demo.cpp -o game_demo $(sdl2-config --libs) -lSDL2_image


#include <algorithm>
#include <iostream>

#include "runtime_map.hpp"
#include "tileset.hpp"

namespace {
    constexpr int kScale = 6;        // ドット絵の拡大率
    constexpr int kLogicalW = 240;   // 内部解像度(論理サイズ)を先に決める
    constexpr int kLogicalH = 120;
    constexpr int kWindowW = kLogicalW * kScale;  // 2880 (= kLogicalWから逆算するので必ず割り切れる)
    constexpr int kWindowH = kLogicalH * kScale;  // 1440
    constexpr int kWallFlagBit = 0;  // 「フラグ0 = 壁」という規約
    constexpr float kPlayerSize = 6.0f;
    constexpr float kPlayerSpeed = 90.0f;  // px/sec (タイルセット座標系, 8px=1タイル)

    // プレイヤーのAABB(x,y,size,size)が壁タイルと重なるか。
    // 四隅の「点」だけを見ると、境界にちょうど接している(めり込んでいない)ときに
    // 誤って衝突判定してしまうことがあるため、実際に重なっているタイル範囲を
    // 半開区間 [x, x+size) で求めて判定する。
    bool collidesWithWall(const RuntimeMap& map, const Tileset& tileset, float x, float y, float size) {
        constexpr float kEpsilon = 0.001f;  // 境界ちょうどを「まだ次のタイルに入っていない」扱いにする
        const int colMin = static_cast<int>(std::floor(x / Tileset::kTileSize));
        const int colMax = static_cast<int>(std::floor((x + size - kEpsilon) / Tileset::kTileSize));
        const int rowMin = static_cast<int>(std::floor(y / Tileset::kTileSize));
        const int rowMax = static_cast<int>(std::floor((y + size - kEpsilon) / Tileset::kTileSize));

        for (int row = rowMin; row <= rowMax; ++row) {
            for (int col = colMin; col <= colMax; ++col) {
                if (map.hasFlagAt(tileset, col, row, kWallFlagBit)) return true;
            }
        }
        return false;
    }

    // 1軸ぶんの移動を解決する。「目的地だけ」を見るのではなく、移動経路をタイル1枚分
    // ずつなめながら壁との衝突を調べる(スイープ)。こうすることで:
    //   - 壁に当たる場合は壁のグリッド境界ぴったりまで詰める(フレームレートに依らず
    //     常に壁に密着して止まる)
    //   - 1フレームの移動量が大きくても壁を飛び越えてすり抜けたりしない
    //   fixedOther: 動かさない側の軸の座標
    //   isXAxis   : true なら x方向の移動、false なら y方向の移動
    //   from, to  : 移動前/移動後(壁を考慮する前)の座標
    float resolveAxisMove(const RuntimeMap& map, const Tileset& tileset, float fixedOther, bool isXAxis, float from,
        float to, float size) {
        if (to == from) return to;

        auto collidesAt = [&](float pos) {
            return isXAxis ? collidesWithWall(map, tileset, pos, fixedOther, size)
                : collidesWithWall(map, tileset, fixedOther, pos, size);
            };

        const bool movingPositive = to > from;
        const float maxStep = static_cast<float>(Tileset::kTileSize);
        float current = from;

        while (current != to) {
            const float next =
                movingPositive ? std::min(current + maxStep, to) : std::max(current - maxStep, to);

            if (collidesAt(next)) {
                // このタイル境界の手前ぴったりまで詰めて終了
                const float leadingEdge = movingPositive ? next + size : next;
                const int tileIndex = static_cast<int>(std::floor(leadingEdge / Tileset::kTileSize));
                return movingPositive ? (tileIndex * Tileset::kTileSize - size) : ((tileIndex + 1) * Tileset::kTileSize);
            }
            current = next;
        }
        return current;
    }

}  // namespace

int main(int argc, char** argv) {
    const std::string imagePath = (argc > 1) ? argv[1] : "img.png";
    const std::string mapPath = (argc > 2) ? argv[2] : "demo_map.dat";

    SDL_Init(SDL_INIT_VIDEO);
    IMG_Init(IMG_INIT_PNG);

    SDL_Window* window = SDL_CreateWindow("runtime map demo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        kWindowW, kWindowH, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!renderer) renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);

    // ドット絵をくっきり拡大するため、内部解像度を論理サイズとして設定。
    // kWindowW/H は kLogicalW/H * kScale で「必ず割り切れる」ように逆算してあるので、
    // 縦横で拡大率がズレる(=見た目が数pxすれる)ことがない。
    SDL_RenderSetLogicalSize(renderer, kLogicalW, kLogicalH);
    SDL_RenderSetIntegerScale(renderer, SDL_TRUE);  // 念のため、常に整数倍率でスケーリングする

    Tileset tileset(renderer, imagePath);

    RuntimeMap map;
    if (!map.loadFromFile(mapPath)) {
        std::cerr << mapPath << " の読み込みに失敗しました\n";
        return 1;
    }

    float playerX = 32.0f, playerY = 32.0f;  // ワールドピクセル座標(8px=1タイル)
    Uint64 prevTicks = SDL_GetPerformanceCounter();

    bool running = true;
    SDL_Event e;
    while (running) {
        while (SDL_PollEvent(&e)) {
            if (e.type == SDL_QUIT) running = false;
            if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_ESCAPE) running = false;
        }

        const Uint64 now = SDL_GetPerformanceCounter();
        const float dt = static_cast<float>(now - prevTicks) / static_cast<float>(SDL_GetPerformanceFrequency());
        prevTicks = now;

        const Uint8* keys = SDL_GetKeyboardState(nullptr);
        float dx = 0.0f, dy = 0.0f;
        if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A]) dx -= 1.0f;
        if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D]) dx += 1.0f;
        if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W]) dy -= 1.0f;
        if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S]) dy += 1.0f;

        // X, Yを別々に解決する(壁のグリッド境界ぴったりまで詰めるので、
        // フレームレートや移動速度に関わらず常に壁に密着して止まる)
        const float newX = playerX + dx * kPlayerSpeed * dt;
        playerX = resolveAxisMove(map, tileset, playerY, /*isXAxis=*/true, playerX, newX, kPlayerSize);

        const float newY = playerY + dy * kPlayerSpeed * dt;
        playerY = resolveAxisMove(map, tileset, playerX, /*isXAxis=*/false, playerY, newY, kPlayerSize);

        // カメラはプレイヤーを中心に(お好みでクランプ等を追加してください)
        SDL_Rect viewport;
        SDL_RenderGetLogicalSize(renderer, &viewport.w, &viewport.h);
        viewport.x = 0;
        viewport.y = 0;
        const float cameraX = playerX - viewport.w * 0.5f;
        const float cameraY = 60 - viewport.h * 0.5f;
		std::cout << "player: (" << playerX << ", " << playerY << "), camera: (" << cameraX << ", " << cameraY << ")\n";

        SDL_SetRenderDrawColor(renderer, 10, 10, 14, 255);
        SDL_RenderClear(renderer);

        map.render(renderer, tileset, cameraX, cameraY, viewport);

        SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
        const SDL_FRect playerRect{ playerX - cameraX, playerY - cameraY, kPlayerSize, kPlayerSize };
        SDL_RenderFillRectF(renderer, &playerRect);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    IMG_Quit();
    SDL_Quit();
    return 0;
}
