#ifndef UTILITY_H
#define UTILITY_H

#include <cstring>
#include <algorithm>
#include <sstream>

#include "arch.h"
#include "debug.h"

template<typename _T>
_T abs(const _T& i) {
    return i < 0 ? -i : i;
}

static inline Instr instr_from_name(const std::string& name) {
    // convert to lowercase
    std::string lower = name;
    for (char& c : lower) {
        c = static_cast<char>(std::tolower(c));
    }
    if (name.substr(0, std::strlen(Prefix::LABEL)) == Prefix::LABEL) {
        return Instr::LABEL;
    }
    if (g_name_instr_map.find(lower) == g_name_instr_map.end()) {
        error("unknown instruction '" << name << "'");
        return Instr::INVALID;
    }
    return g_name_instr_map.at(lower);
}

static inline std::string name_from_instr(Instr i) {
    auto iter = std::find_if(g_name_instr_map.begin(), g_name_instr_map.end(),
        [&](const auto& pair) -> bool {
            return pair.second == i;
        });
    if (iter == g_name_instr_map.end()) {
        return "(unknown instruction)";
    }
    return iter->first;
}

static std::string trim_whitespace(const std::string& to_trim) {
    std::string s = to_trim;
    // trim whitespace left
    s = std::string(std::find_if_not(s.begin(), s.end(), [&](const char& c) -> bool {
        return std::isspace(c);
    }),
        s.end());
    // trim whitespace right
    s.erase(std::find_if_not(s.rbegin(), s.rend(), [&](const char& c) -> bool {
        return std::isspace(c);
    }).base(),
        s.end());
    return s;
}

static bool instr_expects_arg(Instr i) {
    // anything up to STP (not including STP) expects an argument
    if (i < STP
        || i == CALL
        || i == DATA)
        return true;
    // anything else doesn't
    //  but to make sure we log those, in case any slip through that shouldn't
    verbose("assuming that instr '" << name_from_instr(i) << "' expects no args");
    return false;
}

static constexpr bool is_standard_instr(const Instr& i) {
    return i < Instr::END_STD_INSTR_SET;
}

static std::string as_hex_string(std::uint16_t i) {
    std::stringstream ss;
    ss << "0x" << std::hex << i;
    return ss.str();
}

#endif // UTILITY_H
