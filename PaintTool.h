#pragma once
#include "sdlutil.h"

class PaintTool{
public:

};
class ToolBar_Extend {
public:
    SDL_Rect size = { 0,0,0,0 };
    Editor r, g, b, a;
    void init(const SDL_Rect& rec) {
        size = rec;
    }
    
};
