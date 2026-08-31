// ============================================================
// paint_widget.cpp
// PaintWidget の実装（元 paint-3.cpp のロジックを移植）
// ============================================================
#include "Paint_v2.h"
#include <iostream>
#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>

using namespace paintwidget_detail;

// ============================================================
// レイアウト定数（元コードと同じ値）
// ============================================================
static const int   TOOLBAR_H = 52;
static const int   SIDEBAR_W = 220;
static const float ZOOM_MIN = 0.05f;
static const float ZOOM_MAX = 32.0f;
static const int   BRUSH_MAX = 80;
static const int   UNDO_MAX = 50;

static const char* toolNames[] = { "Pen", "Eraser", "Fill", "Select", "Pick" };
static const char  toolKeys[] = { 'P', 'E', 'F', 'S', 'I' };

// ============================================================
// ユーティリティ
// ============================================================
static Uint32 colorToU32(SDL_Color c)
{
    return ((Uint32)c.a << 24) | ((Uint32)c.r << 16) | ((Uint32)c.g << 8) | c.b;
}
static SDL_Color u32ToColor(Uint32 v)
{
    return { (Uint8)(v >> 16), (Uint8)(v >> 8), (Uint8)v, (Uint8)(v >> 24) };
}

static void drawCircleOutline(SDL_Renderer* ren, int cx, int cy, int r)
{
    
    int x = r, y = 0, err = 0;
    while (x >= y) {
        SDL_RenderDrawPoint(ren, cx + x, cy + y); SDL_RenderDrawPoint(ren, cx + y, cy + x);
        SDL_RenderDrawPoint(ren, cx - y, cy + x); SDL_RenderDrawPoint(ren, cx - x, cy + y);
        SDL_RenderDrawPoint(ren, cx - x, cy - y); SDL_RenderDrawPoint(ren, cx - y, cy - x);
        SDL_RenderDrawPoint(ren, cx + y, cy - x); SDL_RenderDrawPoint(ren, cx + x, cy - y);
        y++; err += 2 * y - 1;
        if (2 * err - 2 * x + 1 > 0) { x--; err += 1 - 2 * x; }
    }
    
}

// ============================================================
// フォント（プロセス内で共有。複数の PaintWidget があっても1回だけロードする）
// ============================================================
static TTF_Font* gFont = nullptr;
static TTF_Font* gFontS = nullptr;
static bool      gFontLoadAttempted = false;

static void ensureFontsLoaded(TTF_Font* font = nullptr)
{
    if (gFontLoadAttempted) return;
    gFontLoadAttempted = true;
    if (!TTF_WasInit()) TTF_Init();
	if (!font){
        const char* fonts[] = {
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
            "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf",
            "/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf",
            "/usr/share/fonts/truetype/freefont/FreeSans.ttf", nullptr
        };
        for (int i = 0; fonts[i]; i++) { gFont = TTF_OpenFont(fonts[i], 13); if (gFont)  break; }
        for (int i = 0; fonts[i]; i++) { gFontS = TTF_OpenFont(fonts[i], 11); if (gFontS) break; }
    }
    else {
        gFont = font;
        gFontS = font;
    }
}

static void drawText(SDL_Renderer* ren, const std::string& txt,
    int x, int y, SDL_Color col, TTF_Font* font = nullptr)
{
    TTF_Font* f = font ? font : gFont;
    if (!f) return;
    SDL_Surface* surf = TTF_RenderUTF8_Solid(f, txt.c_str(), col);
    if (!surf) return;
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
    SDL_Rect dst{ x, y, surf->w, surf->h };
    SDL_FreeSurface(surf);
    if (!tex) return;
    SDL_RenderCopy(ren, tex, nullptr, &dst);
    SDL_DestroyTexture(tex);
}

// ============================================================
// paintwidget_detail の各構造体の実装
// ============================================================
namespace paintwidget_detail {

    bool TextField::hitTest(int mx, int my) const {
        return mx >= rect.x && mx < rect.x + rect.w && my >= rect.y && my < rect.y + rect.h;
    }
    void TextField::startEdit(int v) { char b[16]; snprintf(b, 16, "%d", v); text = b; active = true; }
    void TextField::stopEdit() { active = false; }
    int TextField::commitValue() const {
        if (text.empty()) return -1;
        try { int v = std::stoi(text); return std::max(minVal, std::min(maxVal, v)); }
        catch (...) { return -1; }
    }
    void TextField::onKeyDown(SDL_Keycode sym) {
        if (!active) return;
        if (sym == SDLK_BACKSPACE && !text.empty()) text.pop_back();
        else if (sym == SDLK_DELETE) text.clear();
    }
    void TextField::onTextInput(const char* inp) {
        if (!active) return;
        for (const char* p = inp; *p; p++)
            if (isdigit((unsigned char)*p) && (int)text.size() < maxLen) text += *p;
    }
    void TextField::draw(SDL_Renderer* ren, int currentValue) const {
        SDL_Color bg = active ? SDL_Color{ 50, 70, 100, 255 } : SDL_Color{ 45, 45, 55, 255 };
        SDL_SetRenderDrawColor(ren, bg.r, bg.g, bg.b, 255);
        SDL_RenderFillRect(ren, &rect);
        SDL_Color border = active ? SDL_Color{ 100, 150, 220, 255 } : SDL_Color{ 70, 70, 85, 255 };
        SDL_SetRenderDrawColor(ren, border.r, border.g, border.b, 255);
        SDL_RenderDrawRect(ren, &rect);
        std::string disp = active ? text + "|" : std::to_string(currentValue);
        drawText(ren, disp, rect.x + 3, rect.y + 2, { 220, 220, 220, 255 });
    }

    SDL_Rect RgbaSlider::thumbRect(int val) const {
        float t = (float)val / 255.f;
        return { track.x + (int)(t * (track.w - 14)), track.y, 14, track.h };
    }
    bool RgbaSlider::hitTrack(int mx, int my) const {
        return mx >= track.x && mx < track.x + track.w && my >= track.y && my < track.y + track.h;
    }
    int RgbaSlider::xToVal(int mx) const {
        float t = (float)(mx - track.x) / (track.w - 14);
        return (int)(std::max(0.f, std::min(1.f, t)) * 255.f + 0.5f);
    }
    void RgbaSlider::setup(int lx, int y, int lw, const std::string& lbl, SDL_Color bc) {
        label = lbl; barCol = bc;
        int fw = 38;
        track = { lx, y, lw - fw - 4, 14 };
        field.rect = { lx + lw - fw, y - 1, fw, 16 };
        field.maxLen = 3; field.minVal = 0; field.maxVal = 255;
    }
    void RgbaSlider::draw(SDL_Renderer* ren, int val) const {
        drawText(ren, label, track.x, track.y - 18, { 180, 180, 200, 255 });
        SDL_SetRenderDrawColor(ren, 50, 50, 55, 255);
        SDL_RenderFillRect(ren, &track);
        int fw = (int)((float)val / 255.f * (track.w - 14)) + 7;
        SDL_Rect filled{ track.x, track.y, fw, track.h };
        SDL_SetRenderDrawColor(ren, barCol.r, barCol.g, barCol.b, 200);
        SDL_RenderFillRect(ren, &filled);
        SDL_Rect th = thumbRect(val);
        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_RenderFillRect(ren, &th);
        field.draw(ren, val);
    }

    void Button::draw(SDL_Renderer* ren) const {
        SDL_Color c = pressed ? SDL_Color{ 50, 50, 60, 255 } : hovered ? bgHover : bg;
        SDL_SetRenderDrawColor(ren, c.r, c.g, c.b, c.a);
        SDL_RenderFillRect(ren, &rect);
        SDL_SetRenderDrawColor(ren, 120, 120, 130, 255);
        SDL_RenderDrawRect(ren, &rect);
        int tw = 0, th = 0;
        if (gFont) TTF_SizeUTF8(gFont, label.c_str(), &tw, &th);
        drawText(ren, label, rect.x + (rect.w - tw) / 2, rect.y + (rect.h - th) / 2, { 220, 220, 220, 255 });
    }

