/* Virtual Machine for a fictional robot in an Artificial Life sim.

   The game world itself runs on "ticks";  for each tick, each robot will
   execute a small bit of code via its VM.

   Because this is a game, each robot will probably have a maximum number
   of VM instructions it may execute per tick, to keep things challenging
   for the gamer-coder.
*/

#ifndef ROBOTVM_H
#define ROBOTVM_H

#include <array>
#include <memory>
#include "VMInstr.h"

#define VM_ROM_SIZE   (8 * 1024)
#define VM_STACK_SIZE 256
#define VM_RAM_SIZE   ((4 * 1024) - VM_STACK_SIZE)

class VMAssembler;

/**
 Structure for holding the state of all of a VM's registers.
 These should be sorted in the SAME ORDER as the registers
 in the RegName enum or else BAD THINGS WILL HAPPEN.  This
 is intentional for code simplicity and efficiency sake. */
struct vm_regs_t {
    union {   //!< standard acc registers
        sword_t r[4];    //!< Unioned alias for general registers but in array form
        struct {
            sword_t r1,  //!< General-purpose accumulator register 1
                    r2,  //!< General-purpose accumulator register 2
                    r3,  //!< General-purpose accumulator register 3
                    r4;  //!< General-purpose accumulator register 4
        };
    };

    word_t   pc;    //!< Program Counter (synonymous with Instruction Pointer)
    word_t   sp;    //!< Stack Pointer
    word_t   ix;    //!< Index Register

    void setAllGeneralRegistersToZero() noexcept
    {
        r1 = r2 = r3 = r4 = 0;
    }
};

struct vm_errorstate_t {
    bool illegal_instruction = false; //!< CPU detected unrecognized opcode
    bool on_fire = false;             //!< Digital sapient entity owning the VM is on fire

    vm_errorstate_t() {
    }
};

//! Denote result of an executed instruction.  Currently is void for the time being.
typedef void opresult_t;
#define OPRESULT inline opresult_t

class RobotVM {
private:
    std::unique_ptr<VMInstrEmitter> _emitter;

    byte_t _rom[VM_ROM_SIZE];        //!< Read-Only, code goes here
    byte_t _ram[VM_RAM_SIZE];        //!< RAM, data & knowledge here
    byte_t _stack[VM_STACK_SIZE];    //!< Stack is separate from RAM
    vm_regs_t     _regs;             //!< VM's registers
    vm_errorstate_t _errorstate;     //!< Various error flags

    addr_t _rwp;   //!< ROM write pointer for burn()
    bool  _halt;   //!< run() will terminate when this == true

    bool _hexregs; //!< If true, printRegs() print Rs in Base 16, else decimal

    //! Get _regs[] index pertaining to given register enum
    int getRegisterIndex(RegName reg) const;

    //! Get the ROM Write Pointer
    word_t rwp() const noexcept {
        return _rwp;
    }
    //! Set the ROM Write Pointer
    word_t setRWP(word_t val) noexcept {
        return _rwp = val;
    }

	void halt() {
		_halt = true;
	}

protected:
    OPRESULT i_movrm(RegName reg, addr_t addr);
    OPRESULT i_movbrm(RegName reg, addr_t addr);
    OPRESULT i_movmr(addr_t addr, RegName reg);
    OPRESULT i_movrr(RegName reg1, RegName reg2);
    OPRESULT i_movrw(RegName reg1, word_t w);
    OPRESULT i_movrprr(RegName reg1ptr, RegName reg2);
    OPRESULT i_movprrr(RegName reg1, RegName reg2ptr);
    OPRESULT i_swaprr(RegName reg1, RegName reg2);
    OPRESULT i_swaprm(RegName reg, addr_t addr);

    OPRESULT i_zeronil();
    OPRESULT i_andrr(RegName reg1, RegName reg2);
    OPRESULT i_andrw(RegName reg, word_t w);
    OPRESULT i_orrr(RegName reg1, RegName reg2);
    OPRESULT i_orrw(RegName reg, word_t w);
    OPRESULT i_xorrr(RegName reg1, RegName reg2);
    OPRESULT i_xorrw(RegName reg, word_t w);
    OPRESULT i_notr(RegName reg);

