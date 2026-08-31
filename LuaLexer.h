// LuaLexer.h
//
// シンプルな Lua 用字句解析器（レキサー / トークナイザー）。
//
// 特徴:
//   - 入力は std::string（UTF-8 を想定）。日本語などのマルチバイト文字が
//     文字列リテラルやコメントの中に含まれていてもクラッシュしたり
//     文字境界を壊したりしない（0x80 以上のバイトは「文字の続き」として
//     常に安全にスキップするため）。
//   - ソース全体を一度トークン化し、行番号（0 始まり）ごとに
//     引きやすい形で保持する。
//   - getLineTokens(line) で、その行のトークン文字列列と、
//     シンタックスハイライト用の SDL_Color を1トークンずつ対応させて返す。
//
// SDL2 が使える環境では本物の SDL_Color をそのまま使う。
// SDL2 が無い環境（テストビルドなど）でも困らないように、
// <SDL2/SDL.h> が見つからない場合は同じレイアウトの互換構造体を定義する。

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <utility>
#include <tuple>
#include <unordered_set>
#include "sdlutil.h"

namespace lualex {

// トークンの種類
enum class TokenType {
    Keyword,      // and, function, local, if ... など
    Identifier,   // 変数名など（関数名は Function に分類される）
    Function,     // 直後に '(' が続く識別子（関数呼び出し/定義名）
                  // 例: example(x) の "example"、function foo() の "foo"、
                  //     obj.method() の "method"
    Number,       // 123, 1.5, 0x1F, 1e10 ...
    String,       // "..." '...' [[...]] [=[...]=]
    Comment,      // -- ... / --[[ ... ]]
    Operator,     // + - * / == ~= <= .. etc
    Symbol,       // ( ) { } [ ] , ; : .
    Whitespace,   // スペース・タブ・CR（連続する空白は1トークンにまとめる）
    EndOfLine,    // 改行（内部管理用。通常は返却トークンに含めない）
    Unknown,      // 不正/未対応文字
    EndOfFile
};

// 1つのトークン
struct Token {
    TokenType type;
    std::string text;   // トークンの生テキスト（UTF-8バイト列そのまま）
    int line;            // 開始行（0始まり）
    int column;          // 開始桁（0始まり、バイト単位）
};

// トークン文字列と、それに対応する色をセットで返す
struct ColoredToken {
    std::string text;
    SDL_Color color;
};

// ユーザーが定義した関数の情報（`function foo(...)` 系の宣言から収集）。
struct FunctionDefinition {
    std::string name;                  // 最後のセグメントのみ（例: "method"）
    std::string fullName;              // ドット/コロンを含むフルパス（例: "Obj.method", "Obj:greet"）
    std::vector<std::string> params;   // 引数名（":" 定義なら先頭に "self" が自動で入る。可変長引数は "..."）
    int line;                          // "function" キーワードの行（0始まり）
    int column;                        // "function" キーワードの桁（0始まり、バイト単位）
    bool isLocal;                      // "local function foo(...)" かどうか
    bool isMethod;                     // "obj:method(...)" (コロン記法) かどうか
};

class LuaLexer {
public:
    // Lua ソースコード全体を渡してトークン化する。
    explicit LuaLexer(const std::string& source);

    // 全トークンを取得する。
    const std::vector<Token>& tokens() const { return tokens_; }

    // ソースの行数を取得する。
    int lineCount() const { return static_cast<int>(lineStartIndex_.size()); }

    // 指定行（0始まり）に属するトークンだけを取得する。
    // 範囲外の行番号を渡した場合は空を返す。
    std::vector<Token> getLineTokensRaw(int lineNumber) const;

    // 指定行（0始まり）のトークンを、
    //   - vector<string>  : トークン文字列
    //   - vector<SDL_Color>: 各トークンに対応する色
    // のペアとして返す（同じインデックスで対応）。
    // スペース・タブなどの空白も Whitespace トークンとして含まれるため、
    // texts を順番に連結すると（改行を除いた）元の行テキストに一致する。
    // レンダリング時はこれをそのまま等幅フォントで並べれば桁がズレない。
    std::pair<std::vector<std::string>, std::vector<SDL_Color>>
    getLineTokens(int lineNumber) const;

    // 上と同じ内容を ColoredToken のリストとしてまとめて返す版。
    std::vector<ColoredToken> getLineColoredTokens(int lineNumber) const;

    // getLineTokens に加えて、各トークンの種類(TokenType)も一緒に欲しい場合用。
    //   auto [tokens, colors, types] = L.getLineTokensWithTypes(0);
    // のように使う。色から種類を逆引きするのは配色を変えると壊れるので、
    // 種類そのものが要るときはこちらを使うこと。
    std::tuple<std::vector<std::string>, std::vector<SDL_Color>, std::vector<TokenType>>
    getLineTokensWithTypes(int lineNumber) const;

    // トークン種別からハイライト色を取得する（配色テーマはここで変更可能）。
    static SDL_Color colorForType(TokenType type);

    // TokenType を人間が読める文字列に変換する（デバッグ・ログ用）。
    static const char* toString(TokenType type);

    // ソース中で見つかった「名前付きの」関数定義の一覧。
    //   function foo(a, b) ... end
    //   local function foo(a, b) ... end
    //   function Obj.method(a) ... end
    //   function Obj:method(a) ... end   -- params の先頭に暗黙の "self" が入る
    // 無名関数（例: local f = function(x) ... end）は名前が無いので含まれない。
    const std::vector<FunctionDefinition>& definedFunctions() const { return functions_; }

    // 与えられた名前（最後のセグメント）がユーザー定義関数として
    // 記録されているかどうかを高速に調べる。
    bool isUserDefinedFunctionName(const std::string& name) const {
        return functionNames_.count(name) != 0;
    }

private:
    std::string src_;                       // 元のソース（UTF-8バイト列）
    std::vector<Token> tokens_;              // 全トークン
    std::vector<std::vector<int>> lineToTokenIdx_; // 行 -> tokens_ のインデックス一覧
    std::vector<size_t> lineStartIndex_;     // 各行の開始バイト位置（デバッグ用）
    std::vector<FunctionDefinition> functions_;       // 見つかった名前付き関数定義
    std::unordered_set<std::string> functionNames_;   // functions_ の name の集合（検索用）

    void tokenize();
    void classifyFunctionCalls(); // 識別子のうち直後に '(' が続くものを Function に再分類
    void collectFunctionDefinitions(); // "function ..." 宣言を走査して functions_ を構築
};

} // namespace lualex
