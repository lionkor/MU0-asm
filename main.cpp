#include "String.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>
#include <map>

struct instruction_t {
    unsigned int S : 12;
    unsigned int opcode : 4;
};

static std::vector<instruction_t>           instrs;
static std::map<std::string, std::uint16_t> name_address_map;

unsigned parse_opcode(const String& str) {
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
    } else {
        return 0b0111; // STP
    }
}

unsigned parse_S(const String& str) {
    if (!str.startswith("0x")) {
        // must be a name
        std::cout << "parse_S var: " << (*name_address_map.find(String::to_std_string(str))).second << std::endl;
        return (*name_address_map.find(String::to_std_string(str))).second;
    }
    std::stringstream ss;
    ss << std::hex << str.split('x')[1].c_str();
    unsigned int i;
    ss >> i;
    std::cout << "parse_S hex:" << i << std::endl;
    return i;
}

void parse_data_segment(const String& line) {
    // d test = 0x5
    String s = line.substring(line.begin() + 2, line.end());
    std::cout << "data s: " << s << std::endl;
    // test = 0x5
    s = s.replaced(" ", "");
    std::cout << "data s: " << s << std::endl;
    // test=0x5
    // [0]  [1]
    std::uint16_t i = parse_S(s.split('=')[1]);
    std::cout << "data hex i: " << std::hex << i << std::endl;
    instrs.push_back(*reinterpret_cast<instruction_t*>(&i));
    name_address_map.insert_or_assign(String::to_std_string(s.split('=')[0]), instrs.size() - 1);
}

void parse_line(const String& line) {
    instruction_t i { 0, 0 };

    if (line.startswith("d ")) {
        parse_data_segment(line);
        return;
    }

    auto splits = line.split(' ');
    i.opcode    = parse_opcode(splits[0].trimmed().to_lower());
    i.S         = parse_S(splits[1].trimmed());
    instrs.push_back(i);
}

int main(int argc, char** argv) {
    if (argc == 1) {
        return -1;
    }

    String filename(argv[1]);

    std::ifstream file(filename.c_str());

    do {
        std::string s;
        std::getline(file, s);
        String current_line = s.c_str();
        if (!current_line.empty())
            parse_line(current_line);
    } while (!file.eof());

    file.close();

    FILE* fp = fopen("a.out", "wb");

    for (const auto& i : instrs) {
        std::cout << "writing: 0x" << std::hex << *reinterpret_cast<const std::uint16_t*>(&i) << std::endl;
        fwrite(&i, sizeof(std::uint16_t), 1, fp);
    }

    fclose(fp);

    return 0;
}
