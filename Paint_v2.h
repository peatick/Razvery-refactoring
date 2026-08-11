#pragma once
#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdl2/include/SDL_image.h"
#include <deque>
#include <string>
#include <vector>
#include <iostream>

// ------------------------------------------------------------
// 内部で使う小さな部品 (ボタン/スライダー/テキスト欄) は
// このヘッダの外からは使わないので、実装詳細として
// PaintWidgetImpl 名前空間に隠しておく。公開クラスは
// PaintWidget だけ。
// ------------------------------------------------------------
namespace paintwidget_detail {

    struct TextField {
        SDL_Rect    rect = { 0, 0, 60, 18 };
        std::string text;
        bool        active = false;
        int         maxLen = 4;
        int         minVal = 1, maxVal = 9999;

        bool hitTest(int mx, int my) const;
        void startEdit(int v);
        void stopEdit();
        int  commitValue() const;
        void onKeyDown(SDL_Keycode sym);
        void onTextInput(const char* inp);
        void draw(SDL_Renderer* ren, int currentValue) const;
    };

    struct RgbaSlider {
        SDL_Rect    track = { 0, 0, 0, 14 };
        SDL_Color   barCol = { 200, 200, 200, 255 };
        std::string label;
        bool        dragging = false;
        TextField   field;

        SDL_Rect thumbRect(int val) const;
        bool     hitTrack(int mx, int my) const;
        int      xToVal(int mx) const;
        void     setup(int lx, int y, int lw, const std::string& lbl, SDL_Color bc);
        void     draw(SDL_Renderer* ren, int val) const;
    };

    struct Button {
        SDL_Rect    rect = { 0, 0, 0, 0 };
        std::string label;
        bool        pressed = false, hovered = false;
        SDL_Color   bg = { 70, 70, 80, 255 }, bgHover = { 90, 90, 110, 255 };
        void draw(SDL_Renderer* ren) const;
    };

    struct NewCanvasDialog {
        bool      visible = false;
        TextField fieldW, fieldH;
        Button    btnOk, btnCancel;
        int       resultW = 0, resultH = 0;
        bool      confirmed = false;

        void open(int currentW, int currentH);
        void layout(int localW, int localH);
        bool handleMouseDown(int mx, int my);
        bool handleKey(SDL_Keycode sym);
        bool handleTextInput(const char* t);
        void draw(SDL_Renderer* ren, int localW, int localH);
    };

    struct Canvas {
        int                  w = 0, h = 0;
        std::vector<Uint32>  pixels;
        SDL_Texture* tex = nullptr;
        bool                 dirty = false;
        std::deque<std::vector<Uint32>> undoStack, redoStack;

        void pushUndo();
        bool undo();
        bool redo();
        int  undoCount() const { return (int)undoStack.size(); }
        int  redoCount() const { return (int)redoStack.size(); }

        void init(SDL_Renderer* ren, int W, int H);
        void flush();
        Uint32& at(int x, int y) { return pixels[y * w + x]; }
        Uint32  at(int x, int y) const { return pixels[y * w + x]; }
        bool    inBounds(int x, int y) const { return x >= 0 && x < w && y >= 0 && y < h; }

        void blendPixel(int x, int y, SDL_Color col);
        void paintBrush(int cx, int cy, int radius, SDL_Color col);
        void erase(int cx, int cy, int radius);
        void fill(int sx, int sy, SDL_Color col);
        std::vector<Uint32> copyRegion(SDL_Rect r) const;
        void clearRegion(SDL_Rect r);
        void paste(int px, int py, const std::vector<Uint32>& buf, int bw, int bh);
        SDL_Color pickColor(int cx, int cy) const;
        void destroy();
    };

    enum Tool { TOOL_PEN, TOOL_ERASER, TOOL_FILL, TOOL_SELECT, TOOL_EYEDROPPER, TOOL_COUNT };

    struct GridState {
        bool show1 = false, show8 = false, show32 = false;
    };

} // namespace paintwidget_detail


// ============================================================
// PaintWidget : 公開クラス
// ============================================================
class PaintWidget {
public:
    std::string savepath;
    PaintWidget() = default;
    ~PaintWidget() { cleanup(); }

    // コピー禁止（SDL_Texture等の生ポインタを持つため）
    PaintWidget(const PaintWidget&) = delete;
    PaintWidget& operator=(const PaintWidget&) = delete;

    SDL_Texture* canvasTexture_ = nullptr;
    void destroyCanvasTexture() { if (canvasTexture_) { SDL_DestroyTexture(canvasTexture_); canvasTexture_ = nullptr; } }
    // レンダラーと配置矩形を渡して初期化する。
    // フォントはプロセス内で共有ロードされる（初回のみディスクから読む）。
    // initialCanvasW/H で最初のキャンバスサイズを指定できる（省略時 512x512）。
    // 注意: ツールバーのボタン数が多いため、bounds.w は 1000px 以上を
    //       推奨（それより狭いと右側のグリッドボタンとUndo/Redoが重なる）。
    //       bounds.h は 400px 以上を推奨。
    bool init(SDL_Renderer* renderer, SDL_Rect bounds,
        int initialCanvasW = 512, int initialCanvasH = 512, TTF_Font* cusFont = nullptr);
    void dlgNew_OUT();
    // ウィジェットの配置矩形を変更する（親ウィンドウのリサイズ・
    // レイアウト変更時にホストから呼ぶ）。
    void setBounds(const SDL_Rect& bounds);
    const SDL_Rect& getBounds() const { return bounds_; }

