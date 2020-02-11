#include "Parser.h"

#include <fstream>   // std::ifstream, std::ofstream
#include <iostream>  // std::cout, std::cerr
#include <algorithm> // std::find..., std::erase
#include <iomanip>   // std::setw, std::setfill, etc.
#include <cassert>   // assert

#include "debug.h"
#include "utility.h"

Parser::Parser(const std::string& filename) {
    std::ifstream file(filename, std::ios::in);
    if (!file.is_open()) {
        error("file '" << filename << "' not found");
        m_invalid = true;
        return;
    }
    std::size_t   line_nr  = 1;
    std::uint16_t instr_nr = 0;

    do {
        // FIXME: This entire loop needs cleaning up
        std::string s;
        std::getline(file, s);
        // trim comments
        s.erase(std::find(s.begin(), s.end(), '#'), s.end());
        s = trim_whitespace(s);
        // ignore now-empty lines
        if (s.empty()) {
            ++line_nr;
            continue;
        }
        // split instr and arg
        Instr instr = instr_from_name(std::string(std::begin(s), std::find(s.begin(), s.end(), ' ')));
        if (instr == Instr::CALL) {
            // calls into subroutines are implemented by holding the PC before the jump
            // in a data segment so we can jump back to it

            // save acc in subr_acc_loc since we need acc momentarily and don't want to lose data from it
            ++instr_nr;
            verbose("PC: " << instr_nr);
            m_instr_arg_pairs.push_back(instr_arg_pair_t { STO, SUBR_ACC_LOC });

            // now we store the current PC in hex at some location
            // since there is no immediate value instruction we have to hack together
            // a temporary variable with a DATA segment which we jump over so it
            // doesn't get executed.

            // so we add the jump to skip ahead to the second instruction after the jmp
            // to skip the data segment that's coming up
            ++instr_nr;
            verbose("PC: " << instr_nr);
            verbose("instr: JMP " << as_hex_string(instr_nr + 1));
            m_instr_arg_pairs.push_back(instr_arg_pair_t { JMP, as_hex_string(instr_nr + 1) });

            // now we add the data segment to hold the PC to jump back to
            ++instr_nr;
            verbose("PC: " << instr_nr);
            // offset to jump back to
            std::uint16_t offset = 4;
            // we need to use a name here, so we use `__pc__ADDRESS`, where `ADDRESS` is the PC we stored
            std::string pc_store_name = std::string("__pc__") + as_hex_string(instr_nr + offset);
            // let's make an instruction (hacky & wacky)
            instruction_t instr_to_insert;
            instr_to_insert.opcode     = static_cast<std::uint8_t>(JMP);
            instr_to_insert.S          = instr_nr + offset;
            std::string pc_store_instr = pc_store_name + "=" + as_hex_string(reinterpret_cast<std::uint16_t&>(instr_to_insert));
            verbose("instr: " << nameof(pc_store_instr) << ": '" << pc_store_instr << "'");
            m_instr_arg_pairs.push_back(instr_arg_pair_t { DATA, pc_store_instr });

            // load the pc we want to jump to later
            ++instr_nr;
            verbose("PC: " << instr_nr);
            m_instr_arg_pairs.push_back(instr_arg_pair_t { LDA, std::string(Prefix::VAR) + pc_store_name });

            // store it in the pc location
            ++instr_nr;
            verbose("PC: " << instr_nr);
            m_instr_arg_pairs.push_back(instr_arg_pair_t { STO, SUBR_PC_LOC });

            // retore acc
            ++instr_nr;
            verbose("PC: " << instr_nr);
            m_instr_arg_pairs.push_back(instr_arg_pair_t { LDA, SUBR_ACC_LOC });

            // extracting the argument for the call instruction
            std::string arg;
            if (!extract_arg(instr, s, line_nr, arg)) {
                error("subroutine call argument is invalid (this is fatal)");
                m_invalid = true;
            }
            // add the original call instruction as jmp
            ++instr_nr;
            verbose("PC: " << instr_nr);
            verbose("instr: jmp(call) " << arg);
            m_instr_arg_pairs.push_back(instr_arg_pair_t { JMP, arg });
            continue;
        } else if (instr == Instr::RET) {
            // only works if there was a call before and SUBR_PC_LOC is set
            ++instr_nr;
            verbose("PC: " << instr_nr);
            m_instr_arg_pairs.push_back(instr_arg_pair_t { JMP, SUBR_PC_LOC });
        }
        if (instr == Instr::LABEL) {
            parse_label(s, instr_nr);
            ++line_nr;
            continue;
        }
        instr_nr++;
        verbose("PC: " << instr_nr);
        std::string arg;
        if (!extract_arg(instr, s, line_nr, arg))
            continue;
        if (instr == Instr::INVALID) {
            error("source line " << line_nr << ": parsed instruction is unknown / invalid");
            m_invalid = true;
            continue;
        }
        instr_arg_pair_t pair { instr, arg };
        log("source line " << line_nr << ": parsed instr of pair: " << name_from_instr(instr) << " " << arg);
        m_instr_arg_pairs.push_back(pair);
        ++line_nr;
    } while (!file.eof());
    log("parsing done");
}

