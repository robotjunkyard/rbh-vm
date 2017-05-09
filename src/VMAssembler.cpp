#include <cstring>
#include <vector>
#include <memory>
#include <iostream>
#include <sstream>
#include "VMAssembler.h"
#include "VMEmitException.h"
#include "VMInstr.h"

const std::string alphabet =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

const std::string digits =
    "0123456789";

const std::string hexdigits =
    "0123456789ABCDEFabcdef";

const std::string valid_token_chars =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._-:[],";

const std::string valid_param_chars =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._-";

const std::string valid_label_chars =
    "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._-";

const std::string valid_label_first_chars =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz._";

const std::string pseudo_instruction_names[] = { "" };

/** Ensure every character in string \w testMe exists in string \w validchars.
    \param testMe String to test
    \param validchars A string of characters to test testMe against */
bool validateStringChars(const std::string& testMe, const std::string& validchars) {
    for (auto c = testMe.begin(); c != testMe.end(); c++) {
        char ch = *c;
        // invalid character found
        if (validchars.find(ch, 0) == std::string::npos) {
            // printf("invalid char for validation on string '%s': %c\n", testMe.c_str(), ch);
            return false;
        }
    }

    return true;
}


//! Split string into a vector of token strings given a delimiter.
//! \note Yanked from http://stackoverflow.com/a/7408245/437528 because
//! I'm still very laser-focused on the VM stuff and don't want
//! to lose that momentum while I still have it!  If there's legal
//! issues about this then I will rewrite my own later.
std::vector<std::string> splitstring(const std::string &text, char sep) {
    std::vector<std::string> tokens;
    std::size_t start = 0, end = 0;
    while ((end = text.find(sep, start)) != std::string::npos) {
        if (end != start) {
            tokens.push_back(text.substr(start, end - start));
        }
        start = end + 1;
    }
    if (end != start) {
        std::string last = text.substr(start);
        if (last.size() != 0)
            tokens.push_back(last);
    }
    return tokens;
}

VMAssembler::VMAssembler(std::shared_ptr<RobotVM> targetVM,
                         const VMInstrTranscoder& transcoder)
    : _transcoder(transcoder), _targetVM(targetVM), _labeltable(), _pass1tokens() {
}

VMAssembler::~VMAssembler() { }


ASMParserParamToken VMAssembler::classifyparam(const std::string& intoken) const {
    std::string tok = intoken;
    bool isBracketed = false;
    if (tok.size() >= 3)
        isBracketed = ((tok[0] == '[') && (tok[tok.size()-1] == ']'));
    if (isBracketed)
        tok = tok.substr(1,tok.size()-2);  // ... why 2??  weird

    // make sure no junk characters are in
    /*for (auto c = tok.begin(); c != tok.end(); c++)
    {
        if (valid_param_chars.find(*c) == std::string::npos)
            return ASMParserParamToken(ASMParserParamType::INVALID, isBracketed, tok);
    }*/
    if (!validateStringChars(tok, valid_param_chars))
        return ASMParserParamToken(ASMParserParamType::INVALID, isBracketed, tok);

    // CASE: parameter refers to a register

    RegName tryreg = this->_transcoder.stringToRegister(tok);
    if (tryreg != RegName::INVALID) {
        return ASMParserParamToken(isBracketed ? ASMParserParamType::BRACKETED_REGISTER :
                                   ASMParserParamType::LITERAL_REGISTER,
                                   isBracketed,
                                   tok,
                                   // maybe not needed but it doesn't hurt
                                   static_cast<int>(tryreg));
    }

    // CASE: it is a number
    // word_t value16 = 0;
    // first, check if hexadecimal
    if (tok.substr(0,2).compare("0x") == 0) {
        std::string rest = tok.substr(2);
        if (!validateStringChars(rest, hexdigits)) {
            printf("classifyparam(): 0x indicated hexadecimal, but a non-base-16 digit was found in '%s'\n",
                   rest.c_str());
            return ASMParserParamToken(ASMParserParamType::INVALID, isBracketed, intoken);
        }

        int value32 = 0;
        std::istringstream(rest) >> std::hex >> value32;

        return ASMParserParamToken(isBracketed ? ASMParserParamType::BRACKETED_NUMBER :
                                   ASMParserParamType::LITERAL_NUMBER,
                                   isBracketed,
                                   tok,
                                   value32);
    }

    // not a hexadecimal, so maybe plain number?
    if (validateStringChars(tok, digits) ||
            ((tok[0] == '-') && validateStringChars(tok.substr(1), digits))) {
        int value32 = 0;
        std::istringstream(tok) >> std::dec >> value32;
        // value16 = static_cast<word_t>(value32);

        return ASMParserParamToken(isBracketed ? ASMParserParamType::BRACKETED_NUMBER :
                                   ASMParserParamType::LITERAL_NUMBER,
                                   isBracketed,
                                   tok,
                                   value32);
    }

    // okay, so then is it a label?
    if (validateStringChars(tok, valid_label_chars)) {
        // default value to 0 for this, because not all labels are known to the assembler
        // at this point (there could be labels later on not yet seen)
        return ASMParserParamToken(isBracketed ? ASMParserParamType::BRACKETED_LABEL :
                                   ASMParserParamType::LITERAL_LABEL,
                                   isBracketed,
                                   tok);
    }

    return ASMParserParamToken(ASMParserParamType::INVALID, isBracketed, tok);
}

