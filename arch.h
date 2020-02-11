#ifndef ARCH_H
#define ARCH_H

#include <cstdint>
#include <map>
#include <string>

// will be padded by the compiler, but we cannot explicitly
//  pad this as we may cast between this and std::uint16_t at
//  some point.
struct instruction_t {
    unsigned int S : 12;
    unsigned int opcode : 4;
};

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
    LABEL,
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
    { ".label", Instr::LABEL }
};

namespace Prefix {
static constexpr char VAR[]   = "$";
static constexpr char LABEL[] = ".";
}

struct instr_arg_pair_t {
    Instr       instr;
    std::string arg;
};

#endif // ARCH_H
