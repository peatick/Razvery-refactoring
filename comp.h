#pragma once
#include "sol2/sol.hpp"
#include "runtime_map.hpp"
#include "tileset.hpp"
struct pos {
	uint32_t Entity_ID = 0;
	double x = 0;
	double y = 0;
};
struct Render_body {
	uint32_t Entity_ID = 0;
    double w;
    double h;
};
struct Name {
	uint32_t Entity_ID = 0;
	std::string name;
};
struct LuaCom {
    uint32_t Entity_ID = 0;
    sol::table data;
};
struct MapObj {
	uint32_t Entity_ID = 0;
	double x = 0;
	double y = 0;
	double size = 0;
};
struct sprite_render {
	std::string sprite_name;
    SDL_Rect dst;
    SDL_Rect src;
	bool use_src = false; // srcを使うかどうかのフラグ
	bool flip_h = false; // 水平方向に反転するかどうかのフラグ
	bool flip_v = false; // 垂直方向に反転するかどうかのフラグ
};
class scene {
private:
    uint32_t next_entity_id = 1;
    std::vector<uint32_t> free_ids;
	std::vector<sprite_render> sprite_render_requests; // 描画要求のスプライト名を保持するベクター
    int kWallFlagBit = 0;
    // 生きているEntityのIDを一覧で保持（エディタのヒエラルキー表示用）
    std::unordered_set<uint32_t> active_entities;
    bool collidesWithWall(const RuntimeMap& map, const Tileset& tileset, float x, float y, float size) {
        constexpr float kEpsilon = 0.001f;  // 境界ちょうどを「まだ次のタイルに入っていない」扱いにする
        const int colMin = static_cast<int>(std::floor(x / Tileset::kTileSize));
        const int colMax = static_cast<int>(std::floor((x + size - kEpsilon) / Tileset::kTileSize));
        const int rowMin = static_cast<int>(std::floor(y / Tileset::kTileSize));
        const int rowMax = static_cast<int>(std::floor((y + size - kEpsilon) / Tileset::kTileSize));

        for (int row = rowMin; row <= rowMax; ++row) {
            for (int col = colMin; col <= colMax; ++col) {
                if (map.hasFlagAt(tileset, col, row, kWallFlagBit)) return true;
            }
        }
        return false;
    }
    float resolveAxisMove(const RuntimeMap& map, const Tileset& tileset, float fixedOther, bool isXAxis, float from,
        float to, float size) {
        if (to == from) return to;

        auto collidesAt = [&](float pos) {
            return isXAxis ? collidesWithWall(map, tileset, pos, fixedOther, size)
                : collidesWithWall(map, tileset, fixedOther, pos, size);
            };

        const bool movingPositive = to > from;
        const float maxStep = static_cast<float>(Tileset::kTileSize);
        float current = from;

        while (current != to) {
            const float next =
                movingPositive ? std::min(current + maxStep, to) : std::max(current - maxStep, to);

            if (collidesAt(next)) {
                // このタイル境界の手前ぴったりまで詰めて終了
                const float leadingEdge = movingPositive ? next + size : next;
                const int tileIndex = static_cast<int>(std::floor(leadingEdge / Tileset::kTileSize));
                return movingPositive ? (tileIndex * Tileset::kTileSize - size) : ((tileIndex + 1) * Tileset::kTileSize);
            }
            current = next;
        }
        return current;
    }
public:
	SDL_Rect Camera = { 0, 0, 1000, 625 }; // カメラの位置とサイズを保持するSDL_Rect
	SDL_Rect Viewport = { 0, 0, 1000, 625 }; // ビューポートの位置とサイズを保持するSDL_Rect
    int kScale = 6;
    RuntimeMap map;
	std::unique_ptr<Tileset> tileset;
	SDL_Renderer* renderer = nullptr;
	bool loaded_map = false;
    std::string map_path;
	std::string img_path;

