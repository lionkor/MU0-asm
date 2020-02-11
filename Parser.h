#ifndef PARSER_H
#define PARSER_H

#include <cstdint> // std::uint...
#include <string>  // std::string
#include <vector>  // std::vector
#include "arch.h"

struct DataInfo {
    std::uint16_t address;
    std::uint16_t value;
};

// hardcoded location in memory used for the value of pc
// before a jump-into-subroutine (call). the location
// remains usable as always if no calls are used.
#define SUBR_PC_LOC "0xfff"
// hardcoded location to hold the ACC contents while it's
// being used to store the PC in SUBR_PC_LOC
#define SUBR_ACC_LOC "0xffe"

class Parser
{
    enum NumberFormat
    {
        None,
        Hex,
        Dec,
        Bin,
    };

public:
    Parser(const std::string& filename);

    bool extract_arg(Instr instr, const std::string& line, std::size_t line_nr, std::string& arg);
    void write_asm_to(const std::string& filename);
    bool write_to(const std::string& filename);
    void parse_all();
    void write_data_segment(std::uint16_t address, instruction_t& raw_instr);
    void parse_label(const std::string& s, std::uint16_t address);
    void parse_data(const instr_arg_pair_t& pair, std::uint16_t address);
    void parse_standard(const instr_arg_pair_t& pair, instruction_t& raw_instr);
    void parse_arg(const std::string& arg, instruction_t& raw_instr);
    void parse_subroutine(const instr_arg_pair_t& pair, instruction_t& raw_instr);

    NumberFormat  evaluate_number_format(const std::string& arg);
    std::uint16_t parse_number(const std::string& arg);
    std::uint16_t resolve_name(const std::string& name);

    bool invalid() const {
        return m_invalid;
    }

protected:
    // flag is set when an error occurs
    bool                                 m_invalid = false;
    std::map<std::string, DataInfo>      m_data_map;
    std::map<std::string, std::uint16_t> m_label_map;
    std::vector<instr_arg_pair_t>        m_instr_arg_pairs;
    std::vector<instruction_t>           m_instrs;
};

#endif // PARSER_H
