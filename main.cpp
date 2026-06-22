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
    f.addwidget_t<Widget_explorer_u>({100,100,500,200},1,"new");
    f.addwidget_t<Widget_Toolbar_u>({0,0,300,300},1,"Toolbar");
    //f.w_addbtn("UP","Paint","UP", {300,0,40,20},false, true);

    Slider sl;
    sl.bar = {200,200,200,10};
    sl.set(0);
    sl.init(f.renderer.lineH);
    sl.Label = "R:";

    std::vector<std::function<void()>> fs;
    fs.push_back([&] {f.handler.Slider_ev(sl); });
    while (f.running) {

        //f.Widget_Call("Toolbar");
        f.event_test(fs);
        f.renderer.draw_bg({250,250,250,255});
        f.renderer.drw_Slider(sl);
        f.renderer.rend();

    }
    f.exit();
    return 0;
}