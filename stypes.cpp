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

ExpC::ExpC(const string &type, const string &regOrImmStr) : STypeC(STExpression), type(verifyValTypeName(type)), registerOrImmediate(regOrImmStr) {
    if (regOrImmStr[0] != '%' and (type == "INT" or type == "BYTE") and stoi(regOrImmStr) > 255) {
        errorByteTooLarge(yylineno, regOrImmStr);
    }
}

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
            // TODO: Implement div by 0 error checking and handling
            opStr = divOp;
            break;
        default:
            errorMismatch(yylineno);
    }

    codeBuffer.emit(resultReg + " = " + opStr + " " + resultSizeof + " " + exp1->registerOrImmediate + ", " + exp2->registerOrImmediate);
    return shared_ptr<ExpC>(NEW(ExpC, (resultType, resultReg)));
}

void insertToListsFromLists(AddressList &aListFrom, AddressList &aListTo,
                            AddressList &bListFrom, AddressList &bListTo) {
    aListTo.insert(aListTo.end(), aListFrom.begin(), aListFrom.end());
    bListTo.insert(bListTo.end(), bListFrom.begin(), bListFrom.end());
}

shared_ptr<ExpC> ExpC::evalBoolExp(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op) {
    shared_ptr<ExpC> exp1 = DC(ExpC, stype1);
    shared_ptr<ExpC> exp2 = DC(ExpC, stype2);
    shared_ptr<ExpC> resultExp;
    Ralloc &ralloc = Ralloc::instance();
    CodeBuffer &codeBuffer = CodeBuffer::instance();

    // exp2 == null  <==>  op == NOT
    if (not exp1 or (op == NOT) != (exp2 == nullptr)) {
        throw "evalBoolExp must get _Nonnull as first param and the second is _Nullable iff op == NOT";
    }

    if (not exp1->isBool() or not exp2->isBool()) {
        errorMismatch(yylineno);
    }
    int instAddr;
    AddressList falseList;
    AddressList trueList;
    AddressList nextExpList;

    string exp1StartLabel = "";
    string exp2StartLabel = "";

    AddressList exp1FirstList;
    AddressList exp1SecondList;

    AddressList exp2FirstList;
    AddressList exp2SecondList;

    // Emit the llvm ir code
    if (exp1->registerOrImmediate[0] != "") {
        exp1StartLabel = codeBuffer.genLabel();
        instAddr = codeBuffer.emit("br i1 " + exp1->registerOrImmediate + ", label @, label @");
        exp1FirstList = CodeBuffer::makelist(make_pair(instAddr, FIRST));
        exp1SecondList = CodeBuffer::makelist(make_pair(instAddr, SECOND));
    } else {
        exp1StartLabel = exp1->expStartLabel;
        exp1FirstList = exp1->boolTrueList;
        exp1SecondList = exp1->boolFalseList;
    }

    if (exp2) {
        if (exp2->registerOrImmediate[0] != "") {
            exp2StartLabel = codeBuffer.genLabel();
            instAddr = codeBuffer.emit("br i1 " + exp2->registerOrImmediate + ", label @, label @");
            exp2FirstList = CodeBuffer::makelist(make_pair(instAddr, FIRST));
            exp2SecondList = CodeBuffer::makelist(make_pair(instAddr, SECOND));
        } else {
            exp2StartLabel = exp2->expStartLabel;
            exp2FirstList = exp2->boolTrueList;
            exp2SecondList = exp2->boolFalseList;
        }

        // Decide on the integration between backpatch lists of first condiction (AND/OR)
        switch (op) {
            case OR:
                insertToListsFromLists(trueList, exp1FirstList,
                                       nextExpList, exp1SecondList);

                break;
            case AND:
                insertToListsFromLists(nextExpList, exp1FirstList,
                                       falseList, exp1SecondList);
                break;
            default:
                throw "Unsupported operation to evalBoolExp with nonnull exp2";
        }
    }

    // Decide on the integration between backpatch lists of first condiction (AND/OR)
    switch (op) {
        case NOT:
            insertToListsFromLists(falseList, exp1FirstList,
                                   trueList, exp1SecondList);

            break;
        case AND:
        case OR:
            codeBuffer.bpatch(nextExpList, exp2StartLabel);

            insertToListsFromLists(trueList, exp2FirstList,
                                   falseList, exp2SecondList);
            break;
        default:
            throw "Unsupported operation to evalBoolExp";
    }
    resultExp = NEW(ExpC, ("BOOL", ""));
    resultExp->boolFalseList = falseList;
    resultExp->boolTrueList = trueList;
    resultExp->expStartLabel = exp1StartLabel;
    return resultExp;
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
