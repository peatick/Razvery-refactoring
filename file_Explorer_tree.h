#pragma once
#include "Renderer.h"
#include "Events.h"
#include <unordered_set>

class Hscroll_slider {
private:
    bool before = false;
    SDL_Point before_c = {};
    void update_knob_size() {
        if (tgt_lwide <= tgt_rwide) {
            knob = { 0,0,0,0 };
            return;
        }
        float ratio = float(tgt_rwide) / float(tgt_lwide);
        knob.w = int(size.w * ratio);
        knob.h = size.h;
        knob.y = size.y;
    }

    void update_knob_pos() {
        if (tgt_lwide <= tgt_rwide) return;
        float max_move = size.w - knob.w;
        knob.x = size.x + int((scroll_x / diff) * max_move);
    }

    void update_scroll_from_knob() {
        float max_move = size.w - knob.w;
        scroll_x = float(knob.x - size.x) / max_move * diff;
    }
public:
    SDL_Rect size = { 170,620,200,20 };
    SDL_Rect knob = { 0,0,0,0 };

    int tgt_lwide, tgt_rwide, diff;
    float scroll_x = 0;

    void set_l(int tlw, int trw) {
        tgt_lwide = tlw;
        tgt_rwide = trw;
        diff = tlw - trw;

        update_knob_size();
        update_knob_pos();
    }

    void render(Renderer& render) {
        if (tgt_lwide <= tgt_rwide) return;

        SDL_SetRenderDrawColor(render.ren, 220, 220, 220, 255);
        SDL_RenderFillRect(render.ren, &size);

        SDL_SetRenderDrawColor(render.ren, 80, 80, 80, 255);
        SDL_RenderFillRect(render.ren, &knob);
    }

    void eventH(EventHandler& e) {
        if (!e.nl_check()) return;
        SDL_Event& ev = *e.ev;

        // 左ボタン押した瞬間
        if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == SDL_BUTTON_LEFT) {
            SDL_Point p{ ev.button.x, ev.button.y };
            if (SDL_PointInRect(&p, &knob)) {
                before = true;
                before_c = p;
            }
        }

        // 左ボタン離した瞬間
        if (ev.type == SDL_MOUSEBUTTONUP && ev.button.button == SDL_BUTTON_LEFT) {
            before = false;
        }

        // ドラッグ中（押しっぱなし + マウス移動）
        if (before && ev.type == SDL_MOUSEMOTION) {
            int mx = ev.motion.x;
            int dx = mx - before_c.x;

            knob.x += dx;

            // 範囲制限
            if (knob.x < size.x) knob.x = size.x;
            if (knob.x > size.x + size.w - knob.w)
                knob.x = size.x + size.w - knob.w;

            before_c.x = mx;

            update_scroll_from_knob();
        }
    }

};

class File_explorer_tree {
private:
    struct Node {
        fs::path path;
        bool isDir = false;
        bool expanded = false;
        bool selected = false;
        std::vector<Node> children;
		bool text_cache_dirty = true;
		std::unordered_map<std::string, SDL_Texture*> text_cache;
    };

    int HZscroll = 0;
    bool selected = false;
    void clearNode(Node& node)
    {
        // テクスチャ破棄
        for (auto& kv : node.text_cache) {
            SDL_DestroyTexture(kv.second);
        }
        node.text_cache.clear();

        // 子ノードを再帰的に破棄
        for (auto& child : node.children) {
            clearNode(child);
        }
        node.children.clear();
    }