    // 新規キャンバス作成（"New"ボタンやCtrl+Nと同じ動作をコードから呼びたい場合）
    void newCanvas(int w, int h);

    // SDL_PollEvent() で受け取ったイベントをそのまま渡す。
    // bounds の外で発生したマウス押下は無視される（他ウィジェットに委譲）。
    void handleEvent(const SDL_Event& e);

    // 毎フレーム呼ぶ。bounds の中だけに描画する
    // (SDL_RenderSetViewport を使うため、呼び出し前後で
    //  ホストのビューポート/クリップ矩形設定には影響しない)。
    void render();

    // 再描画が必要か（キャンバスの変更やUI操作があったか）
    bool needsRedraw() const { return needRedraw_ || canvasDirty(); }

    // このウィジェットが現在キーボード入力を受け付けるべきか
    // (テキスト欄編集中 or ダイアログ表示中)。ホストが複数ウィジェット
    // 間でフォーカスを管理する際の判定に使える。
    bool wantsTextInput() const;

    // 保有するリソース(キャンバステクスチャ等)を解放する。
    // レンダラー自体はホスト側の所有物なので破棄しない。
    void cleanup();

    // フォントなどプロセス全体で共有しているリソースを解放する。
    // アプリ終了時に一度だけ呼ぶ（複数の PaintWidget を使っていても1回でよい）。
    static void shutdownShared(bool use_uiFont = true);

    void LoadTextureFromPath(const std::string pathStr)
    {
        // ① SDL_LoadBMP の代わりに IMG_Load を使用
        SDL_Surface* surface = IMG_Load(pathStr.c_str());
        if (!surface) {
            SDL_Log("IMG_Load failed: %s", IMG_GetError());
            return;
        }

        // ② ARGB8888 に変換（canvas の内部フォーマットと合わせる）
        SDL_Surface* conv = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_ARGB8888, 0);
        SDL_FreeSurface(surface);
        if (!conv) {
            SDL_Log("SDL_ConvertSurfaceFormat failed: %s", SDL_GetError());
            return;
        }

        // ③ newCanvas でキャンバスを初期化
        newCanvas(conv->w, conv->h);

        // ④ pixels[] にコピー
        SDL_LockSurface(conv);
        memcpy(canvas_.pixels.data(), conv->pixels, conv->w * conv->h * 4);
        SDL_UnlockSurface(conv);
        SDL_FreeSurface(conv);

        // ⑤ pixels[] → テクスチャに反映
        canvas_.flush();
    }
    void SaveTextureToPNG(std::string filename) {
        if (!renderer_ || !canvas_.tex || filename.empty()) return;

        // 1. テクスチャ情報の取得
        int width, height;
        Uint32 format;
        if (SDL_QueryTexture(canvas_.tex, &format, NULL, &width, &height) != 0) {
            SDL_Log("SDL_QueryTexture error: %s", SDL_GetError());
            return;
        }

        // 2. 読み取り用の一時Targetテクスチャを作成
        SDL_Texture* targetTex = SDL_CreateTexture(
            renderer_,
            format,
            SDL_TEXTUREACCESS_TARGET,
            width,
            height
        );
        if (!targetTex) {
            SDL_Log("SDL_CreateTexture error: %s", SDL_GetError());
            return;
        }

        // 3. 現在の RenderTarget を退避
        SDL_Texture* previousTarget = SDL_GetRenderTarget(renderer_);

        // 4. 一時テクスチャをターゲットにして元テクスチャの内容を複製
        SDL_SetTextureBlendMode(targetTex, SDL_BLENDMODE_NONE);
        SDL_SetRenderTarget(renderer_, targetTex);
        SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 0); // アルファ0でクリア
        SDL_RenderClear(renderer_);
        SDL_RenderCopy(renderer_, canvas_.tex, NULL, NULL);

        // 5. ピクセルデータ保存用の Surface を作成
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, format);
        if (!surface) {
            SDL_Log("SDL_CreateRGBSurfaceWithFormat error: %s", SDL_GetError());
            SDL_SetRenderTarget(renderer_, previousTarget);
            SDL_DestroyTexture(targetTex);
            return;
        }

        // 6. 一時ターゲットからピクセル情報を読み込む
        int result = SDL_RenderReadPixels(
            renderer_,
            NULL,
            surface->format->format,
            surface->pixels,
            surface->pitch
        );

        // 7. レンダリングターゲットの復元 & 一時テクスチャの破棄
        SDL_SetRenderTarget(renderer_, previousTarget);
        SDL_DestroyTexture(targetTex);

        if (result != 0) {
            SDL_Log("SDL_RenderReadPixels error: %s", SDL_GetError());
            SDL_FreeSurface(surface);
            return;
        }

        // 8. ★ここを修正: SDL_SaveBMP → IMG_SavePNG に変更
        if (IMG_SavePNG(surface, filename.c_str()) != 0) {
            SDL_Log("IMG_SavePNG error: %s", IMG_GetError());
        }

        // 9. Surface の解放
        SDL_FreeSurface(surface);
    }
