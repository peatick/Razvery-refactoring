#pragma once
#include "sdl2/include/SDL.h"
#include "sdl2/include/SDL_ttf.h"
#include <algorithm>
#include <climits>
#include <deque>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <unordered_map>

enum fileTYPE{
    file_or_dir,
    file,
    dir,
    save
};

namespace fs = std::filesystem;

static constexpr int    WIN_W = 1200;
static constexpr int    WIN_H = 800;
static constexpr int    logical_W = 1200;
static constexpr int    logical_H = 800;
static constexpr int    FONT_SIZE = 18;
static constexpr int    FONTSML_SIZE = 12;
static constexpr int    PG = 16;
static constexpr int    LINE_SPACING = 4;
static constexpr int    CURSOR_WIDTH = 2;
static constexpr int    MAX_UNDO = 500;   // cheap: only diffs stored
static constexpr Uint32 BLINK_MS = 530;

size_t utf8_length(const std::string& s);
std::string utf8_substr(const std::string& s, size_t char_count);
namespace fs = std::filesystem;
namespace utf8 {

    inline int charLen(const std::string& s, int i) {
        if (i < 0 || i >= (int)s.size()) return 0;
        unsigned char c = (unsigned char)s[i];
        if (c < 0x80) return 1;
        if (c < 0xE0) return 2;
        if (c < 0xF0) return 3;
        return 4;
    }
    inline int next(const std::string& s, int i) { return i + charLen(s, i); }
    inline int prev(const std::string& s, int i) {
        if (i <= 0) return 0;
        int p = i - 1;
        while (p > 0 && ((unsigned char)s[p] & 0xC0) == 0x80) --p;
        return p;
    }
    inline int countChars(const std::string& s, int byteLen) {
        int n = 0, i = 0;
        while (i < byteLen) { i = next(s, i); ++n; }
        return n;
    }
    inline int prevWord(const std::string& s, int i) {
        while (i > 0 && s[utf8::prev(s, i)] == ' ') i = utf8::prev(s, i);
        while (i > 0 && s[utf8::prev(s, i)] != ' ') i = utf8::prev(s, i);
        return i;
    }
    inline int nextWord(const std::string& s, int i) {
        int n = (int)s.size();
        while (i < n && s[i] != ' ') i = utf8::next(s, i);
        while (i < n && s[i] == ' ') i = utf8::next(s, i);
        return i;
    }

} // namespace utf8
//  TextBuffer
struct TextBuffer {
    std::vector<std::string> lines;
    TextBuffer() { lines.push_back(""); }

    struct Pos {
        int row = 0, col = 0;
        bool operator==(const Pos& o) const { return row == o.row && col == o.col; }
        bool operator!=(const Pos& o) const { return !(*this == o); }
        bool operator< (const Pos& o) const { return row < o.row || (row == o.row && col < o.col); }
        bool operator<=(const Pos& o) const { return !(o < *this); }
    };

    int numLines()  const { return (int)lines.size(); }
    const std::string& line(int r) const { return lines[r]; }
    int lineLen(int r) const { return (int)lines[r].size(); }

    void clamp(Pos& p) const {
        p.row = std::clamp(p.row, 0, numLines() - 1);
        p.col = std::clamp(p.col, 0, lineLen(p.row));
        while (p.col > 0 && ((unsigned char)lines[p.row][p.col] & 0xC0) == 0x80) --p.col;
    }

    // Insert text at pos; returns position after the inserted text
    Pos insert(Pos pos, const std::string& text) {
        for (char ch : text) {
            if (ch == '\n') {
                std::string rest = lines[pos.row].substr(pos.col);
                lines[pos.row] = lines[pos.row].substr(0, pos.col);
                lines.insert(lines.begin() + pos.row + 1, rest);
                pos.row++; pos.col = 0;
            }
            else {
                lines[pos.row].insert(pos.col, 1, ch);
                pos.col++;
            }
        }
        return pos;
    }

    // Erase [a, b)
    void erase(Pos a, Pos b) {
        if (b <= a) return;
        if (a.row == b.row) {
            lines[a.row].erase(a.col, b.col - a.col);
        }
        else {
            lines[a.row] = lines[a.row].substr(0, a.col) + lines[b.row].substr(b.col);
            lines.erase(lines.begin() + a.row + 1, lines.begin() + b.row + 1);
        }
    }

