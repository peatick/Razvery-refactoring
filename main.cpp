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
    f.addwidget_t<Widget_Ed_u>({0,0,500,500},1,"New_TextEditor");
    f.w_addbtn("UP","Paint","UP", {300,0,40,20},false, true);
    f.es.init({600,0,200,70},{600,0,200,70},f.renderer.lineH);
    f.es.Search_box.noLineNo = true;
    while (f.running) {
        f.Widget_Call("New_TextEditor");
        f.events();
        f.render_obj();
        if(f.q_Btn("UP")){
        }
    }
    f.exit();
    return 0;
}