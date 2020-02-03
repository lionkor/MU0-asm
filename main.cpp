#include "String.h"
#include <iostream>
#include <string>
#include <iomanip>
#include <fstream>
#include <vector>
#include <sstream>
#include <algorithm>
#include <map>

// some ansi color codes
//  (these should work in pretty much any terminal)
#define ansi_reset "\u001b[0m"
#define ansi_red "\u001b[31m"
// error macro to report errors, using std::dec to avoid printing line in hex
//  since for some reason that can happen when x specifies it
#define error(x) std::cerr << __FUNCTION__ << ":" << std::dec << __LINE__ << ": " \
                           << ansi_red << "error: " << ansi_reset << x << std::endl
#define fatal(x) std::cerr << __FUNCTION__ << ":" << std::dec << __LINE__ << ": " \
                           << ansi_red << "fatal: " << ansi_reset << x << std::endl
#define log(x) std::cout << __FUNCTION__ << ":" << std::dec << __LINE__ << ": log: " \
                         << x << std::endl
// print error x and return ret, in a weird loop to prevent Wextra-semi or whatever
#define error_ret(x, ret) \
    do {                  \
        error(x);         \
        return ret;       \
    } while (false)
// macro to turn an expression into a string, for debugging purposes
#define as_string(x) #x

struct instruction_t {
    unsigned int S : 12;
    unsigned int opcode : 4;
};

static std::vector<instruction_t>           instrs;
static std::map<std::string, std::uint16_t> name_address_map;
static std::map<std::string, std::uint16_t> label_address_map;
static std::uint16_t                        last_calling_location;

template<typename _T>
_T abs(const _T& i) {
    return i < 0 ? -i : i;
}

enum Instr : std::uint8_t
{
    LDA = 0b0000,
    STO = 0b0001,
    ADD = 0b0010,
    SUB = 0b0011,
    JMP = 0b0100,
    JGE = 0b0101,
    JNE = 0b0110,
    STP = 0b0111,
    END_STD_INSTR_SET, // less than this means it is a standard instruction
    // extended instr set
    CALL,
    RET,
    // misc
    DATA,
    INVALID,
};


static const std::map<std::string, Instr> g_name_instr_map = {
    { "lda", Instr::LDA },
    { "sto", Instr::STO },
    { "add", Instr::ADD },
    { "sub", Instr::SUB },
    { "jmp", Instr::JMP },
    { "jge", Instr::JGE },
    { "jne", Instr::JNE },
    { "stp", Instr::STP },
    { "call", Instr::CALL },
    { "ret", Instr::RET },
    { "d", Instr::DATA },
};

struct instr_arg_pair_t {
    Instr       instr;
    std::string arg;
};

static inline Instr instr_from_name(const std::string& name) {
    // convert to lowercase
    std::string lower = name;
    for (char& c : lower) {
        c = std::tolower(c);
    }
    if (g_name_instr_map.find(lower) == g_name_instr_map.end()) {
        error("unknown instruction '" << name << "'");
        return Instr::INVALID;
    }
    return g_name_instr_map.at(lower);
}

static inline std::string name_from_instr(Instr i) {
    auto iter = std::find_if(g_name_instr_map.begin(), g_name_instr_map.end(), [&](const auto& pair) -> bool {
        return pair.second == i;
    });
    if (iter == g_name_instr_map.end()) {
        return "(unknown instruction)";
    }
    return iter->first;
}

static std::string& trim_whitespace(std::string& s) {
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
    log("(verbose) assuming that instr '" << name_from_instr(i) << "' expects no args");
    return false;
}

class Parser
{
public:
    Parser(const std::string& filename) {
        std::ifstream file(filename, std::ios::in);
        if (!file.is_open()) {
            error("file '" << filename << "' not found");
            m_invalid = true;
            return;
        }

        std::size_t line = 1;
        do {
            std::string s;
            std::getline(file, s);
            // trim comments
            s.erase(std::find(s.begin(), s.end(), '#'), s.end());
            trim_whitespace(s);
            // ignore now-empty lines
            if (s.empty()) {
                ++line;
                continue;
            }
            // split instr and arg
            Instr       instr = instr_from_name(std::string(std::begin(s), std::find(s.begin(), s.end(), ' ')));
            std::string arg   = "";
            if (instr_expects_arg(instr)) {
                if (std::find(s.begin(), s.end(), ' ') == s.end()) {
                    error("source line " << line << ": argument expected for '" << s << "'");
                    m_invalid = true;
                    continue;
                }
                arg = s.substr(s.find_first_of(' ') + 1);
            } else {
                // ensure that there was no argument given to an instruction that does not expect one,
                //  since this should never happen as this means that wrong assumptions have been made
                if (std::find(s.begin(), s.end(), ' ') != s.end()) { // if there is a space
                    auto trimmed = s.substr(s.find_first_of(' '));
                    trim_whitespace(trimmed);
                    if (!trimmed.empty()) { // if there are characters in the argument except whitespace (which has been trimmed)
                        error("source line " << line << ": argument '" << trimmed << "' supplied to '" << name_from_instr(instr) << "' which does not expect an argument");
                        m_invalid = true;
                        continue;
                    }
                }
            }
            if (instr == Instr::INVALID) {
                error("source line " << line << ": parsed instruction is unknown / invalid");
                m_invalid = true;
                continue;
            }
            instr_arg_pair_t pair { instr, arg };
            log("source line " << line << ": parsed instr of pair: " << name_from_instr(instr) << " " << arg);
            m_instr_arg_pairs.push_back(pair);
            ++line;
        } while (!file.eof());
    }

