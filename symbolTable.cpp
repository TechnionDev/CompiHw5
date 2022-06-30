#include "symbolTable.hpp"

#include "hw3_output.hpp"
#include "parser.tab.hpp"

using namespace output;

using std::get;

SymbolTable::SymbolTable() {
    this->nestedLoopDepth = 0;
    this->currOffset = 0;
    this->addScope();
    this->addSymbol(NEW(FuncIdC, ("print", "VOID", vector<shared_ptr<IdC>>({NEW(IdC, ("msg", "STRING"))}))));
    this->addSymbol(NEW(FuncIdC, ("printi", "VOID", vector<shared_ptr<IdC>>({NEW(IdC, ("i", "INT"))}))));
    // Emit print functions' implementation
    auto &buffer = CodeBuffer::instance();
    buffer.emit("declare i32 @printf(i8*, ...)");
    buffer.emit("declare void @exit(i32)");
    buffer.emit("@.int_specifier = constant [4 x i8] c\"%d\0A\00\"");
    buffer.emit("@.str_specifier = constant [4 x i8] c\"%s\0A\00\"");
    buffer.emit("define void @printi(i32) {");
    buffer.emit("%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.int_specifier, i32 0, i32 0");
    buffer.emit("call i32 (i8*, ...) @printf(i8* %spec_ptr, i32 %0)");
    buffer.emit("ret void");
    buffer.emit("}");
    buffer.emit("define void @print(i8*) {");
    buffer.emit("%spec_ptr = getelementptr [4 x i8], [4 x i8]* @.str_specifier, i32 0, i32 0");
    buffer.emit("call i32 (i8*, ...) @printf(i8* %spec_ptr, i8* %0)");
    buffer.emit("ret void");
    buffer.emit("}");
}

SymbolTable::~SymbolTable() {}

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
        Offset offset = -1;

        for (int i = this->formals.size() - 1; i >= 0; i--) {
            printID(this->formals[i], offset--, this->symTbl[this->formals[i]]->getType());
            this->symTbl.erase(this->formals[i]);
        }

        this->formals.clear();
    } else {
        this->currOffset -= this->scopeSymbols.back().size();
    }

    Offset offset = this->scopeStartOffsets.back();

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

void SymbolTable::addSymbol(shared_ptr<IdC> type) {
    const string &name = type->getName();
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

    type->setOffset(this->currOffset);
    this->symTbl[name] = type;

    if (this->scopeStartOffsets.size() > 1) {
        this->currOffset++;
    }
}

void SymbolTable::addContinue() {
    auto &buffer = CodeBuffer::instance();
    buffer.emit("br label " + this->loopCondStartLabelStack.back());
}

void SymbolTable::addBreak() {
    auto &buffer = CodeBuffer::instance();
    AddressIndPair instruction = make_pair(buffer.emit("br label @"), FIRST);
    this->breakListStack.back().push_back(instruction);
}

void SymbolTable::startLoop(const string &loopCondStartLabel) {
    this->nestedLoopDepth++;
    this->loopCondStartLabelStack.push_back(loopCondStartLabel);
    this->breakListStack.push_back(vector<AddressIndPair>());
}

void SymbolTable::endLoop(const AddressList &falseList) {
    this->nestedLoopDepth--;

    auto &buffer = CodeBuffer::instance();
    string endLoopLabel = buffer.genLabel();
    buffer.bpatch(this->breakListStack.back(), endLoopLabel);
    buffer.bpatch(falseList, endLoopLabel);

    this->breakListStack.pop_back();
    this->loopCondStartLabelStack.pop_back();
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
    Offset offset = 0;
    for (auto it = this->symTbl.begin(); it != this->symTbl.end(); ++it) {
        printID(it->first, offset++, it->second->getType());
    }
}

// Helper functions

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
