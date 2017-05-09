#ifndef VMASSEMBLER_H
#define VMASSEMBLER_H

#include <memory>
#include <unordered_map>

#include "RobotVM.h"
#include "VMInstr.h"

/*enum class ASMParserResult : unsigned char
{
    BURNED_VM_INSTRUCTION,
    SKIPPED_BYTES,
    WROTE_BYTES,
    PARSER_ERROR
};*/

enum class ASMParserTokenType : unsigned char {
    INVALID,              //!< Parser encountered probably-junk input
    NOT_NEEDED,           //!< Tells users of this token there was no error, but to also please discard
    LABEL,                //!< A label denoting a human word pertaining to a memory address
    PSEUDO_INSTRUCTION,   //!< Pseudo-instruction such as ORG, BYTE, WORD, etc
    OPCODE,               //!< Token represents an opcode instruction
    PARAM_BLOCK           //!< Params need to be further broken down \note @see ASMParserParamType
};

enum class ASMParserParamType : unsigned char {
    INVALID,              //!< Parser encountered probably-junk input
    LITERAL_REGISTER,     //!< Param is name of a Register (ex: R1 in 'MOV R1,[foo]'
    BRACKETED_REGISTER,   //!< Param is name of a Register surrounded in brackets (ex: [R2] in 'MOV [R2],R4')
    LITERAL_LABEL,        //!< Param references the name of a label (ex: drawvertices in 'JMP drawvertices')
    BRACKETED_LABEL,      //!< Param is a label surrounded in brackets (ex: [temperature] in 'MOV R1,[temperature]')
    LITERAL_NUMBER,       //!< Param is a number (ex: 3320 in 'ADD R3,3320')
    BRACKETED_NUMBER      //!< Param is a number surrounded in brackets (ex: [0x0100] in 'MOV [0x0100],R4')

};

class ASMParserParamToken {
private:
    ASMParserParamType _type;
    const bool _bracketed;    //!< Enclosed in square brackets?
    const std::string _text;
    int _numValue;

public:
    ASMParserParamToken(ASMParserParamType type, bool bracketed, std::string text,
                        int numValue = 0)
        : _type(type), _bracketed(bracketed), _text(text), _numValue(numValue) {
    }

    ASMParserParamType getType() const {
        return _type;
    }
    bool isValid() const {
        return (_type != ASMParserParamType::INVALID);
    }
    bool isBracketed() const {
        return _bracketed;
    }
    const std::string& getText() const {
        return _text;
    }
    int getValue() const {
        return _numValue;
    }
    void setValue(int val) {
        _numValue = val;
    }
};

class ASMParserToken {
private:
    ASMParserTokenType _type;
    std::string _text;
    word_t _value = 0;     //!< Only relevant when type = label or HumanOpcode, else ignored
    std::vector<ASMParserParamToken> _paramtokens = {};

public:
    ASMParserToken()
        : _type(ASMParserTokenType::INVALID), _text("") {
    }
    ASMParserToken(ASMParserTokenType type, std::string text, word_t value = 0)
        : _type(type), _text(text), _value(value) {
    }
    ASMParserToken(ASMParserTokenType type, std::string text,
                   const std::vector<ASMParserParamToken>& paramtokens,
                   word_t value = 0)
        : _type(type), _text(text), _value(value), _paramtokens(paramtokens) {
    }

    ASMParserToken(const ASMParserToken& other)
        : _type(other.getType()), _text(other.getText()), _value(other._value),
          _paramtokens(other._paramtokens) {

    }

    ASMParserToken& operator=(const ASMParserToken& other) {
        _text = other.getText();
        _type = other.getType();
        _value = other.getValue();
        _paramtokens = std::vector<ASMParserParamToken>(other._paramtokens);
        return *this;
    }

    ASMParserTokenType getType() const {
        return _type;
    }

    bool isValid() const {
        if (_type == ASMParserTokenType::INVALID)
            return false;
        for (auto paramtoken : _paramtokens) {
            if (!paramtoken.isValid())
                return false;
        }

        return true;
    }
    const std::string& getText() const {
        return _text;
    }

