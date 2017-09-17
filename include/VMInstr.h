#ifndef VMINSTR_H
#define VMINSTR_H

#include <string>
#include <cstdint>
#include <unordered_map>
#include <vector>
#include <list>
#include <utility>

std::string printHumanOpcodeStrings();

//! Signed word
typedef std::int16_t  sword_t;
//! Unsigned word
typedef std::uint16_t  word_t;
//! 16-bit, but 32-bit here so C++ can let me overload emit(...), grrrr...
typedef std::uint32_t  addr_t;
//! A byte.  What else did you think it was?  Ice cream?
typedef std::uint8_t   byte_t;

enum class OperandType : char {
    NOT_AN_OPERAND = -2,  //!< Used by assembler's OperandType deduction
    INVALID = -1, //!< Denotes an invalid OperandType
    NIL = 0,      //!< no operands
    RM,           //!< REG, [memaddr]
    MR,			  //!< [memaddr], REG
    RR,           //!< two register arg
    RRR,          //!< REG1, REG2, REG3
    RW,           //!< REG, WORDLiteral
    RB,           //!< REG, BYTELiteral
    M,            //!< Value in [memaddr]
    P,            //!< actual memory address
    R,            //!< unary register
    // RP,         //!< register-and-ptr (same fingerprint as RM, but denoted separately for non-data-transfer ops)
    BBB,          //!< three byte constants
    BB,           //!< two byte constants
    B,            //!< one byte constant
    BW,           //!< byte and word constant
    WB,           //!< word and byte constant
    W,            //!< one word constant
    NUM_OPERAND_TYPES  //!< Not a real operand type, duh. Used in loops, etc to know when to end.
};

/** Officially enumerates names of all the registers.
    \note The VM absolutely *MUST* order its registers in the same order as in this enum. */
enum class RegName : char {
    INVALID = -1,
    R1 = 0,  //!< General Register 1
    R2,      //!< General Register 2
    R3,      //!< General Register 3
    R4,      //!< General Register 4
    PC,      //!< PC = Program Counter
    SP,      //!< SP = Stack Pointer
    IX       //!< IX = Index Register
};

/** String representations of VM registers */
extern const std::vector<std::string> registerStrings;

/** Actual Opcodes, as recognized by the VM's exec(), are enumerated here. */
enum class Opcode : unsigned char {
    NOP = 0,    //!< NIL

    MOV_RM,     //!< REG = [mem16 literal addr]  **
    MOV_MR,     //!< [mem16 literal addr] = REG  **
    MOV_RR,     //!< REG1 = REG2            **
    MOV_RW,     //!< REG = WORD literal     **
    MOVRP_RR,   //!< mem16[REG1] = REG2     **
    MOVPR_RR,   //!< REG = mem16[REG2]      **
    MOVB_RM,    //!< REG = [mem8]           *  untested

    SWAP_RR,    //!< R1 <--> R2 swapped     *  untested
    SWAP_RM,    //!< Reg <--> [mem16] swapped
    ZERO_NIL,   //!< Zero all registers    **
    DUP_R,      //!< Value in REG --> all higher-numbered registers  **

    ADD_RW,     //!< REG += WORD    **
    //ADDW_RM,    //!< REG += [mem16]
    //ADDB_RM,    //!< REG += [mem8]
    ADD_RR,     //!< REG1 = REG1 + REG2          **
    ADD_RRR,    //!< REG1 = REG1 + REG2 + REG3   **
    SUB_RR,     //!< REG1 = REG1 - REG2          **
    MUL_RW,     //!< REG1 = REG1 * WORD          **
    MUL_RR,     //!< REG1 = REG1 * REG2          **
    NEG_R,      //!< REG = -REG                  **

    JMP_W,      //!< PC = PTR                    **
    JNEG_RW,    //!< jump to PTR if REG is -     ** but untested
    JPOS_RW,    //!< jump to PTR if REG is +     ** but untested
    JZERO_RW,   //!< jump to PTR if REG == 0     **
    JNZERO_RW,  //!< jump to PTR if REG != 0     **