private:
    bool canvasDirty() const;

    SDL_Renderer* renderer_ = nullptr;
    SDL_Rect      bounds_ = { 0, 0, 0, 0 };

    // --- 以下、元の PaintApp のメンバをローカル座標系で保持 ---
    int   localW_ = 0, localH_ = 0; // = bounds_.w / bounds_.h
    paintwidget_detail::Canvas canvas_;

    float panX_ = 0, panY_ = 0, zoom_ = 1.f;
    paintwidget_detail::Tool currentTool_ = paintwidget_detail::TOOL_PEN;
    int   brushSize_ = 8;

    int colR_ = 80, colG_ = 130, colB_ = 220, colA_ = 255;
    SDL_Color drawColor() const { return { (Uint8)colR_, (Uint8)colG_, (Uint8)colB_, (Uint8)colA_ }; }

    paintwidget_detail::GridState grid_;
    paintwidget_detail::Button    btnGrid1_, btnGrid8_, btnGrid32_;

    paintwidget_detail::RgbaSlider slR_, slG_, slB_, slA_, slBrush_;
    paintwidget_detail::RgbaSlider* activeSlider_ = nullptr;
    paintwidget_detail::TextField* activeField_ = nullptr;

    paintwidget_detail::NewCanvasDialog dlgNew_;

    paintwidget_detail::Button toolBtns_[paintwidget_detail::TOOL_COUNT];
    paintwidget_detail::Button btnCopy_, btnCut_, btnPaste_, btnClear_, btnUndo_, btnRedo_, btnNew_;
    paintwidget_detail::Button* hoveredBtn_ = nullptr;

    bool     hasSelection_ = false;
    SDL_Rect selection_ = { 0, 0, 0, 0 };
    bool     selectDragging_ = false;
    int      selStartX_ = 0, selStartY_ = 0;

    std::vector<Uint32> clipboard_;
    int clipW_ = 0, clipH_ = 0;
    SDL_Texture* pastePreview_ = nullptr;

    bool pasting_ = false;
    int  pasteX_ = 0, pasteY_ = 0;

    bool  mouseDown_ = false;
    int   lastMX_ = 0, lastMY_ = 0;
    bool  panning_ = false;
    int   panStartMX_ = 0, panStartMY_ = 0;
    float panStartX_ = 0, panStartY_ = 0;

    int   cursorMX_ = 0, cursorMY_ = 0;
    bool  cursorInCanvas_ = false;
    SDL_Color hoverColor_ = { 255, 255, 255, 255 };
    bool      hoverColorValid_ = false;

    bool needRedraw_ = true;
    bool focused_ = false; // bounds内クリックで得るキーボードフォーカス

    // ────────────────────────────────────────────────
    SDL_Rect viewportRect() const; // ローカル座標系でのキャンバス表示領域
    void canvasToScreen(float cx, float cy, float& sx, float& sy) const { sx = cx * zoom_ + panX_; sy = cy * zoom_ + panY_; }
    void screenToCanvas(float sx, float sy, float& cx, float& cy) const { cx = (sx - panX_) / zoom_; cy = (sy - panY_) / zoom_; }
    SDL_Rect normalizedSelection() const;

    void setupUI();
    void centerCanvas();

    void paintLine(int x0, int y0, int x1, int y1);
    void rebuildPastePreview();

    std::vector<paintwidget_detail::TextField*> allFields();
    void commitActiveField();
    void deactivateAllFields();

    // イベントはすべて「ローカル座標(ウィジェット内座標)」で受け取る
    void handleMouseDownLocal(const SDL_Event& e, int lx, int ly);
    void handleMouseUpLocal(const SDL_Event& e);
    void handleMouseMoveLocal(int lx, int ly);
    void handleWheelLocal(const SDL_Event& e, int lx, int ly);
    void handleKeyLocal(const SDL_Event& e);
    void handleTextInputLocal(const SDL_Event& e);

    paintwidget_detail::Button* findButton(int mx, int my);
    paintwidget_detail::TextField* findField(int mx, int my);
    void doButtonAction(paintwidget_detail::Button* btn);

    void renderCanvas();
    void renderGrid();
    void renderBrushCursor();
    void renderEyedropperCursor();
    void renderSelection();
    void renderToolbar();
    void renderSidebar();

    
};