    std::vector<ASMParserParamToken>& getParamTokens() {
        return _paramtokens;
    }

    word_t getValue() const {
        return _value;
    }

    /** Exists because LABEL tokens' will be re-visited and altered to represent
       a memory address sometime after the first pass. */
    void setValue(word_t val) {
        _value = val;
    }

    std::vector<OperandType> deducePossibleOperandTypes () const;
};

class ASMParseLineResult_Pass1 {
private:
    ASMParserToken _labelToken;  //!< Mutable because this will morph into a numeric in later pass
    ASMParserToken _hopcToken;
    ASMParserToken _paramToken;  //!< Mutable also because some instructions refer to labels
                                 //!< which will be mutated into numbers (addresses) in a later pass.

public:
    //! Default constructor creates an object communicating that a parse
    //! attempt returned an invalid result.
    ASMParseLineResult_Pass1()
        : _labelToken(ASMParserToken(ASMParserTokenType::NOT_NEEDED, "NOT_NEEDED")),
          _hopcToken(), _paramToken() {
    }
    ASMParseLineResult_Pass1(ASMParserToken& tok_hopc, ASMParserToken& paramtoken,
                             ASMParserToken& tok_label)
        : _labelToken(tok_label), _hopcToken(tok_hopc), _paramToken(paramtoken) {
    }

    ASMParseLineResult_Pass1& operator= (const ASMParseLineResult_Pass1& other) {
        _labelToken = other._labelToken;
        _hopcToken = other._hopcToken;
        _paramToken = other._paramToken;
        return *this;
    }

    bool isValid() {
        return _hopcToken.isValid();
    }

    ASMParserToken& getLabelToken() {
        return _labelToken;
    }

    ASMParserToken& getHopcToken() {
        return _hopcToken;
    }

    /*HumanOpcode getHumanOpcode() const
    {
        if (_hopcToken.isValid())
            return static_cast<HumanOpcode>(_hopcToken.getValue());
        else
            return HumanOpcode::INVALID;
    }*/

    ASMParserToken& getParamToken() {
        return _paramToken;
    }
};


class VMAssembler {
protected:

private:
    const VMInstrTranscoder& _transcoder;
    mutable std::shared_ptr<RobotVM> _targetVM; //!< Which VM receives ROM burn instructions
    std::unordered_map<std::string,addr_t> _labeltable;

    /** Classifies a raw string token and returns a detailed ASMParserToken
        value with a more definitive decision as to what kind of token it
        is.
        \param token: a pure text representation of the string
        \param which: where on the original line this token was found */
    ASMParserToken classifytoken(const std::string& token, int which, bool line_label_found) const;
    ASMParserParamToken classifyparam(const std::string& intoken) const;

    std::vector<ASMParseLineResult_Pass1> _pass1tokens;

public:
    /** Parses a line of text from an assembly file and translates it into an
        ASMParseLineResult_Pass1 object.  That resulting object's
       Parameter Tokens (via .getParamTokens()) will need to further be
       treated before moving on to some kind of _Pass2 later on, before
       any machine code can be emitted.
       \note This also push_back's the result onto this->_pass1tokens */
    ASMParseLineResult_Pass1 parseline(std::string text);
    void parsetextblock(std::string text);   //! splits into lines and parseline()s each one
    /** Default constructor */
    explicit VMAssembler(std::shared_ptr<RobotVM> targetVM,
                         const VMInstrTranscoder& transcoder);

    bool walkFirstPass(int start_rwp = 0);
    bool walkSecondPass();

    /** Default destructor */
    virtual ~VMAssembler();

    /** Access _targetVM;
     * \return The current value of _targetVM;
     */
    std::shared_ptr<RobotVM> getTargetVM
    () {
        return _targetVM;;
    }

	//!< Clears the memorized tokens & labels, if there were any from a previous use
	void reset() 
	{ 
		_pass1tokens.clear(); 
		_labeltable.clear();
	}
};

#endif // VMASSEMBLER_H
