#include "skelt_f.h"
#include "UI_Cfg.h"
#include "LD.h"

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
    AssetLoader AL;
    auto w_f_tree = wm.mf.addwidget_t<Widget_TreeExplorer>({ 0,20,200,605 }, 1, "Tree_Ex");
    auto w_tab = wm.mf.addwidget_t<Widget_Lua_Tab_s>({200, 40, 800, 430}, 1, "Tabs");
    auto w_EditorTab = wm.mf.addwidget_t<Widget_Tab_System_u>({200, 0, 400, 20}, 1, "EdTab");
    auto w_paint = wm.mf.addwidget_t<Widget_Paint_v2>({ 0, 20, 1000, 605 }, 1, "PaintTool");
    auto w_maped = wm.mf.addwidget_t<Widget_Map_Ed_u>({ 200, 20, 800, 605 }, 1, "MapEditor");

    Btn_LT.beside_Btn(wm, "Men", { "File", "Edit", "View", "Tools", "Setting" }, { 0,0 });
    Btn_LT.Vertical_Btn(wm, "File", { "New Project", "New", "Save", "Save as" }, { 0, 20 });
    wm.mf.w_addbtn("GameEngine_Play", "GameEngine", "Play", {600, 0, 70, 20});
    wm.mf.w_addbtn("GameEngine_Debug", "GameEngine", "Debug", { 670, 0, 70, 20 });

    if (!AL.init()) {
        wm.new_dialog_Open("dir", [&](std::string s) {
            AL.NewProject(s);
            });
    }
    w_f_tree->fe_t.setPath(path2string_s(AL.projectpath));
    w_tab->close_ev = [&](){AL.script_ITR();};


    enum {
        scriptEditor,
        PaintTool,
        MapEditor
    };
    while (wm.running) {
        wm.mf.Widget_Call("EdTab");
        wm.mf.q_Btn("GameEngine_Play");
        wm.mf.q_Btn("GameEngine_Debug");
        if (w_EditorTab->selected() == scriptEditor){
            AL.LuaLex_update();
            if (w_f_tree->fe_t.select_act) {
                w_f_tree->fe_t.select_act = false;
                if (equals_ext(w_f_tree->fe_t.selected_path, ".lua")) {
                    fs::path& SavePath = w_f_tree->fe_t.selected_path;
                    std::string tmp = path2string_s(w_f_tree->fe_t.selected_path);
                    w_tab->add(wm.mf.renderer, SavePath, AL.LuaLex_Scr_shr(tmp));
                }
            }
            wm.mf.Widget_Call("Tree_Ex");
            wm.mf.Widget_Call("Tabs");

            if (wm.mf.q_Btn("Men_File")) {
                if (wm.mf.q_Btn("File_New Project")) {
                    wm.new_dialog_Open("dir", [&](std::string s) {
                        AL.NewProject(s);
                        w_f_tree->fe_t.setPath(s);
                        });
                }
                if (wm.mf.q_Btn("File_New")) {
                    std::string savepath = path2string_s(AL.projectpath / AL.scriptpath);
                    wm.new_dialog_Save_as_Simple(".lua", [&](std::string s) {std::ofstream f(s); AL.script_ITR(); }, savepath);
                }
                if (wm.mf.q_Btn("File_Save")) {
                    if (w_tab->act_Editor()) {
                        if (!w_tab->save_Text()) {
                            std::string savepath = path2string_s(AL.projectpath / AL.scriptpath);
                            wm.new_dialog_Save_as_Simple(".lua", [&](std::string s) {w_tab->save_as(s); AL.script_ITR(); }, savepath);
                        }
                    }
                }
                if (wm.mf.q_Btn("File_Save as")) {
                    std::string savepath = path2string_s(AL.projectpath / AL.scriptpath);
                    if (w_tab->act_Editor()) {
                        wm.new_dialog_Save_as_Simple(".lua", [&](std::string s) {w_tab->save_as(s); AL.script_ITR(); }, savepath);
                    }
                }
            }
            if (w_tab->act_Editor()) {
                if (w_tab->act_Editor()->save_req) {
                    w_tab->act_Editor()->save_req = false;
                    if (wm.vaid_path(w_tab->act_Editor()->ed_u.savepath)) {
                        w_tab->act_Editor()->ed_u.Save_File(w_tab->act_Editor()->ed_u.savepath);
                    }
                    else {
                        std::string savepath = path2string_s(AL.projectpath / AL.scriptpath);
                        wm.new_dialog_Save_as_Simple(".lua", [&](std::string s) {w_tab->save_as(s); }, savepath);
                    }
                }
            }
        }
        if (w_EditorTab->selected() == PaintTool) {
            wm.mf.Widget_Call("PaintTool");
            wm.mf.Widget_Call("Tree_Ex");
            if (w_f_tree->fe_t.select_act) {
                w_f_tree->fe_t.select_act = false;
                if (equals_ext(w_f_tree->fe_t.selected_path, ".png")) {
                    fs::path& SavePath = w_f_tree->fe_t.selected_path;
                    std::string tmp = path2string_s(w_f_tree->fe_t.selected_path);
                    w_paint->paint.LoadTextureFromPath(tmp);
                    w_paint->paint.savepath = tmp;
                }
            }
            if (wm.mf.q_Btn("Men_File")) {
                if (wm.mf.q_Btn("File_New Project")) {
                    wm.new_dialog_Open("dir", [&](std::string s) {
                        AL.NewProject(s);
                        w_f_tree->fe_t.setPath(s);
                        });
                }
                if (wm.mf.q_Btn("File_New")) {
                    w_paint->paint.dlgNew_OUT();
                    w_paint->paint.savepath = "";
                }
                if (wm.mf.q_Btn("File_Save")) {
                    if (wm.vaid_path(w_paint->paint.savepath)) {
                        w_paint->paint.SaveTextureToPNG(w_paint->paint.savepath);
                    }
                    else {
                        std::string savepath = path2string_s(AL.projectpath / AL.imgpath);
                        wm.new_dialog_Save_as_Simple(".png", [&](std::string s) {
                            w_paint->paint.SaveTextureToPNG(s); 
                            w_paint->paint.savepath = s; }, savepath);
                    }
                }
                if (wm.mf.q_Btn("File_Save as")) {
                    std::string savepath = path2string_s(AL.projectpath / AL.imgpath);
                    wm.new_dialog_Save_as_Simple(".png", [&](std::string s) {
                        w_paint->paint.SaveTextureToPNG(s);
                        w_paint->paint.savepath = s;
                        }, savepath);
                }
            }
            if (w_paint->save_req) {
                w_paint->save_req = false;
                if (wm.vaid_path(w_paint->paint.savepath)) {
                    w_paint->paint.SaveTextureToPNG(w_paint->paint.savepath);
                }
                else {
                    std::string savepath = path2string_s(AL.projectpath / AL.imgpath);
                    wm.new_dialog_Save_as_Simple(".png", [&](std::string s) {
                        w_paint->paint.SaveTextureToPNG(s);
                        w_paint->paint.savepath = s; }, savepath);
                }
            }
        }
        if (w_EditorTab->selected() == MapEditor) {
            wm.mf.Widget_Call("MapEditor");
            wm.mf.Widget_Call("Tree_Ex");
            if (w_f_tree->fe_t.select_act) {
                w_f_tree->fe_t.select_act = false;
                if (equals_ext(w_f_tree->fe_t.selected_path, ".dat")) {
                    fs::path& SavePath = w_f_tree->fe_t.selected_path;
                    std::string tmp = path2string_s(w_f_tree->fe_t.selected_path);
                    w_maped->mapName = tmp;
                    w_maped->read_map(tmp);
                }
                else if (equals_ext(w_f_tree->fe_t.selected_path, ".png")) {
                    fs::path& SavePath = w_f_tree->fe_t.selected_path;
                    std::string tmp = path2string_s(w_f_tree->fe_t.selected_path);
                    w_maped->init_Tileset(tmp);
                }
            }
            if (w_maped->save_req) {
                w_maped->save_req = false;
                if (!w_maped->save_map()) {
                    wm.new_dialog_Save_as_Simple(".dat", [&](std::string s) {
                        w_maped->mapName = s;
                        w_maped->save_map();
                        }, AL.projectpath/AL.mapspath);
                }
            }
            if (wm.mf.q_Btn("Men_File")) {
                if (wm.mf.q_Btn("File_New Project")) {
                    wm.new_dialog_Open("dir", [&](std::string s) {
                        AL.NewProject(s);
                        w_f_tree->fe_t.setPath(s);
                        });
                }
                if (wm.mf.q_Btn("File_New")) {
                    w_maped->clear();
                    w_maped->mapName = "";
                }
                if (wm.mf.q_Btn("File_Save")) {
                    if (w_maped->arw_save()) {
                        if (!w_maped->save_map()) {
                            wm.new_dialog_Save_as_Simple(".dat", [&](std::string s) {
                                w_maped->mapName = s;
                                w_maped->save_map();
                                }, AL.projectpath / AL.mapspath);
                        }
                    }
                }
                if (wm.mf.q_Btn("File_Save as")) {
                    if (w_maped->arw_save()) {
                        wm.new_dialog_Save_as_Simple(".dat", [&](std::string s) {
                            w_maped->mapName = s;
                            w_maped->save_map();
                            }, AL.projectpath / AL.mapspath);
                    }
                }
            }
        }
        wm.dialog_EV();
        wm.events();
        wm.render();
    }
   
    wm.exit();
    return 0;
}