ASMParserToken VMAssembler::classifytoken(const std::string& token, int where, bool line_label_found) const {
    const char* const tcstr = token.c_str();
    //! If first token on a line ends with a :, assume it wants to be
    //! a label and sanitize it as such before returning its proper ASMParserToken.
    if ((token.size() > 1) &&
            (token[token.size() - 1] == ':') &&
            (where == 0)) {
        auto labeltext = token.substr(0, token.size() - 1);

        // Disallow first character to be a numeral
        if (valid_label_first_chars.find(token[0]) == std::string::npos) {
            printf("classifytoken(): A label may not begin with a '%c'", token[0]);
            return ASMParserToken(ASMParserTokenType::INVALID, token);
        }

        // Validate rest of the label token
        if (!validateStringChars(labeltext, valid_label_chars)) {
            printf ("  invalid (label) %s\n", tcstr);
            return ASMParserToken(ASMParserTokenType::INVALID, token);
        }

        printf("  label %s\n", tcstr);
        return ASMParserToken(ASMParserTokenType::LABEL, token.substr(0, token.size()-1));
    }

    // Case: this is where an Opcode or Pseudoinstruction should be
    if ((line_label_found  && (where == 1)) ||
            (!line_label_found && (where == 0))) {
        //! \todo Check if this token might alternately represent a pseudo-instruction.
        HumanOpcode hopc = (_transcoder.stringToHumanOpcode(token));
        if (hopc == HumanOpcode::INVALID) {
            printf("classifytoken(): Invalid opcode: '%s'\n", tcstr);
            return ASMParserToken(ASMParserTokenType::INVALID, token);
        }

        printf("  opcode %s\n", tcstr);
        word_t hopcval = static_cast<word_t>(hopc);
        return ASMParserToken(ASMParserTokenType::OPCODE, token, hopcval);
    }

    // PARAMS!
    if ((line_label_found  && (where == 2)) ||
            (!line_label_found && (where == 1))) {
        auto splitParamTokens = splitstring(token, ',');
        std::vector<ASMParserParamToken> classifiedParamTokens;
        for (std::string ptoken : splitParamTokens) {
            classifiedParamTokens.push_back(classifyparam(ptoken));
        }

        printf("  param block %s\n", tcstr);
        ASMParserToken apt(ASMParserTokenType::PARAM_BLOCK, token, classifiedParamTokens);
        return apt;
    } else {
        ASMParserToken apt(ASMParserTokenType::NOT_NEEDED, "No Parameters Specified", {}, 0);
        return apt;
    }

    // All else invalid after these possible columns of text,
    // because this syntax requires params be strictly COMMA-ONLY delimited
    if ((line_label_found  && (where >= 3)) ||
            (!line_label_found && (where >= 2))) {
        printf("classifytoken(): No more tokens should reside on this line, but there are (%s)\n",
               token.c_str());
        return ASMParserToken(ASMParserTokenType::INVALID, token);
    } else {
        ASMParserToken apt(ASMParserTokenType::NOT_NEEDED, "No Parameters Specified", {}, 0);
        return apt;
    }

    printf ("  invalid (tail) %s\n", tcstr);
    return ASMParserToken(ASMParserTokenType::INVALID, token);
}