    // Extract text in [a, b)
    std::string extract(Pos a, Pos b) const {
        if (b <= a) return "";
        std::string out;
        for (int r = a.row; r <= b.row; ++r) {
            int from = (r == a.row) ? a.col : 0;
            int to = (r == b.row) ? b.col : lineLen(r);
            out += lines[r].substr(from, to - from);
            if (r < b.row) out += '\n';
        }
        return out;
    }

    std::string allText() const {
        std::string out;
        for (int i = 0; i < numLines(); ++i) { if (i) out += '\n'; out += lines[i]; }
        return out;
    }
    void setAllText(const std::string& text) {
        lines.clear();
        std::istringstream ss(text); std::string ln;
        while (std::getline(ss, ln)) lines.push_back(ln);
        if (lines.empty()) lines.push_back("");
    }

    // Advance a Pos by walking through a string (to find the end of an insertion)
    static Pos advance(Pos start, const std::string& s) {
        for (char ch : s) {
            if (ch == '\n') { start.row++; start.col = 0; }
            else { start.col++; }
        }
        return start;
    }
};

enum class EditKind { Insert, Delete, Replace };

struct EditCommand {

    TextBuffer::Pos pos;            // start of affected range in the buffer
    std::string     deleted;        // text that was removed
    std::string     inserted;       // text that was added
    TextBuffer::Pos cursorBefore;   // caret before this command (undo target)
    TextBuffer::Pos cursorAfter;    // caret after  this command (redo target)
    EditKind        kind;

    // ---- Apply (redo) -----------------------------------------------
    // Remove 'inserted' length bytes then write 'deleted' back  ← wrong direction
    // Correct: erase deleted-region, insert 'inserted'.
    // On first apply the buffer already has the right content; this is called
    // only from redo, so we must replay: erase what was there, put back inserted.
    TextBuffer::Pos apply(TextBuffer& buf) const {
        // The region that 'deleted' once occupied has since been replaced by
        // 'inserted'. To redo: erase 'inserted' then re-insert 'inserted'... that
        // would be a no-op. The correct redo is:
        //   1. erase 'inserted' span starting at pos
        //   2. insert 'deleted'... NO – that is undo.
        // Redo = erase 'deleted' span, then insert 'inserted':
        //   But at redo-time the buffer already has 'deleted' restored (by undo).
        //   So redo = erase 'deleted' span (starting at pos), insert 'inserted'.
        if (!deleted.empty()) {
            TextBuffer::Pos endDel = TextBuffer::advance(pos, deleted);
            buf.erase(pos, endDel);
        }
        TextBuffer::Pos ret = pos;
        if (!inserted.empty()) ret = buf.insert(pos, inserted);
        return ret; // = cursorAfter
    }

    // ---- Revert (undo) ----------------------------------------------
    TextBuffer::Pos revert(TextBuffer& buf) const {
        // undo = erase 'inserted' span, then restore 'deleted'
        if (!inserted.empty()) {
            TextBuffer::Pos endIns = TextBuffer::advance(pos, inserted);
            buf.erase(pos, endIns);
        }
        if (!deleted.empty()) buf.insert(pos, deleted);
        return cursorBefore;
    }

    // ---- Merge check ------------------------------------------------
    // Returns true if `next` can be folded into *this.
    bool canMerge(const EditCommand& next) const {
        if (kind != next.kind)        return false;
        if (kind == EditKind::Replace) return false;

        if (kind == EditKind::Insert) {
            // Only single non-newline chars
            if (next.inserted.size() != 1)    return false;
            if (next.inserted[0] == '\n')      return false;
            // next must start exactly where this one ended
            return next.pos == cursorAfter;
        }

        // Delete (backspace or forward-delete)
        if (next.deleted.size() != 1)  return false;
        if (next.deleted[0] == '\n')   return false;
        // Backspace: pos moves left each keystroke; forward-delete: pos stays
        bool isBackspace = (next.cursorAfter == next.pos);  // cursor == pos means backspace
        bool thisIsBackspace = (cursorAfter == pos);
        if (isBackspace != thisIsBackspace) return false;        // don't mix directions

        if (isBackspace) {
            // next backspace erases the char just before `pos`
            return next.pos == pos || next.cursorAfter < pos;
        }
        else {
            // forward delete: pos stays the same
            return next.pos == pos;
        }
    }

