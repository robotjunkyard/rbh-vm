#include <vector>
#include <algorithm>
#include <cstdio>
#include <cctype>

#include "VMInstr.h"
#include "VMOpcodeTypes.h"
#include "VMEmitException.h"
#include "VMXCoderException.h"
#include "TextBuffer.h"

//! String representations of the HumanOpcodes
std::vector<std::string> humanopcodestrings = {
    "NOP",
    "MOV",
    "MOVB",
    "MOVRP",
    "MOVPR",
    "SWAP",
    "ZERO",
    "DUP",
    "ADD",
    "SUB",
    "MUL",
    "NEG",
    "JMP",
    "JNEG",
    "JPOS",
    "JZERO",
    "JNZERO",
    "HALT",
    "AND",
    "OR",
    "XOR",
    "NOT",
    "BSL",
    "BSR",
    "ROL",
    "ROR",
    "PUSH",
    "POPB",
    "POPW",
    "BC",
    "RECV",
    "SEND"
};

std::string printHumanOpcodeStrings()
{
    std::string str = "";

    for (const auto& hopcStr : humanopcodestrings)
        str += hopcStr + '\n';

    return str;
}

const std::vector<std::string> registerStrings = {
    "R1", "R2", "R3", "R4", "PC", "SP", "IX"
};

RegName VMInstrTranscoder::stringToRegister(std::string& str) const {
    for (unsigned int i = 0; i < registerStrings.size(); i++) {
        if (str.compare(registerStrings[i]) == 0)
            return static_cast<RegName>(i);
    }

    return RegName::INVALID;
}

HumanOpcode VMInstrTranscoder::stringToHumanOpcode(std::string str) const {
    std::string lcase_check = str;
    std::transform(lcase_check.begin(),
                   lcase_check.end(),
                   lcase_check.begin(), ::tolower);

    /** \bug This lookup isn't very efficient at all */
    for (std::size_t i = 0; i < humanopcodestrings.size(); i++) {
        std::string lcase_arrstr =
            humanopcodestrings[i];
        std::transform(lcase_arrstr.begin(),
                       lcase_arrstr.end(),
                       lcase_arrstr.begin(), ::tolower);

        if (lcase_check.compare(lcase_arrstr) == 0)
            return static_cast<HumanOpcode>(i);
    }

    return HumanOpcode::INVALID;
}

Opcode VMInstrTranscoder::getVMOpcodeFromHumanOpcode(HumanOpcode opc, OperandType ot) const {
    // auto& pairlist = (_operandValidityTable[opc]);

    for (auto pair : (_operandValidityTable[opc]))
        //for (auto pair = pairlist.begin(); pair != pairlist.end(); ++pair)
    {
        if ((pair).first == ot) {
            return (pair).second;
        }
    }

    //  throw VMXCoderException("There is no combination of VM opcode with the given HumanOpcode+OperandType combination");
    return Opcode::INVALID;
}

Opcode VMInstrTranscoder::byteToOpcode(byte_t byte) const {
    int val = static_cast<int>(byte);

    if (val >= static_cast<int>(Opcode::NUM_OPCODES)) {
        throw VMXCoderException("byteToOpcode: out-of-bounds");
        return Opcode::INVALID;
    }

    return static_cast<Opcode>(byte);
}

byte_t VMInstrTranscoder::opcodeToByte(Opcode opc) const {
    return static_cast<byte_t>(opc);
}

void VMInstrTranscoder::initOpcodeByteToOperandTypeTable() {
    for (int i=0; i<static_cast<int>(Opcode::NUM_OPCODES); i++) {
        printf("i = %d\n", i);
        this->_opcodeByteToOperandTypeTable[i] = OperandType::INVALID;
    }

    byte_t* opair = &vmsetup_opc_instr_types[0][0];
    do {
        Opcode opc = static_cast<Opcode>(opair[0]);
        OperandType opt = static_cast<OperandType>(opair[1]);
        printf("opcode(%d) is type %d\n", static_cast<int>(opc),
               static_cast<int>(opt));

        this->_opcodeByteToOperandTypeTable[opair[0]] = opt;

        opair += 2;
        if (opc == Opcode::NOP)
            break;
    } while (true);


    // check if any opcodes were forgotten
    for (int i=0; i<static_cast<int>(Opcode::NUM_OPCODES); i++) {
        OperandType type = this->_opcodeByteToOperandTypeTable[i];
        if (OperandType::INVALID == type) {
            // Please check VMOpcodeTypes.h if this is thrown.
            throw VMXCoderException("An opcode was overlooked during initOpcodeByteToOperandTypeTable()");
        }
    }
}

