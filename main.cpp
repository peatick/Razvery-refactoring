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
    f.addwidget(f.Editor_UW, { 0,0,200,200 }, 1, "Editor");
    f.addwidget(f.Explorer_UW, { 100,0,200,200 }, 2, "Explorer");
    f.addwidget(f.Paint_UW, {300,0,200,200}, 3, "Paint");
    Widget_paint_u* p = f.get<Widget_paint_u>("Paint");
    p->pa_t.new_canvas("News",200,200);

    f.w_addbtn("test", "group", "Button", { 0,200,70,20 }, true, true);
    f.w_addbtn("test2", "group", "Button2", { 70,200,70,20 }, true, true);
    f.w_addbtn("test3", "test2", "Button3", { 70,220,70,20 }, false, true);
    Widget_Ed_u* w = f.get<Widget_Ed_u>("Editor");
    w->ed_u.buf.setAllText("Hello SDL2S");
    while (f.running) {
        /*
        f.Widget_Call("Editor");
        f.Widget_Call("Explorer");
        f.q_Btn("test");
        if (f.q_Btn("test2")) {
            if (f.q_Btn("test3")) std::cout << "push_btn clicked!" << std::endl;
        }
        */
        f.Widget_Call("Paint");
        f.events();
        f.render_obj();
    }
    f.exit();
    return 0;
}