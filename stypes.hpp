#ifndef STYPES_H_
#define STYPES_H_

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "bp.hpp"

using std::exception;
using std::map;
using std::shared_ptr;
using std::string;
using std::vector;

typedef enum {
    STStatements,
    STStatement,
    STExpression,
    STId,
    STCall,
    STString,
    STStd,
    STRetType
} SymbolType;

typedef int Offset;

const string &verifyAllTypeNames(const string &type);
const string &verifyValTypeName(const string &type);
const string &verifyRetTypeName(const string &type);
const string &verifyVarTypeName(const string &type);

struct Exception : public std::exception {
    string s;
    Exception(string ss) : s(ss) {}
    ~Exception() throw() {}  // Updated
    const char *what() const throw() { return s.c_str(); }
};

class STypeC {
    SymbolType symType;

   public:
    STypeC(SymbolType symType);
    virtual ~STypeC() = default;
};

typedef shared_ptr<STypeC> STypePtr;

class RetTypeNameC : public STypeC {
    string type;

   public:
    const string &getTypeName() const;
    RetTypeNameC(const string &type);
};

class VarTypeNameC : public RetTypeNameC {
   public:
    VarTypeNameC(const string &type);
};

class IdC : public STypeC {
    string name;
    string type;
    string registerName;

   public:
    Offset offset;

    IdC(const string &varName, const string &type);
    const string &getName() const;
    virtual const string &getType() const;
    void setOffset(Offset offset);
    Offset getOffset() const;
    const string &getRegisterName() const;
    void setRegisterName(string registerName);
};

class FuncIdC : public IdC {
    vector<string> argTypes;
    map<string, string> mapFormalNameToReg;
    string retType;

   public:
    FuncIdC(const string &name, const string &type, const vector<shared_ptr<IdC>> &formals, bool isPredefined = false);
    const vector<string> &getArgTypes() const;
    vector<string> &getArgTypes();
    const string &getType() const;
    // Create FuncIdC with opening a scope
    static shared_ptr<FuncIdC> startFuncIdWithScope(const string &name, shared_ptr<RetTypeNameC> type, const vector<shared_ptr<IdC>> &formals);
    static void endFuncIdScope();
};

class ExpC : public STypeC {
    string type;
    string registerOrImmediate;

    // string expStartLabel;

   public:
    ExpC(const string &type, const string &reg);
    const string &getType() const;
    bool isInt() const;
    bool isBool() const;
    bool isString() const;
    bool isByte() const;

    // string getExpStartLabel() const;
    string getRegOrImmResult();

    // Get result of bin operation on two expressions
    static shared_ptr<ExpC> getBinOpResult(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op);
    // Get shared_ptr<ExpC> by casting exp from srcType to dstType
    static shared_ptr<ExpC> getCastResult(shared_ptr<STypeC> dstStype, shared_ptr<STypeC> expStype);
    // Get shared_ptr<ExpC> from a function call
    static shared_ptr<ExpC> getCallResult(shared_ptr<FuncIdC> funcIdStype, shared_ptr<STypeC> argsStype);
    // Get shared_ptr<ExpC> from variable ID
    static shared_ptr<ExpC> loadIdValue(shared_ptr<IdC> idSymbol, string stackVariablesPtrReg);
    // Get shared_ptr<ExpC> from string literal
    static shared_ptr<ExpC> loadStringLiteralAddr(string literal);
    // Get shared_ptr<ExpC> from the result of comparing this and otherScExp
    static shared_ptr<ExpC> getCmpResult(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op);
};

class ShortCircuitBool : public ExpC, public std::enable_shared_from_this<ShortCircuitBool> {
    // Used only by bool expressions
    AddressList boolFalseList;
    AddressList boolTrueList;

   public:
    ShortCircuitBool(const AddressList &trueList, const AddressList &falseList);
    ShortCircuitBool(shared_ptr<ExpC> boolExp);

    // Short-Circuit eval bool value
    shared_ptr<ShortCircuitBool> evalBool(shared_ptr<STypeC> otherScExpStype, shared_ptr<STypeC> rightOperandStartStype, int op);

    AddressList &getFalseList();
    AddressList &getTrueList();
    shared_ptr<ExpC> finallizeToExpC();

    // Ansure there's a true and false lists. If either is zero, force concrete value for the expression and create conditional jump and add placeholders to falselist
    // void ensureTrueFalseList();
};

class CallC : public STypeC {
    string type;
    string symbol;

   public:
    CallC(const string &type, const string &symbol);
    const string &getType() const;
};

template <typename T>
class StdType : public STypeC {
    T value;

   public:
    StdType(T value) : STypeC(STStd), value(value){};
    const T &getValue() const { return value; };
    T &getValue() { return value; };
    // const T &operator()() const { return value; };
    // T &operator()() { return value; };
};

// helper functions:
shared_ptr<STypeC> handleExpList(shared_ptr<STypeC> exp, shared_ptr<STypeC> list);
bool isImpliedCastAllowed(shared_ptr<STypeC> rawExp1, shared_ptr<STypeC> rawExp2);
bool areStrTypesCompatible(const string &typeStr1, const string &typeStr2);
void verifyBoolType(shared_ptr<STypeC> exp);
string typeNameToLlvmType(const string &typeName);
void handleIfStart(shared_ptr<STypeC> conditionStype);
shared_ptr<STypeC> handleIfEnd(shared_ptr<STypeC> conditionStype, bool hasElse = false);
void handleElseEnd(shared_ptr<STypeC> endIfListStype);
void handleWhileStart(shared_ptr<STypeC> conditionStype, string startLabel);
void handleWhileEnd(shared_ptr<STypeC> endIfListStype);
// Retain last bool ExpC in a static variable for later use
shared_ptr<ShortCircuitBool> saveScBool(shared_ptr<STypeC> boolExpStype);
shared_ptr<ShortCircuitBool> getLastScBool();
shared_ptr<ExpC> assureExpC(shared_ptr<STypeC> expStype);
shared_ptr<ShortCircuitBool> assureScBool(shared_ptr<STypeC> scBoolStype);

#define YYSTYPE STypePtr
#define NEW(x, y) (std::shared_ptr<x>(new x y))
#define NEWSTD(x) (std::shared_ptr<StdType<x>>(new StdType<x>(x())))
#define NEWSTD_V(x, y) (std::shared_ptr<StdType<x>>(new StdType<x>(x y)))
#define STYPE2STD(t, x) (dynamic_pointer_cast<StdType<t>>(x)->getValue())
#define DC(t, x) (dynamic_pointer_cast<t>(x))
#define VECS(x) STYPE2STD(vector<string>, x)

#endif