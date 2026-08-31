// game_demo.cpp
// RuntimeMap を使った最小のゲームループ例。
// 「フラグ0 = 壁」という規約で、プレイヤー(赤い四角)が壁をすり抜けないようにする。
//
// 操作: 矢印キー / WASD で移動。Escで終了。
//
// ビルド:
//   g++ -std=c++20 $(sdl2-config --cflags) game_demo.cpp -o game_demo $(sdl2-config --libs) -lSDL2_image

#include <iostream>

#include "runtime_map.hpp"
#include "tileset.hpp"

namespace {
constexpr int kWindowW = 640;
constexpr int kWindowH = 480;
constexpr int kWallFlagBit = 0;  // 「フラグ0 = 壁」という規約
constexpr float kPlayerSize = 6.0f;
constexpr float kPlayerSpeed = 90.0f;  // px/sec (タイルセット座標系, 8px=1タイル)

// プレイヤーのAABB(x,y,size,size)が壁タイルと重なるか
bool collidesWithWall(const RuntimeMap& map, const Tileset& tileset, float x, float y, float size) {
    // 四隅をチェックすれば矩形と壁タイルの重なりは検出できる
    const float points[4][2] = {{x, y}, {x + size, y}, {x, y + size}, {x + size, y + size}};
    for (const auto& p : points) {
        if (map.hasFlagAtWorldPixel(tileset, p[0], p[1], kWallFlagBit)) return true;
    }
    return false;
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

    // ドット絵をくっきり拡大するため、内部解像度を論理サイズとして設定
    SDL_RenderSetLogicalSize(renderer, kWindowW / 3, kWindowH / 3);

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

        // X, Yを別々に動かして衝突判定することで、壁際を滑るような移動になる
        const float newX = playerX + dx * kPlayerSpeed * dt;
        if (!collidesWithWall(map, tileset, newX, playerY, kPlayerSize)) playerX = newX;

        const float newY = playerY + dy * kPlayerSpeed * dt;
        if (!collidesWithWall(map, tileset, playerX, newY, kPlayerSize)) playerY = newY;

        // カメラはプレイヤーを中心に(お好みでクランプ等を追加してください)
        SDL_Rect viewport;
        SDL_RenderGetLogicalSize(renderer, &viewport.w, &viewport.h);
        viewport.x = 0;
        viewport.y = 0;
        const float cameraX = playerX - viewport.w * 0.5f;
        const float cameraY = playerY - viewport.h * 0.5f;

        SDL_SetRenderDrawColor(renderer, 10, 10, 14, 255);
        SDL_RenderClear(renderer);

        map.render(renderer, tileset, cameraX, cameraY, viewport);

        SDL_SetRenderDrawColor(renderer, 220, 50, 50, 255);
        const SDL_FRect playerRect{playerX - cameraX, playerY - cameraY, kPlayerSize, kPlayerSize};
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