    // --- 再帰的にディレクトリを列挙 ---
    void buildTree(Node& node) {
        // path → Node* のマップを作る（コピーしない）
        std::unordered_map<std::string, Node*> oldMap;
        for (auto& old : node.children) {
            oldMap[old.path.string()] = &old;
        }

        std::vector<Node> newChildren;

        try {
            for (auto& entry : fs::directory_iterator(node.path)) {
                const std::string p = entry.path().string();

                Node* old = nullptr;
                if (oldMap.count(p)) {
                    old = oldMap[p];
                }

                Node child;

                if (old) {
                    // 古い Node を丸ごと引き継ぐ（select や deeper な children も保持）
                    child = *old;
                }
                else {
                    // 新規
                    child.path = entry.path();
                    child.isDir = entry.is_directory();
                    child.expanded = false;
                    child.selected = false;
                }

                // ディレクトリで expanded なら deeper を更新
                if (child.isDir && child.expanded) {
                    buildTree(child);
                }

                newChildren.push_back(std::move(child));
            }
        }
        catch (const fs::filesystem_error& e) {
            std::cerr << "filesystem error: " << e.what() << "\n";
        }

        // ソート
        std::sort(newChildren.begin(), newChildren.end(),
            [](const Node& a, const Node& b) {
                if (a.isDir != b.isDir)
                    return a.isDir > b.isDir;
                return a.path.filename() < b.path.filename();
            });

        node.children = std::move(newChildren);
    }

    void filename2string(const fs::path& p, std::string& str) {
        std::u8string u8temp = p.filename().u8string();
        str = std::string(reinterpret_cast<const char*>(u8temp.c_str()));
    }
    // --- フォルダ優先 + 名前順ソート ---
    void sortTree(Node& node) {
        std::sort(node.children.begin(), node.children.end(),
            [](const Node& a, const Node& b) {
                if (a.isDir != b.isDir)
                    return a.isDir > b.isDir; // Dir → File
                return a.path.filename() < b.path.filename();
            });

        for (auto& child : node.children) {
            if (child.isDir) {
                sortTree(child);
            }
        }
    }


    void all_tree_noneselect(Node& node) {
        for (auto& n : node.children) {
            n.selected = false;
            if (!node.children.empty()) {
                all_tree_noneselect(n);
            }
        }
    }

    int eventH_Recursive(EventHandler& e, Node& node, int row)
    {
        for (int i = 0; i < node.children.size(); ++i) {
            Node& child = node.children[i];
            int y = size.y + (row) * 20;
            // 描画範囲外なら終了
            if (y + 20 > size.y + size.h)
                return row;
            SDL_Rect click_rect = { size.x + 10, y, size.w - 20, 20 };
            // クリック処理
            if (SDL_PointInRect(e.nmP, &click_rect)) {
                all_tree_noneselect(root);  // 全非選択
                if (e.L_clicks(2)) {        // ダブルクリックで展開
                    child.expanded = !child.expanded;
                    if (child.expanded) {
                        buildTree(child);
                    }
                    else {
                        clearNode(child);
                    }
                }
                child.selected = true;
                selected = true;
            }
            row++; // 次の行へ
            // 展開されているなら子を再帰処理
            if (child.expanded) {
                row = eventH_Recursive(e, child, row);
            }
        }
        return row;
    }

    void textcache_destroy(Renderer& render, Node& node) {
        for (auto& [key, tex] : node.text_cache) {
            if (tex) {
                SDL_DestroyTexture(tex);
            }
        }
        node.text_cache.clear();
    }
    SDL_Texture* create_text_texture(Renderer& render, const std::string& text) {
        SDL_Color color = { 0, 0, 0, 255 };
        SDL_Surface* surf = TTF_RenderUTF8_Solid(render.getFont(), text.c_str(), color);
        if (!surf) return nullptr;
        SDL_Texture* tex = SDL_CreateTextureFromSurface(render.ren, surf);
        SDL_FreeSurface(surf);
        return tex;
    }

    void create_textcache(Renderer& render, Node& node) {
        textcache_destroy(render, node);
        std::string path_str;
        for (const auto& child : node.children) {
            filename2string(child.path, path_str);
            SDL_Color color = { 0, 0, 0, 255 };
            SDL_Surface* surf = TTF_RenderUTF8_Solid(render.getFont(), path_str.c_str(), color);
            if (surf) {
                SDL_Texture* tex = SDL_CreateTextureFromSurface(render.ren, surf);
                node.text_cache[path_str] = tex;
                SDL_FreeSurface(surf);
            }
        }
    }

    struct RowItem {
        Node* node;
        int depth;
    };

