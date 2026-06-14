#pragma once
#include "sdlutil.h"

class PaintTool{
public:
    struct p_canvas{
        int w = 0;
        int h = 0;
        SDL_Surface* paint_canvas;
    };
    SDL_Color pencil_col = {0,0,0,255};
    std::unordered_map<std::string, p_canvas> canvas;
    SDL_Rect size = {0,0,0,0};
    SDL_Rect scopes = {0,0,0,0};
    std::string now_canvas = "";
    void init(const SDL_Rect& rec){
        size = rec;
        scopes = {0,0,rec.w,rec.h};
    }
    void new_canvas(const std::string& name,const int& w,const int& h){
        if(canvas.contains(name)) return;
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(
            0, w, h, 32, SDL_PIXELFORMAT_RGBA32
        );
        if (!surface) {
            SDL_Log("Failed to create surface: %s", SDL_GetError());
            return;
        }
        Uint32 color = SDL_MapRGB(surface->format, 255, 255, 255); // 白色
        SDL_FillRect(surface, NULL, color); // NULL → 全面塗りつぶし
        canvas[name] = {w, h, surface};
        scopes = {0, 0, w, h};
    }
    void set_pixel(const std::string& name, SDL_Point p, SDL_Color col)
{
    if(!canvas.contains(name)) return;
    SDL_Surface* surface = canvas[name].paint_canvas;
    int x = p.x - size.x;
    int y = p.y - size.y;
    // 必要ならロック
    Uint32 color = SDL_MapRGBA(surface->format,
                           col.r,
                           col.g,
                           col.b,
                           col.a);

    if (SDL_MUSTLOCK(surface)) {
        SDL_LockSurface(surface);
    }

    Uint8* pixel_ptr = (Uint8*)surface->pixels
        + y * surface->pitch
        + x * surface->format->BytesPerPixel;

    switch (surface->format->BytesPerPixel) {
        case 1:
            *pixel_ptr = color;
            break;

        case 2:
            *(Uint16*)pixel_ptr = color;
            break;

        case 3:
            if (SDL_BYTEORDER == SDL_BIG_ENDIAN) {
                pixel_ptr[0] = (color >> 16) & 0xFF;
                pixel_ptr[1] = (color >> 8) & 0xFF;
                pixel_ptr[2] = color & 0xFF;
            } else {
                pixel_ptr[0] = color & 0xFF;
                pixel_ptr[1] = (color >> 8) & 0xFF;
                pixel_ptr[2] = (color >> 16) & 0xFF;
            }
            break;

        case 4:
            *(Uint32*)pixel_ptr = color;
            break;
    }

    if (SDL_MUSTLOCK(surface)) {
        SDL_UnlockSurface(surface);
    }    
    }
    SDL_Point scope2abs(SDL_Point p){
        float p_x = p.x - size.x;
        float p_y = p.y - size.y;
        p_x = std::clamp(p_x, 0.0f, float(size.w));
        p_y = std::clamp(p_y, 0.0f, float(size.h));
        p_x = (p_x - scopes.x) * (float(scopes.w) / float(size.w)) + size.x;
        p_y = (p_y - scopes.y) * (float(scopes.h) / float(size.h)) + size.y;
        return SDL_Point{int(p_x),int(p_y)};
    }

    void destruct_surf(){
        for (auto& cw : canvas){
            SDL_FreeSurface(cw.second.paint_canvas);
        }
    }
};
class ToolBar_Extend {
public:
    SDL_Rect size = { 0,0,0,0 };
    Editor r, g, b, a;
    void init(const SDL_Rect& rec) {
        size = rec;
    }
    
};
