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
    void endLoop(AddressList &falseList);
    // pair<AddressList, AddressList> getBreakAndContAddrLists();
    shared_ptr<IdC> getVarSymbol(const string &name);
    shared_ptr<FuncIdC> getFuncSymbol(const string &name, bool shouldError = true);
    void printSymbolTable();
    int getCurrentScopeDepth() const;
};

void verifyMainExists(SymbolTable &symbolTable);
void tryAddSymbolWithExp(SymbolTable &symbolTable, shared_ptr<STypeC> rawSymbol,
                         shared_ptr<STypeC> rawExp);
void addAutoSymbolWithExp(SymbolTable &symbolTable, shared_ptr<STypeC> rawId,
                          shared_ptr<STypeC> rawExp);
void tryAssignExp(SymbolTable &symbolTable, shared_ptr<STypeC> rawId, shared_ptr<STypeC> rawExp);

void emitAssign(shared_ptr<IdC> symbol, shared_ptr<ExpC> exp, string stackVariablesPtrReg);
void addUninitializedSymbol(SymbolTable &symbolTable, shared_ptr<STypeC> rawSymbol);
void handleReturn(shared_ptr<RetTypeNameC> retType);
void handleReturnExp(shared_ptr<RetTypeNameC> retType, shared_ptr<STypeC> rawExp);
void emitReturn(shared_ptr<RetTypeNameC> retType, shared_ptr<ExpC> exp);

#endif