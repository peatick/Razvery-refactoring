#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include "sdlutil.h"
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <string>
#include <vector>

size_t utf8_length(const std::string& s) {
    size_t count = 0;
    for (size_t i = 0; i < s.size(); ) {
        unsigned char c = s[i];
        size_t char_len;

        if (c < 0x80) char_len = 1;                 // 1 byte (ASCII)
        else if ((c >> 5) == 0x6) char_len = 2;     // 110xxxxx
        else if ((c >> 4) == 0xE) char_len = 3;     // 1110xxxx
        else if ((c >> 3) == 0x1E) char_len = 4;    // 11110xxx
        else throw std::runtime_error("Invalid UTF-8");

        i += char_len;
        count++;
    }
    return count;
}

std::string utf8_substr(const std::string& s, size_t char_count) {
    size_t i = 0;
    size_t chars = 0;

    while (i < s.size() && chars < char_count) {
        unsigned char c = s[i];
        size_t char_len = 1;

        if ((c & 0x80) == 0x00) char_len = 1;
        else if ((c & 0xE0) == 0xC0) char_len = 2;
        else if ((c & 0xF0) == 0xE0) char_len = 3;
        else if ((c & 0xF8) == 0xF0) char_len = 4;

        i += char_len;
        chars++;
    }
    return s.substr(0, i);
}

fs::path str2path(std::string sp) {
    std::u8string u8 = reinterpret_cast<const char8_t*>(sp.c_str());
    return fs::path(reinterpret_cast<const char*>(u8.c_str()));
}

std::string path2string_s(const fs::path& p) {
    std::string str;
    std::u8string u8temp = p.u8string();
    str = std::string(reinterpret_cast<const char*>(u8temp.c_str()));
    return str;
}

bool equals_ignore_case(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) return false;
    return std::equal(a.begin(), a.end(), b.begin(), [](unsigned char c1, unsigned char c2) {
        return std::tolower(c1) == std::tolower(c2);
        });
}

bool equals_ext(fs::path a, std::string path_b) {
    std::string path_a = path2string_s(a.extension());
    if (path_a.empty() || path_b.empty()) return false;
    return equals_ignore_case(path_a, path_b);
}
//  Diff-based Undo/Redo  (Command Pattern)
//
//  EditCommand stores:
//    pos          – start of the affected region (before the edit)
//    deleted      – bytes removed  (empty for pure insert)
//    inserted     – bytes added    (empty for pure delete)
//    cursorBefore – caret restored on undo
//    cursorAfter  – caret restored on redo
//
//  apply()  re-does the command on the buffer (used by redo)
//  revert() un-does the command on the buffer (used by undo)
//
//  Merging rules:
//    • Consecutive single-char InsertText calls → merged into one Insert command
//      (split on newline so Undo works intuitively across line breaks)
//    • Consecutive Backspace calls → merged into one Delete command
//    • Consecutive forward-Delete calls → merged into one Delete command
//    • Replace / paste / cut → never merged