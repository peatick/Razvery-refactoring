#include "skelt_f.h"
#include "UI_Cfg.h"
int main(int argc, char* argv[]) {
    window_Manager wm;
	wm.win_w = 1000;
	wm.win_h = 625;
	wm.logic_w = 1000;
	wm.logic_h = 625;
    if (!wm.init(argc, argv)) {
        return 1;
    }
    UI_BTNS_MEN Btn_LT;
    
	//wm.mf.addwidget_t<Widget_Ed_u>({ 220, 40, 680, 450 }, 1, "Texteditor_1");
	auto w_paint = wm.mf.addwidget_t<Widget_Paint_v2>({ 0, 20, 1000, 605 }, 1, "Draw_tools");
    Btn_LT.beside_Btn(wm, "Men", { "File", "Edit", "View", "Tools", "Setting" }, {0,0});
    Btn_LT.Vertical_Btn(wm, "File", {"New", "Open", "Save", "Save as"}, {0, 20});

    while (wm.running) {
        wm.mf.Widget_Call("Draw_tools");
        if (wm.mf.q_Btn("Men_File")) {
            if(wm.mf.q_Btn("File_New")) {
				w_paint->paint.dlgNew_OUT();
            }
            if(wm.mf.q_Btn("File_Open")) {
                wm.new_dialog_Open("img", [&](std::string s) {w_paint->paint.LoadTextureFromPath(s); });
            }
            if(wm.mf.q_Btn("File_Save")) {
                if (!w_paint->paint.savepath.empty()) {
                    fs::path p = str2path(w_paint->paint.savepath);
                    if (fs::exists(p)) {
                        w_paint->paint.SaveTextureToPNG(w_paint->paint.savepath);
                    }
                    else {
                        wm.new_dialog_Save_as(".png", [&](std::string s) {w_paint->paint.SaveTextureToPNG(s); w_paint->paint.savepath = s; });
                    }
                }
            }
            if(wm.mf.q_Btn("File_Save as")) {
                wm.new_dialog_Save_as(".png", [&](std::string s) {w_paint->paint.SaveTextureToPNG(s); w_paint->paint.savepath = s; });
            }
        }
        wm.mf.q_Btn("Men_Edit");
        wm.mf.q_Btn("Men_View");
        wm.mf.q_Btn("Men_Tools");
        wm.mf.q_Btn("Men_Setting");

        
        wm.dialog_EV();
        wm.events();
        wm.render();
    }

    wm.exit();
    return 0;
}