    void mergeWith(const EditCommand& next) {
        if (kind == EditKind::Insert) {
            inserted += next.inserted;
            cursorAfter = next.cursorAfter;
        }
        else {
            bool isBackspace = (cursorAfter == pos);
            if (isBackspace) {
                // prepend newly deleted char (it was to the left)
                deleted = next.deleted + deleted;
                pos = next.pos;
                cursorAfter = next.cursorAfter;
            }
            else {
                // append (forward delete)
                deleted += next.deleted;
                cursorAfter = next.cursorAfter;
            }
        }
    }
};
//  UndoHistory
struct UndoHistory {
    std::deque<EditCommand> undoStack;
    std::deque<EditCommand> redoStack;

    bool canUndo() const { return !undoStack.empty(); }
    bool canRedo() const { return !redoStack.empty(); }

    void push(EditCommand cmd) {
        if (!undoStack.empty() && undoStack.back().canMerge(cmd)) {
            undoStack.back().mergeWith(cmd);
        }
        else {
            if ((int)undoStack.size() >= MAX_UNDO) undoStack.pop_front();
            undoStack.push_back(std::move(cmd));
        }
        redoStack.clear();
    }

    TextBuffer::Pos undo(TextBuffer& buf) {
        EditCommand cmd = undoStack.back(); undoStack.pop_back();
        auto pos = cmd.revert(buf);
        redoStack.push_back(std::move(cmd));
        return pos;
    }

    TextBuffer::Pos redo(TextBuffer& buf) {
        EditCommand cmd = redoStack.back(); redoStack.pop_back();
        auto pos = cmd.apply(buf);
        undoStack.push_back(std::move(cmd));
        return pos;
    }

    std::size_t undoCount() const { return undoStack.size(); }
    std::size_t redoCount() const { return redoStack.size(); }
};
class Editor {
public:
    bool lim_f = false;
    int limit = 0;
    TextBuffer      buf;
    TextBuffer::Pos cursor;
    TextBuffer::Pos selAnchor;
    bool            hasSelection = false;
	bool            noLineNo = false;
    int PADDING = PG;
    // IME
    std::string imeComposing;
    int         imeCursor = 0;

    // Undo
    UndoHistory history;

    // Layout (set by Renderer each frame)
    int scrollRow = 0, scrollX = 0;
    int lineH = 0, viewRows = 0, viewW = 0;

    // txbox-locate
	SDL_Rect TX_Rect = { 10, 20, 600, 400 };

    // Caret blink
    Uint32 lastBlink = 0;
    bool   caretOn = true;

    void init() { cursor = { 0,0 }; selAnchor = { 0,0 }; }
    void set_init(SDL_Rect r,std::string str,int lH){
        lineH = lH;
		TX_Rect = r;
        buf.setAllText(str);
        init();
    }
    // ---- Selection ----
    TextBuffer::Pos selMin() const { return cursor < selAnchor ? cursor : selAnchor; }
    TextBuffer::Pos selMax() const { return cursor < selAnchor ? selAnchor : cursor; }
    void clearSelection() { hasSelection = false; selAnchor = cursor; }
    void startSelection() { if (!hasSelection) { hasSelection = true; selAnchor = cursor; } }
    void selectAll() {
        hasSelection = true; selAnchor = { 0,0 };
        cursor = { buf.numLines() - 1, buf.lineLen(buf.numLines() - 1) };
    }
    std::string selectedText() const {
        return hasSelection ? buf.extract(selMin(), selMax()) : "";
    }

    // ---- Text insertion (records Insert or Replace command) ----
    void insertText(const std::string& text) {
        if (utf8_length(buf.line(cursor.row)) + utf8_length(text) > limit && lim_f) {
            return;
        }
        EditCommand cmd;
        cmd.cursorBefore = cursor;

        if (hasSelection) {
            cmd.kind = EditKind::Replace;
            cmd.pos = selMin();
            cmd.deleted = buf.extract(selMin(), selMax());
            buf.erase(selMin(), selMax());
            cursor = selMin();
            clearSelection();
        }
        else {
            cmd.kind = EditKind::Insert;
            cmd.pos = cursor;
        }

        cmd.inserted = text;
        cursor = buf.insert(cursor, text);
        cmd.cursorAfter = cursor;
        clearSelection();
        history.push(cmd);
        ensureCursorVisible();
    }