    void NewCanvasDialog::open(int currentW, int currentH) {
        visible = true; confirmed = false;
        fieldW.minVal = 1; fieldW.maxVal = 9999; fieldW.maxLen = 4;
        fieldH.minVal = 1; fieldH.maxVal = 9999; fieldH.maxLen = 4;
        fieldW.startEdit(currentW);
        fieldH.startEdit(currentH);
    }
    void NewCanvasDialog::layout(int localW, int localH) {
        int dw = 320, dh = 180;
        int dx = (localW - dw) / 2, dy = (localH - dh) / 2;
        fieldW.rect = { dx + 100, dy + 50, 100, 22 };
        fieldH.rect = { dx + 100, dy + 90, 100, 22 };
        btnOk.rect = { dx + 60, dy + 130, 80, 28 }; btnOk.label = "OK";
        btnCancel.rect = { dx + 180, dy + 130, 80, 28 }; btnCancel.label = "Cancel";
    }
    bool NewCanvasDialog::handleMouseDown(int mx, int my) {
        if (!visible) return false;
        if (fieldW.hitTest(mx, my)) { fieldW.active = true; fieldH.active = false; SDL_StartTextInput(); return true; }
        if (fieldH.hitTest(mx, my)) { fieldH.active = true; fieldW.active = false; SDL_StartTextInput(); return true; }
        auto hit = [&](Button& b) { return mx >= b.rect.x && mx < b.rect.x + b.rect.w && my >= b.rect.y && my < b.rect.y + b.rect.h; };
        if (hit(btnOk)) {
            int w = fieldW.commitValue(), h = fieldH.commitValue();
            if (w > 0 && h > 0) { resultW = w; resultH = h; confirmed = true; visible = false; SDL_StopTextInput(); }
            return true;
        }
        if (hit(btnCancel)) { visible = false; SDL_StopTextInput(); return true; }
        return true;
    }
    bool NewCanvasDialog::handleKey(SDL_Keycode sym) {
        if (!visible) return false;
        if (sym == SDLK_TAB) {
            if (fieldW.active) { fieldW.active = false; fieldH.active = true; }
            else { fieldH.active = false; fieldW.active = true; }
            return true;
        }
        if (sym == SDLK_RETURN || sym == SDLK_KP_ENTER) {
            int w = fieldW.commitValue(), h = fieldH.commitValue();
            if (w > 0 && h > 0) { resultW = w; resultH = h; confirmed = true; visible = false; SDL_StopTextInput(); }
            return true;
        }
        if (sym == SDLK_ESCAPE) { visible = false; SDL_StopTextInput(); return true; }
        if (fieldW.active) fieldW.onKeyDown(sym);
        if (fieldH.active) fieldH.onKeyDown(sym);
        return true;
    }
    bool NewCanvasDialog::handleTextInput(const char* t) {
        if (!visible) return false;
        if (fieldW.active) fieldW.onTextInput(t);
        if (fieldH.active) fieldH.onTextInput(t);
        return true;
    }
    void NewCanvasDialog::draw(SDL_Renderer* ren, int localW, int localH) {
        if (!visible) return;
        layout(localW, localH);
        int dw = 320, dh = 180;
        int dx = (localW - dw) / 2, dy = (localH - dh) / 2;
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 150);
        SDL_Rect shade = { 0, 0, localW, localH }; SDL_RenderFillRect(ren, &shade);
        SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_NONE);
        SDL_Rect dlg = { dx, dy, dw, dh };
        SDL_SetRenderDrawColor(ren, 50, 52, 62, 255); SDL_RenderFillRect(ren, &dlg);
        SDL_SetRenderDrawColor(ren, 100, 100, 120, 255); SDL_RenderDrawRect(ren, &dlg);
        SDL_Rect title = { dx, dy, dw, 28 };
        SDL_SetRenderDrawColor(ren, 40, 80, 140, 255); SDL_RenderFillRect(ren, &title);
        drawText(ren, "New Canvas", dx + 10, dy + 6, { 220, 220, 230, 255 });
        drawText(ren, "Width:", dx + 20, dy + 54, { 180, 180, 200, 255 });
        drawText(ren, "Height:", dx + 20, dy + 94, { 180, 180, 200, 255 });
        fieldW.draw(ren, fieldW.commitValue() > 0 ? fieldW.commitValue() : 0);
        fieldH.draw(ren, fieldH.commitValue() > 0 ? fieldH.commitValue() : 0);
        btnOk.draw(ren); btnCancel.draw(ren);
        drawText(ren, "Tab: switch  Enter: OK  Esc: Cancel", dx + 10, dy + dh - 22, { 100, 100, 120, 255 }, gFontS);
    }

    void Canvas::pushUndo() {
        if ((int)undoStack.size() >= UNDO_MAX) undoStack.pop_front();
        undoStack.push_back(pixels); redoStack.clear();
    }
    bool Canvas::undo() {
        if (undoStack.empty()) return false;
        redoStack.push_back(pixels);
        if ((int)redoStack.size() > UNDO_MAX) redoStack.pop_front();
        pixels = undoStack.back(); undoStack.pop_back(); dirty = true; return true;
    }
    bool Canvas::redo() {
        if (redoStack.empty()) return false;
        undoStack.push_back(pixels);
        if ((int)undoStack.size() > UNDO_MAX) undoStack.pop_front();
        pixels = redoStack.back(); redoStack.pop_back(); dirty = true; return true;
    }
    void Canvas::init(SDL_Renderer* ren, int W, int H) {
        // 初期状態は白ではなく「完全に透明」(alpha=0) にする。
        // こうしておくと未着色の部分がチェッカー柄(市松模様)のまま見え、
        // 書き出したPNG等でも背景が透明として扱える。
        w = W; h = H; pixels.assign((size_t)W * H, 0x00000000);
        undoStack.clear(); redoStack.clear();
        if (tex) SDL_DestroyTexture(tex);
        tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, W, H);
        // テクスチャのアルファ値を実際に反映させて描画するために必須。
        // これが無いと(デフォルトの BLENDMODE_NONE のままだと)、
        // コピー時にアルファが無視されて常に不透明として描画されてしまう。
        SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
        flush();
    }
    void Canvas::flush() { if (!tex) return; SDL_UpdateTexture(tex, nullptr, pixels.data(), w * 4); dirty = false; }
    void Canvas::blendPixel(int x, int y, SDL_Color col) {
        if (!inBounds(x, y)) return;
        if (col.a == 0) return;
        Uint32& dst = at(x, y);
        if (col.a == 255) { dst = colorToU32(col); return; }

        // 背景が透明(alpha=0)の場合でも縁が黒ずまないよう、
        // 出力先のアルファ値も考慮した正式な "source-over" 合成を行う。
        // (背景が不透明な場合は従来と同じ結果になる)
        SDL_Color d = u32ToColor(dst);
        float     sa = col.a / 255.f;
        float     da = d.a / 255.f;
        float     outA = sa + da * (1.f - sa);
        if (outA <= 0.0001f) { dst = 0; return; }
        auto comp = [&](Uint8 sc, Uint8 dc) -> Uint8 {
            float v = (sc * sa + dc * da * (1.f - sa)) / outA;
            return (Uint8)std::round(std::max(0.f, std::min(255.f, v)));
            };
        SDL_Color out;
        out.r = comp(col.r, d.r); out.g = comp(col.g, d.g); out.b = comp(col.b, d.b);
        out.a = (Uint8)std::round(std::max(0.f, std::min(255.f, outA * 255.f)));
        dst = colorToU32(out);
    }
    void Canvas::paintBrush(int cx, int cy, int radius, SDL_Color col) {
		if (radius <= 1) { if (inBounds(cx, cy)) blendPixel(cx, cy, col); dirty = true; return; }
        for (int dy = -radius; dy <= radius; dy++) {
            for (int dx = -radius; dx <= radius; dx++) {
                // 円の半径内であればアルファ減衰を行わずにそのまま描画
                if (dx * dx + dy * dy <= radius * radius) {
                    blendPixel(cx + dx, cy + dy, col);
                }
            }
        }
        dirty = true;
    }
    void Canvas::erase(int cx, int cy, int radius) {
        // 消しゴムは「白で塗る」のではなく、透明に戻す
        if (radius <= 1) { if (inBounds(cx, cy)) at(cx, cy) = 0x00000000; dirty = true; return; }
        for (int dy = -radius; dy <= radius; dy++) for (int dx = -radius; dx <= radius; dx++)
            if (dx * dx + dy * dy <= radius * radius && inBounds(cx + dx, cy + dy)) at(cx + dx, cy + dy) = 0x00000000;
        dirty = true;
    }
    void Canvas::fill(int sx, int sy, SDL_Color col) {
        if (!inBounds(sx, sy)) return;
        Uint32 target = at(sx, sy), fc = colorToU32(col); if (target == fc) return;
        std::vector<int> stk; stk.reserve(4096); stk.push_back(sy * w + sx);
        while (!stk.empty()) {
            int idx = stk.back(); stk.pop_back(); if (pixels[idx] != target) continue;
            pixels[idx] = fc; int x = idx % w, y = idx / w;
            if (x > 0)     stk.push_back(idx - 1);
            if (x < w - 1) stk.push_back(idx + 1);
            if (y > 0)     stk.push_back(idx - w);
            if (y < h - 1) stk.push_back(idx + w);
        }
        dirty = true;
    }
    std::vector<Uint32> Canvas::copyRegion(SDL_Rect r) const {
        r.x = std::max(0, r.x); r.y = std::max(0, r.y);
        r.w = std::min(r.w, w - r.x); r.h = std::min(r.h, h - r.y);
        std::vector<Uint32> buf((size_t)r.w * r.h);
        for (int row = 0; row < r.h; row++) memcpy(buf.data() + row * r.w, pixels.data() + (size_t)(r.y + row) * w + r.x, r.w * 4);
        return buf;
    }
    void Canvas::clearRegion(SDL_Rect r) {
        r.x = std::max(0, r.x); r.y = std::max(0, r.y);
        r.w = std::min(r.w, w - r.x); r.h = std::min(r.h, h - r.y);
        for (int row = 0; row < r.h; row++) for (int col = 0; col < r.w; col++) at(r.x + col, r.y + row) = 0x00000000;
        dirty = true;
    }
    void Canvas::paste(int px, int py, const std::vector<Uint32>& buf, int bw, int bh) {
        for (int row = 0; row < bh; row++) for (int col = 0; col < bw; col++)
            if (inBounds(px + col, py + row)) at(px + col, py + row) = buf[row * bw + col];
        dirty = true;
    }
    SDL_Color Canvas::pickColor(int cx, int cy) const {
        if (!inBounds(cx, cy)) return { 255, 255, 255, 255 };
        return u32ToColor(at(cx, cy));
    }
    void Canvas::destroy() {
        if (tex) { SDL_DestroyTexture(tex); tex = nullptr; }
        pixels.clear(); undoStack.clear(); redoStack.clear();
    }

} // namespace paintwidget_detail


