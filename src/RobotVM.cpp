#include <cstdio>
#include <cstring>
#include <cassert>
#include <sstream>
#include <iomanip>
#include <string>
#include "RobotVM.h"
#include "VMInstr.h"
#include "Typedefs.h"

#include <exception>

inline int mod (int a, int b) {
    if (b < 0)
        return mod(a, -b);
    int ret = a % b;
    if(ret < 0)
        ret+=b;
    return ret;
}

RobotVM::RobotVM()
    : _emitter(new VMInstrEmitter()), _regs(), _errorstate(), _rwp(0), _halt(false),
      _hexregs(false) {
    memset(&_rom[0],   0,  NELEMS(_rom));
    memset(&_ram[0],   0,  NELEMS(_ram));
    memset(&_stack[0], 0,  NELEMS(_stack));
    _regs.setAllGeneralRegistersToZero();
}

RobotVM::~RobotVM() {
    printf("RobotVM::~RobotVM() ...\n");
}

int RobotVM::getRegisterIndex(RegName reg) const {
    switch (reg) {
    case RegName::R1:
        return 0;
    case RegName::R2:
        return 1;
    case RegName::R3:
        return 2;
    case RegName::R4:
        return 3;
    case RegName::PC:
        return 4;
    case RegName::SP:
        return 5;
    default:
        throw std::exception();
    }
}

bool RobotVM::burn(const vm_instr_t& instr) {
    const byte_t* srcbytes = reinterpret_cast<const byte_t*>(&instr);
    byte_t*       dstbytes = &_rom[_rwp];
    VMInstrEmitter&     em = *_emitter;
    Opcode          opcode = instr.opcode;
    unsigned int       len = em.instructionLengthOfOperandType(em.getOperandTypeOfOpcode(opcode));

    assert(len <= sizeof(instr));

    memcpy(dstbytes, srcbytes, len);
    _rwp += len;

    printf("burn(): burned %d bytes...\n", len);

    return true;
}

bool RobotVM::burn(const std::vector<byte_t>& bytes) {
    if (_rwp + bytes.size() >= VM_ROM_SIZE)
        return false;

    for ( const byte_t byte : bytes ) {
        _rom[_rwp] = byte;
        _rwp++;
    }

    return true;
}

bool RobotVM::burn(const std::vector<word_t>& words) {
    if (_rwp + (2 * words.size()) >= VM_ROM_SIZE)
        return false;

    word_t* dst = reinterpret_cast<word_t*>(&_rom[0]);
    for ( const word_t word : words ) {
        dst[_rwp] = word;
        _rwp += 2;;
    }

    return true;
}

void RobotVM::printROM() const {
    printf("ROM:\n");
    unsigned int i = 0;
    for (i = 0; i < _rwp; i++) {
        if (mod (i, 16) == 0)
            printf("%04x : ", i);
        printf("%02x ", _rom[i]);
        if (mod(i+1, 16) == 0)
            printf("\n");
    }
    if (mod(i+1, 16) != 0)
        printf("\n");

    printf("--- END ROM ---\n");
    fflush(0);
}

std::string RobotVM::printROMToString() const {
    std::stringstream res;
    res << "ROM:" << std::endl << std::setfill('0') << std::hex;
    unsigned int i = 0;
    for (i = 0; i < _rwp; i++) {
        if (mod (i, 16) == 0)
            res << std::setw(4) << i << " : ";
        res << std::setw(2) << static_cast<int>(_rom[i]) << " ";
        if (mod(i+1, 16) == 0)
            res << std::endl;
    }

    if (mod(i+1, 16) != 0)
        res << std::endl;

    res << "--- END ROM ---\n";
    return res.str();
}

void RobotVM::printRAM(unsigned int count) const {
    printf("RAM:\n");
    for (unsigned int i = 0; i < count; i++) {
        if (mod (i, 16) == 0)
            printf("%04x : ", i);
        printf("%02x ", _ram[i]);
        if (mod(i+1, 16) == 0)
            printf("\n");
    }

    printf("--- END RAM ---\n");
    fflush(0);
}

void RobotVM::printStack() const {
    if (_regs.sp == 0)
        return;

    unsigned int i = 0;
    for (i=0; i < static_cast<unsigned int>(_regs.sp); i++) {
        if (mod (i, 16) == 0)
            printf("    ST%02x:", i);
        printf("%02x ", _stack[i]);
        if (mod(i+1, 16) == 0)
            printf("\n");
    }

    if (mod(i+1, 16) != 0)
        printf("\n");
    // printf("--- END STACK ---\n");
}