void Parser::write_asm_to(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    if (!file.is_open()) {
        error("could not open file '" << filename << "'");
        return;
    }

    std::uint16_t instr_nr = 0;
    for (instr_arg_pair_t& pair : m_instr_arg_pairs) {
        std::stringstream ss;
        ss << "    " << name_from_instr(pair.instr) << " ";
        if (pair.arg == SUBR_ACC_LOC)
            ss << "$SUBR_ACC_LOC";
        else if (pair.arg == SUBR_PC_LOC)
            ss << "$SUBR_PC_LOC";
        else
            ss << pair.arg;
        std::string line    = ss.str();
        int         padding = static_cast<int>(40 - line.size());
        file << line << std::setfill(' ') << std::setw(padding) << " # PC: 0x"
             << std::setfill('0') << std::setw(4) << std::hex << instr_nr << std::endl;
        ++instr_nr;
        auto iter = std::find_if(m_label_map.begin(), m_label_map.end(),
            [&](auto& l_pair) -> bool {
                return l_pair.second == instr_nr;
            });
        if (iter != m_label_map.end()) {
            file << iter->first << ":" << std::endl;
        }
    }
}

bool Parser::extract_arg(Instr instr, const std::string& line, std::size_t line_nr, std::string& arg) {
    if (instr_expects_arg(instr)) {
        if (std::find(line.begin(), line.end(), ' ') == line.end()) {
            error("source line " << line_nr << ": argument expected for '" << line << "'");
            m_invalid = true;
            return false;
        }
        arg = line.substr(line.find_first_of(' ') + 1);
    } else {
        // ensure that there was no argument given to an instruction that does not expect one,
        //  since this should never happen as this would mean that wrong assumptions were made
        if (std::find(line.begin(), line.end(), ' ') != line.end()) { // if there is a space
            auto trimmed = trim_whitespace(line.substr(line.find_first_of(' ')));
            // if there are characters in the argument except whitespace (which has been trimmed)
            if (!trimmed.empty()) {
                error("source line " << line_nr << ": argument '" << trimmed
                                     << "' supplied to '" << name_from_instr(instr)
                                     << "' which does not expect an argument");
                m_invalid = true;
                return false;
            }
        }
    }
    return true;
}

void Parser::parse_all() {
    // first parse data segments
    for (std::uint16_t i = 0; i < m_instr_arg_pairs.size(); ++i) {
        auto pair = m_instr_arg_pairs.at(i);
        if (pair.instr == Instr::DATA) {
            parse_data(pair, i);
        }
    }
    for (std::uint16_t i = 0; i < m_instr_arg_pairs.size(); ++i) {
        auto pair = m_instr_arg_pairs.at(i);

        instruction_t raw_instr;
        if (is_standard_instr(pair.instr)) {
            parse_standard(pair, raw_instr);
        } else if (pair.instr == Instr::DATA) {
            write_data_segment(i, raw_instr);
        } else if (pair.instr == Instr::LABEL) {
            // already handled, early breakout
            continue;
        } else if (pair.instr == Instr::CALL
                   || pair.instr == Instr::RET) {
            // already handled, early breakout
            continue;
        } else {
            error("no parser found for '" << name_from_instr(pair.instr) << "'");
            m_invalid = true;
        }
        m_instrs.push_back(raw_instr);
    }
}

