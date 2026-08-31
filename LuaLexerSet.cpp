// LuaLexerSet.cpp
#include "LuaLexerSet.h"

namespace lualex {

LuaLexerSet::LuaLexerSet(const std::unordered_map<std::string, std::string>& sources) {
    for (const auto& [key, source] : sources) {
        set(key, source);
    }
}

void LuaLexerSet::set(const std::string& key, const std::string& source) {
    // LuaLexer はデフォルト構築できないため insert_or_assign を使う
    // （operator[] は default construct + assign が必要になるため使えない）。
    lexers_.insert_or_assign(key, LuaLexer(source));
}

void LuaLexerSet::remove(const std::string& key) {
    lexers_.erase(key);
}

bool LuaLexerSet::contains(const std::string& key) const {
    return lexers_.find(key) != lexers_.end();
}

std::vector<std::string> LuaLexerSet::keys() const {
    std::vector<std::string> result;
    result.reserve(lexers_.size());
    for (const auto& [key, lexer] : lexers_) {
        (void)lexer;
        result.push_back(key);
    }
    return result;
}

const LuaLexer* LuaLexerSet::find(const std::string& key) const {
    auto it = lexers_.find(key);
    if (it == lexers_.end()) return nullptr;
    return &it->second;
}

std::pair<std::vector<std::string>, std::vector<SDL_Color>>
LuaLexerSet::getLineTokens(const std::string& key, int lineNumber) const {
    const LuaLexer* lexer = find(key);
    if (!lexer) return { {}, {} };
    return lexer->getLineTokens(lineNumber);
}

std::tuple<std::vector<std::string>, std::vector<SDL_Color>, std::vector<TokenType>>
LuaLexerSet::getLineTokensWithTypes(const std::string& key, int lineNumber) const {
    const LuaLexer* lexer = find(key);
    if (!lexer) return { {}, {}, {} };
    return lexer->getLineTokensWithTypes(lineNumber);
}

std::vector<FunctionDefinition> LuaLexerSet::definedFunctions(const std::string& key) const {
    const LuaLexer* lexer = find(key);
    if (!lexer) return {};
    return lexer->definedFunctions();
}

std::unordered_map<std::string, std::vector<FunctionDefinition>>
LuaLexerSet::allDefinedFunctions() const {
    std::unordered_map<std::string, std::vector<FunctionDefinition>> result;
    for (const auto& [key, lexer] : lexers_) {
        result[key] = lexer.definedFunctions();
    }
    return result;
}

bool LuaLexerSet::isUserDefinedFunctionName(const std::string& name) const {
    for (const auto& [key, lexer] : lexers_) {
        (void)key;
        if (lexer.isUserDefinedFunctionName(name)) return true;
    }
    return false;
}

void LuaLexerSet::reset() {
    lexers_.clear();
}

std::optional<std::string> LuaLexerSet::findDefiningKey(const std::string& name) const {
    for (const auto& [key, lexer] : lexers_) {
        if (lexer.isUserDefinedFunctionName(name)) return key;
    }
    return std::nullopt;
}

} // namespace lualex