    HALT_NIL,   //!< set halt bit    **

    // bit ops
    AND_RR,     //!< REG1 = REG1 & REG2             **
    AND_RW,     //!< REG1 = REG1 & WORD             ** untested
    OR_RR,      //!< REG1 = REG1 | REG2             **
    OR_RW,      //!< REG1 = REG1 | WORD             ** untested
    XOR_RR,     //!< REG1 = REG1 ^ REG2             **
    XOR_RW,     //!< REG1 = REG1 ^ WORD             ** untested
    NOT_R,      //!< REG = ~REG                     **

    BSL_R,      //!< REG bits shift left 1          **
    BSR_R,      //!< REG bits shift right 1         **
    ROL_R,      //!< REG bits rotate left 1         **
    ROR_R,      //!< REG bits rotate right 1        **

    PUSH_R,     //!< Push value of REG on stack     **
    PUSH_W,     //!< Push WORD on stack             **
    PUSH_B,     //!< Push BYTE on stack             **
    POPB_R,     //!< Pop BYTE from stack into REG   **
    POPW_R,     //!< Pop WORD from stack into REG   **

    RECV_RB,    //!< \todo Receive WORD from port id (BYTE) and load into REG
    SEND_RB,    //!< \todo Send value of REG to port id (BYTE)

    BC_RRR,     //!< string copy from ram[reg1ptr] to ram[reg2ptr] for reg3 bytes, up to MAX_BC_BYTES

    // CADD,     //!< Cascading Add:  R3 += R4 ; R2 += R3; R1 += R2
    // possible gameplay-world instructions
    // TURN_R,     //!< turn left if register is +, else turn right if -

    // debug/devel instructions
    // SAY_R,

    NUM_OPCODES,   //!< Not a real vm opcode, duh. Used in loops, etc. to know when to terminate.
    INVALID = 255  //!< Dummy opcode
};

/** Human-readable opcodes. These will get expanded by the Assembler, which itself
    may or may not have its own set of even simpler, "Even-more-human" opcodes that
    map to these. */
enum class HumanOpcode : unsigned char {
    NOP = 0,     //!< Do nothing
    MOV,         //!< Move a word between registers and/or memory (except two memory locations)
    MOVB,        //!< Move a byte between registers and/or memory (except two memory locations)
    MOVRP,       //!< Move a value in Reg1 into a memory location denoted by value in Reg2
    MOVPR,       //!< Move a value in a memory location denoted by value in Reg2 into Reg1
    SWAP,        //!< Swap value between two registers, or a register and a memory location
    ZERO,        //!< Set all general registers to 0
    DUP,         //!< All general registers higher than Reg get the value of Reg.
    //!< For example, DUP R1 will mean R2, R3, and R4 get R1's value.
    //!< DUP R2 will only affect R3 and R4, etc.

    ADD,         //!< Add a value in Reg
    SUB,         //!< Subtract a value in Reg
    MUL,         //!< Multiply Reg by another value
    NEG,         //!< REG = -REG



    JMP,         //!< Set Program Counter to another location.
                 //!< \note Silent ignore if JMP tries to hop to its own line.
    JNEG,
    JPOS,
    JZERO,       //!< Jump to Pointer if Register is not equal to 0.
    JNZERO,      //!< Jump to Pointer if Register is equal to 0

    HALT,        //!< Set VM's Halt flag to true, terminating execution.

    AND,         //!< Bitwise AND
    OR,          //!< Bitwise OR
    XOR,         //!< Bitwise XOR
    NOT,         //!< Bitwise NOT
    BSL,         //!< Left-shift bits in Register once
    BSR,         //!< Right-shift bits in Register once
    ROL,         //!< Left-rotate bits in Register once
    ROR,         //!< Right-rotate bits in Register once

    PUSH,        //!< Push a word or byte onto the stack
    POPB,        //!< Pop a single byte from the stack
    POPW,        //!< Pop a single word from the stack

    BC,          //!< Block Copy chunk of data from one RAM location to another.

    RECV,        //!< \todo Implement this
    SEND,        //!< \todo Implement this