    bool invalid() const {
        return m_invalid;
    }

protected:
    // flag is set when an error occurs
    bool                                 m_invalid = false;
    std::map<std::string, std::uint16_t> m_name_address_map;
    std::map<std::string, std::uint16_t> m_label_address_map;
    std::vector<instr_arg_pair_t>        m_instr_arg_pairs;
};

static unsigned parse_opcode(const String& str, const std::vector<String>& lines, std::size_t index) {
    if (str == "lda") {
        return 0b0000;
    } else if (str == "sto") {
        return 0b0001;
    } else if (str == "add") {
        return 0b0010;
    } else if (str == "sub") {
        return 0b0011;
    } else if (str == "jmp") {
        return 0b0100;
    } else if (str == "jge") {
        return 0b0101;
    } else if (str == "jne") {
        return 0b0110;
    } else if (str == "stp") {
        return 0b0111; // STP
    } else {
        std::cout << "UNKNOWN INSTRUCTION: '" << str << "'" << std::endl;
        return 0b1111;
    }
}

static unsigned parse_S(const String& str, const std::vector<String>& lines, std::size_t index) {
    if (!str.startswith("0x") && !str.startswith(">")
        && (lines.at(index).to_lower().startswith("jmp")
            || lines.at(index).to_lower().startswith("jne")
            || lines.at(index).to_lower().startswith("jge"))) {
        std::stringstream ss;
        ss << str.c_str();
        int i;
        ss >> i;
        auto jmp_to = instrs.size() + i;
        std::cout << "relative jump parsed: jump to " << std::hex << jmp_to << std::endl;
        return jmp_to;
    } else if (!str.startswith("0x") && str.startswith(">")) { // this is REALLY shitty FIXME
        auto label_addr = (*label_address_map.find(std::string(str.c_str() + 1))).second;
        std::cout << "parse_S label: " << label_addr << std::endl;
        return label_addr;
    } else if (!str.startswith("0x")) { // this is even worse FIXME
        // must be a name
        auto name_addr = (*name_address_map.find(String::to_std_string(str))).second;
        std::cout << "parse_S var: " << name_addr << std::endl;
        return name_addr;
    }
    std::stringstream ss;
    ss << std::hex << str.split('x')[1].c_str();
    unsigned int i;
    ss >> i;
    std::cout << "parse_S hex:" << i << std::endl;
    return i;
}

static void parse_data_segment(const String& line, const std::vector<String>& lines, std::size_t index) {
    // d test = 0x5
    String s = line.substring(line.begin() + 2, line.end());
    std::cout << "data s: " << s << std::endl;
    // test = 0x5
    s = s.replaced(" ", "");
    std::cout << "data s: " << s << std::endl;
    // test=0x5
    // [0]  [1]
    std::uint16_t i = parse_S(s.split('=')[1], lines, index);
    std::cout << "data hex i: " << std::hex << i << std::endl;
    instrs.push_back(*reinterpret_cast<instruction_t*>(&i));
    name_address_map.insert_or_assign(String::to_std_string(s.split('=')[0]), instrs.size() - 1);
}