// ============================================================
// PaintWidget 実装
// ============================================================
bool PaintWidget::init(SDL_Renderer* renderer, SDL_Rect bounds, int initialCanvasW, int initialCanvasH, TTF_Font* cusFont)
{
    renderer_ = renderer;
    bounds_ = bounds;
    localW_ = bounds.w;
    localH_ = bounds.h;

    ensureFontsLoaded(cusFont);

    setupUI();
    newCanvas(initialCanvasW, initialCanvasH);

    // 起動直後に新規キャンバスダイアログを開く（元アプリと同じ挙動）
    dlgNew_.open(initialCanvasW, initialCanvasH);
    return true;
}
void PaintWidget::dlgNew_OUT() {
	dlgNew_.open(canvas_.w, canvas_.h);
}

void PaintWidget::setBounds(const SDL_Rect& bounds)
{
    bounds_ = bounds;
    localW_ = bounds.w;
    localH_ = bounds.h;
    setupUI();
    needRedraw_ = true;
}

void PaintWidget::cleanup()
{
    if (pastePreview_) { SDL_DestroyTexture(pastePreview_); pastePreview_ = nullptr; }
    canvas_.destroy();
}

void PaintWidget::shutdownShared(bool use_uiFont)
{
    if (!use_uiFont) {
        if (gFont) { TTF_CloseFont(gFont);  gFont = nullptr; }
        if (gFontS) { TTF_CloseFont(gFontS); gFontS = nullptr; }
    }
    gFontLoadAttempted = false;
}

bool PaintWidget::canvasDirty() const { return canvas_.dirty; }

bool PaintWidget::wantsTextInput() const
{
    return dlgNew_.visible || activeField_ != nullptr;
}

// ────────────────────────────────────────────────
SDL_Rect PaintWidget::viewportRect() const
{
    return { SIDEBAR_W, TOOLBAR_H, localW_ - SIDEBAR_W, localH_ - TOOLBAR_H };
}

SDL_Rect PaintWidget::normalizedSelection() const {
    SDL_Rect r = selection_;
    if (r.w < 0) { r.x += r.w; r.w = -r.w; } if (r.h < 0) { r.y += r.h; r.h = -r.h; } return r;
}

void PaintWidget::setupUI()
{
    int bx = 200, by = 8, bw = 46, bh = 36;
    for (int i = 0; i < TOOL_COUNT; i++) {
        toolBtns_[i].rect = { bx + i * (bw + 3), by, bw, bh };
        toolBtns_[i].label = toolNames[i];
    }
    int ex = bx + TOOL_COUNT * (bw + 3) + 14;
    btnNew_.rect = { ex, by, 40, bh };       btnNew_.label = "New";
    btnCopy_.rect = { ex + 40, by, 40, bh };  btnCopy_.label = "Copy";
    btnCut_.rect = { ex + 80, by, 40, bh };  btnCut_.label = "Cut";
    btnPaste_.rect = { ex + 120, by, 40, bh }; btnPaste_.label = "Paste";
    btnClear_.rect = { ex + 160, by, 40, bh }; btnClear_.label = "Clear";
    btnUndo_.rect = { ex + 210, by, 40, bh }; btnUndo_.label = "Undo";
    btnRedo_.rect = { ex + 250, by, 40, bh }; btnRedo_.label = "Redo";

    int gx = localW_ - SIDEBAR_W - 10;
    btnGrid1_.rect = { gx - 10, by, 50, bh };        btnGrid1_.label = "G:1px";
    btnGrid8_.rect = { gx + 44, by, 50, bh };   btnGrid8_.label = "G:8px";
    btnGrid32_.rect = { gx + 98, by, 58, bh };  btnGrid32_.label = "G:32px";

    int lx = localW_ - SIDEBAR_W + 10, lw = SIDEBAR_W - 20, ly = TOOLBAR_H + 100;
    slR_.setup(lx, ly, lw, "R", { 220, 80, 80, 255 });    ly += 40;
    slG_.setup(lx, ly, lw, "G", { 80, 200, 80, 255 });    ly += 40;
    slB_.setup(lx, ly, lw, "B", { 80, 120, 220, 255 });   ly += 40;
    slA_.setup(lx, ly, lw, "A", { 180, 180, 180, 255 });  ly += 50;
    slBrush_.setup(lx, ly, lw, "Size", { 120, 160, 220, 255 });
    slBrush_.field.maxLen = 2; slBrush_.field.minVal = 1; slBrush_.field.maxVal = BRUSH_MAX;
}

void PaintWidget::centerCanvas()
{
    SDL_Rect vp = viewportRect();
    panX_ = vp.x + (vp.w - canvas_.w * zoom_) / 2.f;
    panY_ = vp.y + (vp.h - canvas_.h * zoom_) / 2.f;
}

void PaintWidget::newCanvas(int w, int h)
{
    canvas_.init(renderer_, w, h);
    zoom_ = 1.f;
    centerCanvas();
    hasSelection_ = false; pasting_ = false;
    needRedraw_ = true;
}

// ────────────────────────────────────────────────
std::vector<TextField*> PaintWidget::allFields()
{
    return { &slR_.field, &slG_.field, &slB_.field, &slA_.field, &slBrush_.field };
}