    void insertNewline() { insertText("\n"); }

    // ---- Backspace (records Delete command) ----
    void backspace() {
        if (hasSelection) { deleteSelection(); return; }
        if (cursor.col == 0 && cursor.row == 0) return;

        TextBuffer::Pos from = cursor;
        if (cursor.col == 0) {
            from = { cursor.row - 1, buf.lineLen(cursor.row - 1) };
        }
        else {
            from.col = utf8::prev(buf.line(cursor.row), cursor.col);
        }

        EditCommand cmd;
        cmd.kind = EditKind::Delete;
        cmd.pos = from;
        cmd.deleted = buf.extract(from, cursor);
        cmd.cursorBefore = cursor;
        cmd.cursorAfter = from;   // cursor == pos ⟹ backspace flag for merging
        buf.erase(from, cursor);
        cursor = from;
        clearSelection();
        history.push(cmd);
        ensureCursorVisible();
    }

    // ---- Forward delete (records Delete command) ----
    void deleteForward() {
        if (hasSelection) { deleteSelection(); return; }
        TextBuffer::Pos to = cursor;
        if (to.col < buf.lineLen(to.row))
            to.col = utf8::next(buf.line(to.row), to.col);
        else if (to.row < buf.numLines() - 1)
            to = { cursor.row + 1, 0 };
        else return;

        EditCommand cmd;
        cmd.kind = EditKind::Delete;
        cmd.pos = cursor;
        cmd.deleted = buf.extract(cursor, to);
        cmd.cursorBefore = cursor;
        cmd.cursorAfter = cursor;  // cursor stays ⟹ forward-delete flag
        buf.erase(cursor, to);
        clearSelection();
        history.push(cmd);
        ensureCursorVisible();
    }

    // ---- Delete selection (records Replace command with empty insert) ----
    void deleteSelection() {
        if (!hasSelection) return;
        EditCommand cmd;
        cmd.kind = EditKind::Replace;
        cmd.pos = selMin();
        cmd.deleted = buf.extract(selMin(), selMax());
        cmd.inserted = "";
        cmd.cursorBefore = cursor;
        cmd.cursorAfter = selMin();
        buf.erase(selMin(), selMax());
        cursor = selMin();
        clearSelection();
        history.push(cmd);
        ensureCursorVisible();
    }

    // ---- Undo / Redo ----
    void doUndo() {
        if (!history.canUndo()) return;
        cursor = history.undo(buf);
        clearSelection(); buf.clamp(cursor);
        ensureCursorVisible();
    }
    void doRedo() {
        if (!history.canRedo()) return;
        cursor = history.redo(buf);
        clearSelection(); buf.clamp(cursor);
        ensureCursorVisible();
    }

    // ---- Cursor movement ----
    void moveCursor(TextBuffer::Pos p, bool select) {
        if (select) startSelection(); else clearSelection();
        cursor = p; buf.clamp(cursor); ensureCursorVisible();
    }
    void moveLeft(bool sel, bool word) {
        TextBuffer::Pos p = cursor;
        if (!sel && hasSelection) { p = selMin(); clearSelection(); cursor = p; ensureCursorVisible(); return; }
        if (word) {
            if (p.col > 0) p.col = utf8::prevWord(buf.line(p.row), p.col);
            else if (p.row > 0) { p.row--; p.col = buf.lineLen(p.row); }
        }
        else {
            if (p.col > 0) p.col = utf8::prev(buf.line(p.row), p.col);
            else if (p.row > 0) { p.row--; p.col = buf.lineLen(p.row); }
        }
        moveCursor(p, sel);
    }
    void moveRight(bool sel, bool word) {
        TextBuffer::Pos p = cursor;
        if (!sel && hasSelection) { p = selMax(); clearSelection(); cursor = p; ensureCursorVisible(); return; }
        if (word) {
            if (p.col < buf.lineLen(p.row)) p.col = utf8::nextWord(buf.line(p.row), p.col);
            else if (p.row < buf.numLines() - 1) { p.row++; p.col = 0; }
        }
        else {
            if (p.col < buf.lineLen(p.row)) p.col = utf8::next(buf.line(p.row), p.col);
            else if (p.row < buf.numLines() - 1) { p.row++; p.col = 0; }
        }
        moveCursor(p, sel);
    }
    void moveUp(bool sel) {
        if (cursor.row == 0) { moveCursor({ 0,0 }, sel); return; }
        moveCursor({ cursor.row - 1, cursor.col }, sel);
    }
    void moveDown(bool sel) {
        if (cursor.row == buf.numLines() - 1) { moveCursor({ cursor.row, buf.lineLen(cursor.row) }, sel); return; }
        moveCursor({ cursor.row + 1, cursor.col }, sel);
    }
    void moveHome(bool sel, bool ctrl) {
        moveCursor(ctrl ? TextBuffer::Pos{ 0,0 } : TextBuffer::Pos{ cursor.row,0 }, sel);
    }
    void moveEnd(bool sel, bool ctrl) {
        int lr = ctrl ? buf.numLines() - 1 : cursor.row;
        moveCursor({ lr, buf.lineLen(lr) }, sel);
    }