    void build_rows(Node& node, int depth, std::vector<RowItem>& rows)
    {
        for (auto& child : node.children) {
            rows.push_back({ &child, depth });

            if (child.expanded && child.isDir) {
                build_rows(child, depth + 1, rows);
            }
        }
    }

    

public:
    SDL_Rect size = { 170, 20, 200, 500 };
    Uint32 last_update = 0;
    Uint32 update_delay = 2000;
    bool update = false;
    int scrollrow = 0;
    bool fs_text_cache_dirty = true;
    
    Hscroll_slider hsl;


    Node root;

    // --- 初期化 ---
    void init() {
        root.path = fs::current_path();
        root.isDir = true;
        buildTree(root);
    }

    // --- パス変更 ---
    void setPath(const std::string& p) {
        std::u8string u8(p.begin(), p.end());
        fs::path new_path(u8.begin(), u8.end());

        if (!fs::exists(new_path)) {
            std::cerr << "Path does not exist: " << p << "\n";
            return;
        }

        root.path = fs::absolute(new_path);
        root.isDir = true;

        buildTree(root);
        fs_text_cache_dirty = true;
    }

    // --- 更新タイマー ---
    void tickupdate() {
        Uint32 now = SDL_GetTicks();
        if (now - last_update > update_delay) {
            update = true;
            last_update = now;
            buildTree(root);
        }
    }

    void render(Renderer& render)
    {
        tickupdate();
        SDL_SetRenderDrawColor(render.ren, 230, 230, 230, 255);
        SDL_RenderFillRect(render.ren, &size);
        // --- ツリーをフラット化 ---
        std::vector<RowItem> rows;
        build_rows(root, 0, rows);
        // --- スクロール位置から描画 ---
        int max_rows = size.h / 20;
        int drw_x = size.x + HZscroll;
        int max_length = 0;
        int now_length = 0;
        int str_long = 0;
        SDL_RenderSetClipRect(render.ren, &size);
        for (int i = scrollrow; i < rows.size() && i < scrollrow + max_rows; ++i) {
            RowItem& item = rows[i];
            Node& child = *item.node;
            int y = size.y + (i - scrollrow) * 20;
            std::string path_str;
            filename2string(child.path, path_str);
            str_long = render.textWidth(path_str);
            now_length = (25 + item.depth * 20 + str_long);
            if (max_length < now_length) {
                max_length = now_length;
            }
            // 選択背景
            if (child.selected) {
                SDL_SetRenderDrawColor(render.ren, 71, 190, 255, 255);
                SDL_Rect r = { size.x, y, size.w, 20 };
                SDL_RenderFillRect(render.ren, &r);
            }
            // アイコン
            int icon_x = drw_x + 5 + item.depth * 20;
            if (child.isDir)
                render.drawtexture(render.folderIcon, icon_x, y + 2);
            else
                render.drawtexture(render.fileIcon, icon_x, y + 2);
            // テキスト
            

            if (root.text_cache.find(path_str) != root.text_cache.end())
                render.drawtexture_clip(root.text_cache[path_str], icon_x + 20, y, size.w - 20 + -1 * HZscroll, 20);
            else
                root.text_cache[path_str] = create_text_texture(render, path_str);
        }
        SDL_RenderSetClipRect(render.ren, nullptr);
        hsl.set_l(max_length, size.w);
        hsl.render(render);

        HZscroll = int(hsl.scroll_x * -1);
    }

    void eventH(EventHandler& e) {
        if (!e.nl_check()) return;
        hsl.eventH(e);
        if (!SDL_PointInRect(e.nmP, &size)) { 
            return; 
        }
        switch (e.ev->type) {
        case SDL_MOUSEWHEEL:
            std::vector<RowItem> rows;
            build_rows(root, 0, rows);
            int visible_rows = size.h / 20;
            int total_rows = rows.size();
            if (total_rows - visible_rows >= 1)
                scrollrow = std::clamp(int(scrollrow - e.ev->wheel.y), 0, total_rows - visible_rows);
            break;
        }
		if (e.L_click()) {
            selected = false;
            eventH_Recursive(e, root, -scrollrow);
            if (!selected) {
                all_tree_noneselect(root);
            }
		}
    }

    void destroy() {
        clearNode(root);
    }
};