#pragma once
#include "sdlutil.h"
#include "LuaLexerSet.h"
#include <regex>
#include <unordered_set>
#include <fstream>
#include <sstream>
class Editor_syntaxed {
public:
    Editor TextEditor;
    std::string savepath;
    std::string* EditorName;
    bool no_save = true;

    LuaKey_Lex Lua_src_Lex = {nullptr, nullptr, "", nullptr};

    struct Hint_Pl {
        std::string bef;
        std::string fn_name;
        std::string param;
    };

    void reset() {
        Editor News;
        News.set_init(TextEditor.TX_Rect, "", TextEditor.lineH);
        TextEditor = News;
        no_save = false;
    }

    void Open_File(std::string save_path, LuaKey_Lex src = {nullptr,nullptr,"",nullptr}) {
        reset();
        savepath = save_path;
        fs::path sp = str2path(savepath);
        std::ifstream ifs(sp);
        if (!ifs) return;
        std::stringstream ss;
        ss << ifs.rdbuf();
        std::string File_str = ss.str();
        TextEditor.buf.setAllText(File_str);
        no_save = false;
        Lua_src_Lex = src;
    }

    void Src_sync() {
        if (!Lua_src_Lex.srcs) return;
        std::unordered_map<std::string, std::string>& sr = *Lua_src_Lex.srcs;
        sr[Lua_src_Lex.key] = TextEditor.buf.allText();
    }

    void Save_File(std::string save_path) {
        savepath = save_path;
        fs::path sp = str2path(savepath);
        std::ofstream ofs(sp);
        if (!ofs) return;

        ofs << TextEditor.buf.allText();

        no_save = false;
    }

    Hint_Pl tok(const int& ln, const int& cl) {
        // 1. トークン・色の取得
        if (!Lua_src_Lex.LLSet) return { "","","" };
        lualex::LuaLexerSet& LL = *Lua_src_Lex.LLSet;
        const lualex::LuaLexer& L = *LL.find(Lua_src_Lex.key);
        auto [tokens, colors, types] = L.getLineTokensWithTypes(ln);
        // サイズの一致チェック（範囲外アクセスの防止）
        if (tokens.size() != types.size()) {
            return {"","",""};
        }
        std::string str_C;
        std::vector<Hint_Pl> Nest_F;
        for (size_t i = 0; i < tokens.size(); ++i) {
            str_C += tokens[i];
            // カラム位置を超えたら処理中断
            if (utf8_length(str_C) > cl) {
                break;
            }
            // 関数トークンの処理
            if (types[i] == lualex::TokenType::Function) {
                if (LL.isUserDefinedFunctionName(tokens[i])) {
                    Hint_Pl Func_Tips;
                    Func_Tips.bef = str_C;
                    for (const auto& LK : LL.allDefinedFunctions()) {
                        for (const auto& fn : LK.second) {
                            if (tokens[i] == fn.name) {
                                Func_Tips.fn_name = fn.name;
                                Func_Tips.param = "(";

                                // 引数リストの安全な文字列結合
                                for (size_t k = 0; k < fn.params.size(); ++k) {
                                    Func_Tips.param += fn.params[k];
                                    if (k + 1 < fn.params.size()) {
                                        Func_Tips.param += ", ";
                                    }
                                }
                                Func_Tips.param += ")";
                                break; // 該当する関数が見つかったら探索終了
                            }
                        }
                    }
                    if (!Func_Tips.fn_name.empty()) {
                        Nest_F.push_back(Func_Tips);
                    }
                }
            }
            // 2. pop_back の安全な呼出し（アンダーフロー防止）
            if (tokens[i] == ")") {
                if (!Nest_F.empty()) {
                    Nest_F.pop_back();
                }
            }
        }
        // 3. 現在入れ子になっている一番内側の関数シグネチャを返す
        if (!Nest_F.empty()) {
            return Nest_F.back();
        }
        return {"","",""};
    }


};