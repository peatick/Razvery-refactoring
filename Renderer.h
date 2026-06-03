#pragma once
#include "sdlutil.h"
#include "widget.h"

class Renderer {
    SDL_Window* win = nullptr;
    TTF_Font* font = nullptr;
    TTF_Font* font_sml = nullptr;

    SDL_Color colBg = { 30,  30,  35,  255 };
    SDL_Color colText = { 220, 220, 210, 255 };
    SDL_Color colSel = { 60,  100, 170, 180 };
    SDL_Color colCaret = { 230, 200, 100, 255 };
    SDL_Color colLineno = { 80,  80,  100, 255 };
    SDL_Color colIme = { 100, 200, 255, 255 };
    SDL_Color colImeBg = { 50,  60,  80,  200 };


public:
    SDL_Renderer* ren = nullptr;
    SDL_Texture* folderIcon = nullptr;
    SDL_Texture* fileIcon = nullptr;
    int lineH = 0;
    int PADDING = PG;
    bool init(const char* fontPath) {

        if (SDL_Init(SDL_INIT_VIDEO) < 0) return false;
        if (TTF_Init() < 0) return false;

        win = SDL_CreateWindow("SDL2", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIN_W, WIN_H, SDL_WINDOW_RESIZABLE);
        if (!win) return false;
        ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
        if (!ren) return false;

        SDL_RenderSetLogicalSize(ren, logical_W, logical_H);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "nearest");

