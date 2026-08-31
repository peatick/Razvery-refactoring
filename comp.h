#pragma once
#include "sol2/sol.hpp"
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

class scene {
private:
    uint32_t next_entity_id = 1;
    std::vector<uint32_t> free_ids;

    // 生きているEntityのIDを一覧で保持（エディタのヒエラルキー表示用）
    std::unordered_set<uint32_t> active_entities;

public:
    // 各コンポーネントのストレージ
    std::vector<pos> Pos;
    std::vector<Render_body> Render_Body;
    std::vector<Name> Name_Body;
	std::unordered_map<std::string, std::vector<LuaCom>> LuaCom_Body;
    void init(sol::state& lua) {
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
		lua["create_entity"] = [this]() {
			return this->create_entity();
			}; 
        lua["add_newCom"] = [this](uint32_t entity_id, sol::table data, std::string com_type) {
            if (com_type == "Render_body") {
                Render_Body.push_back({ entity_id, data["w"], data["h"] });
            }
            else if (com_type == "pos") {
                Pos.push_back({ entity_id, data["x"], data["y"] });
            }
            else if (com_type == "Name") {
                Name_Body.push_back({ entity_id, data["name"] });
            }
            };
        lua["add_newLuaCom"] = [this](uint32_t entity_id, sol::table data, std::string com_type) {
			if (LuaCom_Body.find(com_type) == LuaCom_Body.end()) {
				LuaCom_Body[com_type] = std::vector<LuaCom>();
			}
            LuaCom_Body[com_type].push_back({ entity_id, data });
            };
		lua["get_all_entities"] = [this]() {
			return this->get_all_entities();
			}; 
		lua["get_lua_com"] = [this](uint32_t entity_id, std::string com_type) -> sol::table {
			if (LuaCom_Body.find(com_type) != LuaCom_Body.end()) {
				for (const auto& lua_com : LuaCom_Body[com_type]) {
					if (lua_com.Entity_ID == entity_id) {
						return lua_com.data;
					}
				}
			}
			return sol::nil; // 見つからなかった場合はnilを返す
			};
		lua["remove_entity"] = [this](uint32_t entity_id) {
			this->destroy_entity(entity_id);
			};
        lua["get_all_any_com"] = [this, &lua]() {
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
			return all_lua_com;
            };
        lua.script(R"(
            function get_entity_with(...)
                local com_types = {...}
                local match_entities = {}
                local entitys = get_all_entities()
                local all_coms = get_all_any_com()
                for _, entity_id in ipairs(entitys) do
                    local has_all = true
                    local ent_components = {} -- 条件に合うエンティティのコンポーネントをまとめる箱

                    for _, com_type in ipairs(com_types) do
                        local com_table = all_coms[com_type]
            
                        if com_table and com_table[entity_id] then
                            -- 指定されたコンポーネントデータを保持しておく
                            ent_components[com_type] = com_table[entity_id]
                        else
                            has_all = false
                            break
                        end
                    end
                    -- すべてのコンポーネントを持っていたら、データごと格納！
                    if has_all then
                        match_entities[entity_id] = ent_components
                    end
                end
                return match_entities
            end
            function for_each(com_name, callback_func)
	            for ent_id, com_data in pairs(get_entity_with(table.unpack(com_name))) do
		            callback_func(com_data)
	            end
            end
        )");

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

    void update_Renderer(Renderer& ren) {
        SDL_Rect te;
        SDL_Rect Po;
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
    }

};