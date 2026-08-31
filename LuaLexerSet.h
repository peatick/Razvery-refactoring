// LuaLexerSet.h
//
// 複数の Lua ソース（例: 複数ファイル、複数バッファ）を
// std::unordered_map<std::string, std::string>（キー -> ソース文字列）でまとめて渡し、
// キーごとに LuaLexer を保持・管理するための薄いラッパー。
//
// 想定用途:
//   - キー = ファイル名 / バッファID など
//   - エディタで複数タブを開いていて、それぞれの行トークンを引きたい
//   - あるファイルで呼ばれている関数が、別ファイルで定義されたものか調べたい
//     （isUserDefinedFunctionName はキーをまたいで検索する）

#pragma once

#include "LuaLexer.h"
#include <unordered_map>
#include <optional>

namespace lualex {

class LuaLexerSet {
public:
    LuaLexerSet() = default;

    // 最初から複数ソースをまとめて渡して構築する。
    //   std::unordered_map<std::string, std::string> sources = {
    //       {"main.lua", "..."},
    //       {"utils.lua", "..."},
    //   };
    //   LuaLexerSet set(sources);
    explicit LuaLexerSet(const std::unordered_map<std::string, std::string>& sources);

    // キーを指定してソースを追加・更新する（既にあれば作り直す）。
    void set(const std::string& key, const std::string& source);

    // 指定キーを削除する。
    void remove(const std::string& key);

    // 指定キーが存在するか。
    bool contains(const std::string& key) const;

    // 登録されている全キー一覧。
    std::vector<std::string> keys() const;

    // 指定キーの LuaLexer を取得する（無ければ nullptr）。
    // 行トークン取得・関数一覧取得など、LuaLexer の全機能はこれ経由で使える。
    const LuaLexer* find(const std::string& key) const;

    // --- よく使う操作のショートカット（キーが無ければ空を返す。例外は投げない） ---

    // 指定キー・行番号（0始まり）のトークン文字列と色を返す。
    std::pair<std::vector<std::string>, std::vector<SDL_Color>>
    getLineTokens(const std::string& key, int lineNumber) const;

    // 上に加えて各トークンの種類(TokenType)も返す。
    std::tuple<std::vector<std::string>, std::vector<SDL_Color>, std::vector<TokenType>>
    getLineTokensWithTypes(const std::string& key, int lineNumber) const;

    // 指定キー内で見つかった名前付き関数定義の一覧。キーが無ければ空。
    std::vector<FunctionDefinition> definedFunctions(const std::string& key) const;

    // 登録されている全キー分の関数定義を、キーごとにまとめて返す。
    std::unordered_map<std::string, std::vector<FunctionDefinition>> allDefinedFunctions() const;

    // 指定した名前の関数が、登録されているどれかのソース内で
    // 定義されているかどうかを調べる（ファイルをまたいだ検索）。
    bool isUserDefinedFunctionName(const std::string& name) const;

    // 指定した名前の関数がどのキー（ファイル）で定義されているかを返す。
    // 見つからなければ std::nullopt。複数ファイルに同名関数がある場合は
    // 最初に見つかったものを返す。
    std::optional<std::string> findDefiningKey(const std::string& name) const;

    void reset();
private:
    std::unordered_map<std::string, LuaLexer> lexers_;
};

} // namespace lualex

struct LuaKey_Lex {
    lualex::LuaLexerSet* LLSet = nullptr;
    const lualex::LuaLexer* LL = nullptr;

    std::string key = "";
    std::unordered_map<std::string, std::string>* srcs = nullptr;
};