bool Parser::write_to(const std::string& filename) {
    // doing this with fstreams is hacky, so we do it the C-way for now
    //  ... although suggestions on making this more safe are welcome
    FILE* fp = fopen(filename.c_str(), "wb");

    if (!fp) {
        error("file could not be openend for write: '" << filename << "'. error reported as: '");
        perror("fopen");
        return false;
    }

    for (const auto& i : m_instrs) {
        verbose("writing: 0x" << std::setfill('0') << std::setw(4) << std::hex
                              << *reinterpret_cast<const std::uint16_t*>(&i));
        fwrite(&i, sizeof(std::uint16_t), 1, fp);
    }

    fclose(fp);
    return true;
}

void Parser::write_data_segment(uint16_t address, instruction_t& raw_instr) {
    auto iter = std::find_if(m_data_map.begin(), m_data_map.end(),
        [&](const auto& elem) -> bool {
            return elem.second.address == address;
        });
    if (iter == m_data_map.end()) {
        error("could not find address in data map (internal error)");
        m_invalid = true;
        return;
    }
    verbose("writing data '0x" << std::setfill('0') << std::setw(4) << std::hex
                               << iter->second.value << "' for data named '"
                               << iter->first << "'");
    raw_instr = reinterpret_cast<instruction_t&>(iter->second.value);
}

void Parser::parse_label(const std::string& s, uint16_t address) {
    std::string s_trimmed = trim_whitespace(s);
    s_trimmed             = s_trimmed.substr(std::strlen(Prefix::LABEL));
    auto iter             = s_trimmed.find(':');
    if (iter == std::string::npos) {
        error("label declaration '" << s << "' expects ':' at the end");
        m_invalid = true;
        return;
    }
    s_trimmed = s_trimmed.substr(0, s_trimmed.size() - 1);
    m_label_map.emplace(std::move(s_trimmed), std::move(address));
}

void Parser::parse_data(const instr_arg_pair_t& pair, uint16_t address) {
    // should only be called on data
    assert(pair.instr == Instr::DATA);
    // this is handled earlier
    assert(!pair.arg.empty());
    // check for format 'name=N'
    if (pair.arg.find('=') == std::string::npos) {
        error("invalid format for data: '" << pair.arg << "', must be of format 'name=N'");
        m_invalid = true;
        return;
    }
    std::string name = pair.arg.substr(0, pair.arg.find('='));
    std::string rhs;
    // check if there even is a rhs (name + 1 is 'name='),
    // otherwise the substr fails. if this fails then rhs is
    // empty and this triggers an error a few lines down
    if (name.size() + 1 < pair.arg.size())
        rhs = pair.arg.substr(pair.arg.find('=') + 1);

    name = trim_whitespace(name);
    rhs  = trim_whitespace(rhs);

    if (name.empty()) {
        error("in argument '" << pair.arg << "' to data: name cannot be empty");
        m_invalid = true;
        return;
    }
    if (rhs.empty()) {
        error("in argument '" << pair.arg << "' to data: right hand side cannot be empty");
        m_invalid = true;
        return;
    }

    // rhs needs to be a number
    auto format = evaluate_number_format(rhs);
    if (format == NumberFormat::None) {
        error("in right hand side '" << rhs << "' in data argument '"
                                     << pair.arg << "': right hand side has to be a value type (number)");
        m_invalid = true;
    }

    DataInfo dat;
    dat.value   = parse_number(rhs);
    dat.address = address;

    m_data_map.emplace(std::move(name), std::move(dat));
}

void Parser::parse_standard(const instr_arg_pair_t& pair, instruction_t& raw_instr) {
    // FIXME: This can probably be a map
    switch (pair.instr) {
    case LDA:
    case STO:
    case ADD:
    case SUB:
    case JMP:
    case JGE:
    case JNE:
    case STP:
        // Instr enum defines these as their opcode values already
        raw_instr.opcode = static_cast<std::uint16_t>(pair.instr);
        break;
    case CALL:
    case RET:
    case DATA:
    case INVALID:
    case LABEL:
    case END_STD_INSTR_SET:
        // not reachable
        assert(false);
    }

    if (!instr_expects_arg(pair.instr)) {
        // early return for instructions without arguments
        raw_instr.S = 0;
        return;
    }
    // sanity check
    assert(!pair.arg.empty());
    parse_arg(pair.arg, raw_instr);
}

