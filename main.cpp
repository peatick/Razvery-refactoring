#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdlutil.h"
#include "Renderer.h"
#include "Events.h"
#include "sdl_frame.h"
#include "PaintTool.h"
#include "skelt_f.h"
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <string>
#include <vector>
/*
int main(int argc, char* argv[]) {
    SKEL_Frame f;
	
    if (!f.init(argc,argv)) {
        return 1;
    }
    //f.addwidget_t<Widget_Ed_u>({ 0, 0, 500, 100 }, 1, "new_txt");
    //f.addwidget_t<Widget_explorer_u>({100,100,500,200},1,"new");
    //f.addwidget_t<Widget_d_Toolbar_u>({0,0,300,300},1,"Toolbar");
    //f.addwidget_t<Widget_drw_tools>({10,10,900,600},1,"Paint");
    //f.w_addbtn("M_File","MenB","File", {0,0,70,20},false, true);
    std::vector<SKEL_Frame::idAndname> insa;
    insa.push_back({"File_M","File"});
    insa.push_back({"Edit_M","Edit"});
    insa.push_back({"View_M","View"});
    insa.push_back({"Tools_M","Tools"});
    insa.push_back({"Stt_M","Setting"});
    f.BtnAutoset_Beside(insa,"Men",{0,0,70,20});
    //std::vector<std::function<void()>> fs;
    //fs.push_back([&] {f.handler.paintApp_ev(dr); });
    while (f.running) {

        //f.Widget_Call("Paint");
        f.q_Btn("File_M");
        f.q_Btn("Edit_M");
        f.q_Btn("View_M");
        f.q_Btn("Tools_M");
        f.q_Btn("Stt_M");
        f.events();
        f.render_obj();
        
        //f.event_test(fs);
        //f.renderer.draw_bg({250,250,250,255});
		//f.renderer.drw_PaintTool(dr);
        //dr.pan();
		//f.renderer.drw_toolbar(drw_tb);
        //f.renderer.rend();
    }
    f.exit();
    return 0;
}
*/



int main(int argc, char* argv[]) {
    window_Manager wm;
    if (!wm.init(argc, argv)) {
        return 1;
    }

	wm.new_window(800, 600, 800, 600, "window2");

	wm.main_frame.addwidget_t<Widget_Ed_u>({ 0, 0, 500, 100 }, 1, "new_txt");

    while (wm.running) {
		wm.main_frame.Widget_Call("new_txt");
        wm.events();
        wm.render();
    }

    wm.exit();
    return 0;
}