void RobotVM::printRegs(vm_regs_t regs) const {
    if (!_hexregs)
        printf("  P:[%04x] / R:[%6d][%6d][%6d][%6d]\n", regs.pc, regs.r1, regs.r2, regs.r3, regs.r4);
    else
        printf("  P:[%04x] / R:[%04x][%04x][%04x][%04x]\n",
               regs.pc,
               (word_t)(regs.r1),
               (word_t)(regs.r2),
               (word_t)(regs.r3),
               (word_t)(regs.r4));
}

void RobotVM::exec(const vm_instr_t& instr) {
    volatile const byte_t* const params = const_cast<volatile const byte_t*>(&(instr.bytes[0]));

    // syntactic sugar time.  most of these will go to waste.
    const byte_t         b1    = *(params);

    // Uncomment when BB and BBB-type operands are used.
    // Commented out for now to eliminate compiler warnings.
    // const byte_t         b2    = *(params+1);
    // const byte_t         b3    = *(params+2);

    // unfortunately, can't simply reinterpret_cast from byte_t to RegName,
    // this needs to happen at i_* function call when needed
    volatile const RegName* const reg1p = reinterpret_cast<volatile const RegName*>(params);
    volatile const RegName* const reg2p = reinterpret_cast<volatile const RegName*>(params+1);
    volatile const RegName* const reg3p = reinterpret_cast<volatile const RegName*>(params+2);

    volatile const word_t* const  w1p   = reinterpret_cast<volatile const word_t*>(params);
    volatile const word_t* const  w2p   = reinterpret_cast<volatile const word_t*>(params+1);
    const word_t         w1    = *w1p;
    const word_t         w2    = *w2p;

    // kludge!  addr_t is 32-bit only because GCC was being stupid with
    //   letting me overload all those emit() functions.  As far as our
    //   16-bit VM is concerned, it's still a 16-bit memory address, so
    //   just static_cast the existing w1 and w2.  This will (I sure hope!)
    //   always be a number within 0..65535 despite being an int.
    const addr_t         a1    = static_cast<volatile const addr_t>(w1);
    const addr_t         a2    = static_cast<volatile const addr_t>(w2);

    word_t  pcbefore = _regs.pc;

    switch (instr.opcode) {
    case Opcode::NOP:
        goto good;
    case Opcode::JMP_W:
        i_jmpw(a1);
        goto good;
    case Opcode::JNEG_RW:
        i_jnegrw(*reg1p, a2);
        goto good;
    case Opcode::JPOS_RW:
        i_jposrw(*reg1p, a2);
        goto good;
    case Opcode::JZERO_RW:
        i_jzerorw(*reg1p, a2);
        goto good;
    case Opcode::JNZERO_RW:
        i_jnzerorw(*reg1p, a2);
        goto good;
    case Opcode::MOV_RM:
        i_movrm(*reg1p, a2);
        goto good;
    case Opcode::MOVB_RM:
        i_movbrm(*reg1p, a2);
        goto good;
    case Opcode::MOV_MR:
        i_movmr(a1, *reg2p);
        goto good;
    case Opcode::MOV_RR:
        i_movrr(*reg1p, *reg2p);
        goto good;
    case Opcode::MOV_RW:
        i_movrw(*reg1p, w2);
        goto good;
    case Opcode::MOVRP_RR:
        i_movrprr(*reg1p, *reg2p);
        goto good;
    case Opcode::MOVPR_RR:
        i_movprrr(*reg1p, *reg2p);
        goto good;
    case Opcode::SWAP_RR:
        i_swaprr(*reg1p, *reg2p);
        goto good;
    case Opcode::SWAP_RM:
        i_swaprm(*reg1p, a2);
        goto good;
    case Opcode::BC_RRR:
        i_bcrrr(*reg1p, *reg2p, *reg3p);
        goto good;
    case Opcode::ZERO_NIL:
        i_zeronil();
        goto good;
    case Opcode::AND_RR:
        i_andrr(*reg1p, *reg2p);
        goto good;
    case Opcode::AND_RW:
        i_andrw(*reg1p, w2);
        goto good;
    case Opcode::OR_RR:
        i_orrr(*reg1p, *reg2p);
        goto good;
    case Opcode::OR_RW:
        i_orrw(*reg1p, w2);
        goto good;
    case Opcode::XOR_RR:
        i_xorrr(*reg1p, *reg2p);
        goto good;
    case Opcode::XOR_RW:
        i_xorrw(*reg1p, w2);
        goto good;
    case Opcode::NOT_R:
        i_notr(*reg1p);
        goto good;
    case Opcode::BSL_R:
        i_bslr(*reg1p);
        goto good;
    case Opcode::BSR_R:
        i_bsrr(*reg1p);
        goto good;
    case Opcode::ROL_R:
        i_rolr(*reg1p);
        goto good;
    case Opcode::ROR_R:
        i_rorr(*reg1p);
        goto good;
    case Opcode::HALT_NIL:
        i_haltnil();
        goto good;
    case Opcode::ADD_RW:
        i_addrw(*reg1p, w2);
        goto good;
    case Opcode::ADD_RR:
        i_addrr(*reg1p, *reg2p);
        goto good;
    case Opcode::ADD_RRR:
        i_addrrr(*reg1p, *reg2p, *reg3p);
        goto good;
    case Opcode::SUB_RR:
        i_subrr(*reg1p, *reg2p);
        goto good;
    case Opcode::MUL_RW:
        i_mulrw(*reg1p, w2);
        goto good;
    case Opcode::MUL_RR:
        i_mulrr(*reg1p, *reg2p);
        goto good;
    case Opcode::NEG_R:
        i_negr(*reg1p);
        goto good;
    case Opcode::DUP_R:
        i_dupr(*reg1p);
        goto good;
    case Opcode::PUSH_R:
        i_pushr(*reg1p);
        goto good;
    case Opcode::PUSH_W:
        i_pushw(w1);
        goto good;
    case Opcode::PUSH_B:
        i_pushb(b1);
        goto good;
    case Opcode::POPW_R:
        i_popwr(*reg1p);
        goto good;
    case Opcode::POPB_R:
        i_popbr(*reg1p);
        goto good;
    case Opcode::RECV_RB:  // todo
    case Opcode::SEND_RB:  // todo
    case Opcode::NUM_OPCODES:
    case Opcode::INVALID:
        i_hcfnil();
        goto good;
        // leave out 'default' -- good for compiler to catch unconsidered
        // cases for this enum and warn about them
    }

    printf("unknown opcode #%02d\n", static_cast<int>(instr.opcode));
    return;

good:
    vm_regs_t printregs = _regs;
    printregs.pc = pcbefore;
    printRegs(printregs);
    printStack();
    return;
}

