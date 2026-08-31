// LuaLexer.cpp
#include "LuaLexer.h"
#include <unordered_set>
#include <cctype>

namespace lualex {

namespace {

// UTF-8 マルチバイト文字の後続/先頭バイトかどうか。
// UTF-8 では継続バイト・先頭バイトは常に 0x80 以上、
// Lua の記号・演算子・キーワードは全て ASCII (0x7F 以下) なので、
// 「0x80 以上のバイトは常に識別子/文字列/コメントの中身の一部として扱う」
// というルールだけで、マルチバイト文字を分断せずに安全に読み飛ばせる。
inline bool isHighByte(unsigned char c) {
    return c >= 0x80;
}

inline bool isIdentStart(unsigned char c) {
    return std::isalpha(c) || c == '_' || isHighByte(c);
}

inline bool isIdentContinue(unsigned char c) {
    return std::isalnum(c) || c == '_' || isHighByte(c);
}

inline bool isDigitByte(unsigned char c) {
    return c >= '0' && c <= '9';
}

const std::unordered_set<std::string>& keywordSet() {
    static const std::unordered_set<std::string> kw = {
        "and", "break", "do", "else", "elseif", "end", "false", "for",
        "function", "goto", "if", "in", "local", "nil", "not", "or",
        "repeat", "return", "then", "true", "until", "while"
    };
    return kw;
}

} // namespace

LuaLexer::LuaLexer(const std::string& source) : src_(source) {
    tokenize();
    classifyFunctionCalls();
    collectFunctionDefinitions();
}

// "[" の位置から始まる長括弧 [=*[ を判定する。
// pos は '[' を指しているとする。マッチすれば '=' の個数(level)を返し、
// posAfterOpen に開き括弧の直後の位置を書き込む。マッチしなければ -1。
static int matchLongBracketOpen(const std::string& s, size_t pos, size_t& posAfterOpen) {
    size_t p = pos;
    if (p >= s.size() || s[p] != '[') return -1;
    ++p;
    int level = 0;
    while (p < s.size() && s[p] == '=') {
        ++level;
        ++p;
    }
    if (p < s.size() && s[p] == '[') {
        posAfterOpen = p + 1;
        return level;
    }
    return -1;
}

void LuaLexer::tokenize() {
    tokens_.clear();
    lineToTokenIdx_.clear();
    lineStartIndex_.clear();

    const std::string& s = src_;
    size_t n = s.size();
    size_t pos = 0;
    int line = 0;
    int col = 0;

    lineStartIndex_.push_back(0);
    lineToTokenIdx_.emplace_back(); // 行0用

    auto ensureLine = [&](int ln) {
        while (static_cast<int>(lineToTokenIdx_.size()) <= ln) {
            lineToTokenIdx_.emplace_back();
            lineStartIndex_.push_back(pos);
        }
    };

    auto pushToken = [&](TokenType type, size_t start, size_t len, int tline, int tcol) {
        Token t;
        t.type = type;
        t.text = s.substr(start, len);
        t.line = tline;
        t.column = tcol;
        ensureLine(tline);
        lineToTokenIdx_[tline].push_back(static_cast<int>(tokens_.size()));
        tokens_.push_back(std::move(t));
    };

    // 現在位置からの1バイトを読み、必要なら line/col を更新して進める。
    auto advance = [&]() {
        if (pos < n) {
            if (s[pos] == '\n') {
                ++pos;
                ++line;
                col = 0;
                ensureLine(line);
            } else {
                ++pos;
                ++col;
            }
        }
    };

    while (pos < n) {
        unsigned char c = static_cast<unsigned char>(s[pos]);

        // 改行そのものはトークンにしない（行の区切りとして消費するだけ）
        if (c == '\n') {
            advance();
            continue;
        }

        int startLine = line;
        int startCol = col;
        size_t startPos = pos;

        // 空白類（スペース・タブ・CR）はまとめて1トークンにして返す。
        // これを捨てずに返すことで、texts を連結すれば元の行テキストと
        // 一致するようになり、レンダリング側で桁位置がズレなくなる。
        if (c == ' ' || c == '\t' || c == '\r') {
            while (pos < n) {
                unsigned char cc = static_cast<unsigned char>(s[pos]);
                if (cc == ' ' || cc == '\t' || cc == '\r') {
                    advance();
                } else {
                    break;
                }
            }
            pushToken(TokenType::Whitespace, startPos, pos - startPos, startLine, startCol);
            continue;
        }

        // コメント
        if (c == '-' && pos + 1 < n && s[pos + 1] == '-') {
            size_t p = pos + 2;
            size_t afterOpen;
            int level = -1;
            if (p < n && s[p] == '[') {
                level = matchLongBracketOpen(s, p, afterOpen);
            }
            if (level >= 0) {
                // 長コメント --[[ ... ]] / --[=[ ... ]=] ...
                std::string closer = "]" + std::string(level, '=') + "]";
                size_t searchPos = afterOpen;
                size_t closeAt = s.find(closer, searchPos);
                size_t endPos = (closeAt == std::string::npos) ? n : closeAt + closer.size();
                while (pos < endPos) advance();
                pushToken(TokenType::Comment, startPos, pos - startPos, startLine, startCol);
            } else {
                // 行コメント -- ...
                while (pos < n && s[pos] != '\n') advance();
                pushToken(TokenType::Comment, startPos, pos - startPos, startLine, startCol);
            }
            continue;
        }

        // 長文字列 [[ ... ]] / [=[ ... ]=]
        if (c == '[') {
            size_t afterOpen;
            int level = matchLongBracketOpen(s, pos, afterOpen);
            if (level >= 0) {
                std::string closer = "]" + std::string(level, '=') + "]";
                size_t closeAt = s.find(closer, afterOpen);
                size_t endPos = (closeAt == std::string::npos) ? n : closeAt + closer.size();
                while (pos < endPos) advance();
                pushToken(TokenType::String, startPos, pos - startPos, startLine, startCol);
                continue;
            }
            // 単なる '[' シンボルとして下に流す
        }

        // 短い文字列 "..." '...'
        if (c == '"' || c == '\'') {
            char quote = static_cast<char>(c);
            advance(); // 開き引用符
            while (pos < n) {
                unsigned char cc = static_cast<unsigned char>(s[pos]);
                if (cc == '\\') {
                    advance(); // バックスラッシュ
                    if (pos < n) advance(); // エスケープされた1バイト（マルチバイトの先頭でも1バイトだけ進めれば十分安全）
                    continue;
                }
                if (cc == static_cast<unsigned char>(quote)) {
                    advance(); // 閉じ引用符
                    break;
                }
                if (cc == '\n') {
                    // 本来 Lua ではエラーだが、壊れず先に進めるためここで文字列を打ち切る
                    break;
                }
                advance();
            }
            pushToken(TokenType::String, startPos, pos - startPos, startLine, startCol);
            continue;
        }

        // 数値
        if (isDigitByte(c) || (c == '.' && pos + 1 < n && isDigitByte(static_cast<unsigned char>(s[pos + 1])))) {
            bool isHex = false;
            if (c == '0' && pos + 1 < n && (s[pos + 1] == 'x' || s[pos + 1] == 'X')) {
                isHex = true;
                advance();
                advance();
            }
            auto isHexDigit = [](unsigned char ch) {
                return isDigitByte(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
            };
            while (pos < n) {
                unsigned char cc = static_cast<unsigned char>(s[pos]);
                if (isHex ? isHexDigit(cc) : isDigitByte(cc)) {
                    advance();
                } else if (cc == '.') {
                    advance();
                } else if (!isHex && (cc == 'e' || cc == 'E')) {
                    advance();
                    if (pos < n && (s[pos] == '+' || s[pos] == '-')) advance();
                } else if (isHex && (cc == 'p' || cc == 'P')) {
                    advance();
                    if (pos < n && (s[pos] == '+' || s[pos] == '-')) advance();
                } else {
                    break;
                }
            }
            pushToken(TokenType::Number, startPos, pos - startPos, startLine, startCol);
            continue;
        }

        // 識別子 / キーワード
        if (isIdentStart(c)) {
            while (pos < n && isIdentContinue(static_cast<unsigned char>(s[pos]))) advance();
            std::string word = s.substr(startPos, pos - startPos);
            TokenType type = keywordSet().count(word) ? TokenType::Keyword : TokenType::Identifier;
            pushToken(type, startPos, pos - startPos, startLine, startCol);
            continue;
        }

        // 複数文字演算子
        static const char* multiOps[] = {
            "...", "..", "::", "==", "~=", "<=", ">=", "//", "<<", ">>"
        };
        bool matchedMulti = false;
        for (const char* op : multiOps) {
            size_t len = std::string(op).size();
            if (pos + len <= n && s.compare(pos, len, op) == 0) {
                for (size_t i = 0; i < len; ++i) advance();
                pushToken(TokenType::Operator, startPos, pos - startPos, startLine, startCol);
                matchedMulti = true;
                break;
            }
        }
        if (matchedMulti) continue;

        // 1文字の演算子・記号
        static const std::string opChars = "+-*/%^#&~|<>=";
        static const std::string symChars = "(){}[];:,.";
        if (opChars.find(static_cast<char>(c)) != std::string::npos) {
            advance();
            pushToken(TokenType::Operator, startPos, pos - startPos, startLine, startCol);
            continue;
        }
        if (symChars.find(static_cast<char>(c)) != std::string::npos) {
            advance();
            pushToken(TokenType::Symbol, startPos, pos - startPos, startLine, startCol);
            continue;
        }

        // それ以外（未対応/不正バイト、あるいは単独の高位バイトなど）は
        // Unknown として1バイトずつ安全に消費し、絶対にクラッシュさせない。
        advance();
        pushToken(TokenType::Unknown, startPos, pos - startPos, startLine, startCol);
    }
}

void LuaLexer::classifyFunctionCalls() {
    // 識別子の直後（空白を挟んでもよい）に '(' が続く場合、
    // その識別子を「関数」として色分けする。
    // これは `function foo(...)` の foo、`example(x)` の example、
    // `obj.method(...)` の method のいずれもカバーする。
    for (size_t i = 0; i < tokens_.size(); ++i) {
        if (tokens_[i].type != TokenType::Identifier) continue;
        size_t j = i + 1;
        while (j < tokens_.size() && tokens_[j].type == TokenType::Whitespace) ++j;
        if (j < tokens_.size() && tokens_[j].type == TokenType::Symbol && tokens_[j].text == "(") {
            tokens_[i].type = TokenType::Function;
        }
    }
}

void LuaLexer::collectFunctionDefinitions() {
    functions_.clear();
    functionNames_.clear();

    // 空白・コメントを飛ばして、idx 以降で最初に意味のあるトークンの
    // インデックスを返す（idx 自身が意味のあるトークンならそのまま idx）。
    auto nextSignificant = [&](size_t idx) -> size_t {
        while (idx < tokens_.size() &&
               (tokens_[idx].type == TokenType::Whitespace ||
                tokens_[idx].type == TokenType::Comment)) {
            ++idx;
        }
        return idx;
    };

    for (size_t i = 0; i < tokens_.size(); ++i) {
        if (tokens_[i].type != TokenType::Keyword || tokens_[i].text != "function") continue;

        // "function" の直前（空白・コメントを除く）が "local" かどうか
        bool isLocal = false;
        {
            size_t p = i;
            while (p > 0) {
                --p;
                if (tokens_[p].type == TokenType::Whitespace || tokens_[p].type == TokenType::Comment) continue;
                if (tokens_[p].type == TokenType::Keyword && tokens_[p].text == "local") isLocal = true;
                break;
            }
        }

        int defLine = tokens_[i].line;
        int defCol = tokens_[i].column;

        size_t j = nextSignificant(i + 1);

        // 無名関数（例: local f = function(x) ... end）には名前が無いので対象外
        if (j >= tokens_.size() ||
            !(tokens_[j].type == TokenType::Identifier || tokens_[j].type == TokenType::Function)) {
            continue;
        }

        // 名前チェーンを読む: name ('.' name)* (':' name)?
        std::string fullName;
        std::string lastName;
        bool isMethod = false;
        while (j < tokens_.size() &&
               (tokens_[j].type == TokenType::Identifier || tokens_[j].type == TokenType::Function)) {
            lastName = tokens_[j].text;
            fullName += lastName;
            j = nextSignificant(j + 1);
            if (j < tokens_.size() && tokens_[j].type == TokenType::Symbol && tokens_[j].text == ".") {
                fullName += ".";
                j = nextSignificant(j + 1);
                continue;
            }
            if (j < tokens_.size() && tokens_[j].type == TokenType::Symbol && tokens_[j].text == ":") {
                fullName += ":";
                isMethod = true;
                j = nextSignificant(j + 1);
                continue;
            }
            break;
        }

        // 名前の直後は '(' のはず。そうでなければ想定外の構文なので諦める。
        if (j >= tokens_.size() || !(tokens_[j].type == TokenType::Symbol && tokens_[j].text == "(")) {
            continue;
        }
        j = nextSignificant(j + 1);

        std::vector<std::string> params;
        if (isMethod) {
            params.push_back("self"); // ':' 記法は暗黙的に self が第一引数になる
        }
        while (j < tokens_.size() && !(tokens_[j].type == TokenType::Symbol && tokens_[j].text == ")")) {
            if (tokens_[j].type == TokenType::Identifier) {
                params.push_back(tokens_[j].text);
            } else if (tokens_[j].type == TokenType::Operator && tokens_[j].text == "...") {
                params.push_back("...");
            }
            // ',' や想定外のトークンはそのまま読み飛ばす
            j = nextSignificant(j + 1);
        }

        FunctionDefinition def;
        def.name = lastName;
        def.fullName = fullName;
        def.params = std::move(params);
        def.line = defLine;
        def.column = defCol;
        def.isLocal = isLocal;
        def.isMethod = isMethod;

        functionNames_.insert(def.name);
        functions_.push_back(std::move(def));
    }
}

std::vector<Token> LuaLexer::getLineTokensRaw(int lineNumber) const {
    std::vector<Token> result;
    if (lineNumber < 0 || lineNumber >= static_cast<int>(lineToTokenIdx_.size())) {
        return result;
    }
    result.reserve(lineToTokenIdx_[lineNumber].size());
    for (int idx : lineToTokenIdx_[lineNumber]) {
        result.push_back(tokens_[idx]);
    }
    return result;
}

std::pair<std::vector<std::string>, std::vector<SDL_Color>>
LuaLexer::getLineTokens(int lineNumber) const {
    std::vector<std::string> texts;
    std::vector<SDL_Color> colors;
    if (lineNumber < 0 || lineNumber >= static_cast<int>(lineToTokenIdx_.size())) {
        return { texts, colors };
    }
    const auto& idxs = lineToTokenIdx_[lineNumber];
    texts.reserve(idxs.size());
    colors.reserve(idxs.size());
    for (int idx : idxs) {
        const Token& t = tokens_[idx];
        texts.push_back(t.text);
        colors.push_back(colorForType(t.type));
    }
    return { texts, colors };
}

std::vector<ColoredToken> LuaLexer::getLineColoredTokens(int lineNumber) const {
    std::vector<ColoredToken> result;
    if (lineNumber < 0 || lineNumber >= static_cast<int>(lineToTokenIdx_.size())) {
        return result;
    }
    const auto& idxs = lineToTokenIdx_[lineNumber];
    result.reserve(idxs.size());
    for (int idx : idxs) {
        const Token& t = tokens_[idx];
        result.push_back(ColoredToken{ t.text, colorForType(t.type) });
    }
    return result;
}

std::tuple<std::vector<std::string>, std::vector<SDL_Color>, std::vector<TokenType>>
LuaLexer::getLineTokensWithTypes(int lineNumber) const {
    std::vector<std::string> texts;
    std::vector<SDL_Color> colors;
    std::vector<TokenType> types;
    if (lineNumber < 0 || lineNumber >= static_cast<int>(lineToTokenIdx_.size())) {
        return { texts, colors, types };
    }
    const auto& idxs = lineToTokenIdx_[lineNumber];
    texts.reserve(idxs.size());
    colors.reserve(idxs.size());
    types.reserve(idxs.size());
    for (int idx : idxs) {
        const Token& t = tokens_[idx];
        texts.push_back(t.text);
        colors.push_back(colorForType(t.type));
        types.push_back(t.type);
    }
    return { texts, colors, types };
}

const char* LuaLexer::toString(TokenType type) {
    switch (type) {
        case TokenType::Keyword:    return "Keyword";
        case TokenType::Identifier: return "Identifier";
        case TokenType::Function:   return "Function";
        case TokenType::Number:     return "Number";
        case TokenType::String:     return "String";
        case TokenType::Comment:    return "Comment";
        case TokenType::Operator:   return "Operator";
        case TokenType::Symbol:     return "Symbol";
        case TokenType::Whitespace: return "Whitespace";
        case TokenType::EndOfLine:  return "EndOfLine";
        case TokenType::Unknown:    return "Unknown";
        case TokenType::EndOfFile:  return "EndOfFile";
        default:                    return "?";
    }
}

SDL_Color LuaLexer::colorForType(TokenType type) {
    switch (type) {
        case TokenType::Keyword:    return SDL_Color{ 86, 156, 214, 255 };  // 青系
        case TokenType::Identifier: return SDL_Color{ 220, 220, 220, 255 }; // 明灰色
        case TokenType::Function:   return SDL_Color{ 166, 226, 46, 255 }; // 緑系（関数名）
        case TokenType::Number:     return SDL_Color{ 181, 206, 168, 255 }; // 黄緑
        case TokenType::String:     return SDL_Color{ 206, 145, 120, 255 }; // オレンジ系
        case TokenType::Comment:    return SDL_Color{ 106, 153, 85, 255 };  // 緑
        case TokenType::Operator:   return SDL_Color{ 212, 212, 212, 255 }; // 灰白
        case TokenType::Symbol:     return SDL_Color{ 200, 200, 200, 255 }; // 灰白
        case TokenType::Whitespace: return SDL_Color{ 255, 255, 255, 0 };   // 透明（グリフを描かず幅だけ使う想定）
        case TokenType::Unknown:    return SDL_Color{ 255, 0, 0, 255 };     // 赤（要注意表示）
        default:                    return SDL_Color{ 255, 255, 255, 255 };
    }
}

} // namespace lualex