    NUM_HUMANOPCODES, //!< Not a real opcode, duh. Used for loops, etc.

    INVALID = 255
};

extern std::vector<std::string> humanopcodestrings;

struct vm_instr_t {
    union {
        struct {
            volatile Opcode opcode;   //!< First byte of any instruction is the Opcode
            volatile byte_t bytes[3]; //!< Denotes parameters of instruction, different depending on what the Opcode is.
        };

        // volatile unsigned int i32;
    };

    explicit vm_instr_t()
        : opcode(Opcode::INVALID) {
    }

    explicit vm_instr_t (Opcode opc)
        : opcode(opc) {
    }

    explicit vm_instr_t (Opcode opc, byte_t b1, byte_t b2, byte_t b3)
        : opcode(opc) {
        bytes[0] = b1;
        bytes[1] = b2, bytes[2] = b3;
    }


    explicit vm_instr_t (Opcode opc, byte_t b1, byte_t b2)
        : opcode(opc) {
        bytes[0] = b1;
        bytes[1] = b2;
    }

    explicit vm_instr_t (Opcode opc, byte_t b1)
        : opcode(opc) {
        bytes[0] = b1;
    }

    explicit vm_instr_t (Opcode opc, word_t w)
        : opcode(opc) {
        volatile word_t* wptr = reinterpret_cast<volatile word_t*>(&bytes[0]);
        *wptr = w;
        bytes[2] = 0xFF;
    }

    explicit vm_instr_t (Opcode opc, addr_t ptr)
        : opcode(opc) {
        volatile word_t* wptr = reinterpret_cast<volatile word_t*>(&bytes[0]);
        word_t  val = static_cast<word_t>(ptr);
        *wptr = val;
        bytes[2] = 0xFF;
    }

    explicit vm_instr_t (Opcode opc, word_t b16, byte_t b8)
        : opcode(opc) {
        volatile word_t* wptr = reinterpret_cast<volatile word_t*>(&bytes[0]);
        *wptr = b16;
        bytes[2] = b8;
    }

    explicit vm_instr_t (Opcode opc, byte_t b8, word_t b16)
        : opcode(opc) {
        bytes[0] = b8;
        volatile word_t* wptr = reinterpret_cast<volatile word_t*>(&bytes[1]);
        *wptr = b16;
    }
};

/** Produced by VMInstrEmitter.emit() to specify results of an emit operation.
    Always check values of members 'valid' and 'length' before doing anything
    with a returned emit. */
struct vm_instr_emit_info_t {
    volatile int length;    //!< Different depending on Opcode of 'instr'; will be -1 if emit() detected an invalid request
    volatile bool valid;    //!< This will only be 'true' if emit() was successful.
    vm_instr_t instr;       //!< Instruction constructed by emit()

    //! Default constructor defaults to an invalid state, with length = 1,
    //! valid = false, and instr = Opcode::INVALID.
    vm_instr_emit_info_t()
        : length(-1), valid(false), instr(Opcode::INVALID) {
    }

    vm_instr_emit_info_t(vm_instr_t i, int len)
        : length(len), valid(true), instr(i) {
    }
};

// uuugh
// this trouble just to use it as a hashtable key
namespace std {
template<>
struct hash<HumanOpcode> {
    typedef HumanOpcode argument_type;
    typedef size_t result_type;

    result_type operator () (const argument_type& x) const {
        using type = typename std::underlying_type<argument_type>::type;
        return std::hash<type>()(static_cast<type>(x));
    }
};
}

/** A VM Instruction Transcoder translates between this program's internal human-facing
    representations of things, and actual VM internal representations of those same
    instructions. */
class VMInstrTranscoder {
private:
    std::vector<OperandType> _opcodeByteToOperandTypeTable; //!< Gets initialized from data in the
    //!< vmsetup_opc_instr_types table.
    //!< @see vmsetup_opc_instr_types