void PaintWidget::commitActiveField()
{
    if (!activeField_) return;
    int v = activeField_->commitValue();
    if (v >= 0) {
        if (activeField_ == &slR_.field) colR_ = v;
        else if (activeField_ == &slG_.field) colG_ = v;
        else if (activeField_ == &slB_.field) colB_ = v;
        else if (activeField_ == &slA_.field) colA_ = v;
        else if (activeField_ == &slBrush_.field) brushSize_ = std::max(1, std::min(BRUSH_MAX, v));
    }
    activeField_->stopEdit(); activeField_ = nullptr; needRedraw_ = true;
}
void PaintWidget::deactivateAllFields()
{
    commitActiveField();
    for (auto* f : allFields()) f->stopEdit();
    activeField_ = nullptr;
}

// ────────────────────────────────────────────────
void PaintWidget::paintLine(int x0, int y0, int x1, int y1)
{
    SDL_Color col = drawColor();
    int dx = abs(x1 - x0), dy = abs(y1 - y0), sx = (x0 < x1) ? 1 : -1, sy = (y0 < y1) ? 1 : -1, err = dx - dy;
    while (true) {
        if (currentTool_ == TOOL_PEN) canvas_.paintBrush(x0, y0, brushSize_, col);
        else                          canvas_.erase(x0, y0, brushSize_);
        if (x0 == x1 && y0 == y1) break;
        int e2 = 2 * err;
        if (e2 > -dy) { err -= dy; x0 += sx; } if (e2 < dx) { err += dx; y0 += sy; }
    }
}

void PaintWidget::rebuildPastePreview()
{
    if (pastePreview_) { SDL_DestroyTexture(pastePreview_); pastePreview_ = nullptr; }
    if (clipboard_.empty() || clipW_ <= 0 || clipH_ <= 0) return;
    pastePreview_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, clipW_, clipH_);
    SDL_SetTextureBlendMode(pastePreview_, SDL_BLENDMODE_BLEND); // 透明ピクセルを正しく合成表示する
    SDL_UpdateTexture(pastePreview_, nullptr, clipboard_.data(), clipW_ * 4);
    SDL_SetTextureAlphaMod(pastePreview_, 180);
}

// ────────────────────────────────────────────────
Button* PaintWidget::findButton(int mx, int my)
{
    auto hit = [&](Button& b) { return mx >= b.rect.x && mx < b.rect.x + b.rect.w && my >= b.rect.y && my < b.rect.y + b.rect.h; };
    for (auto& b : toolBtns_) if (hit(b)) return &b;
    if (hit(btnNew_))    return &btnNew_;
    if (hit(btnCopy_))   return &btnCopy_;
    if (hit(btnCut_))    return &btnCut_;
    if (hit(btnPaste_))  return &btnPaste_;
    if (hit(btnClear_))  return &btnClear_;
    if (hit(btnUndo_))   return &btnUndo_;
    if (hit(btnRedo_))   return &btnRedo_;
    if (hit(btnGrid1_))  return &btnGrid1_;
    if (hit(btnGrid8_))  return &btnGrid8_;
    if (hit(btnGrid32_)) return &btnGrid32_;
    return nullptr;
}
TextField* PaintWidget::findField(int mx, int my)
{
    for (auto* f : allFields()) if (f->hitTest(mx, my)) return f;
    return nullptr;
}

void PaintWidget::doButtonAction(Button* btn)
{
    if (!btn) return;
    for (int i = 0; i < TOOL_COUNT; i++) if (btn == &toolBtns_[i]) { currentTool_ = (Tool)i; return; }

    if (btn == &btnNew_) { dlgNew_.open(canvas_.w, canvas_.h); SDL_StartTextInput(); }
    else if (btn == &btnGrid1_)  grid_.show1 = !grid_.show1;
    else if (btn == &btnGrid8_)  grid_.show8 = !grid_.show8;
    else if (btn == &btnGrid32_) grid_.show32 = !grid_.show32;
    else if (btn == &btnCopy_) {
        if (hasSelection_) { SDL_Rect r = normalizedSelection(); clipboard_ = canvas_.copyRegion(r); clipW_ = r.w; clipH_ = r.h; rebuildPastePreview(); }
    }
    else if (btn == &btnCut_) {
        if (hasSelection_) { SDL_Rect r = normalizedSelection(); canvas_.pushUndo(); clipboard_ = canvas_.copyRegion(r); clipW_ = r.w; clipH_ = r.h; rebuildPastePreview(); canvas_.clearRegion(r); hasSelection_ = false; }
    }
    else if (btn == &btnPaste_) {
        if (!clipboard_.empty()) { SDL_Rect vp = viewportRect(); float cx, cy; screenToCanvas(vp.x + vp.w / 2.f, vp.y + vp.h / 2.f, cx, cy); pasteX_ = (int)cx - clipW_ / 2; pasteY_ = (int)cy - clipH_ / 2; pasting_ = true; }
    }
    else if (btn == &btnClear_) {
        canvas_.pushUndo(); SDL_Rect all = { 0, 0, canvas_.w, canvas_.h }; canvas_.clearRegion(all);
    }
    else if (btn == &btnUndo_) { canvas_.undo(); }
    else if (btn == &btnRedo_) { canvas_.redo(); }
}

// ============================================================
// イベント処理（ホストから絶対座標のイベントを受け取り、bounds_ に
// 対する相対(ローカル)座標に変換してから各ハンドラに渡す）
// ============================================================
void PaintWidget::handleEvent(const SDL_Event& e)
{
    switch (e.type) {
    case SDL_MOUSEBUTTONDOWN: {
        int lx = e.button.x - bounds_.x, ly = e.button.y - bounds_.y;
        // bounds の外で起きたクリックは他のウィジェット宛てなので無視する
        if (lx < 0 || ly < 0 || lx >= bounds_.w || ly >= bounds_.h) return;
        focused_ = true;
        handleMouseDownLocal(e, lx, ly);
        break;
    }
    case SDL_MOUSEBUTTONUP: {
        // ドラッグ中に bounds の外でボタンを離すこともあるので常に処理する
        handleMouseUpLocal(e);
        break;
    }
    case SDL_MOUSEMOTION: {
        int lx = e.motion.x - bounds_.x, ly = e.motion.y - bounds_.y;
        handleMouseMoveLocal(lx, ly);
        break;
    }
    case SDL_MOUSEWHEEL: {
        int gmx, gmy; SDL_GetMouseState(&gmx, &gmy);
        int lx = gmx - bounds_.x, ly = gmy - bounds_.y;
        if (lx < 0 || ly < 0 || lx >= bounds_.w || ly >= bounds_.h) return;
        handleWheelLocal(e, lx, ly);
        break;
    }
    case SDL_KEYDOWN:
        // このウィジェットがフォーカスを持っている（クリックされた実績がある）
        // か、ダイアログ/テキスト編集中のときだけキー入力を消費する。
        if (focused_ || wantsTextInput()) handleKeyLocal(e);
        break;
    case SDL_TEXTINPUT:
        if (focused_ || wantsTextInput()) handleTextInputLocal(e);
        break;
    default:
        break;
    }
}

