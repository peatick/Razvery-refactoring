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
	PaintApp dr;
	dr.size = { 0,0,500,500 };
    if (!f.init(argc,argv)) {
        return 1;
    }
    f.addwidget_t<Widget_Ed_u>({ 0, 0, 500, 100 }, 1, "new_txt");
    f.addwidget_t<Widget_explorer_u>({100,100,500,200},1,"new");
    f.addwidget_t<Widget_d_Toolbar_u>({0,0,300,300},1,"Toolbar");
    f.w_addbtn("UP","Paint","UP", {300,0,40,20},false, true);
    
	dr.setSize(500, 500, f.renderer.ren);

    std::vector<std::function<void()>> fs;
    fs.push_back([&] {f.handler.paintApp_ev(dr); });
    while (f.running) {

        //f.Widget_Call("Toolbar");
        //f.q_Btn("UP");
        //f.events();
        //f.render_obj();
        
        f.event_test(fs);
        f.renderer.draw_bg({250,250,250,255});
		f.renderer.drw_PaintTool(dr);
		//f.renderer.drw_toolbar(drw_tb);
        f.renderer.rend();
        
    }
    f.exit();
    return 0;
}