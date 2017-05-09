#ifndef VMXCODEREXCEPTION_H
#define VMXCODEREXCEPTION_H

#include <exception>
#include <string>

//!< Class for exceptions thrown by VM transcoder
class VMXCoderException : public std::exception {
protected:
    const char* what() const noexcept {
        return _what.c_str();
    }

private:
    const std::string _what;

public:
    /** Default constructor */
    /*VMXCoderException(const std::string& msg)
        : _what(msg) {
    }*/

    explicit VMXCoderException(const char* const msg)
        : _what(msg) {
    }

    VMXCoderException(const VMXCoderException& other)
        : _what(const_cast<char*>(other.what())) {
    }

    VMXCoderException& operator= (const VMXCoderException& other) {
        // _what = const_cast<std::string&&>(std::string(other.what()));
        const_cast<std::string&>(_what) = other.what();
        return *this;
    }

    virtual ~VMXCoderException() { }
};


#endif // VMXCODEREXCEPTION_H
