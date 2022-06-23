#include "stypes.hpp"

#include "hw3_output.hpp"
#include "parser.tab.hpp"
#include "ralloc.hpp"

using namespace output;
using std::reverse;

STypeC::STypeC(SymbolType symType) : symType(symType) {}

const string &verifyAllTypeNames(const string &type) {
    if (type == "INT" or type == "BOOL" or type == "BYTE" or type == "VOID" or type == "STRING" or type == "BAD_VIRTUAL_CALL") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
}

const string &verifyValTypeName(const string &type) {
    if (verifyAllTypeNames(type) != "VOID") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
}

const string &verifyRetTypeName(const string &type) {
    if (verifyAllTypeNames(type) != "STRING") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
}

const string &verifyVarTypeName(const string &type) {
    if (verifyAllTypeNames(type) != "VOID") {
        return type;
    } else {
        errorMismatch(yylineno);
    }
}

RetTypeNameC::RetTypeNameC(const string &type) : STypeC(STRetType), type(verifyRetTypeName(type)) {}

VarTypeNameC::VarTypeNameC(const string &type) : RetTypeNameC(verifyVarTypeName(type)) {}

ExpC::ExpC(const string &type) : STypeC(STExpression), type(verifyValTypeName(type), reg("")) {}

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
    shared_ptr<ExpC> exp1 = DC(ExpC, rawExp1);
    shared_ptr<ExpC> exp2 = DC(ExpC, rawExp2);
    shared_ptr<ExpC> resultExp;
    string resultType;
    string opStr;
    string resultReg = Ralloc::getInstance().getNextReg();

    if (not isImpliedCastAllowed(stype1, stype2)) {
        errorMismatch(yylineno);
    }

    resultType = (exp1->isInt() or exp2->isInt()) ? "INT" : "BYTE";

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
            // BYTE is upcasted to INT
            opStr = resultType == "INT" ? "sdiv" : "udiv";
            break;
        default:
            errorMismatch(yylineno);
    }
    

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

SymbolTable::SymbolTable() {
    this->nestedLoopDepth = 0;
    this->currOffset = 0;
    this->addScope();
    this->addSymbol("print", NEW(FuncIdC, ("print", "VOID", vector<string>({"STRING"}))));
    this->addSymbol("printi", NEW(FuncIdC, ("printi", "VOID", vector<string>({"INT"}))));
}
SymbolTable::~SymbolTable() {
}

void SymbolTable::addScope(int funcArgCount) {
    if (not((funcArgCount >= 0 and this->scopeStartOffsets.size() == 1) or (this->scopeStartOffsets.size() > 1 and funcArgCount == 0) or this->scopeStartOffsets.size() == 0)) {
        throw "Code error. We should only add a scope of a function when we are in the global scope";
    }
    this->scopeSymbols.push_back(vector<string>());
    this->scopeStartOffsets.push_back(this->currOffset);
}

void SymbolTable::removeScope() {
    endScope();

    string funcTypeStr;
    shared_ptr<FuncIdC> funcId;
    vector<string> argTypes;
    // If we are going back into the global scope
    if (this->scopeSymbols.size() == 2) {
        this->currOffset = 0;
        // For each string in the last scope, remove it from the symbol table
        int offset = -1;
        for (int i = this->formals.size() - 1; i >= 0; i--) {
            printID(this->formals[i], offset--, this->symTbl[this->formals[i]]->getType());
            this->symTbl.erase(this->formals[i]);
        }
        this->formals.clear();
    } else {
        this->currOffset -= this->scopeSymbols.back().size();
    }
    int offset = this->scopeStartOffsets.back();
    for (string s : this->scopeSymbols.back()) {
        if ((funcId = DC(FuncIdC, this->symTbl[s])) != nullptr) {
            funcTypeStr = makeFunctionType(funcId->getType(), funcId->getArgTypes());
            printID(s, offset, funcTypeStr);
        } else {
            printID(s, offset++, this->symTbl[s]->getType());
        }
        this->symTbl.erase(s);
    }

    scopeSymbols.pop_back();
    scopeStartOffsets.pop_back();
}

const string &RetTypeNameC::getTypeName() const {
    return this->type;
}

void SymbolTable::addFormal(shared_ptr<IdC> type) {
    if (type == nullptr) {
        throw "Can't add a nullptr formal to the symbol table";
    }
    if (this->symTbl[type->getName()] != nullptr) {
        errorDef(yylineno, type->getName());
    }
    this->formals.push_back(type->getName());
    this->symTbl[type->getName()] = type;
}

void SymbolTable::addSymbol(const string &name, shared_ptr<IdC> type) {
    // Check that the symbol doesn't exist in the scope yet
    if (type == nullptr) {
        throw "Can't add a nullptr symbol to the symbol table";
    }
    if (type->getType() == "STRING") {
        errorMismatch(yylineno);
    }

    if (this->symTbl[name] != nullptr) {
        errorDef(yylineno, name);
    }
    this->scopeSymbols.back().push_back(name);
    this->symTbl[name] = type;

    if (this->scopeStartOffsets.size() > 1) {
        this->currOffset++;
    }
}

shared_ptr<IdC> SymbolTable::getVarSymbol(const string &name) {
    auto symbol = this->symTbl[name];

    // Check that the symbol exists in the symbol table
    if (symbol == nullptr or DC(FuncIdC, symbol) != nullptr) {
        errorUndef(yylineno, name);
    }

    return symbol;
}

shared_ptr<FuncIdC> SymbolTable::getFuncSymbol(const string &name, bool shouldError) {
    auto symbol = this->symTbl[name];

    // Check that the symbol exists in the symbol table
    shared_ptr<FuncIdC> funcSym = nullptr;

    if ((symbol == nullptr or (funcSym = DC(FuncIdC, symbol)) == nullptr) and shouldError) {
        errorUndefFunc(yylineno, name);
    }

    return funcSym;
}

void SymbolTable::printSymbolTable() {
    int offset = 0;
    for (auto it = this->symTbl.begin(); it != this->symTbl.end(); ++it) {
        printID(it->first, offset++, it->second->getType());
    }
}

// helper functions:

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

void verifyMainExists(SymbolTable &symbolTable) {
    auto token = yylex();
    if (token) {
    }
    auto mainFunc = symbolTable.getFuncSymbol("main", false);

    if (mainFunc == nullptr or mainFunc->getType() != "VOID" or mainFunc->getArgTypes().size() != 0) {
        errorMainMissing();
    }

    symbolTable.removeScope();
}