void PaintWidget::handleMouseDownLocal(const SDL_Event& e, int mx, int my)
{
    needRedraw_ = true;

    if (dlgNew_.visible) { dlgNew_.handleMouseDown(mx, my); return; }

    if (e.button.button == SDL_BUTTON_LEFT) {
        TextField* tf = findField(mx, my);
        if (tf) {
            deactivateAllFields();
            int cur = 0;
            if (tf == &slR_.field) cur = colR_; else if (tf == &slG_.field) cur = colG_;
            else if (tf == &slB_.field) cur = colB_; else if (tf == &slA_.field) cur = colA_;
            else if (tf == &slBrush_.field) cur = brushSize_;
            tf->startEdit(cur); activeField_ = tf; SDL_StartTextInput(); return;
        }
        if (activeField_) { commitActiveField(); SDL_StopTextInput(); }

        auto hitSl = [&](RgbaSlider& sl) -> bool { return sl.hitTrack(mx, my) && !sl.field.hitTest(mx, my); };
        if (hitSl(slR_)) { activeSlider_ = &slR_;     slR_.dragging = true;     colR_ = slR_.xToVal(mx); return; }
        if (hitSl(slG_)) { activeSlider_ = &slG_;     slG_.dragging = true;     colG_ = slG_.xToVal(mx); return; }
        if (hitSl(slB_)) { activeSlider_ = &slB_;     slB_.dragging = true;     colB_ = slB_.xToVal(mx); return; }
        if (hitSl(slA_)) { activeSlider_ = &slA_;     slA_.dragging = true;     colA_ = slA_.xToVal(mx); return; }
        if (hitSl(slBrush_)) { activeSlider_ = &slBrush_; slBrush_.dragging = true; brushSize_ = std::max(1, (int)(slBrush_.xToVal(mx) * (BRUSH_MAX - 1) / 255) + 1); return; }
    }

    if (Button* btn = findButton(mx, my)) { btn->pressed = true; doButtonAction(btn); return; }

    SDL_Rect vp = viewportRect();
    bool inVp = (mx >= vp.x && mx < vp.x + vp.w && my >= vp.y && my < vp.y + vp.h);

    if (e.button.button == SDL_BUTTON_MIDDLE) {
        panning_ = true; panStartMX_ = mx; panStartMY_ = my; panStartX_ = panX_; panStartY_ = panY_; return;
    }
    if (!inVp) return;

    float cx, cy; screenToCanvas((float)mx, (float)my, cx, cy);

    if (e.button.button == SDL_BUTTON_LEFT) {
        if (pasting_) { canvas_.pushUndo(); canvas_.paste(pasteX_, pasteY_, clipboard_, clipW_, clipH_); pasting_ = false; return; }
        switch (currentTool_) {
        case TOOL_SELECT:
            hasSelection_ = false; selectDragging_ = true;
            selStartX_ = (int)cx; selStartY_ = (int)cy; selection_ = { selStartX_, selStartY_, 0, 0 }; break;
        case TOOL_FILL:
            canvas_.pushUndo(); canvas_.fill((int)cx, (int)cy, drawColor()); break;
        case TOOL_EYEDROPPER: {
            SDL_Color p = canvas_.pickColor((int)cx, (int)cy);
            colR_ = p.r; colG_ = p.g; colB_ = p.b; colA_ = p.a; currentTool_ = TOOL_PEN; break;
        }
        default:
            canvas_.pushUndo(); mouseDown_ = true;
            lastMX_ = (int)cx; lastMY_ = (int)cy;
            //canvas_.paintBrush((int)cx, (int)cy, brushSize_, drawColor()); 
            break;
        }
    }
    if (e.button.button == SDL_BUTTON_RIGHT) {
        panning_ = true; panStartMX_ = mx; panStartMY_ = my; panStartX_ = panX_; panStartY_ = panY_;
    }
}

void PaintWidget::handleMouseUpLocal(const SDL_Event& e)
{
    needRedraw_ = true;
    if (activeSlider_) { activeSlider_->dragging = false; activeSlider_ = nullptr; }
    for (auto& b : toolBtns_) b.pressed = false;
    btnNew_.pressed = btnCopy_.pressed = btnCut_.pressed = btnPaste_.pressed =
        btnClear_.pressed = btnUndo_.pressed = btnRedo_.pressed =
        btnGrid1_.pressed = btnGrid8_.pressed = btnGrid32_.pressed = false;
    if (e.button.button == SDL_BUTTON_MIDDLE || e.button.button == SDL_BUTTON_RIGHT) panning_ = false;
    if (e.button.button == SDL_BUTTON_LEFT) {
        mouseDown_ = false;
        if (selectDragging_) { selectDragging_ = false; SDL_Rect r = normalizedSelection(); hasSelection_ = (r.w > 2 && r.h > 2); }
    }
}

void PaintWidget::handleMouseMoveLocal(int mx, int my)
{
    cursorMX_ = mx; cursorMY_ = my; needRedraw_ = true;

    if (activeSlider_) {
        if (activeSlider_ == &slR_)      colR_ = slR_.xToVal(mx);
        else if (activeSlider_ == &slG_) colG_ = slG_.xToVal(mx);
        else if (activeSlider_ == &slB_) colB_ = slB_.xToVal(mx);
        else if (activeSlider_ == &slA_) colA_ = slA_.xToVal(mx);
        else if (activeSlider_ == &slBrush_) brushSize_ = std::max(1, (int)(slBrush_.xToVal(mx) * (BRUSH_MAX - 1) / 255) + 1);
        return;
    }

    hoveredBtn_ = findButton(mx, my);
    for (auto& b : toolBtns_) b.hovered = (&b == hoveredBtn_);
    btnNew_.hovered = (&btnNew_ == hoveredBtn_); btnCopy_.hovered = (&btnCopy_ == hoveredBtn_);
    btnCut_.hovered = (&btnCut_ == hoveredBtn_); btnPaste_.hovered = (&btnPaste_ == hoveredBtn_);
    btnClear_.hovered = (&btnClear_ == hoveredBtn_); btnUndo_.hovered = (&btnUndo_ == hoveredBtn_);
    btnRedo_.hovered = (&btnRedo_ == hoveredBtn_);
    btnGrid1_.hovered = (&btnGrid1_ == hoveredBtn_); btnGrid8_.hovered = (&btnGrid8_ == hoveredBtn_);
    btnGrid32_.hovered = (&btnGrid32_ == hoveredBtn_);

    SDL_Rect vp = viewportRect();
    cursorInCanvas_ = (mx >= vp.x && mx < vp.x + vp.w && my >= vp.y && my < vp.y + vp.h);
    if (cursorInCanvas_) { float cx, cy; screenToCanvas((float)mx, (float)my, cx, cy); hoverColor_ = canvas_.pickColor((int)cx, (int)cy); hoverColorValid_ = true; }
    else hoverColorValid_ = false;

    if (panning_) { panX_ = panStartX_ + (mx - panStartMX_); panY_ = panStartY_ + (my - panStartMY_); return; }

    float cx, cy; screenToCanvas((float)mx, (float)my, cx, cy);
    if (pasting_) { pasteX_ = (int)cx - clipW_ / 2; pasteY_ = (int)cy - clipH_ / 2; return; }
    if (selectDragging_) { selection_.w = (int)cx - selStartX_; selection_.h = (int)cy - selStartY_; return; }
    if (mouseDown_) { int icx = (int)cx, icy = (int)cy; paintLine(lastMX_, lastMY_, icx, icy); lastMX_ = icx; lastMY_ = icy; }
}

void PaintWidget::handleWheelLocal(const SDL_Event& e, int mx, int my)
{
    if (dlgNew_.visible) return;
    float old = zoom_;
    zoom_ = std::max(ZOOM_MIN, std::min(ZOOM_MAX, zoom_ * ((e.wheel.y > 0) ? 1.15f : 1.f / 1.15f)));
    panX_ = mx - (mx - panX_) * (zoom_ / old); panY_ = my - (my - panY_) * (zoom_ / old);
    needRedraw_ = true;
}

