/* To be used in VMInstr.cpp when populating tables */

#ifndef VMOPCODETYPES_H_INCLUDED
#define VMOPCODETYPES_H_INCLUDED

// enum-to-byte
#define EB(x) (static_cast<byte_t>(x))
#define TPAIR(x, y)  { EB(x), EB(y) }

/** Table for specifying the parameter fingerprints of various opcodes. */
byte_t vmsetup_opc_instr_types[][2] = {
    TPAIR(Opcode::MOV_RM,   OperandType::RM ),
    TPAIR(Opcode::MOV_MR,   OperandType::MR ),
    TPAIR(Opcode::MOV_RR,   OperandType::RR ),
    TPAIR(Opcode::MOV_RW,   OperandType::RW ),
    TPAIR(Opcode::MOVRP_RR, OperandType::RR ),
    TPAIR(Opcode::MOVPR_RR, OperandType::RR ),
    TPAIR(Opcode::MOVB_RM,  OperandType::RM ),
    TPAIR(Opcode::SWAP_RR,  OperandType::RR ),
    TPAIR(Opcode::SWAP_RM,  OperandType::RM ),
    TPAIR(Opcode::BC_RRR,   OperandType::RRR),
    TPAIR(Opcode::ZERO_NIL, OperandType::NIL),
    TPAIR(Opcode::DUP_R,    OperandType::R  ),
    TPAIR(Opcode::ADD_RW,   OperandType::RW ),
    TPAIR(Opcode::HALT_NIL, OperandType::NIL),
//      TPAIR(Opcode::ADDW_RM,  OperandType::RM ),
//      TPAIR(Opcode::ADDB_RM,  OperandType::RM ),
    TPAIR(Opcode::ADD_RR,   OperandType::RR ),
    TPAIR(Opcode::ADD_RRR,  OperandType::RRR),
    TPAIR(Opcode::MUL_RW,   OperandType::RW ),
    TPAIR(Opcode::MUL_RR,   OperandType::RR ),
    TPAIR(Opcode::NEG_R,    OperandType::R  ),
    TPAIR(Opcode::SUB_RR,   OperandType::RR ),
    TPAIR(Opcode::JMP_W,    OperandType::P  ),
    TPAIR(Opcode::JNEG_RW,  OperandType::RW ),
    TPAIR(Opcode::JPOS_RW,  OperandType::RW ),
    TPAIR(Opcode::JZERO_RW, OperandType::RW ),
    TPAIR(Opcode::JNZERO_RW,OperandType::RW ),
    TPAIR(Opcode::AND_RR,   OperandType::RR ),
    TPAIR(Opcode::AND_RW,   OperandType::RW ),
    TPAIR(Opcode::OR_RR,    OperandType::RR ),
    TPAIR(Opcode::OR_RW,    OperandType::RW ),
    TPAIR(Opcode::XOR_RR,   OperandType::RR ),
    TPAIR(Opcode::XOR_RW,   OperandType::RW ),
    TPAIR(Opcode::NOT_R,    OperandType::R  ),
    TPAIR(Opcode::BSL_R,    OperandType::R  ),
    TPAIR(Opcode::BSR_R,    OperandType::R  ),
    TPAIR(Opcode::ROL_R,    OperandType::R  ),
    TPAIR(Opcode::ROR_R,    OperandType::R  ),
    TPAIR(Opcode::PUSH_R,   OperandType::R  ),
    TPAIR(Opcode::PUSH_W,   OperandType::W  ),
    TPAIR(Opcode::PUSH_B,   OperandType::B  ),
    TPAIR(Opcode::POPB_R,   OperandType::R  ),
    TPAIR(Opcode::POPW_R,   OperandType::R  ),
    TPAIR(Opcode::RECV_RB,  OperandType::RB ),
    TPAIR(Opcode::SEND_RB,  OperandType::RB ),

    /* ALWAYS keep NOP at end of this array to tell
       table-builder to stop crawling after this one */
    TPAIR(Opcode::NOP,      OperandType::NIL)
};

#endif // VMOPCODELENGTHS_H_INCLUDED