void RobotVM::putstr(addr_t ramloc, const char* const str) {
    char* absloc = reinterpret_cast<char*>(&_ram[ramloc]);
	memcpy((void*)absloc, (void*)str, strlen(str));
	//strncpy_s(absloc, str, strlen(str));
}

/*! Execute single instruction located at ROM[pc].

    Remember step() is not supposed to honor _halt.
    step() does, and shall, execute whatever resides at
    the Program Counter. It will, however, always
    increment the Program Counter unconditionally. */
void RobotVM::step() {
    VMInstrEmitter& em = *_emitter;
    vm_regs_t& regs = _regs;
    const byte_t* const romptr = &_rom[0];

    // 32-bit "window" starting at an address in ROM, the
    // address being the value of the (P)rogram (C)ounter register
    const vm_instr_t* const instr = reinterpret_cast<const vm_instr_t*>(romptr+regs.pc);

    Opcode opc = instr->opcode;
    OperandType opt = em.getOperandTypeOfOpcode(opc);
    int len = em.instructionLengthOfOperandType(opt);

    word_t oldpc = regs.pc;
    exec(*instr);

    // auto-increment program counter iff previous instruction
    // did not already alter the PC on its own
    //
    // consequently, this means "addr: JMP addr" will not lock
    // up the machine in an infinite loop.  but that is such
    // an unsignificant edge-case, I see no need to "solve" it.
    if (oldpc == regs.pc)
        regs.pc = regs.pc + len;

	if (static_cast<addr_t>(regs.pc) > _rwp)
	{
		printf("PC exceeded ROM Write Pointer\n");
		halt();
	}
}

void RobotVM::run() {
    _halt = false;
    while (!_halt) step();
}

// === CPU STUFF! ===
#define ASSERTREG(x) assert((0 <= static_cast<byte_t>(x)) && (static_cast<byte_t>(x) <= 4))

// THIS is why it's important that Register enums are equally
// congruent in defined order to their ordering in vm_regs_t
#define REGVAL(x) _regs.r[static_cast<word_t>(x)]
opresult_t RobotVM::i_movrm(RegName reg, addr_t addr) {
    ASSERTREG(reg);
    word_t* wram = reinterpret_cast<word_t*>(&_ram[addr]);
    REGVAL(reg) = *wram;
}

opresult_t RobotVM::i_movbrm(RegName reg, addr_t addr) {
    ASSERTREG(reg);
    byte_t* wram = reinterpret_cast<byte_t*>(&_ram[addr]);
    REGVAL(reg) = static_cast<word_t>(*wram);
}

