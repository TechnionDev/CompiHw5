#include "stypes.hpp"

#include "bp.hpp"
#include "hw3_output.hpp"
#include "parser.tab.hpp"
#include "ralloc.hpp"

using namespace output;

STypeC::STypeC(SymbolType symType) : symType(symType) {}

const string &verifyAllTypeNames(const string &type) {
    if (type == "INT" or type == "BOOL" or type == "BYTE" or type == "VOID" or type == "STRING" or type == "BAD_VIRTUAL_CALL") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
    // Never reaches here because errorMismatch exits
    return type;
}

const string &verifyValTypeName(const string &type) {
    if (verifyAllTypeNames(type) != "VOID") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
    // Never reaches here because errorMismatch exits
    return type;
}

const string &verifyRetTypeName(const string &type) {
    if (verifyAllTypeNames(type) != "STRING") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
    // Never reaches here because errorMismatch exits
    return type;
}

const string &verifyVarTypeName(const string &type) {
    if (verifyAllTypeNames(type) != "VOID") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
    // Never reaches here because errorMismatch exits
    return type;
}

RetTypeNameC::RetTypeNameC(const string &type) : STypeC(STRetType), type(verifyRetTypeName(type)) {}

VarTypeNameC::VarTypeNameC(const string &type) : RetTypeNameC(verifyVarTypeName(type)) {}

ExpC::ExpC(const string &type, const string &regOrImmStr) : STypeC(STExpression), type(verifyValTypeName(type)), registerOrImmediate(regOrImmStr) {}

bool ExpC::isInt() const {
    return this->type == "INT";
}

bool ExpC::isBool() const {
    return this->type == "BOOL";
}

bool ExpC::isString() const {
    return this->type == "STRING";
}

bool ExpC::isByte() const {
    return this->type == "BYTE";
}

shared_ptr<ExpC> ExpC::getBinOpResult(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op) {
    shared_ptr<ExpC> exp1 = DC(ExpC, stype1);
    shared_ptr<ExpC> exp2 = DC(ExpC, stype2);
    shared_ptr<ExpC> resultExp;
    Ralloc &ralloc = Ralloc::instance();
    CodeBuffer &codeBuffer = CodeBuffer::instance();
    string resultSizeof;
    string resultType;
    string divOp;
    string opStr;
    string resultReg = ralloc.getNextReg();

    if (not isImpliedCastAllowed(stype1, stype2)) {
        errorMismatch(yylineno);
    }
    if (exp1->isInt() or exp2->isInt()) {
        resultSizeof = "i32";
        resultType = "INT";
        // BYTE is upcasted to INT
        divOp = "sdiv";
    } else {
        resultSizeof = "i8";
        resultType = "BYTE";
        divOp = "udiv";
    }

    // Emit the llvm ir code
    switch (op) {
        case ADDOP:
            opStr = "add";
            break;
        case SUBOP:
            opStr = "sub";
            break;
        case MULOP:
            opStr = "mul";
            break;
        case DIVOP:
            opStr = divOp;
            break;
        default:
            errorMismatch(yylineno);
    }

    codeBuffer.emit(resultReg + " = " + opStr + " " + resultSizeof + " " + exp1->registerOrImmediate + ", " + exp2->registerOrImmediate);
    return shared_ptr<ExpC>(NEW(ExpC, (resultType, resultReg)));
}

const string &ExpC::getType() const { return type; }

IdC::IdC(const string &varName, const string &type) : STypeC(STId), name(varName), type(verifyVarTypeName(type)) {}

const string &IdC::getName() const {
    return this->name;
}

const string &IdC::getType() const {
    return this->type;
}

CallC::CallC(const string &type, const string &symbol) : STypeC(STCall), type(verifyRetTypeName(type)), symbol(symbol) {}

FuncIdC::FuncIdC(const string &name, const string &type, const vector<string> &argTypes) : IdC(name, "BAD_VIRTUAL_CALL"), argTypes(argTypes) {
    this->retType = verifyRetTypeName(type);
}

const string &FuncIdC::getType() const {
    return this->retType;
}

const vector<string> &FuncIdC::getArgTypes() const {
    return this->argTypes;
}
vector<string> &FuncIdC::getArgTypes() {
    return this->argTypes;
}

const string &RetTypeNameC::getTypeName() const {
    return this->type;
}

// Helper functions

bool isImpliedCastAllowed(shared_ptr<STypeC> rawExp1, shared_ptr<STypeC> rawExp2) {
    auto exp1 = DC(ExpC, rawExp1);
    auto exp2 = DC(ExpC, rawExp2);

    bool isExp1IntOrByte = exp1->isByte() or exp1->isInt();
    bool isExp2IntOrByte = exp2->isByte() or exp2->isInt();

    bool canCastImplicitly = isExp1IntOrByte and isExp2IntOrByte;

    return canCastImplicitly;
}

bool areStrTypesCompatible(const string &typeStr1, const string &typeStr2) {
    bool areEqual = typeStr1 == typeStr2;
    bool areIntAndByte = typeStr1 == "INT" and typeStr2 == "BYTE";

    bool canCastImplicitly = areEqual or areIntAndByte;

    return canCastImplicitly;
}

void verifyBoolType(shared_ptr<STypeC> exp) {
    auto expType = DC(ExpC, exp);
    if (not expType->isBool()) {
        errorMismatch(yylineno);
    }
}
