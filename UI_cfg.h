#pragma once
#include "skelt_f.h"
#include <initializer_list>

class UI_BTNS_MEN {
private:
	std::vector<SKEL_Frame::idAndname> beside;
	std::vector<SKEL_Frame::idAndname> vert;
public:

    void beside_Btn(window_Manager& w, std::string men, std::initializer_list<std::string> args, SDL_Point Start) {
        for (const auto& str : args) {
            beside.push_back({men + "_" + str, str});
        }
        w.mf.BtnAutoset_Beside(beside, men, {Start.x, Start.y, 70, 20});
    }
    void Vertical_Btn(window_Manager& w, std::string men, std::initializer_list<std::string> args, SDL_Point Start) {
        for (const auto& str : args) {
            vert.push_back({ men + "_" + str, str });
        }
        w.mf.BtnAutoset_Vertical_c(vert, men, { Start.x, Start.y, 70, 20 }, false, false);
    }
};