opresult_t RobotVM::i_movmr(addr_t addr, RegName reg) {
    ASSERTREG(reg);
    word_t* wram = reinterpret_cast<word_t*>(&_ram[addr]);
    *wram = REGVAL(reg);
}

opresult_t RobotVM::i_movrr(RegName reg1, RegName reg2) {
    ASSERTREG(reg1);
    ASSERTREG(reg2);
    REGVAL(reg1) = REGVAL(reg2);
}

opresult_t RobotVM::i_movrw(RegName reg, word_t w) {
    ASSERTREG(reg);
    REGVAL(reg) = w;
}

opresult_t RobotVM::i_movrprr(RegName reg1ptr, RegName reg2) {
    unsigned short usaddr = (REGVAL(reg1ptr));
    word_t* wram = reinterpret_cast<word_t*>(&_ram[usaddr]);
    *wram = REGVAL(reg2);
}

opresult_t RobotVM::i_movprrr(RegName reg1, RegName reg2ptr) {
    unsigned short usaddr = (REGVAL(reg2ptr));
    word_t* wram = reinterpret_cast<word_t*>(&_ram[usaddr]);
    REGVAL(reg1) = *wram;
}

opresult_t RobotVM::i_zeronil() {
    _regs.r1 = 0;
    _regs.r2 = 0;
    _regs.r3 = 0;
    _regs.r4 = 0;
}

opresult_t RobotVM::i_haltnil() {
	halt();
}

opresult_t RobotVM::i_addrw(RegName reg, word_t w) {
    REGVAL(reg) = REGVAL(reg) + w;
}

opresult_t RobotVM::i_addrr(RegName reg1, RegName reg2) {
    REGVAL(reg1) += REGVAL(reg2);
}

opresult_t RobotVM::i_addrrr(RegName reg1, RegName reg2, RegName reg3) {
    REGVAL(reg1) += REGVAL(reg2) + REGVAL(reg3);
}

opresult_t RobotVM::i_mulrw(RegName reg, word_t w) {
    REGVAL(reg) *= w;
}

opresult_t RobotVM::i_mulrr(RegName reg1, RegName reg2) {
    REGVAL(reg1) *= REGVAL(reg2);
}

opresult_t RobotVM::i_negr(RegName reg) {
    REGVAL(reg) = -REGVAL(reg);
}

opresult_t RobotVM::i_dupr(RegName reg) {
    for (int idx = this->getRegisterIndex(reg);
            idx < 4; idx++) {
        _regs.r[idx] = REGVAL(reg);
    }
}

opresult_t RobotVM::i_subrr(RegName reg1, RegName reg2) {
    REGVAL(reg1) -= REGVAL(reg2);
}

opresult_t RobotVM::i_jmpw(addr_t ptr) {
	printf("JMP: %d\n", ptr);
    _regs.pc = static_cast<word_t>(ptr);
}

opresult_t RobotVM::i_jnegrw(RegName reg, addr_t ptr) {
    if (REGVAL(reg) < 0)
        _regs.pc = static_cast<word_t>(ptr);
}

opresult_t RobotVM::i_jposrw(RegName reg, addr_t ptr) {
    if (REGVAL(reg) > 1)
        _regs.pc = static_cast<word_t>(ptr);
}

opresult_t RobotVM::i_jzerorw(RegName reg, addr_t ptr) {
    if (REGVAL(reg) == 0)
        _regs.pc = static_cast<word_t>(ptr);
}

opresult_t RobotVM::i_jnzerorw(RegName reg, addr_t ptr) {
    word_t rv = REGVAL(reg);

    if (rv != 0)
        _regs.pc = static_cast<word_t>(ptr);

}

opresult_t RobotVM::i_pushr(RegName reg) {
    if (_regs.sp >= (VM_STACK_SIZE - 2)) // 2 bc we push word not byte
        return;

    word_t* wptr = reinterpret_cast<word_t*>(&(_stack[_regs.sp]));
    *wptr = REGVAL(reg);
    _regs.sp += 2;
}

opresult_t RobotVM::i_pushw(word_t w) {
    if (_regs.sp >= (VM_STACK_SIZE - 2)) { // 2 bc we push word not byte
        printf("warning: ignoring PUSH word on full stack (sp=%d)\n", _regs.sp);
        return;
    }

    word_t* wptr = reinterpret_cast<word_t*>(&(_stack[_regs.sp]));
    *wptr = w;
    _regs.sp += 2;
}

