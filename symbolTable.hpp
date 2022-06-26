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
    map<string, shared_ptr<IdC> > symTbl;
    vector<int> scopeStartOffsets;
    vector<string> formals;
    vector<vector<string> > scopeSymbols;
    int currOffset;

   public:
    shared_ptr<RetTypeNameC> retType;
    int nestedLoopDepth;
    SymbolTable();
    ~SymbolTable();
    void addScope(int funcArgCount = 0);
    void removeScope();
    void addSymbol(const string &name, shared_ptr<IdC> type);
    void addFormal(shared_ptr<IdC> type);
    shared_ptr<IdC> getVarSymbol(const string &name);
    shared_ptr<FuncIdC> getFuncSymbol(const string &name, bool shouldError = true);
    void printSymbolTable();
};

void verifyMainExists(SymbolTable &symbolTable);

#endif