ASMParseLineResult_Pass1 VMAssembler::parseline(std::string line) {
    std::vector<ASMParserParamToken> paramTokens;

    /* formats for a line:

    0) label:   PSEUDOOPCODE [pseudoopcode's parameters]...
    1) [label:]       OPCODE [operand1],[operand2],[operand3] */

    /* PSEUDOOPCODES:

       ORG loc  : increment target VM rom-writer/burn() pointer to abs position 'loc'
       BYTE b1,[b2,b3,...,bn]  : write bytes to location
       WORD w1,[w2,w3,...,wn]  : write 2-byte words to location */

    // Step 1: break line into tokens, space-delimited
    std::vector<std::string> tokens = splitstring(line, ' ');

// ---------------------------------
    printf("---\n");
    for (auto i = tokens.begin(); i != tokens.end(); i++) {
        printf("T [%s]\n", (*i).c_str());
    }
    printf("\n");
// ----------------------------------

    ASMParserToken labeltoken = ASMParserToken(ASMParserTokenType::NOT_NEEDED, "LABEL_NOT_NEEDED"),
                   hopctoken,
                   paramtoken = ASMParserToken(ASMParserTokenType::NOT_NEEDED, "PARAMS_NOT_NEEDED");

    bool was_label_found = false;

    for (unsigned int i = 0; i < tokens.size(); i++) {
        const std::string& token = tokens[i];

        // make sure all characters in token are valid characters first
        if (!validateStringChars(token, valid_token_chars)) {
            printf("invalid chars in token '%s'\n", token.c_str());
            return {} ;
        }

        // ok, all characters valid... now investigate token more thoroughly
        ASMParserToken aptok = classifytoken(token, i, was_label_found);
        if (!aptok.isValid()) {
            printf("parseline(): Invalid token found: %s\n", aptok.getText().c_str());
            return {} ;
        } else {
            switch(aptok.getType()) {
            case ASMParserTokenType::LABEL:
                labeltoken = aptok;
                break;
            case ASMParserTokenType::PSEUDO_INSTRUCTION:
                throw "Derp";
                break;
            case ASMParserTokenType::OPCODE:
                hopctoken = aptok;
                break;
            case ASMParserTokenType::PARAM_BLOCK:
                paramtoken = aptok;
                break;
            default:
                return {};
            }
        }
        if (i == 0)
            was_label_found = (aptok.getType() == ASMParserTokenType::LABEL);
    }

    ASMParseLineResult_Pass1 result(hopctoken, paramtoken, labeltoken);
    this->_pass1tokens.push_back(result);
    return result;
}

void VMAssembler::parsetextblock(std::string text) {
    std::vector<std::string> lines = splitstring(text, '\n');

    for (std::string line : lines) {
        parseline(line);
    }
}

bool within(int i, int min, int max) {
    return ((i >= min) && (i<= max));
}

bool withinbyte(int i) {
    return
        (within(static_cast<word_t>(i), 0, 255) ||
         within(static_cast<sword_t>(i), -128, 127));
}

std::vector<OperandType> ASMParserToken::deducePossibleOperandTypes () const {
    typedef ASMParserParamType APT;
    typedef OperandType OT;
    char rep[4] = { 0, 0, 0, 0 };   // null-terminated string

    auto t = *this;
    if (this->getType() != ASMParserTokenType::PARAM_BLOCK)
        throw "deducePossibleOperandTypes was called on an ASMParserToken which does not hold a parameter block!";

    if (_paramtokens.size() == 0)
        return { OT::NIL };
    if (_paramtokens.size() > 3)
        return { OT::INVALID };   // should exception be thrown??

    // build string representation of parameter list
    for (unsigned int i = 0; i < _paramtokens.size(); i++) {
        auto& nthToken = _paramtokens[i];
        APT paramType = nthToken.getType();
        switch (paramType) {
        case APT::LITERAL_REGISTER:
            rep[i] = 'R';
            break;
        case APT::BRACKETED_REGISTER:
            rep[i] = 'P';
            break;
        case APT::LITERAL_LABEL:
            rep[i] = 'W';
            break;
        case APT::BRACKETED_NUMBER:    /* VVVVVVVVV */
        case APT::BRACKETED_LABEL:
            rep[i] = 'M';
            break;
        case APT::LITERAL_NUMBER: {
            int value = _paramtokens[i].getValue();
            rep[i] = (withinbyte(value)) ? 'B' : 'W';
            break;
        }
        case APT::INVALID:
            throw "Invalid";
            break;
        }
    }

    std::string srep = rep;

    printf("deduce: srep = %s\n", srep.c_str());

    if (srep.compare("RM") == 0) return { OT::RM };
    if (srep.compare("MR") == 0) return { OT::MR };
    if (srep.compare("RR") == 0) return { OT::RR };
    if (srep.compare("RP") == 0) return { OT::RW };
    if (srep.compare("PR") == 0) return { OT::RR };
    if (srep.compare("RRR") == 0) return { OT::RRR };
    if (srep.compare("RW") == 0) return { OT::RW };
    if (srep.compare("RB") == 0) return { OT::RB, OT::RW };
    if (srep.compare("M") == 0) return { OT::M };
    if (srep.compare("P") == 0) return { OT::P };  // remember this is [REGISTER], not a [LABEL]/label
    if (srep.compare("R") == 0) return { OT::R };
    if (srep.compare("BBB") == 0) return { OT::BBB };
    if (srep.compare("BB") == 0) return { OT::BB, OT::BW, OT::WB };
	if (srep.compare("W") == 0) return { OT::W };
    if (srep.compare("B") == 0) return { OT::B, OT::W };
    if (srep.compare("BW") == 0) return { OT::BW };
    if (srep.compare("WB") == 0) return { OT::WB };

    return { OT::INVALID };
}