void PaintWidget::handleKeyLocal(const SDL_Event& e)
{
    if (dlgNew_.visible) { if (dlgNew_.handleKey(e.key.keysym.sym)) needRedraw_ = true; return; }

    bool ctrl = (e.key.keysym.mod & KMOD_CTRL) != 0;
    bool shift = (e.key.keysym.mod & KMOD_SHIFT) != 0;
    needRedraw_ = true;

    if (activeField_) {
        switch (e.key.keysym.sym) {
        case SDLK_RETURN: case SDLK_KP_ENTER: commitActiveField(); SDL_StopTextInput(); return;
        case SDLK_ESCAPE: activeField_->stopEdit(); activeField_ = nullptr; SDL_StopTextInput(); return;
        case SDLK_BACKSPACE: activeField_->onKeyDown(SDLK_BACKSPACE); return;
        case SDLK_DELETE:    activeField_->onKeyDown(SDLK_DELETE); return;
        default: return;
        }
    }

    if (ctrl) {
        switch (e.key.keysym.sym) {
        case SDLK_n: dlgNew_.open(canvas_.w, canvas_.h); SDL_StartTextInput(); break;
        case SDLK_z: if (shift) canvas_.redo(); else canvas_.undo(); break;
        case SDLK_y: canvas_.redo(); break;
        case SDLK_c:
            if (hasSelection_) { SDL_Rect r = normalizedSelection(); clipboard_ = canvas_.copyRegion(r); clipW_ = r.w; clipH_ = r.h; rebuildPastePreview(); } break;
        case SDLK_x:
            if (hasSelection_) { SDL_Rect r = normalizedSelection(); canvas_.pushUndo(); clipboard_ = canvas_.copyRegion(r); clipW_ = r.w; clipH_ = r.h; rebuildPastePreview(); canvas_.clearRegion(r); hasSelection_ = false; } break;
        case SDLK_v:
            if (!clipboard_.empty()) { SDL_Rect vp = viewportRect(); float cx, cy; screenToCanvas(vp.x + vp.w / 2.f, vp.y + vp.h / 2.f, cx, cy); pasteX_ = (int)cx - clipW_ / 2; pasteY_ = (int)cy - clipH_ / 2; pasting_ = true; } break;
        case SDLK_a: hasSelection_ = true; selection_ = { 0, 0, canvas_.w, canvas_.h }; break;
        }
    }
    else {
        SDL_Keycode sym = e.key.keysym.sym;
        for (int i = 0; i < TOOL_COUNT; i++)
            if (sym == (SDL_Keycode)tolower(toolKeys[i]) || sym == (SDL_Keycode)toolKeys[i]) { currentTool_ = (Tool)i; return; }
        switch (sym) {
        case SDLK_ESCAPE: pasting_ = false; hasSelection_ = false; break;
        case SDLK_EQUALS: case SDLK_KP_PLUS:  brushSize_ = std::min(BRUSH_MAX, brushSize_ + 2); break;
        case SDLK_MINUS:  case SDLK_KP_MINUS: brushSize_ = std::max(1, brushSize_ - 2); break;
        case SDLK_0: case SDLK_KP_0: zoom_ = 1.f; centerCanvas(); break;
        case SDLK_1: grid_.show1 = !grid_.show1;  break;
        case SDLK_8: grid_.show8 = !grid_.show8;  break;
        case SDLK_3: grid_.show32 = !grid_.show32; break;
        }
    }
}

void PaintWidget::handleTextInputLocal(const SDL_Event& e)
{
    if (dlgNew_.visible) { dlgNew_.handleTextInput(e.text.text); needRedraw_ = true; return; }
    if (activeField_) { activeField_->onTextInput(e.text.text); needRedraw_ = true; }
}

// ============================================================
// レンダリング
//
// SDL_RenderSetViewport(renderer_, &bounds_) を使うと、以後の
// 描画コマンドの原点が bounds_.x, bounds_.y に移動し、かつ
// bounds_ の外にはみ出さないようクリップもされる。
// これにより、内部の描画コードは元コードのまま
// (0,0)-(localW_,localH_) の座標系で書ける。
// ============================================================
void PaintWidget::render()
{
    if (!renderer_) return;

    // ★ ウィジェット用のテクスチャがなければ作成
    if (!canvasTexture_) {
        canvasTexture_ = SDL_CreateTexture(
            renderer_,
            SDL_PIXELFORMAT_ARGB8888,
            SDL_TEXTUREACCESS_TARGET,
            localW_, localH_
        );
    }

    // ★ レンダリングターゲットをテクスチャに設定
    SDL_SetRenderTarget(renderer_, canvasTexture_);
    SDL_SetRenderDrawColor(renderer_, 30, 30, 35, 255);
    SDL_RenderClear(renderer_);

    // すべての描画をテクスチャに対して行う
    renderCanvas();
    renderGrid();
    if (currentTool_ == TOOL_EYEDROPPER) renderEyedropperCursor();
    else                                 renderBrushCursor();
    renderSelection();
    renderToolbar();
    renderSidebar();
    dlgNew_.draw(renderer_, localW_, localH_);

    // ウィジェットの外枠
    SDL_SetRenderDrawColor(renderer_, 90, 90, 105, 255);
    SDL_Rect self = { 0, 0, localW_, localH_ };
    SDL_RenderDrawRect(renderer_, &self);

    // ★ レンダリングターゲットを元に戻す
    SDL_SetRenderTarget(renderer_, nullptr);

    // ★ テクスチャをメインレンダラーに描画
    SDL_RenderCopy(renderer_, canvasTexture_, nullptr, &bounds_);

    if (dlgNew_.confirmed) {
        dlgNew_.confirmed = false;
        newCanvas(dlgNew_.resultW, dlgNew_.resultH);
    }

    needRedraw_ = false;
}

