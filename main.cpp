#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"
#include "sdl_frame.h"
#include "PaintTool.h"
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

int main(int argc, char* argv[]) {
    S_Frame f;
    if (!f.init(argc,argv)) {
        return 1;
    }
    f.addwidget(f.Paint_UW, {0,0,200,200}, 3, "Paint");
    Widget_paint_u* p = f.get<Widget_paint_u>("Paint");
    p->pa_t.new_canvas("NewA",200,200);
    p->pa_t.now_canvas = "NewA";
    f.w_addbtn("UP","Paint","UP", {300,0,40,20},false, true);

    while (f.running) {
        f.Widget_Call("Paint");
        f.events();
        f.render_obj();
        if(f.q_Btn("UP")){
            p->pa_t.scopes.w = p->pa_t.scopes.w - 10;
            std::cout << p->pa_t.scopes.w;
            p->pa_t.scopes.h = p->pa_t.scopes.h - 10;
        }
    }
    f.exit();
    return 0;
}