bool VMAssembler::walkFirstPass(int start_rwp) {
    int simulatedRWP = start_rwp;
    for (ASMParseLineResult_Pass1 p1tok : _pass1tokens) {
        ASMParserToken labeltoken = p1tok.getLabelToken();

        if (labeltoken.getType() != ASMParserTokenType::NOT_NEEDED) {
            std::string label = labeltoken.getText();
            if (label.compare("") != 0) {
                if (_labeltable.find(label) != _labeltable.end()) {
                    printf("walkFirstPass(): label '%s' already defined!  Terminating.\n", label.c_str());
                    return false;
                }

                _labeltable[label] = simulatedRWP;
                printf("* found label '%s' at position %d\n", label.c_str(), simulatedRWP);
                p1tok.getLabelToken().setValue(simulatedRWP);
            }
        }

        // now figure out how much to increment simulatedRWP,
        // and/or catch incorrect Opcode:OperandType combinations attempted
        ASMParserToken hopcToken = p1tok.getHopcToken();

        // auto htotype = hopcToken.getType();
        HumanOpcode hopc = static_cast<HumanOpcode>(hopcToken.getValue());
		if (hopc == HumanOpcode::JMP)
			printf("Break\n");

        Opcode opc = Opcode::INVALID;
        ASMParserToken paramToken = p1tok.getParamToken();
        if (paramToken.getType() != ASMParserTokenType::NOT_NEEDED) {
            auto possibleOperandTypes = paramToken.deducePossibleOperandTypes();
            for (OperandType optype : possibleOperandTypes) {
                opc = _transcoder.getVMOpcodeFromHumanOpcode(hopc, optype);
                if (opc != Opcode::INVALID) {
                    int len = _transcoder.instructionLengthOfOperandType(optype);
                    simulatedRWP += len;
                    break;
                }
            }
        } else { // no-param opcode
            opc = _transcoder.getVMOpcodeFromHumanOpcode(hopc, OperandType::NIL);
        }

        // by now, if it's invalid, something went pooey
        if (opc == Opcode::INVALID) {
            printf("walkFirstPass(): could not find (Opcode,OperandType) match for\n");
            printf("      (HumanOpcode,PossibleOperandTypes) pairing\n");
            printf("      paramText: {%s}\n", p1tok.getParamToken().getText().c_str());
            return false;
        }
    }

    // now sweep tokens again. plopping VALUE onto any LABEL tokens where applicable
    if (_labeltable.size() > 0) {
        printf("\n* labels resolved:\n");
        for (auto keyval : _labeltable)
            printf("  [%s] = %d\n", keyval.first.c_str(), keyval.second);

        for (ASMParseLineResult_Pass1& p1tok : _pass1tokens) {
            // update the Label Token.  not REALLY needed or used by
            // anything but that could change in the distant future
            ASMParserToken* labeltoken = &(p1tok.getLabelToken());
            ASMParserTokenType tokentype = labeltoken->getType();
            if (tokentype == ASMParserTokenType::LABEL) {
                word_t labelValue = _labeltable[labeltoken->getText()];
                labeltoken->setValue(labelValue);
            }

            // this is the more important part for label resolving for
            // use in things like JMP, MOV [addr], etc
            ASMParserToken* paramtoken = &(p1tok.getParamToken());
            tokentype = paramtoken->getType();
            if (tokentype == ASMParserTokenType::PARAM_BLOCK) {
                for (ASMParserParamToken& parmtok : paramtoken->getParamTokens()) {
                    if ((parmtok.getType() == ASMParserParamType::LITERAL_LABEL) ||
                            (parmtok.getType() == ASMParserParamType::BRACKETED_LABEL)) {
                        parmtok.setValue(_labeltable[parmtok.getText()]);
                    }
                }
            }
        }
    }

    return true;
}