    // ---- Clipboard ----
    void copy() { if (hasSelection) SDL_SetClipboardText(selectedText().c_str()); }
    void cut() { if (!hasSelection) return; copy(); deleteSelection(); }
    void paste() {
        if (!SDL_HasClipboardText()) return;
        char* txt = SDL_GetClipboardText();
        if (txt) {
            std::string s(txt);
            SDL_free(txt);

            // ★ CR を除去（Windows の CRLF 対策）
            s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());

            insertText(s);
        }
    }


    // ---- Scroll ----
    void ensureCursorVisible() {
        if (lineH <= 0 || viewRows <= 0) return;
        if (cursor.row < scrollRow) scrollRow = cursor.row;
        if (cursor.row >= scrollRow + viewRows) scrollRow = cursor.row - viewRows + 1;
    }

    // ---- Mouse hit-test ----
    TextBuffer::Pos hitTest(int px, int py, TTF_Font* font) const {
        if (noLineNo) {
            px -= TX_Rect.x + PADDING - 14;
        }
        else {
            px -= TX_Rect.x + PADDING + 34;
        }
        py -= TX_Rect.y;
        int row = scrollRow + (py - PADDING) / lineH;
        row = std::clamp(row, 0, buf.numLines() - 1);
        const std::string& ln = buf.line(row);
        if (ln.empty()) return { row, 0 };
        int best = 0, bestDist = INT_MAX, i = 0;
        while (true) {
            int w = 0, h = 0; TTF_SizeUTF8(font, ln.substr(0, i).c_str(), &w, &h);
            int d = std::abs(w - (px - PADDING + scrollX));
            if (d < bestDist) { bestDist = d; best = i; }
            if (i >= (int)ln.size()) break;
            i = utf8::next(ln, i);
        }
        return { row, best };
    }

    // ---- Caret blink ----
    void tickBlink() {
        Uint32 now = SDL_GetTicks();
        if (now - lastBlink > BLINK_MS) { caretOn = !caretOn; lastBlink = now; }
    }
    void resetBlink() { caretOn = true; lastBlink = SDL_GetTicks(); }
};

struct Editors {
    Editor ed;
    std::string name = "new Editor";
    bool saved = false;
};

struct file_enum{
    fs::path file_path;
};

class File_explorer{
public:
    std::vector<file_enum> file_list;
    fs::path path_box = fs::current_path();
    Uint32 last_update = 0;
    Uint32 update_delay = 500;
    bool update = false;
    int scrollrow = 0;
    void tickupdate(){
        Uint32 now = SDL_GetTicks();
        if (now - last_update > update_delay){
            update = true;
            last_update = now;
        }
    }
    void file_sort(){
        std::sort(file_list.begin(), file_list.end(), [](const file_enum& a, const file_enum& b) {
            if (fs::is_directory(a.file_path) && !fs::is_directory(b.file_path)) return true;
            if (!fs::is_directory(a.file_path) && fs::is_directory(b.file_path)) return false;
            return a.file_path.filename() < b.file_path.filename();
        });
	}
    void file_lister() {
        file_list.clear();
        try {
            for (auto& f : fs::directory_iterator(path_box)) {
                file_enum fe;
                fe.file_path = f.path();
                file_list.push_back(fe);
            }
            file_sort();
        }
        catch (const fs::filesystem_error& e) {
            // ログを出すなど
            std::cerr << "filesystem error: " << e.what() << "\n";
        }
    }
};