        font = TTF_OpenFont(fontPath, FONT_SIZE);
        font_sml = TTF_OpenFont(fontPath, FONT_SIZE);
        if (!font) {
            const char* fb[] = {
                "fonts\\PixelMplus10-Bold.ttf",
                "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
                "/usr/share/fonts/TTF/DejaVuSansMono.ttf",
                "/System/Library/Fonts/Menlo.ttc",
                "/System/Library/Fonts/Monaco.ttf",
                //"fonts\\misaki_gothic_2nd.ttf",
                "C:\\Windows\\Fonts\\consola.ttf",
                nullptr
            };
            for (int i = 0; fb[i] && !font; ++i) font = TTF_OpenFont(fb[i], FONT_SIZE);
            for (int i = 0; fb[i] && !font_sml; ++i) font_sml = TTF_OpenFont(fb[i], FONTSML_SIZE);
        }
        if (!font) {
            SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Font: %s", TTF_GetError());
            return false;
        }
        int tw, th; TTF_SizeUTF8(font, "M", &tw, &th);
        lineH = th + LINE_SPACING;
        SDL_StartTextInput();
        return true;
    }

    void destroy() {
        if (font) TTF_CloseFont(font);
        if (font_sml) TTF_CloseFont(font_sml);
        if (ren)  SDL_DestroyRenderer(ren);
        if (win)  SDL_DestroyWindow(win);
        if (folderIcon) SDL_DestroyTexture(folderIcon);
		if (fileIcon) SDL_DestroyTexture(fileIcon);
        TTF_Quit(); SDL_Quit();
    }

    TTF_Font* getFont() { return font; }
    TTF_Font* getFont_sml() { return font_sml; }
    ;
    int textWidth(const std::string& s) {
        if (s.empty()) return 0;
        int w = 0, h = 0; TTF_SizeUTF8(font, s.c_str(), &w, &h); return w;
    }
    int smltextWidth(const std::string& s) {
        if (s.empty()) return 0;
        int w = 0, h = 0; TTF_SizeUTF8(font_sml, s.c_str(), &w, &h); return w;
    }
    void drawText(const std::string& s, int x, int y, SDL_Color col) {
        if (s.empty()) return;
        SDL_Surface* surf = TTF_RenderUTF8_Blended(font, s.c_str(), col);
        if (!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (!tex) return;
        int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        SDL_Rect dst = { x,y,w,h };
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    void drawtexture(SDL_Texture* tex, int x, int y) {
        if (!tex) return;
        int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        SDL_Rect dst = { x,y,w,h };
        SDL_RenderCopy(ren, tex, nullptr, &dst);
    }
    void drawsmlText(const std::string& s, int x, int y, SDL_Color col) {
        if (s.empty()) return;
        SDL_Surface* surf = TTF_RenderUTF8_Blended(font_sml, s.c_str(), col);
        if (!surf) return;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
        SDL_FreeSurface(surf);
        if (!tex) return;
        int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
        SDL_Rect dst = { x,y,w,h };
        SDL_RenderCopy(ren, tex, nullptr, &dst);
        SDL_DestroyTexture(tex);
    }
    void adjustHorizontalScroll(Editor& ed) {
        const std::string& ln = ed.buf.line(ed.cursor.row);
        int cx = textWidth(ln.substr(0, ed.cursor.col));
        int left = ed.scrollX;
        int right = ed.scrollX + ed.viewW;

        if (cx < left) {
            ed.scrollX = cx;
        }
        else if (cx > right - 50) {
            ed.scrollX = cx - ed.viewW + 50;
        }

        if (ed.scrollX < 0) ed.scrollX = 0;
    }
    void adjustHorizontalScroll_sh(Editor& ed) {
        const std::string& ln = ed.buf.line(ed.cursor.row);
        int cx = smltextWidth(ln.substr(0, ed.cursor.col));

        int left = ed.scrollX;
        int right = ed.scrollX + ed.viewW;

        if (cx < left) {
            ed.scrollX = cx;
        }
        else if (cx > right - 2) {
            ed.scrollX = cx - ed.viewW + 2;
        }

        if (ed.scrollX < 0) ed.scrollX = 0;
    }
    void cls(int r, int g, int b, int a) {
        SDL_SetRenderDrawColor(ren, r, g, b, a); // R,G,B,A（ここでは青）
        SDL_RenderClear(ren);                       // 背景を青で塗る
    }
    void draw_bg(SDL_Color back) {
        cls(0, 0, 0, 255);
        SDL_SetRenderDrawColor(ren, back.r, back.g, back.b, back.a);
        SDL_Rect r = { 0,0,WIN_W,WIN_H };
        SDL_RenderFillRect(ren, &r);
    }
    void rend() {
        SDL_RenderPresent(ren);
    }
    void TextBox(Editor& ed) {
        int winW, winH; SDL_GetWindowSize(win, &winW, &winH);
        int linenoW = ed.noLineNo ? 0 : 50;
        ed.lineH = lineH;
        ed.viewRows = (ed.TX_Rect.h - PADDING * 2) / lineH;
        ed.viewW = ed.TX_Rect.w - linenoW - PADDING;
        SDL_Rect bgrect = ed.TX_Rect;
        SDL_SetRenderDrawColor(ren, colBg.r, colBg.g, colBg.b, 255);
        SDL_RenderFillRect(ren, &bgrect);

        SDL_Rect clip = { linenoW + PADDING + ed.TX_Rect.x, PADDING + ed.TX_Rect.y, ed.TX_Rect.w - (linenoW + PADDING), (ed.TX_Rect.y + ed.TX_Rect.h) - PADDING * 2 };
        SDL_RenderSetClipRect(ren, &clip);

        int x0 = linenoW + PADDING - ed.scrollX;
        int last = std::min(ed.scrollRow + ed.viewRows + 1, ed.buf.numLines());

        for (int row = ed.scrollRow; row < last; ++row) {
            int y = PADDING + (row - ed.scrollRow) * lineH;
            const std::string& ln = ed.buf.line(row);

            // Selection
            if (ed.hasSelection) {
                auto a = ed.selMin(), b = ed.selMax();
                if (row >= a.row && row <= b.row) {
                    int xf = (row == a.row) ? textWidth(ln.substr(0, a.col)) : 0;
                    int xt = (row == b.row) ? textWidth(ln.substr(0, b.col)) : textWidth(ln) + 8;
                    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(ren, colSel.r, colSel.g, colSel.b, colSel.a);
                    SDL_Rect sr = { x0 + xf + ed.TX_Rect.x,y + ed.TX_Rect.y,xt - xf,lineH };
                    SDL_RenderFillRect(ren, &sr);
                }
            }

            if (!ln.empty()) drawText(ln, x0 + ed.TX_Rect.x, y + ed.TX_Rect.y, colText);

            // IME preedit
            if (!ed.imeComposing.empty() && row == ed.cursor.row) {
                int cx = textWidth(ln.substr(0, ed.cursor.col));
                int pw = textWidth(ed.imeComposing);
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, colImeBg.r, colImeBg.g, colImeBg.b, colImeBg.a);
                SDL_Rect pr = { x0 + cx + ed.TX_Rect.x,y + ed.TX_Rect.y,pw,lineH }; SDL_RenderFillRect(ren, &pr);
                SDL_SetRenderDrawColor(ren, colIme.r, colIme.g, colIme.b, 220);
                //アンダーライン
                SDL_RenderDrawLine(ren, x0 + cx + ed.TX_Rect.x, y + lineH - 2 + ed.TX_Rect.y, x0 + cx + pw + ed.TX_Rect.x, y + lineH - 2 + ed.TX_Rect.y);
                drawText(ed.imeComposing, x0 + cx + ed.TX_Rect.x, y + ed.TX_Rect.y, colIme);
                //int icx=cx+textWidth(ed.imeComposing.substr(0,ed.imeCursor));
                int icx = cx + textWidth(utf8_substr(ed.imeComposing, ed.imeCursor));
                SDL_SetRenderDrawColor(ren, colIme.r, colIme.g, colIme.b, 255);
                SDL_Rect cr = { x0 + icx + ed.TX_Rect.x,y + ed.TX_Rect.y,CURSOR_WIDTH,lineH }; SDL_RenderFillRect(ren, &cr);
            }

            // Caret
            if (ed.caretOn && row == ed.cursor.row && ed.imeComposing.empty()) {
                int cx = textWidth(ln.substr(0, ed.cursor.col));
                SDL_SetRenderDrawColor(ren, colCaret.r, colCaret.g, colCaret.b, 255);
                SDL_Rect cr = { x0 + cx + ed.TX_Rect.x,y + ed.TX_Rect.y,CURSOR_WIDTH,lineH }; SDL_RenderFillRect(ren, &cr);
            }
        }

        SDL_RenderSetClipRect(ren, nullptr);
        if (!ed.noLineNo) {
            // Line numbers
            SDL_SetRenderDrawColor(ren, 40, 40, 48, 255);
            SDL_Rect lnbg = { ed.TX_Rect.x, ed.TX_Rect.y, 50, ed.TX_Rect.h }; SDL_RenderFillRect(ren, &lnbg);
            for (int r = ed.scrollRow; r < last; ++r)
                drawText(std::to_string(r + 1), ed.TX_Rect.x + 5 + PADDING, ed.TX_Rect.y + (r - ed.scrollRow) * lineH + PADDING, colLineno);
        }
        // Status bar
        SDL_SetRenderDrawColor(ren, 20, 20, 25, 255);
        SDL_Rect sb = { ed.TX_Rect.x,ed.TX_Rect.y + ed.TX_Rect.h - 24,ed.TX_Rect.w, 24 }; SDL_RenderFillRect(ren, &sb);
        std::string status =
            "Ln " + std::to_string(ed.cursor.row + 1) +
            " Col " + std::to_string(utf8::countChars(ed.buf.line(ed.cursor.row), ed.cursor.col) + 1) +
            " Undo:" + std::to_string(ed.history.undoCount()) +
            " Redo:" + std::to_string(ed.history.redoCount());
        drawText(status, PADDING + ed.TX_Rect.x, ed.TX_Rect.y + ed.TX_Rect.h - PADDING - 4, colLineno);
        adjustHorizontalScroll(ed);
    }
    void TextBoxsh(Editor& ed) {
        int winW, winH; SDL_GetWindowSize(win, &winW, &winH);
        int linenoW = ed.noLineNo ? 0 : 50;
        ed.lineH = lineH;
        ed.viewRows = (ed.TX_Rect.h - ed.PADDING * 2) / lineH;
        ed.viewW = ed.TX_Rect.w - linenoW - ed.PADDING * 2;
        SDL_Rect bgrect = { ed.TX_Rect.x, ed.TX_Rect.y, ed.TX_Rect.w, ed.TX_Rect.h };
        SDL_SetRenderDrawColor(ren, 250, 250, 250, 255);
        SDL_RenderFillRect(ren, &bgrect);
        SDL_Color textB = { 5,5,5,255 };
        SDL_Rect clip = { linenoW + ed.PADDING + ed.TX_Rect.x - 10, ed.PADDING + ed.TX_Rect.y, ed.TX_Rect.w - (linenoW + ed.PADDING), (ed.TX_Rect.y + ed.TX_Rect.h) - ed.PADDING * 2 };
        SDL_RenderSetClipRect(ren, &clip);

        int x0 = linenoW + ed.PADDING - ed.scrollX - 10;
        int last = std::min(ed.scrollRow + ed.viewRows + 1, ed.buf.numLines());

        for (int row = ed.scrollRow; row < last; ++row) {
            int y = ed.PADDING + (row - ed.scrollRow) * lineH;
            const std::string& ln = ed.buf.line(row);
            int sp = FONTSML_SIZE;
            // Selection
            if (ed.hasSelection) {
                auto a = ed.selMin(), b = ed.selMax();
                if (row >= a.row && row <= b.row) {
                    int xf = (row == a.row) ? smltextWidth(ln.substr(0, a.col)) : 0;
                    int xt = (row == b.row) ? smltextWidth(ln.substr(0, b.col)) : smltextWidth(ln) + 8;
                    SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                    SDL_SetRenderDrawColor(ren, colSel.r, colSel.g, colSel.b, colSel.a);
                    SDL_Rect sr = { x0 + xf + ed.TX_Rect.x,y + ed.TX_Rect.y,xt - xf,sp };
                    SDL_RenderFillRect(ren, &sr);
                }
            }

            if (!ln.empty()) drawsmlText(ln, x0 + ed.TX_Rect.x, y + ed.TX_Rect.y, textB);

            // IME preedit
            if (!ed.imeComposing.empty() && row == ed.cursor.row) {
                int cx = smltextWidth(ln.substr(0, ed.cursor.col));
                int pw = smltextWidth(ed.imeComposing);
                SDL_SetRenderDrawBlendMode(ren, SDL_BLENDMODE_BLEND);
                SDL_SetRenderDrawColor(ren, colImeBg.r, colImeBg.g, colImeBg.b, colImeBg.a);
                SDL_Rect pr = { x0 + cx + ed.TX_Rect.x,y + ed.TX_Rect.y,pw,FONTSML_SIZE }; SDL_RenderFillRect(ren, &pr);
                SDL_SetRenderDrawColor(ren, colIme.r, colIme.g, colIme.b, 220);
                //アンダーライン

                SDL_RenderDrawLine(ren, x0 + cx + ed.TX_Rect.x, y + ed.TX_Rect.y + sp, x0 + cx + pw + ed.TX_Rect.x, y + ed.TX_Rect.y + sp);
                drawsmlText(ed.imeComposing, x0 + cx + ed.TX_Rect.x, y + ed.TX_Rect.y, colIme);
                int icx = cx + smltextWidth(utf8_substr(ed.imeComposing, ed.imeCursor));
                SDL_SetRenderDrawColor(ren, colIme.r, colIme.g, colIme.b, 255);
                SDL_Rect cr = { x0 + icx + ed.TX_Rect.x,y + ed.TX_Rect.y,CURSOR_WIDTH,FONTSML_SIZE }; SDL_RenderFillRect(ren, &cr);
            }

            // Caret
            if (ed.caretOn && row == ed.cursor.row && ed.imeComposing.empty()) {
                int cx = smltextWidth(ln.substr(0, ed.cursor.col));
                SDL_SetRenderDrawColor(ren, colCaret.r, colCaret.g, colCaret.b, 255);
                SDL_Rect cr = { x0 + cx + ed.TX_Rect.x,y + ed.TX_Rect.y,CURSOR_WIDTH,FONTSML_SIZE }; SDL_RenderFillRect(ren, &cr);
            }
        }
        SDL_RenderSetClipRect(ren, nullptr);
        adjustHorizontalScroll_sh(ed);
    }
    void drawFilledTriangle(SDL_Point p1, SDL_Point p2, SDL_Point p3)
    {
        // y 座標でソート
        if (p2.y < p1.y) { SDL_Point tmp = p1; p1 = p2; p2 = tmp; }
        if (p3.y < p1.y) { SDL_Point tmp = p1; p1 = p3; p3 = tmp; }
        if (p3.y < p2.y) { SDL_Point tmp = p2; p2 = p3; p3 = tmp; }

        float dx1 = 0, dx2 = 0, dx3 = 0;

        if (p2.y - p1.y != 0) dx1 = (float)(p2.x - p1.x) / (p2.y - p1.y);
        if (p3.y - p1.y != 0) dx2 = (float)(p3.x - p1.x) / (p3.y - p1.y);
        if (p3.y - p2.y != 0) dx3 = (float)(p3.x - p2.x) / (p3.y - p2.y);

        float sx = p1.x;
        float ex = p1.x;

        // 上半分
        for (int y = p1.y; y <= p2.y; y++) {
            SDL_RenderDrawLine(ren, (int)sx, y, (int)ex, y);
            sx += dx1;
            ex += dx2;
        }

        sx = p2.x;

        // 下半分
        for (int y = p2.y; y <= p3.y; y++) {
            SDL_RenderDrawLine(ren, (int)sx, y, (int)ex, y);
            sx += dx3;
            ex += dx2;
        }
    }
    void dir_icon(int x, int y, float size) {
        SDL_SetRenderDrawColor(ren, 255, 200, 82, 255);
        SDL_Rect r = { x,y + 3 * size,17 * size,12 * size };
        SDL_RenderFillRect(ren, &r);
        r = { x,y, int(5 * size) , int(3 * size) };
        SDL_RenderFillRect(ren, &r);
    }
    void file_icon(int x, int y, float size) {
        SDL_SetRenderDrawColor(ren, 220, 220, 220, 255);
        SDL_Rect r = { x,y,12 * size,15 * size };
        r = { x,y + int(4 * size),int(12 * size),int(15 * size - 4 * size) };
        SDL_RenderFillRect(ren, &r);
        r = { x,y,int(12 * size - 4 * size),int(15 * size) };
        SDL_RenderFillRect(ren, &r);
        drawFilledTriangle({ int(x + 12 * size - 4 * size),y }, { int(x + 12 * size - 4 * size),int(y + 4 * size) }, { int(x + 11 * size),int(y + 4 * size) });
        SDL_SetRenderDrawColor(ren, 5, 5, 5, 255);
        SDL_RenderDrawLine(ren, x + 12 * size - 4 * size, y, x + 12 * size - 4 * size, y + 4 * size);
        SDL_RenderDrawLine(ren, x + 12 * size - 4 * size, y + 4 * size, x + 11 * size, y + 4 * size);
    }
    void mouse_logical_pos(int& mouse_x, int& mouse_y) {
        SDL_GetMouseState(&mouse_x, &mouse_y);
        float mx, my;
        SDL_RenderWindowToLogical(ren, mouse_x, mouse_y, &mx, &my);
        mouse_x = (int)mx; mouse_y = (int)my;
    }
    void closs(SDL_Rect& rt, SDL_Color cols, int pd) {
        SDL_SetRenderDrawColor(ren, cols.r, cols.g, cols.b, cols.a);
        int svLine_1[4] = { rt.x + pd,rt.y + pd,rt.x + rt.w - pd,rt.y + rt.h - pd };
        int svLine_2[4] = { rt.x + pd,rt.y + rt.h - pd,rt.x + rt.w - pd,rt.y + pd };
        SDL_RenderDrawLine(ren, svLine_1[0], svLine_1[1], svLine_1[2], svLine_1[3]);
        SDL_RenderDrawLine(ren, svLine_2[0], svLine_2[1], svLine_2[2], svLine_2[3]);
    }
    void init_icon_tex() {
        folderIcon = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
		SDL_SetRenderTarget(ren, folderIcon);
		SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
		SDL_RenderClear(ren);
		dir_icon(0, 0, 1.0f);
		SDL_SetRenderTarget(ren, nullptr);
        SDL_SetTextureBlendMode(folderIcon, SDL_BLENDMODE_BLEND);
        fileIcon = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 20, 20);
        SDL_SetRenderTarget(ren, fileIcon);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
        SDL_RenderClear(ren);
        file_icon(0, 0, 1.0f);
        SDL_SetRenderTarget(ren, nullptr);
		SDL_SetTextureBlendMode(fileIcon, SDL_BLENDMODE_BLEND);
    }
    void drawtexture_clip(SDL_Texture* tex, int x, int y,int clip_w, int clip_h) {
        if (!tex) return;
        int w, h; SDL_QueryTexture(tex, nullptr, nullptr, &w, &h);
		if (clip_w < w) w = clip_w;
        if (clip_h < h) h = clip_h;
		if (clip_w >= w) clip_w = w;
        if (clip_h >= h) clip_h = h;
        SDL_Rect dst = { x,y,w,h };
		SDL_Rect clip = { 0,0,clip_w,clip_h };
        SDL_RenderCopy(ren, tex, &clip, &dst);
    }
    void update_fs_cache(File_explorer& f) {
        if(!f.fs_text_cache_dirty) return;
        // Implementation for updating file system cache
        f.fs_text_cache_dirty = false;
		for (auto& [_, tex] : f.fs_text_cache) {
			SDL_DestroyTexture(tex);
		}
		f.fs_text_cache.clear();
		for (const auto& fe : f.file_list) {
			std::string filename;
			f.filename2string(fe.file_path, filename);
			SDL_Surface* surf = TTF_RenderUTF8_Blended(font, filename.c_str(), { 10,10,10,255 });
			if (!surf) continue;
			SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
			SDL_FreeSurface(surf);
			if (!tex) continue;
			f.fs_text_cache[filename] = tex;
		}
    }
	void fs_texture_init(File_explorer& f) {
		std::string bka = "<-";
		SDL_Surface* surf = TTF_RenderUTF8_Blended(font, bka.c_str(), {10,10,10,255});
		if (!surf) return;
		SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, surf);
		SDL_FreeSurface(surf);
		if (!tex) return;
		f.back_arrw_tex = tex;
        f.Selected_rect_tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, f.size.w, 20);
        SDL_SetRenderTarget(ren, f.Selected_rect_tex);
        SDL_SetRenderDrawColor(ren, 0, 0, 0, 0);
        SDL_RenderClear(ren);
        SDL_SetRenderDrawColor(ren, 71, 190, 255, 255);
		SDL_Rect r = { 0,0,f.size.w,20 };
		SDL_RenderFillRect(ren, &r);
        SDL_SetRenderTarget(ren, nullptr);
        SDL_SetTextureBlendMode(f.Selected_rect_tex, SDL_BLENDMODE_BLEND);
	}
    void drw_file_explorer(File_explorer& f){
        SDL_SetRenderDrawColor(ren,200,200,200,255);
        SDL_RenderFillRect(ren,&f.size);
        if(f.update){
            f.update = false;
        }
		int start_x = f.size.x + 25;
		int start_y = f.size.y + 30;
        int viewRow = (f.size.h - f.under_box.h - 30) / 20;
		SDL_Color textC = { 10,10,10,255 };
		update_fs_cache(f);
		std::string path_str;
        for (int i = 0; i < f.file_list.size(); i++) {
            if (viewRow - 1 < i) break;
            if (i + f.scrollrow < f.file_list.size()) {
				if (f.file_list[i + f.scrollrow].selected) {
					drawtexture(f.Selected_rect_tex, f.size.x, start_y + (i * 20));
				}
                if (f.file_list[i + f.scrollrow].isDir) {
					drawtexture(folderIcon, f.size.x + 5, start_y + (i * 20) + 2);
                }
                else {
					drawtexture(fileIcon, f.size.x + 5, start_y + (i * 20) + 2);
                }
                f.filename2string(f.file_list[i +f.scrollrow].file_path, path_str);
				drawtexture_clip(f.fs_text_cache[path_str], start_x, start_y + (i * 20), f.size.w - start_x, 20);
            }
        }
        TextBoxsh(f.path_box_ed);
        drawtexture(f.back_arrw_tex, f.size.x + 10, f.size.y + 5);
    }
};