void PaintWidget::renderCanvas()
{
    if (canvas_.dirty) canvas_.flush();

    const SDL_Rect vp = viewportRect();
    const float vpL = (float)vp.x, vpT = (float)vp.y;
    const float vpR = (float)(vp.x + vp.w), vpB = (float)(vp.y + vp.h);

    // --- 1. 背景チェッカーボード ---
    constexpr float gx = 16.0f;
    constexpr float grid2 = gx * 2.0f;

    SDL_SetRenderDrawColor(renderer_, 180, 180, 180, 255);
    SDL_RenderFillRect(renderer_, &vp);

    float ox = std::fmod(std::fmod(panX_, grid2) + grid2, grid2);
    float oy = std::fmod(std::fmod(panY_, grid2) + grid2, grid2);
    float startX = vpL + std::fmod(std::fmod(ox - std::fmod(vpL, grid2), grid2) + grid2, grid2) - grid2;
    float startY = vpT + std::fmod(std::fmod(oy - std::fmod(vpT, grid2), grid2) + grid2, grid2) - grid2;

    SDL_SetRenderDrawColor(renderer_, 210, 210, 210, 255);

    auto drawClippedCellF = [&](float cx, float cy) {
        float dl = std::max(cx, vpL), dt = std::max(cy, vpT);
        float dr = std::min(cx + gx, vpR), db = std::min(cy + gx, vpB);
        if (dr > dl && db > dt) {
            SDL_FRect r = { dl, dt, dr - dl, db - dt };
            SDL_RenderFillRectF(renderer_, &r);
        }
        };

    for (float y = startY; y < vpB; y += grid2) {
        for (float x = startX; x < vpR; x += grid2) {
            drawClippedCellF(x, y);
            drawClippedCellF(x + gx, y + gx);
        }
    }

    // --- テクスチャ描画: src(大まか・整数) → dst(srcからの順算・float) ---
    // 逆算(dst→src)を一切行わないため、丸め誤差が蓄積しない。
    // 最終ピクセル境界はCPU丸めでなくGPUシザー(SetClipRect)に委ねる。
    auto drawTexturedClipped = [&](SDL_Texture* tex, int texW, int texH,
        float fullLeft, float fullTop, float zoomX, float zoomY) {

            float fullRight = fullLeft + texW * zoomX;
            float fullBottom = fullTop + texH * zoomY;

            // 完全に画面外ならスキップ
            if (fullRight <= vpL || fullLeft >= vpR || fullBottom <= vpT || fullTop >= vpB) return;

            // vpをテクスチャ空間へ逆算するのは「大まかなsrc選定」のためだけに使う。
            // ここで生じる誤差は後段のdst順算+GPUクリップで吸収されるので問題ない。
            float texX1 = (vpL - fullLeft) / zoomX;
            float texY1 = (vpT - fullTop) / zoomY;
            float texX2 = (vpR - fullLeft) / zoomX;
            float texY2 = (vpB - fullTop) / zoomY;

            // 外側に1テクセル分の余裕を持たせてfloor/ceil(境界の欠けを防止)
            SDL_Rect src;
            src.x = std::clamp((int)std::floor(texX1) - 1, 0, texW);
            src.y = std::clamp((int)std::floor(texY1) - 1, 0, texH);
            int srcRight = std::clamp((int)std::ceil(texX2) + 1, 0, texW);
            int srcBottom = std::clamp((int)std::ceil(texY2) + 1, 0, texH);
            src.w = srcRight - src.x;
            src.h = srcBottom - src.y;
            if (src.w <= 0 || src.h <= 0) return;

            // ★ dst は src(整数)から順算のみで求める。逆算を経由しないので誤差が蓄積しない。
            SDL_FRect dstF;
            dstF.x = fullLeft + src.x * zoomX;
            dstF.y = fullTop + src.y * zoomY;
            dstF.w = src.w * zoomX;
            dstF.h = src.h * zoomY;

            // dstF は vp より少し大きい可能性があるが、それはGPUのシザーで正確にクリップされる
            SDL_RenderCopyF(renderer_, tex, &src, &dstF);
        };

    auto drawBorderF = [&](float x1, float y1, float x2, float y2) {
        float cx1 = std::max(x1, vpL), cx2 = std::min(x2, vpR);
        float cy1 = std::max(y1, vpT), cy2 = std::min(y2, vpB);
        if (y1 >= vpT && y1 <= vpB && cx1 <= cx2) SDL_RenderDrawLineF(renderer_, cx1, y1, cx2, y1);
        if (y2 >= vpT && y2 <= vpB && cx1 <= cx2) SDL_RenderDrawLineF(renderer_, cx1, y2, cx2, y2);
        if (x1 >= vpL && x1 <= vpR && cy1 <= cy2) SDL_RenderDrawLineF(renderer_, x1, cy1, x1, cy2);
        if (x2 >= vpL && x2 <= vpR && cy1 <= cy2) SDL_RenderDrawLineF(renderer_, x2, cy1, x2, cy2);
        };

    // クリップはこのブロックの間だけ有効化(ステート変更1回・軽量)
    SDL_RenderSetClipRect(renderer_, &vp);

    // --- 2. メインキャンバス ---
    const float fullLeft = panX_;
    const float fullTop = panY_;
    const float fullRight = panX_ + canvas_.w * zoom_;
    const float fullBottom = panY_ + canvas_.h * zoom_;

    drawTexturedClipped(canvas_.tex, canvas_.w, canvas_.h, fullLeft, fullTop, zoom_, zoom_);

    SDL_SetRenderDrawColor(renderer_, 80, 80, 80, 255);
    drawBorderF(fullLeft, fullTop, fullRight, fullBottom);

    // --- 3. 貼り付けプレビュー ---
    if (pasting_ && pastePreview_) {
        float sx, sy, ex, ey;
        canvasToScreen(pasteX_, pasteY_, sx, sy);
        canvasToScreen(pasteX_ + clipW_, pasteY_ + clipH_, ex, ey);

        float pZoomX = (ex - sx) / (float)clipW_;
        float pZoomY = (ey - sy) / (float)clipH_;

        drawTexturedClipped(pastePreview_, clipW_, clipH_, sx, sy, pZoomX, pZoomY);

        SDL_SetRenderDrawColor(renderer_, 0, 120, 255, 200);
        drawBorderF(sx, sy, ex, ey);
    }

    SDL_RenderSetClipRect(renderer_, nullptr);
}
void PaintWidget::renderGrid()
{
    if (!grid_.show1 && !grid_.show8 && !grid_.show32) return;

    SDL_Rect vp = viewportRect();
    SDL_RenderSetClipRect(renderer_, &vp);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);

    auto drawGrid = [&](int cellPx, SDL_Color col, float minZoomToShow) {
        if (zoom_ < minZoomToShow) return;
        float screenCell = cellPx * zoom_;
        float cx0 = panX_, cy0 = panY_;
        float cx1 = panX_ + canvas_.w * zoom_, cy1 = panY_ + canvas_.h * zoom_;
        SDL_SetRenderDrawColor(renderer_, col.r, col.g, col.b, col.a);
        float startX = cx0;
        for (float sx = startX; sx <= cx1 + 1; sx += screenCell) {
            if (sx < vp.x || sx > vp.x + vp.w) continue;
            int isx = (int)(sx + 0.5f);
            SDL_RenderDrawLine(renderer_, isx, (int)std::max((float)vp.y, cy0),
                isx, (int)std::min((float)(vp.y + vp.h), cy1));
        }
        float startY = cy0;
        for (float sy = startY; sy <= cy1 + 1; sy += screenCell) {
            if (sy < vp.y || sy > vp.y + vp.h) continue;
            int isy = (int)(sy + 0.5f);
            SDL_RenderDrawLine(renderer_, (int)std::max((float)vp.x, cx0), isy,
                (int)std::min((float)(vp.x + vp.w), cx1), isy);
        }
        };

    if (grid_.show32) drawGrid(32, { 200, 220, 255, 90 }, 0.3f);
    if (grid_.show8)  drawGrid(8, { 150, 190, 255, 80 }, 1.0f);
    if (grid_.show1)  drawGrid(1, { 120, 160, 220, 100 }, 8.0f);

    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(renderer_, nullptr);
}

void PaintWidget::renderBrushCursor()
{
    if (currentTool_ != TOOL_PEN && currentTool_ != TOOL_ERASER) return;
    if (!cursorInCanvas_) return;
    int sr = std::max(2, (int)(brushSize_ * zoom_));
    SDL_Rect vp = viewportRect();
    SDL_RenderSetClipRect(renderer_, &vp);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 200);
    drawCircleOutline(renderer_, cursorMX_, cursorMY_, sr + 1);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 200);
    drawCircleOutline(renderer_, cursorMX_, cursorMY_, sr);
    if (currentTool_ == TOOL_ERASER) {
        SDL_SetRenderDrawColor(renderer_, 255, 80, 80, 180);
        drawCircleOutline(renderer_, cursorMX_, cursorMY_, std::max(1, sr - 1));
    }
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 160);
    SDL_RenderDrawLine(renderer_, cursorMX_ - 5, cursorMY_, cursorMX_ + 5, cursorMY_);
    SDL_RenderDrawLine(renderer_, cursorMX_, cursorMY_ - 5, cursorMX_, cursorMY_ + 5);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 160);
    SDL_RenderDrawLine(renderer_, cursorMX_ - 4, cursorMY_, cursorMX_ + 4, cursorMY_);
    SDL_RenderDrawLine(renderer_, cursorMX_, cursorMY_ - 4, cursorMX_, cursorMY_ + 4);
    SDL_RenderSetClipRect(renderer_, nullptr);
}

void PaintWidget::renderEyedropperCursor()
{
    if (!cursorInCanvas_) return;
    SDL_Rect vp = viewportRect();
    SDL_RenderSetClipRect(renderer_, &vp);
    int sz = 14;
    SDL_Rect outer = { cursorMX_, cursorMY_, sz, sz };
    if (hoverColorValid_) { SDL_SetRenderDrawColor(renderer_, hoverColor_.r, hoverColor_.g, hoverColor_.b, 220); SDL_RenderFillRect(renderer_, &outer); }
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255); SDL_RenderDrawRect(renderer_, &outer);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 200);
    SDL_Rect inner = { cursorMX_ + 1, cursorMY_ + 1, sz - 2, sz - 2 }; SDL_RenderDrawRect(renderer_, &inner);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 180);
    SDL_RenderDrawLine(renderer_, cursorMX_ - 8, cursorMY_ - 1, cursorMX_ - 1, cursorMY_ - 1);
    SDL_RenderDrawLine(renderer_, cursorMX_ - 1, cursorMY_ - 8, cursorMX_ - 1, cursorMY_ - 1);
    SDL_RenderSetClipRect(renderer_, nullptr);
}

void PaintWidget::renderSelection()
{
    if (!hasSelection_ && !selectDragging_) return;
    SDL_Rect r = normalizedSelection();
    float sx, sy, ex, ey;
    canvasToScreen(r.x, r.y, sx, sy); canvasToScreen(r.x + r.w, r.y + r.h, ex, ey);
    SDL_Rect screenR = { (int)sx, (int)sy, (int)(ex - sx), (int)(ey - sy) };
    SDL_Rect vp = viewportRect();
    SDL_RenderSetClipRect(renderer_, &vp);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 100, 160, 255, 40); SDL_RenderFillRect(renderer_, &screenR);
    SDL_SetRenderDrawColor(renderer_, 255, 255, 255, 255); SDL_RenderDrawRect(renderer_, &screenR);
    SDL_SetRenderDrawColor(renderer_, 0, 80, 200, 255);
    SDL_Rect in2 = { screenR.x + 1, screenR.y + 1, screenR.w - 2, screenR.h - 2 }; SDL_RenderDrawRect(renderer_, &in2);
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
    SDL_RenderSetClipRect(renderer_, nullptr);
}

