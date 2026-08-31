#pragma once
#include <fstream>
#include <string>
#include <unordered_map>
#include <iostream>
#include "sdlutil.h"

class Config {
public:
    bool load(const std::string& path) {
        std::ifstream file(str2path(path));
        if (!file) return false;

        std::string line, section;
        while (std::getline(file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;

            // セクション名 [Section]
            if (line.front() == '[' && line.back() == ']') {
                section = line.substr(1, line.size() - 2);
                continue;
            }

            // key=value
            auto pos = line.find('=');
            if (pos == std::string::npos) continue;

            std::string key = trim(line.substr(0, pos));
            std::string value = trim(line.substr(pos + 1));

            data[section + "." + key] = value;
        }
        return true;
    }



    std::string get(const std::string& section, const std::string& key,
        const std::string& def = "") const {
        auto it = data.find(section + "." + key);
        return (it != data.end()) ? it->second : def;
    }

private:
    std::unordered_map<std::string, std::string> data;

    static std::string trim(const std::string& s) {
        const char* ws = " \t\r\n";
        size_t start = s.find_first_not_of(ws);
        size_t end = s.find_last_not_of(ws);
        return (start == std::string::npos) ? "" : s.substr(start, end - start + 1);
    }
};