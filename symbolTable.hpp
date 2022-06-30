#ifndef SYMBOL_TABLE_HPP_
#define SYMBOL_TABLE_HPP_

#include <map>
#include <memory>
#include <string>

#include "stypes.hpp"

using std::map;
using std::shared_ptr;
using std::string;

class SymbolTable {
    map<string, shared_ptr<IdC>> symTbl;
    vector<Offset> scopeStartOffsets;
    vector<string> formals;
    vector<vector<string>> scopeSymbols;

    // For loops
    vector<AddressList> breakListStack;
    vector<string> loopCondStartLabelStack;
    Offset currOffset;

   public:
    shared_ptr<RetTypeNameC> retType;
    string stackVariablesPtrReg;
    int nestedLoopDepth;
    SymbolTable();
    ~SymbolTable();
    void addScope(int funcArgCount = 0);
    void removeScope();
    void addSymbol(shared_ptr<IdC> type);
    void addFormal(shared_ptr<IdC> type);
    void addContinue();
    void addBreak();
    void startLoop(const string &loopCondStart);
    void endLoop(const AddressList &falseList);
    // pair<AddressList, AddressList> getBreakAndContAddrLists();
    shared_ptr<IdC> getVarSymbol(const string &name);
    shared_ptr<FuncIdC> getFuncSymbol(const string &name, bool shouldError = true);
    void printSymbolTable();
};

void verifyMainExists(SymbolTable &symbolTable);

#endif