opresult_t RobotVM::i_pushb(byte_t b) {
    if (_regs.sp >= (VM_STACK_SIZE - 1)) {
        printf("warning: ignoring PUSH byte on full stack (sp=%d)\n", _regs.sp);
        return;
    }

    _stack[_regs.sp] = b;
    _regs.sp++;
}

opresult_t RobotVM::i_popwr(RegName reg) {
    if (_regs.sp == 0) {
        printf("warning: POPW_R when SP==0\n");
        return;   // silent fail
    }
    if (_regs.sp == 1)
        _regs.sp--;
    else
        _regs.sp -= 2;

    word_t* wptr = reinterpret_cast<word_t*>(&(_stack[_regs.sp]));
    REGVAL(reg) = *wptr;
}

opresult_t RobotVM::i_popbr(RegName reg) {
    if (_regs.sp == 0) {
        printf("warning: POPB_R when SP==0\n");
        return;   // silent fail
    }

    byte_t* bptr = reinterpret_cast<byte_t*>(&(_stack[--(_regs.sp)]));
    REGVAL(reg) = static_cast<word_t>(*bptr);
}

opresult_t RobotVM::i_andrr(RegName reg1, RegName reg2) {
    REGVAL(reg1) = REGVAL(reg1) & REGVAL(reg2);
}

opresult_t RobotVM::i_andrw(RegName reg, word_t w) {
    REGVAL(reg) = REGVAL(reg) & w;
}

opresult_t RobotVM::i_orrr(RegName reg1, RegName reg2) {
    REGVAL(reg1) = REGVAL(reg1) | REGVAL(reg2);
}

opresult_t RobotVM::i_orrw(RegName reg, word_t w) {
    REGVAL(reg) = REGVAL(reg) | w;
}

opresult_t RobotVM::i_xorrr(RegName reg1, RegName reg2) {
    REGVAL(reg1) = REGVAL(reg1) ^ REGVAL(reg2);
}

opresult_t RobotVM::i_xorrw(RegName reg, word_t w) {
    REGVAL(reg) = REGVAL(reg) ^ w;
}

opresult_t RobotVM::i_notr(RegName reg) {
    REGVAL(reg) = ~(REGVAL(reg));
}

opresult_t RobotVM::i_bcrrr(RegName reg1ptr, RegName reg2ptr, RegName reg3bytes) {
    addr_t  relSrcAddr = static_cast<addr_t>(REGVAL(reg1ptr));
    addr_t  relDstAddr = static_cast<addr_t>(REGVAL(reg2ptr));

    byte_t* src = &_ram[relSrcAddr];
    byte_t* dst = &_ram[relDstAddr];

    int amt = static_cast<int>(REGVAL(reg3bytes));
    if (amt < 0) amt = 0;
    if (relSrcAddr + amt >= (VM_RAM_SIZE -1)) {
        throw "Not sure what to do yet.  Crash the VM for now, lol";
    }

    memcpy(dst, src, amt);
}

opresult_t RobotVM::i_swaprr(RegName reg1, RegName reg2) {
    word_t tmp = REGVAL(reg2);
    REGVAL(reg2) = REGVAL(reg1);
    REGVAL(reg1) = tmp;
}

opresult_t RobotVM::i_swaprm(RegName reg, addr_t addr) {
    word_t* wram = reinterpret_cast<word_t*>(&_ram[addr]);
    word_t tmp = *wram;
    *wram = REGVAL(reg);
    REGVAL(reg) = tmp;
}

opresult_t RobotVM::i_bslr(RegName reg) {
    // tmp assignment required because C bit-shift op was treating even
    // 16-bit words as 32-bit int!!
    ASSERTREG(reg);
    word_t tmp = static_cast<word_t>(REGVAL(reg));
    tmp <<= 1;
    REGVAL(reg) = tmp;
}

opresult_t RobotVM::i_bsrr(RegName reg) {
    // tmp assignment required because C bit-shift op was treating even
    // 16-bit words as 32-bit int!!
    ASSERTREG(reg);
    word_t tmp = static_cast<word_t>(REGVAL(reg));
    tmp >>= 1;
    REGVAL(reg) = tmp;
}

opresult_t RobotVM::i_rolr(RegName reg) {
    word_t x = REGVAL(reg);
    const int shift = 1;
    x = (x << shift) | (x >> (sizeof(x)*8 - shift));
    REGVAL(reg) = x;
}

opresult_t RobotVM::i_rorr(RegName reg) {
    word_t x = REGVAL(reg);
    const int shift = 1;
    x = (x >> shift) | (x << (sizeof(x)*8 - shift));
    REGVAL(reg) = x;
}

opresult_t RobotVM::i_hcfnil() {
	halt();
    _errorstate.on_fire = true;
}
