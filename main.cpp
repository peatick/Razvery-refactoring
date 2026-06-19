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
    //f.addwidget_t<Widget_Ed_u>({0,0,1000,800},1,"New_TextEditor");
    f.addwidget_t<Widget_Toolbar_u>({0,0,300,300},1,"Toolbar");
    //f.w_addbtn("UP","Paint","UP", {300,0,40,20},false, true);
    while (f.running) {

        f.Widget_Call("Toolbar");
        f.events();
        f.render_obj();
    }
    f.exit();
    return 0;
}