	sol::environment lua_env;  // Lua環境を保持するメンバ変数
    sol::protected_function lua_update_func;  // Luaのupdate関数を保持するメンバ変数
    // 各コンポーネントのストレージ
    std::vector<pos> Pos;
    std::vector<Render_body> Render_Body;
    std::vector<Name> Name_Body;
	std::vector<MapObj> MapObj_Body;
	std::unordered_map<std::string, std::vector<LuaCom>> LuaCom_Body;
	std::unordered_map<std::string, SDL_Texture*> Sprite_Textures;
    void init(sol::state& lua, sol::environment& env) {
        // usertype自体はstateに登録(型システムはグローバル)
        lua.new_usertype<pos>("pos",
            "Entity_ID", &pos::Entity_ID,
            "x", &pos::x,
            "y", &pos::y
        );
        lua.new_usertype<Render_body>("Render_body",
            "Entity_ID", &Render_body::Entity_ID,
            "w", &Render_body::w,
            "h", &Render_body::h
        );
        lua.new_usertype<Name>("Name",
            "Entity_ID", &Name::Entity_ID,
            "name", &Name::name
        );
        lua.new_usertype<LuaCom>("LuaCom",
            "Entity_ID", &LuaCom::Entity_ID,
            "data", &LuaCom::data
        );
		lua.new_usertype<MapObj>("MapObj",
			"Entity_ID", &MapObj::Entity_ID,
			"x", &MapObj::x,
			"y", &MapObj::y,
			"size", &MapObj::size
		);
        env["move_cam"] = [this](int x, int y) {
			if (x < 0) x = 0;
			if (y < 0) y = 0;
            this->Camera.x = x;
            this->Camera.y = y;
        };

		env["load_map"] = [this](std::string map_name, std::string img_name) {
			loaded_map = this->map.loadFromFile(map_path + "/" + map_name);
			tileset = std::make_unique<Tileset>(renderer, img_path + "/" + img_name);
			};
		env["load_sprite"] = [this](std::string sprite_name, std::string img_name) {
			SDL_Surface* surface = IMG_Load((img_path + "/" + img_name).c_str());

			if (!surface) {
				throw std::runtime_error("Failed to load image: " + img_name);
			}
			SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
			SDL_FreeSurface(surface);
			if (!texture) {
				throw std::runtime_error("Failed to create texture from image: " + img_name);
			}
			Sprite_Textures[sprite_name] = texture;
			};
		env["draw_sprite"] = [this](std::string sprite_name, double x, double y, double w, double h) {
            sprite_render_requests.push_back({ sprite_name, 
                { static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h)
            } });
			};
		// 関数はenvに登録する(envを見ているスクリプトだけが使える)
        env["create_entity"] = [this]() {
            return this->create_entity();
            };
		env["resove_moveX"] = [this](double new_x,double dx, double dy, double size) {
            return resolveAxisMove(this->map, *this->tileset, dy, true, dx, new_x, size);
			};
        env["resove_moveY"] = [this](double new_y, double dx, double dy, double size) {
            return resolveAxisMove(this->map, *this->tileset, dx, false, dy, new_y, size);
            };
        env["add_newCom"] = [this](uint32_t entity_id, sol::table data, std::string com_type) {
            if (com_type == "Render_body") {
                Render_Body.push_back({ entity_id, data["w"], data["h"] });
            }
            else if (com_type == "pos") {
                Pos.push_back({ entity_id, data["x"], data["y"] });
            }
            else if (com_type == "Name") {
                Name_Body.push_back({ entity_id, data["name"] });
            }
            else if (com_type == "MapObj") {
                MapObj_Body.push_back({ entity_id, data["x"], data["y"], data["size"] });
            }
            };
		env["camera_pos"] = [this]() {
			return std::make_tuple(this->Camera.x, this->Camera.y);
			};
        env["add_newLuaCom"] = [this](uint32_t entity_id, sol::table data, std::string com_type) {
            if (LuaCom_Body.find(com_type) == LuaCom_Body.end()) {
                LuaCom_Body[com_type] = std::vector<LuaCom>();
            }
            LuaCom_Body[com_type].push_back({ entity_id, data });
            };

        env["get_all_entities"] = [this]() {
            return this->get_all_entities();
            };

        env["get_lua_com"] = [this](uint32_t entity_id, std::string com_type) -> sol::table {
            if (LuaCom_Body.find(com_type) != LuaCom_Body.end()) {
                for (const auto& lua_com : LuaCom_Body[com_type]) {
                    if (lua_com.Entity_ID == entity_id) {
                        return lua_com.data;
                    }
                }
            }
            return sol::nil;
            };

        env["remove_entity"] = [this](uint32_t entity_id) {
            this->destroy_entity(entity_id);
            };

        // ここもlua参照でtableを作るのは変わらないが、キャプチャにenvも足す
        env["get_all_any_com"] = [this, &lua]() {
            sol::table all_lua_com = lua.create_table();
            for (const auto& [com_type, vec] : LuaCom_Body) {
                sol::table com_table = lua.create_table();
                for (const auto& lua_com : vec) {
                    com_table[lua_com.Entity_ID] = lua_com.data;
                }
                all_lua_com[com_type] = com_table;
            }
            all_lua_com["Render_body"] = lua.create_table();
            for (auto& r : Render_Body) {
                all_lua_com["Render_body"][r.Entity_ID] = std::ref(r);
            }
            all_lua_com["pos"] = lua.create_table();
            for (auto& p : Pos) {
                all_lua_com["pos"][p.Entity_ID] = std::ref(p);
            }
            all_lua_com["Name"] = lua.create_table();
            for (auto& n : Name_Body) {
                all_lua_com["Name"][n.Entity_ID] = std::ref(n);
            }
			all_lua_com["MapObj"] = lua.create_table();
			for (auto& m : MapObj_Body) {
				all_lua_com["MapObj"][m.Entity_ID] = std::ref(m);
			}
            return all_lua_com;
            };