OperandType VMInstrTranscoder::getOperandTypeOfOpcode(Opcode opc) {
    return _opcodeByteToOperandTypeTable[static_cast<int>(opc)];
}

void VMInstrTranscoder::initOperandTypeLengthTable() {
    typedef OperandType OT;

    // ugly fucking stupid monkey language, lol
    // let's use C++'s not-really-a-macro system
    // to make this less of an eyesore.
#define OTLEN(x, y)   _operandTypeLengthTable[static_cast<unsigned char>(x)] = y

    for (int i=0; i<static_cast<int>(OT::NUM_OPERAND_TYPES); i++) {
        OTLEN(i, -1);  // any that get missed will be easily noticable in debugger later on
    }

    OTLEN(OT::NIL,1);
    OTLEN(OT::RM, 4);
    OTLEN(OT::MR, 4);
    OTLEN(OT::RR, 3);
    OTLEN(OT::RRR,4);
    OTLEN(OT::RW, 4);
    OTLEN(OT::RB, 3);
    OTLEN(OT::P,  3);
    OTLEN(OT::R,  2);
    // OTLEN(OT::RP, 4);
    OTLEN(OT::BBB,4);
    OTLEN(OT::BB, 3);
    OTLEN(OT::B,  2);
    OTLEN(OT::BW, 4);
    OTLEN(OT::WB, 4);
    OTLEN(OT::W,  3);
#undef OTLEN
}

int VMInstrTranscoder::instructionLengthOfOperandType(OperandType opt) const {
    int opt_cast = static_cast<int>(opt);
    int len = _operandTypeLengthTable[opt_cast];

    return len;
}