vm_instr_emit_info_t secondPassEmit(HumanOpcode hopc, OperandType opt, ASMParserToken& primeparamtok,
                                    const VMInstrEmitter& e) {
    int value[3] = { 0, 0, 0 };
    auto& paramtokens = primeparamtok.getParamTokens();
    if (paramtokens.size() > 3) {
        printf("secondPassEmit() error: paramtokens.size() > 3!!\n");
        return {};
    }

    for (int i = 0; i < paramtokens.size(); i++) {
        const ASMParserParamToken& ptok = paramtokens[i];
        value[i] = ptok.getValue();
    }

    typedef OperandType OT;
    switch(opt) {
    case OT::NIL:
        return e.emit(hopc);
    case OT::RM :
        return e.emit(hopc, static_cast<RegName>(value[0]), static_cast<addr_t> (value[1]));
    case OT::MR :
        return e.emit(hopc, static_cast<addr_t> (value[0]), static_cast<RegName>(value[1]));
    case OT::R  :
        return e.emit(hopc, static_cast<RegName>(value[0]));
    case OT::RR :
        return e.emit(hopc, static_cast<RegName>(value[0]), static_cast<RegName>(value[1]));
    case OT::RRR:
        return e.emit(hopc, static_cast<RegName>(value[0]), static_cast<RegName>(value[1]),
                      static_cast<RegName>(value[2]));
    case OT::RW :
        return e.emit(hopc, static_cast<RegName>(value[0]), static_cast<word_t> (value[1]));
    case OT::RB :
        return e.emit(hopc, static_cast<RegName>(value[0]), static_cast<byte_t> (value[1]));
    case OT::M  :
        return e.emit(hopc, static_cast<addr_t> (value[0]));
    case OT::P  :
        return e.emit(hopc, static_cast<RegName>(value[0]));
    case OT::BBB:
        return e.emit(hopc, static_cast<byte_t> (value[0]), static_cast<byte_t> (value[1]),
                      static_cast<byte_t> (value[2]));
    case OT::BB :
        return e.emit(hopc, static_cast<byte_t> (value[0]), static_cast<byte_t> (value[1]));
    case OT::B  :
        return e.emit(hopc, static_cast<byte_t> (value[0]));
    case OT::BW :
        return e.emit(hopc, static_cast<byte_t> (value[0]), static_cast<word_t> (value[0]));
    case OT::WB :
        return e.emit(hopc, static_cast<word_t> (value[0]), static_cast<byte_t> (value[1]));
    case OT::W  :
        return e.emit(hopc, static_cast<word_t> (value[0]));
    // handle possible junk, IF it ever happens then it means a bug trickled down to here
    case OT::NOT_AN_OPERAND:
    case OT::INVALID:
    case OT::NUM_OPERAND_TYPES:
        throw new VMEmitException("Junk OperandType passed to secondPassEmit", hopc);
    }
}

bool VMAssembler::walkSecondPass() {
    RobotVM& vm = *_targetVM;
    const VMInstrEmitter& e = dynamic_cast<const VMInstrEmitter&>(_transcoder);

    for (ASMParseLineResult_Pass1& p1tok : _pass1tokens) {
        const  ASMParserToken&  hopctok  = p1tok.getHopcToken();
        ASMParserToken&  paramtok = p1tok.getParamToken();

        const HumanOpcode hopc = static_cast<HumanOpcode>(hopctok.getValue());
        std::vector<OperandType> possibleots;
        if (paramtok.getType() == ASMParserTokenType::NOT_NEEDED)
            possibleots = { OperandType::NIL };
        else
            possibleots = paramtok.deducePossibleOperandTypes();

        for (OperandType ot : possibleots) {
            Opcode mopc = e.getVMOpcodeFromHumanOpcode(hopc, ot);
            if (mopc != Opcode::INVALID) {
                // yes, hopc is intentional, not mopc, since
                // VMInstrEmitter.emit takes HumanOpcode param.
                // this is merely last-minute validation
                vm_instr_emit_info_t emitinfo = secondPassEmit(hopc, ot, paramtok, e);

                vm.burn(emitinfo.instr);
                goto good;
            }
        }

        printf("walkSecondPass(): error! check in debugger\n");
        return false;
good: {}
    }

    return true;
}
