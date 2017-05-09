#ifndef VMINSTREXCEPTION_H
#define VMINSTREXCEPTION_H

#include <stdexcept>
#include "VMInstr.h"

class VMEmitException : public std::exception {
protected:
    const char* what() const noexcept {
        return _what;
    }
    HumanOpcode hopc() const {
        return _hopc;
    }

private:
    const char* _what;
    HumanOpcode _hopc;

public:
    /** Default constructor */
    VMEmitException(const char* const msg, HumanOpcode hopc)
        : _what(const_cast<char*>(msg)), _hopc(hopc) {
    }
    VMEmitException(const VMEmitException& other)
        : _what(const_cast<char*>(other.what())), _hopc(other.hopc()) {
    }

    VMEmitException& operator= (const VMEmitException& other) {
        _what = const_cast<char*>(other.what());
        _hopc = other.hopc();
        return *this;
    }

    virtual ~VMEmitException();
};

#endif // VMINSTREXCEPTION_H