// this would be so much prettier in Lisp
void VMInstrTranscoder::initOperandValidityTable() {
    auto& ovt = _operandValidityTable;
    typedef OperandType OT;
    typedef Opcode OP;

    // translate HumanOpcode + mode to VM Opcode
    ovt[HumanOpcode::MOV] = {
        {OT::RM, OP::MOV_RM},
        {OT::MR, OP::MOV_MR},
        {OT::RR, OP::MOV_RR},
        {OT::RW, OP::MOV_RW}
    };

    ovt[HumanOpcode::MOVRP] = {
        {OT::RR, OP::MOVRP_RR}
    };

    ovt[HumanOpcode::MOVPR] = {
        {OT::RR, OP::MOVPR_RR}
    };

    ovt[HumanOpcode::SWAP] = {
        {OT::RR, OP::SWAP_RR},
        {OT::RM, OP::SWAP_RM}
    };

    ovt[HumanOpcode::ZERO] = {
        {OT::NIL, OP::ZERO_NIL}
    };

    ovt[HumanOpcode::DUP] = {
        {OT::R, OP::DUP_R}
    };

    ovt[HumanOpcode::ADD] = {
        {OT::RW, OP::ADD_RW},
        {OT::RR, OP::ADD_RR},
        {OT::RRR, OP::ADD_RRR}
    };

    ovt[HumanOpcode::SUB] = {
        {OT::RR, OP::SUB_RR}
    };

    ovt[HumanOpcode::MUL] = {
        {OT::RW, OP::MUL_RW},
        {OT::RR, OP::MUL_RR}
    };

    ovt[HumanOpcode::NEG] = {
        {OT::R, OP::NEG_R}
    };

    ovt[HumanOpcode::JMP] = {
        {OT::W, OP::JMP_W}
    };

    ovt[HumanOpcode::JNEG] = {
        {OT::RW, OP::JNEG_RW}
    };

    ovt[HumanOpcode::JPOS] = {
        {OT::RW, OP::JPOS_RW}
    };

    ovt[HumanOpcode::JZERO] = {
        {OT::RW, OP::JZERO_RW}
    };

    ovt[HumanOpcode::JNZERO] = {
        {OT::RW, OP::JNZERO_RW}
    };

    ovt[HumanOpcode::HALT] = {
        {OT::NIL, OP::HALT_NIL}
    };

    ovt[HumanOpcode::AND] = {
        {OT::RR, OP::AND_RR},
        {OT::RW, OP::AND_RW}
    };

    ovt[HumanOpcode::OR] = {
        {OT::RR, OP::OR_RR},
        {OT::RW, OP::OR_RW}
    };

    ovt[HumanOpcode::XOR] = {
        {OT::RR, OP::XOR_RR},
        {OT::RW, OP::XOR_RW}
    };

    ovt[HumanOpcode::NOT] = {
        {OT::R, OP::NOT_R}
    };

    ovt[HumanOpcode::BSL] = {
        {OT::R, OP::BSL_R}
    };

    ovt[HumanOpcode::BSR] = {
        {OT::R, OP::BSR_R}
    };

    ovt[HumanOpcode::ROL] = {
        {OT::R, OP::ROL_R}
    };

    ovt[HumanOpcode::ROR] = {
        {OT::R, OP::ROR_R}
    };


    ovt[HumanOpcode::BC] = {
        {OT::RRR, OP::BC_RRR}
    };

    ovt[HumanOpcode::PUSH] = {
        {OT::R, OP::PUSH_R},
        {OT::W, OP::PUSH_W},
        {OT::B, OP::PUSH_B}
    };

    ovt[HumanOpcode::POPB] = {
        {OT::R, OP::POPB_R},
    };
    ovt[HumanOpcode::POPW] = {
        {OT::R, OP::POPW_R},
    };
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, RegName reg1, addr_t addr2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::RM);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("RM nonexistant", opc);
        return {};  // defaults to invalid
    }

    int ireg = static_cast<int>(reg1);
    printf("EMIT [%s %s %d]\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           registerStrings[ireg].c_str(), addr2);
    return vm_instr_emit_info_t(vm_instr_t(vmopc, static_cast<byte_t>(reg1),
                                           static_cast<word_t>(addr2)),  4);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, addr_t addr1, RegName reg2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::MR);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("MR nonexistant", opc);
        return {};  // defaults to invalid
    }

    int ireg = static_cast<int>(reg2);
    printf("EMIT %s %d %s\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           addr1, registerStrings[ireg].c_str());

    return vm_instr_emit_info_t(vm_instr_t(vmopc, static_cast<word_t>(addr1), static_cast<byte_t>(reg2)), 4);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, RegName reg1, RegName reg2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::RR);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("RR nonexistant", opc);
        return {};  // defaults to invalid
    }

    int ireg1 = static_cast<int>(reg1),
        ireg2 = static_cast<int>(reg2);
    printf("EMIT %s %s %s\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           registerStrings[ireg1].c_str(), registerStrings[ireg2].c_str());

    return vm_instr_emit_info_t(vm_instr_t(vmopc, static_cast<byte_t>(reg1), static_cast<byte_t>(reg2)), 3);
}


vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, RegName reg1, RegName reg2, RegName reg3) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::RRR);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("RRR nonexistant", opc);
        return {};  // defaults to invalid
    }

    int ireg1 = static_cast<int>(reg1),
        ireg2 = static_cast<int>(reg2),
        ireg3 = static_cast<int>(reg3);

    printf("EMIT %s %s %s %s\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           registerStrings[ireg1].c_str(),
           registerStrings[ireg2].c_str(),
           registerStrings[ireg3].c_str());

    return vm_instr_emit_info_t(vm_instr_t(vmopc, static_cast<byte_t>(reg1),
                                           static_cast<byte_t>(reg2), static_cast<byte_t>(reg3)),  4);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, RegName reg1, word_t w2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::RW);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("RW nonexistant", opc);
        return {};  // defaults to invalid
    }

    int ireg1 = static_cast<int>(reg1);
    printf("EMIT %s %s %d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           registerStrings[ireg1].c_str(), w2);

    return vm_instr_emit_info_t(vm_instr_t(vmopc, static_cast<byte_t>(reg1), static_cast<word_t>(w2)), 4);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, RegName reg1, byte_t b2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::RB);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("RB nonexistant", opc);
        return {};  // defaults to invalid
    }

    int ireg1 = static_cast<int>(reg1);
    printf("EMIT %s %s %d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           registerStrings[ireg1].c_str(), b2);

    return vm_instr_emit_info_t(vm_instr_t(vmopc, static_cast<byte_t>(reg1), static_cast<byte_t>(b2)), 3);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, RegName reg) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::R);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("R nonexistant", opc);
        return {};  // defaults to invalid
    }

    int ireg = static_cast<int>(reg);
    printf("EMIT %s %s\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           registerStrings[ireg].c_str());
    return vm_instr_emit_info_t(vm_instr_t(vmopc, static_cast<byte_t>(reg)),  2);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, byte_t b1, byte_t b2, byte_t b3) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::BBB);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("RBBB nonexistant", opc);
        return {};  // defaults to invalid
    }

    printf("EMIT %s %d %d %d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           b1, b2, b3);
    return vm_instr_emit_info_t(vm_instr_t(vmopc, b1, b2, b3),  4);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, byte_t b1, byte_t b2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::BB);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("BB nonexistant", opc);
        return {};  // defaults to invalid
    }

    printf("EMIT %s %d %d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           b1, b2);
    return vm_instr_emit_info_t(vm_instr_t(vmopc, b1, b2), 3);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, byte_t b1) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::B);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("B nonexistant", opc);
        return {};  // defaults to invalid
    }

    printf("EMIT %s %d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           b1);
    return vm_instr_emit_info_t(vm_instr_t(vmopc, b1), 2);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, byte_t b1, word_t w2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::BW);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("BW nonexistant", opc);
        return {};  // defaults to invalid
    }

    printf("EMIT %s b%d w%d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           b1, w2);
    // preserve all widths of an opc, byte, and word.  4 bytes
    return vm_instr_emit_info_t(vm_instr_t(vmopc, b1, w2), 4);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, word_t w1, byte_t b2) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::WB);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("WB nonexistant", opc);
        return {};  // defaults to invalid
    }

    printf("EMIT %s w%d b%d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           w1, b2);
    return vm_instr_emit_info_t(vm_instr_t(vmopc, w1, b2), 4);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, word_t w1) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::W);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("W nonexistant", opc);
        return {};  // defaults to invalid
    }

    printf("EMIT %s w%d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           w1);
    return vm_instr_emit_info_t(vm_instr_t(vmopc, w1), 3);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc, addr_t ptr) const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::P);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("P nonexistant", opc);
        return {};  // defaults to invalid
    }

    // word_t ptr16 = static_cast<word_t>(ptr);
    vm_instr_t instr(vmopc, ptr);

    printf("EMIT %s p%d\n", humanopcodestrings[static_cast<int>(opc)].c_str(),
           ptr);
    // preserve all widths of an opc and a word.  3 bytes
    return vm_instr_emit_info_t(instr, 3);
}

vm_instr_emit_info_t VMInstrEmitter::emit(HumanOpcode opc)  const {
    Opcode vmopc = getVMOpcodeFromHumanOpcode(opc, OperandType::NIL);
    if (Opcode::INVALID == vmopc) {
        throw VMEmitException("NIL-param nonexistant", opc);
        return {};  // defaults to invalid
    }
    printf("EMIT %s\n", humanopcodestrings[static_cast<int>(opc)].c_str());
    // preserve all widths of an opc and a word.  3 bytes
    return vm_instr_emit_info_t(vm_instr_t(vmopc), 1);
}