// we parse as non-const-reference so we can trim out comments
static void parse_line(std::vector<String>& lines, std::size_t index) {
    instruction_t i { 0, 0 };

    // skip full line comments
    if (lines.at(index).trimmed().startswith("#")) {
        return;
    }

    // skip labels since we already parsed them
    if (lines.at(index).trimmed().startswith(">")) {
        return;
    }

    // trim out comments
    if (lines.at(index).find('#') != lines.at(index).end()) {
        // we found a comment start somewhere in the line
        // so we just split it there and ignore the right side
        lines.at(index) = lines.at(index).split('#')[0];
    }

    if (lines.at(index).startswith("d ")) {
        parse_data_segment(lines.at(index).trimmed(), lines, index);
        return;
    }

    auto splits = lines.at(index).trimmed().split(' ');
    std::cout << "current line: " << lines.at(index) << std::endl;
    i.opcode = parse_opcode(splits.at(0).trimmed().to_lower(), lines, index);
    std::cout << "parsed opcode: 0x"
              << i.opcode << std::endl;
    if (splits.size() >= 2)
        i.S = parse_S(splits.at(1).trimmed(), lines, index);
    else
        i.S = 0;
    std::cout << "full parsed instruction: 0x" << i.opcode
              << std::hex << std::setfill('0') << std::setw(3)
              << i.S << std::endl;
    instrs.push_back(i);
}

static std::size_t count_instructions_until_index(const std::vector<String>& lines, std::size_t index) {
    std::size_t instr_count = 0;
    for (std::size_t i = 0; i < index; ++i) {
        String str = lines.at(i);
        if (!str.trimmed().empty()
            && !str.startswith("#")
            && !str.startswith(">")) {
            ++instr_count;
        }
    }
    return instr_count;
}

static std::size_t count_indices_until_instruction_number(const std::vector<String>& lines, std::size_t instr_count) {
    std::size_t i              = 0;
    std::size_t my_instr_count = 0;
    for (; my_instr_count < instr_count && instr_count < lines.size() && i < instr_count; ++i) {
        String str = lines.at(i);
        if (!str.trimmed().empty()
            && !str.startswith("#")
            && !str.startswith(">")) {
            ++my_instr_count;
        }
    }
    return i;
}

static void resolve_subroutines(std::vector<String>& lines, std::size_t start_index) {
    std::size_t instr_count = 0;
    for (std::size_t i = start_index; i < lines.size(); ++i) {
        String& str = lines.at(i);
        if (!str.trimmed().empty()
            && !str.startswith("#")
            && !str.startswith(">")) {
            ++instr_count;
        }
        if (str.to_lower().startswith("call")) {
            auto label    = str.split(' ').at(1);
            auto location = label_address_map.at(std::string(label.c_str() + 1));

            std::stringstream ss;
            ss << location;
            std::string s;
            ss >> std::hex >> s;
            std::cout << "found subroutine call: '" << str;
            str = String((std::string("JMP 0x") + s).c_str());
            std::cout << "', replaced with '" << str << "'" << std::endl;
            last_calling_location = instr_count;
            // find next RET
            for (std::size_t k = count_indices_until_instruction_number(lines, location); k < lines.size(); ++k) {
                String& str = lines.at(k);
                std::cout << "current line: " << str << std::endl;
                if (str.to_lower().startswith("ret")) {
                    std::cout << "found subroutine ret: '" << str;
                    str = String::format("JMP 0x", HexFormat<std::uint16_t>(last_calling_location));
                    std::cout << "', replaced with '" << str << "'" << std::endl;
                    break;
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    if (argc == 1) {
        return -1;
    }

    Parser parser(argv[1]);
    if (parser.invalid()) {
        fatal("errors occurred during initial parsing.");
        return -1;
    }

    return 0;

    String filename(argv[1]);

    std::ifstream file(filename.c_str());

    std::vector<String> lines;

    do {
        std::string s;
        std::getline(file, s);
        String current_line = s.c_str();
        if (!current_line.trimmed().empty()) {
            lines.push_back(current_line.trimmed());
        }
    } while (!file.eof());

    file.close();

    // parse labels first
    std::size_t instr_count { 0 };
    for (std::size_t i = 0; i < lines.size(); ++i) {
        auto str = lines.at(i).trimmed();
        // FIXME: this will fall apart if we add more special line types
        if (!str.empty()
            && !str.startswith("#")
            && !str.startswith(">")) {
            ++instr_count;
        } else if (str.startswith(">")) {
            std::cout << "found a label: " << str << std::endl;
            label_address_map.insert_or_assign(std::string(str.c_str() + 1), instr_count);
            std::cout << " -> location: " << std::hex << instr_count << std::endl;
        }
    }

    // parse calls & returns
    //  we basically just fake subroutines by jumping around
    resolve_subroutines(lines, 0);

    // parse instructions and compile
    for (std::size_t i = 0; i < lines.size(); ++i) {
        parse_line(lines, i);
    }

    FILE* fp = fopen("a.out", "wb");

    for (const auto& i : instrs) {
        std::cout << "writing: 0x" << std::hex << *reinterpret_cast<const std::uint16_t*>(&i) << std::endl;
        fwrite(&i, sizeof(std::uint16_t), 1, fp);
    }

    fclose(fp);

    return 0;
}