    mutable std::vector<int> _operandTypeLengthTable; //!< Table initialized with information about how
    //!< many bytes an instruction is be depending on what
    //!< parameters its operand takes.
    mutable std::unordered_map<HumanOpcode,
            std::list<std::pair<OperandType,Opcode>>>
            _operandValidityTable;  //!< Table initialized with information about what possible ACTUAL Opcodes can be emitted by a HumanOpcode depending on what the given parameters to that HumanOpcode are.

    void initOpcodeByteToOperandTypeTable();
    void initOperandValidityTable();
    void initOperandTypeLengthTable();

    void populateTables() {
        initOpcodeByteToOperandTypeTable();
        initOperandTypeLengthTable();
        initOperandValidityTable();
    }

public:
    Opcode byteToOpcode(byte_t byte) const; //!< Returns a (VM) Opcode enum based on a given byte..  Will return Opcode::INVALID on a bogus byte.
    byte_t opcodeToByte(Opcode opc) const;  //!< Returns a byte representation of a VM Opcode enum.
    OperandType getOperandTypeOfOpcode(Opcode opc); //!< Return the OperandType of a VM Opcode
    HumanOpcode opcodeToHumanOpcode(Opcode) const;  //!< \todo This function is not yet implemented
    HumanOpcode stringToHumanOpcode(std::string str) const;
    RegName stringToRegister(std::string& str) const;
    Opcode getVMOpcodeFromHumanOpcode(HumanOpcode opc, OperandType ot) const;
    int instructionLengthOfOperandType(OperandType opt) const; //!< Returns length an instruction would be if its Opcode were of given OperandType.

    VMInstrTranscoder()
        : _opcodeByteToOperandTypeTable(static_cast<int>(Opcode::NUM_OPCODES)),
          _operandTypeLengthTable(static_cast<int>(OperandType::NUM_OPERAND_TYPES)),
          _operandValidityTable() {
        populateTables();
    }

    virtual ~VMInstrTranscoder() {
    }
};

/*! This class contains very-overloaded emit() function, which always begins with
    a HumanOpcode as its first argument, followed by any combination of valid
    parameters of types RegName, addr_t, word_t, or byte_t.

    Always check the 'valid' member of the returning vm_instr_emit_info_t before
    using the vm_instr_t within it!! */
class VMInstrEmitter : public VMInstrTranscoder {
public:
    VMInstrEmitter() {
    }

    vm_instr_emit_info_t emit(HumanOpcode opc, RegName reg, addr_t memaddr) const ;   // RM and RP
    vm_instr_emit_info_t emit(HumanOpcode opc, addr_t des,  RegName src)  const ;     // MR
    vm_instr_emit_info_t emit(HumanOpcode opc, RegName des, RegName src)  const ;     // RR
    vm_instr_emit_info_t emit(HumanOpcode opc, RegName reg1, RegName reg2, RegName reg3)  const ;  //RRR
    vm_instr_emit_info_t emit(HumanOpcode opc, RegName des, word_t src) const ;       // RW
    vm_instr_emit_info_t emit(HumanOpcode opc, RegName des, byte_t src) const ;       // RB
    vm_instr_emit_info_t emit(HumanOpcode opc, RegName reg) const ;					  // R
    vm_instr_emit_info_t emit(HumanOpcode opc, byte_t b1, byte_t b2, byte_t b3) const ;  // BBB
    vm_instr_emit_info_t emit(HumanOpcode opc, byte_t b1, byte_t b2) const ;		  // BB
    vm_instr_emit_info_t emit(HumanOpcode opc, byte_t b1) const ;					  // B
    vm_instr_emit_info_t emit(HumanOpcode opc, byte_t b1, word_t w1) const ;		  // BW
    vm_instr_emit_info_t emit(HumanOpcode opc, word_t w1, byte_t b1) const ;		  // WB
    vm_instr_emit_info_t emit(HumanOpcode opc, word_t w1) const ;					  // W
    vm_instr_emit_info_t emit(HumanOpcode opc, addr_t ptr) const ;					  // P
    vm_instr_emit_info_t emit(HumanOpcode opc) const ;								  // NIL

    virtual ~VMInstrEmitter() {
    }
};


#endif // VMINSTR_H