void Parser::parse_arg(const std::string& arg, instruction_t& raw_instr) {
    switch (evaluate_number_format(arg)) {
    case NumberFormat::None:
        raw_instr.S = resolve_name(arg);
        verbose(arg << " has number format None");
        break;
    case NumberFormat::Hex:
        raw_instr.S = static_cast<std::uint16_t>(std::stoul(arg, nullptr, 16));
        verbose(arg << " has number format Hex");
        break;
    case NumberFormat::Dec:
        raw_instr.S = static_cast<std::uint16_t>(std::stoul(arg, nullptr, 10));
        verbose(arg << " has number format Dec");
        break;
    case NumberFormat::Bin:
        error("binary number format is not yet implemented.");
        m_invalid = true;
        break;
    }
}

Parser::NumberFormat Parser::evaluate_number_format(const std::string& arg) {
    // number formats:
    //  - HEX: 0xN
    //  - DEC: N
    //  - BIN: 0bN // not supported (yet)

    if (arg.find("0x") == 0) {
        // it's probably hex
        // we now check that the rest of it is only hex-digits.
        // if we find something then we know it's not valid hex,
        // which is an error since names also cannot start with a
        // digit.
        if (std::find_if(arg.begin() + 2, arg.end(), [&](auto& c) {
                // isxdigit returns 0 if c IS valid hex char
                return std::isxdigit(static_cast<unsigned char>(c)) != 0;
            })
            == arg.end()) {
            error("argument '" << arg << "' starts with '0x' like "
                               << " a hex number, but isn't valid hex.");
            m_invalid = true;
            return NumberFormat::None;
        }
        // now we know it's definitely hex
        return NumberFormat::Hex;
    } else if (std::isdigit(arg.at(0))) {
        // it's probably dec
        if (std::find_if(arg.begin(), arg.end(), [&](auto& c) {
                return std::isdigit(c) != 0;
            })
            == arg.end()) {
            error("argument '" << arg << "' starts with a digit like "
                               << "a decimal number, but is not a "
                               << "valid decimal number format.");
            m_invalid = true;
            return NumberFormat::None;
        }
        // we are pretty sure it's decimal now
        return NumberFormat::Dec;
    } else {
        return NumberFormat::None;
    }
}

uint16_t Parser::parse_number(const std::string& arg) {
    switch (evaluate_number_format(arg)) {
    case NumberFormat::Hex:
        return static_cast<std::uint16_t>(std::stoul(arg, nullptr, 16));
        verbose(arg << " has number format Hex");
    case NumberFormat::Dec:
        return static_cast<std::uint16_t>(std::stoul(arg, nullptr, 10));
        verbose(arg << " has number format Dec");
    case NumberFormat::Bin:
        // not implemented
    case NumberFormat::None:
        error("argument '" << arg << "' is not a number");
        m_invalid = true;
    }
    return 0;
}

void Parser::parse_subroutine(const instr_arg_pair_t& pair, instruction_t& raw_instr) {
    error("not implemented");
    m_invalid = true;
}

uint16_t Parser::resolve_name(const std::string& name) {
    if (name.find(Prefix::VAR) == std::string::npos
        && name.find(Prefix::LABEL) == std::string::npos) {
        error("usage of name '" << name << "' requires prefix '" << Prefix::VAR << "'");
        m_invalid = true;
        return 0;
    }

    if (name.find(Prefix::VAR) != std::string::npos) {
        std::string trimmed_name = name.substr(std::strlen(Prefix::VAR));
        auto        found        = std::find_if(m_data_map.begin(), m_data_map.end(),
            [&](auto& elem) -> bool {
                return elem.first == trimmed_name;
            });
        if (found != m_data_map.end()) {
            verbose("found var prefix in " << name);
            return found->second.address;
        }
    } else if (name.find(Prefix::LABEL) != std::string::npos) {
        std::string trimmed_name = name.substr(std::strlen(Prefix::LABEL));
        auto        other_found  = std::find_if(m_label_map.begin(), m_label_map.end(),
            [&](auto& elem) -> bool {
                return elem.first == trimmed_name;
            });
        if (other_found != m_label_map.end()) {
            verbose("found label prefix in " << name);
            return other_found->second;
        }
    }

    // at this point the name could not be resolved
    error("name '" << name << "' could not be resolved as data or label");
    m_invalid = true;
    return 0;
}