    OPRESULT i_bslr(RegName reg);
    OPRESULT i_bsrr(RegName reg);
    OPRESULT i_rorr(RegName reg);
    OPRESULT i_rolr(RegName reg);

    OPRESULT i_addrw(RegName reg, word_t w);
    OPRESULT i_addrr(RegName reg1, RegName reg2);
    OPRESULT i_addrrr(RegName reg1, RegName reg2, RegName reg3);
    OPRESULT i_subrr(RegName reg1, RegName reg2);
    OPRESULT i_mulrw(RegName reg, word_t w);
    OPRESULT i_mulrr(RegName reg1, RegName reg2);
    OPRESULT i_negr(RegName reg);
    OPRESULT i_dupr(RegName reg);

    OPRESULT i_pushr(RegName reg);
    OPRESULT i_pushw(word_t w);
    OPRESULT i_pushb(byte_t b);
    OPRESULT i_popwr(RegName reg);
    OPRESULT i_popbr(RegName reg);

    OPRESULT i_bcrrr(RegName reg1ptr, RegName reg2ptr, RegName reg3bytes);

    OPRESULT i_jmpw(addr_t ptr);
    OPRESULT i_jnegrw(RegName reg, addr_t ptr);
    OPRESULT i_jposrw(RegName reg, addr_t ptr);
    OPRESULT i_jzerorw(RegName reg, addr_t ptr);
    OPRESULT i_jnzerorw(RegName reg, addr_t ptr);

    OPRESULT i_haltnil();
    OPRESULT i_hcfnil();

public:
    /** Default constructor */
    RobotVM();
    /** Default destructor */
    virtual ~RobotVM();

    //! Burn instruction to ROM at [_rwp++]
    bool burn(const vm_instr_t& instr);
    //! Burn array of bytes to ROM at [_rwp] and _rwp+=len
    bool burn(const std::vector<byte_t>& bytes);
    //! Burn array of words to ROM at [_rwp] and _rwp+=len
    bool burn(const std::vector<word_t>& words);

    //! Dump the contents of the ROM to standard output.
    void printROM() const;
    //! Dump the contents of the ROM to a string
    std::string printROMToString() const;
    //! Dump the contents of RAM to standard output.
    void printRAM(unsigned int count) const;
    //! Dump the contents of the registers to standard output.
    void printRegs(vm_regs_t regs) const;
    //! Returns the amount of ROM burned (NOT its capacity!)
    word_t romsize() const {
        return _rwp;
    }
    //! If true, printRegs() prints registers in hexadecimal
    bool hexRegs() const {
        return _hexregs;
    }
    //! Set to true to make printRegs() use hexadecimal.  Else, signed 16-bit integers.
    void hexRegs(bool val) {
        _hexregs = val;
    }
    //! Dump the contents of the stack to stdout.  Does nothing if stack is empty.
    void printStack() const;
    //! Place a string into a location of RAM
    void putstr(addr_t ramloc, const char* const str);

    //! Returns a reference to the VMInstrEmitter object utilized by this object.
    VMInstrEmitter& emitter() const {
        return *_emitter;
    }

    //! Get the value of Program Counter
    word_t getPC() const {
        return _regs.pc;
    }
    //! Get the value of Stack Pointer
    word_t getSP() const {
        return _regs.sp;
    }

    const vm_regs_t& getRegs() const { return _regs; }

    //! Runs step() until halt flag is true
    void run();
    //! Runs a single instruction and increments PC by the instruction's byte length.
    void step();
    //! Executes a single VM instruction
    void exec(const vm_instr_t& instr);

    bool isHalted() const { return _halt; }

	//! Resets the PC, Rom Write Pointer, and general registers back to 0, and un-Halts the machine if it was Halted.
	void reset()
	{
		_rwp = 0;
		_regs.pc = 0;
		_regs.setAllGeneralRegistersToZero();
		_halt = false;
	}
};

#undef OPRESULT

#endif // ROBOTVM_H