void PaintWidget::renderToolbar()
{
    SDL_Rect tb = { 0, 0, localW_, TOOLBAR_H };
    SDL_SetRenderDrawColor(renderer_, 45, 45, 52, 255); SDL_RenderFillRect(renderer_, &tb);
    SDL_SetRenderDrawColor(renderer_, 60, 60, 70, 255);
    SDL_Rect bdr = { 0, TOOLBAR_H - 1, localW_, 1 }; SDL_RenderFillRect(renderer_, &bdr);

    for (int i = 0; i < TOOL_COUNT; i++) {
        toolBtns_[i].bg = (currentTool_ == (Tool)i) ? SDL_Color{ 70, 110, 180, 255 } : SDL_Color{ 60, 60, 72, 255 };
        toolBtns_[i].draw(renderer_);
    }
    btnNew_.draw(renderer_);
    btnCopy_.draw(renderer_); btnCut_.draw(renderer_); btnPaste_.draw(renderer_); btnClear_.draw(renderer_);
    btnUndo_.bg = (canvas_.undoCount() > 0) ? SDL_Color{ 60, 90, 80, 255 } : SDL_Color{ 50, 50, 58, 255 };
    btnRedo_.bg = (canvas_.redoCount() > 0) ? SDL_Color{ 60, 90, 80, 255 } : SDL_Color{ 50, 50, 58, 255 };
    btnUndo_.draw(renderer_); btnRedo_.draw(renderer_);

    btnGrid1_.bg = grid_.show1 ? SDL_Color{ 80, 110, 160, 255 } : SDL_Color{ 55, 55, 68, 255 };
    btnGrid8_.bg = grid_.show8 ? SDL_Color{ 80, 110, 160, 255 } : SDL_Color{ 55, 55, 68, 255 };
    btnGrid32_.bg = grid_.show32 ? SDL_Color{ 80, 110, 160, 255 } : SDL_Color{ 55, 55, 68, 255 };
    btnGrid1_.draw(renderer_); btnGrid8_.draw(renderer_); btnGrid32_.draw(renderer_);

    char buf[32]; snprintf(buf, 32, "%.0f%%  %dx%d", zoom_ * 100, canvas_.w, canvas_.h);
    // グリッドボタン群の右端より必ず右に表示し、幅が狭くても重ならないようにする
    int gridBlockEnd = btnGrid32_.rect.x + btnGrid32_.rect.w + 10;
    drawText(renderer_, buf, gridBlockEnd, 18, { 160, 160, 170, 255 }, gFontS);
}

void PaintWidget::renderSidebar()
{
    int lx = localW_ - SIDEBAR_W;
    SDL_Rect sb = { lx, TOOLBAR_H, SIDEBAR_W, localH_ - TOOLBAR_H };
    SDL_SetRenderDrawColor(renderer_, 38, 38, 45, 255); SDL_RenderFillRect(renderer_, &sb);
    SDL_SetRenderDrawColor(renderer_, 55, 55, 65, 255);
    SDL_Rect bdr = { lx, TOOLBAR_H, 1, localH_ - TOOLBAR_H }; SDL_RenderFillRect(renderer_, &bdr);

    drawText(renderer_, "Color & Brush", lx + 10, TOOLBAR_H + 8, { 160, 160, 180, 255 });

    {
        int py = TOOLBAR_H + 26;
        SDL_SetRenderDrawColor(renderer_, 160, 160, 160, 255);
        SDL_Rect cpbase = { lx + 10, py, SIDEBAR_W - 20, 32 }; SDL_RenderFillRect(renderer_, &cpbase);
        SDL_SetRenderDrawColor(renderer_, 200, 200, 200, 255);
        for (int row = 0; row < 3; row++) for (int col = 0; col < 12; col++) if ((row + col) % 2 == 0) {
            SDL_Rect r = { lx + 10 + col * 16, py + row * 11, 16, 11 }; SDL_RenderFillRect(renderer_, &r);
        }
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, colR_, colG_, colB_, colA_);
        SDL_RenderFillRect(renderer_, &cpbase);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer_, 80, 80, 90, 255); SDL_RenderDrawRect(renderer_, &cpbase);
        char buf[24]; snprintf(buf, 24, "#%02X%02X%02X%02X", colR_, colG_, colB_, colA_);
        drawText(renderer_, buf, lx + 10, py + 34, { 160, 160, 180, 255 });
    }
    if (currentTool_ == TOOL_EYEDROPPER && hoverColorValid_) {
        int py = TOOLBAR_H + 26;
        drawText(renderer_, "Pick:", lx + 90, py + 34, { 140, 200, 140, 255 });
        SDL_SetRenderDrawColor(renderer_, hoverColor_.r, hoverColor_.g, hoverColor_.b, 255);
        SDL_Rect pr = { lx + 128, py + 32, SIDEBAR_W - 138, 14 }; SDL_RenderFillRect(renderer_, &pr);
        SDL_SetRenderDrawColor(renderer_, 100, 100, 110, 255); SDL_RenderDrawRect(renderer_, &pr);
    }

    auto drawSl = [&](RgbaSlider& sl, int val, int y) {
        sl.track.y = y; sl.field.rect.y = y - 1;
        sl.track.x = lx + 10; sl.field.rect.x = lx + 10 + sl.track.w + 4;
        sl.draw(renderer_, val);
        };
    int sy = TOOLBAR_H + 98;
    drawSl(slR_, colR_, sy); sy += 40; drawSl(slG_, colG_, sy); sy += 40;
    drawSl(slB_, colB_, sy); sy += 40; drawSl(slA_, colA_, sy); sy += 50;
    drawText(renderer_, "Brush Size", lx + 10, sy - 18, { 160, 160, 180, 255 });
    drawSl(slBrush_, brushSize_ * (255 / BRUSH_MAX), sy); sy += 40;

    {
        int px = lx + SIDEBAR_W / 2, py = sy + 16;
        int r = std::min(brushSize_, (SIDEBAR_W / 2 - 10));
        SDL_Color col = drawColor();
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
        SDL_SetRenderDrawColor(renderer_, col.r, col.g, col.b, col.a);
        for (int dy = -r; dy <= r; dy++) for (int dx = -r; dx <= r; dx++) if (dx * dx + dy * dy <= r * r) SDL_RenderDrawPoint(renderer_, px + dx, py + dy);
        SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_NONE);
        SDL_SetRenderDrawColor(renderer_, 100, 100, 120, 180); drawCircleOutline(renderer_, px, py, r + 1);
    }

    {
        int ty = localH_ - 90;
        drawText(renderer_, toolNames[currentTool_], lx + 10, ty, { 200, 200, 220, 255 });
        char buf[64];
        snprintf(buf, 64, "Brush:%dpx  Undo:%d", brushSize_, canvas_.undoCount());
        drawText(renderer_, buf, lx + 10, ty + 16, { 140, 150, 160, 255 });
        std::string gs = "Grid:";
        if (grid_.show1)  gs += "1 ";
        if (grid_.show8)  gs += "8 ";
        if (grid_.show32) gs += "32";
        if (!grid_.show1 && !grid_.show8 && !grid_.show32) gs += "off";
        drawText(renderer_, gs, lx + 10, ty + 32, { 100, 160, 200, 255 });
        if (hasSelection_) { SDL_Rect r = normalizedSelection(); snprintf(buf, 64, "Sel: %dx%d", r.w, r.h); drawText(renderer_, buf, lx + 10, ty + 48, { 100, 180, 255, 255 }); }
        if (!clipboard_.empty()) { snprintf(buf, 64, "Clip: %dx%d", clipW_, clipH_); drawText(renderer_, buf, lx + 10, ty + 64, { 100, 220, 150, 255 }); }
        if (hoverColorValid_) { snprintf(buf, 64, "Px:%d,%d,%d", hoverColor_.r, hoverColor_.g, hoverColor_.b); drawText(renderer_, buf, lx + 10, ty + 80, { 160, 180, 160, 255 }); }
    }
}