        // Lua側で書かれたヘルパー関数もenv上で実行する
        lua.script(R"(
        function get_entity_with(...)
            local com_types = {...}
            local match_entities = {}
            local entitys = get_all_entities()
            local all_coms = get_all_any_com()
            for _, entity_id in ipairs(entitys) do
                local has_all = true
                local ent_components = {}

                for _, com_type in ipairs(com_types) do
                    local com_table = all_coms[com_type]

                    if com_table and com_table[entity_id] then
                        ent_components[com_type] = com_table[entity_id]
                    else
                        has_all = false
                        break
                    end
                end
                if has_all then
                    match_entities[entity_id] = ent_components
                end
            end
            return match_entities
        end
        function for_each(com_name, callback_func)
            for ent_id, com_data in pairs(get_entity_with(table.unpack(com_name))) do
                callback_func(com_data, ent_id)
            end
        end
    )", env);
    }
    uint32_t create_entity() {
        uint32_t id;
        if (!free_ids.empty()) {
            id = free_ids.back();
            free_ids.pop_back();
        }
        else {
            id = next_entity_id++;
        }

        active_entities.insert(id); // 生存リストに追加
        return id;
    }
    void destroy_entity(uint32_t entity_id) {
        auto remove_by_id = [entity_id](auto& vec) {
            vec.erase(
                std::remove_if(vec.begin(), vec.end(),
                    [entity_id](const auto& item) { return item.Entity_ID == entity_id; }),
                vec.end()
            );
            };

        remove_by_id(Pos);
        remove_by_id(Render_Body);
        remove_by_id(Name_Body);
		remove_by_id(MapObj_Body);
		for (auto& [com_type, vec] : LuaCom_Body) {
			remove_by_id(vec);
		}
        active_entities.erase(entity_id); // 生存リストから削除
        free_ids.push_back(entity_id);
    }
    // エディタ（ImGuiなど）から「一覧を取得する」ための関数
    const std::unordered_set<uint32_t>& get_all_entities() const {
        return active_entities;
    }

    void newcom_Pos(uint32_t Entity,SDL_Point p) {
        Pos.push_back({ Entity, double(p.x), double(p.y) });
    }
    void newcon_Render(uint32_t Entity, double w, double h) {
        Render_Body.push_back({Entity, w, h});
    }
	void newcon_Name(uint32_t Entity, std::string name) {
		Name_Body.push_back({ Entity, name });
	}
	void newcon_MapObj(uint32_t Entity, double x, double y, double size) {
		MapObj_Body.push_back({ Entity, x, y, size });
	}

    void update_Renderer(Renderer& ren) {
        SDL_Rect te;
        SDL_Rect Po;
		SDL_Texture* Tgt = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_TARGET,
            1000 / 5, 625 / 5
        );
        SDL_SetTextureScaleMode(Tgt, SDL_ScaleModeNearest);
        if (loaded_map && tileset) {
			SDL_SetRenderTarget(renderer, Tgt);
			SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
			SDL_RenderClear(renderer);
			map.render(renderer, *tileset, static_cast<float>(Camera.x), static_cast<float>(Camera.y), Viewport);
			for (auto& m : MapObj_Body) {
				te = { static_cast<int>(m.x) - Camera.x, 
                    static_cast<int>(m.y) - Camera.y,
                    static_cast<int>(m.size), static_cast<int>(m.size) };
				SDL_SetRenderDrawColor(ren.ren, 255, 255, 0, 255);
				SDL_RenderFillRect(ren.ren, &te);
			}
			SDL_SetRenderTarget(renderer, nullptr);
			SDL_RenderCopy(renderer, Tgt, nullptr, &Viewport);
        }

        for (auto& r : Render_Body) {
            for (auto& p : Pos) {
                if (r.Entity_ID == p.Entity_ID) {
                    te = { static_cast<int>(p.x) - static_cast<int>(r.w) / 2, static_cast<int>(p.y) - static_cast<int>(r.h) / 2, static_cast<int>(r.w), static_cast<int>(r.h) };
                    Po = { static_cast<int>(p.x) - 1, static_cast<int>(p.y) - 1, 3, 3 };
                    SDL_SetRenderDrawColor(ren.ren, 255, 0, 0, 255);
                    SDL_RenderFillRect(ren.ren, &te);
                    SDL_SetRenderDrawColor(ren.ren, 0, 255, 0, 255);
					SDL_RenderFillRect(ren.ren, &Po);
                }
            }
        }
		for (auto& sr : sprite_render_requests) {
			auto it = Sprite_Textures.find(sr.sprite_name);
			if (it != Sprite_Textures.end()) {
				SDL_RenderCopy(ren.ren, it->second, sr.use_src ? &sr.src : nullptr, &sr.dst);
			}
		}
		SDL_DestroyTexture(Tgt);
    }

};