#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdlutil.h"
#include "ui_cfg.h"
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <string>
#include <vector>



int main(int argc, char* argv[]) {
    const char* fontPath = (argc > 1) ? argv[1] : "";

    Renderer renderer;
    if (!renderer.init(fontPath)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "Init: %s | %s", SDL_GetError(), TTF_GetError());
        return 1;
    }
    
    file_explorer file_ex;
	file_ex.F_X = 0; file_ex.F_Y = 20;
    file_ex.F_W = 200;file_ex.F_H = WIN_H - 20;
    workspace ws;
    ws.init(renderer.lineH);
    /*
        "SDL2 テキストエディタ  (差分方式 Undo/Redo)\n"
        "IME入力（日本語・中国語・韓国語）対応\n"
        "ショートカット:\n"
        "  Ctrl+A            全選択\n"
        "  Ctrl+C            コピー\n"
        "  Ctrl+X            カット\n"
        "  Ctrl+V            ペースト\n"
        "  Ctrl+Z            アンドゥ\n"
        "  Ctrl+Y / Ctrl+Shift+Z  リドゥ\n"
        "  Shift+矢印        選択\n"
        "  Ctrl+矢印         単語単位移動\n"
        "  Home/End          行頭/行末\n"
        "  Ctrl+Home/End     文書頭/末尾\n"
        "  ダブルクリック    単語選択\n"
    */
    file_ex.init(renderer.lineH);
    UI ui;
	UI_CFG ui_cfg;
    over_wiget file_picker;
    file_picker.ui = &ui;
    file_picker.init(renderer.ren,renderer.lineH);
	ui_cfg.init(ui);
    Ev_h ev_ha;
    int mx = 0;
    int my = 0;
	int mousex = 0, mousey = 0;
    bool running = true, mouseDown = false;
	bool bef_mousebtn = false, now_mousebtn = false;

    auto imitate_btn = [&renderer, &ui](std::string but_key) {
		renderer.btn_draw(ui, but_key);
		return ui.button(but_key);
    };
    auto imitate_spbtn = [&renderer, &ui](std::string b_key){
        renderer.sp_button(ui,b_key);
        return ui.sp_btn_cl(b_key);
    };
    
    ws.push_workspace("name","hehehe",renderer.lineH);
    while (running) {
        SDL_Event e;
        Editor& ed = ws.work_s[ws.active].edits;
        while (SDL_PollEvent(&e)) {
            mousex = e.button.x;
            mousey = e.button.y;
            switch (e.type) {
            case SDL_QUIT: running = false; break;
            }
			SDL_Point mouse_P = { mx, my };
            if(ui.layer == 0){
                if(!ui.z){
                    ws.search_box_event(e, mouse_P);
                    textEditEvent(e, ed, renderer, mouseDown, mx, my, !ws.search_mode);
                    textEditEvent_sh(e, ws.search_box, renderer, mouseDown, mx, my, ws.search_mode);
                    textEditEvent_sh(e, file_ex.ed, renderer, mouseDown, mx, my, true);
                    file_explorer_event(e, file_ex, renderer, true);
                }
            }else if(ui.layer == 1){
                ev_ha.wiget_ev(ui,mouse_P,e,file_picker,renderer,mouseDown,mx,my,".txt");
            }
            if (e.type == SDL_MOUSEBUTTONDOWN) {
                mx = e.button.x;
                my = e.button.y;
            }
			renderer.mouse_logical_pos(mousex, mousey);
			ui.mousePos = { mousex, mousey };
			now_mousebtn = e.type == SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT;
			ui.Left_click = ui.pulse_click(bef_mousebtn, now_mousebtn);
            ws.ev_hitscan(ui.mousePos,SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT);
            ws.ev_close(ui.mousePos,SDL_MOUSEBUTTONDOWN && e.button.button == SDL_BUTTON_LEFT);
            bef_mousebtn = now_mousebtn;
        }
        renderer.draw_bg({250,250,250,255});
        renderer.TextBox(ed);
        renderer.update_fs_explorer(file_ex);
        renderer.drw_tab(ws);
		renderer.search_box(ws.search_box, ws.search_results, ws.search_index, ws.search_mode);
        if (imitate_btn("File")) {
            if(imitate_btn("New")){
                
            }
            if(imitate_btn("Open")){
                ui.layer = 1;
                file_picker.file_pick.path_set(true,fs::current_path());
            }
            imitate_btn("Save");
            imitate_btn("Save as...");
            imitate_btn("Quit");
        }
        if (imitate_btn("Edit")) {
            
        }
        if (imitate_btn("View")) {
            if (imitate_btn("Texteditor")) {
                ws.ws_linenum(imitate_btn("LineNum"));
            }
        }
        else {
			ui.group_off("View", "");
        }
        if(ui.layer == 1){
            bool close = false;
            ui.group_off("File","");
            renderer.drw_wiget(file_picker);
            ui.sp_btns[file_picker.wiget_n].ishide = false;
            ui.button_map[file_picker.wiget_n + "_open"].ishidden = false;
            if(imitate_btn(file_picker.wiget_n + "_open")){
                std::string ln = file_picker.names.buf.line(0);
                if(!(ln == "")){
                    fs::path f_path = ln;
                    f_path = file_picker.file_pick.p / f_path;
                    if(fs::exists(f_path)){
                        std::cout << f_path << std::endl;
                        ed.buf.setAllText(file_picker.file_pick.read_txt_file(f_path));
                        close = true;
                    }
                }
            }
            if (imitate_spbtn(file_picker.wiget_n) || close){
                ui.layer = 0;
                ui.z = false;
                ui.sp_btns[file_picker.wiget_n].ishide = true;
                ui.sp_btns[file_picker.wiget_n].clicked = false;
                ui.button_map[file_picker.wiget_n + "_open"].selected = false;
                ui.button_map[file_picker.wiget_n + "_open"].ishidden = true;
            }
        }
        renderer.rend();
		ui.flont();
    }
    renderer.destroy();
    return 0;
}