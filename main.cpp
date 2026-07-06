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
	Drws_Toolbar drw_tb;
	drw_tb.init({ 500,0,300,500 }, f.renderer.lineH);
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
    fs.push_back([&] {f.handler.drw_toolbar_ev(drw_tb); });
    while (f.running) {

        //f.Widget_Call("Toolbar");
        //f.q_Btn("UP");
        //f.events();
        //f.render_obj();
        
        f.event_test(fs);
        f.renderer.draw_bg({250,250,250,255});
		f.renderer.drw_PaintTool(dr);
        dr.pan();
		f.renderer.drw_toolbar(drw_tb);
        f.renderer.rend();
		dr.setColor(drw_tb.now_color);
    }
    f.exit();
    return 0;
}