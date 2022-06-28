#ifndef STYPES_H_
#define STYPES_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

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
typedef shared_ptr<STypeC> STypePtr;

const string &verifyAllTypeNames(const string &type);
const string &verifyValTypeName(const string &type);
const string &verifyRetTypeName(const string &type);
const string &verifyVarTypeName(const string &type);

class STypeC {
    SymbolType symType;

   public:
    STypeC(SymbolType symType);
    virtual ~STypeC() = default;
};

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

class ExpC : public STypeC {
    string type;
    string registerOrImmediate;

    // Used only by bool expressions
    AddressList boolFalseList;
    AddressList boolTrueList;
    string expStartLabel;

   public:
    ExpC(const string &type, const string &reg);
    const string &getType() const;
    bool isInt() const;
    bool isBool() const;
    bool isString() const;
    bool isByte() const;
    // Get result of bin operation on two expressions
    static shared_ptr<ExpC> getBinOpResult(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op);
    // Short-Circuit eval bool value
    static shared_ptr<ExpC> evalBool(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op);
    // Get shared_ptr<ExpC> from the result of comparing exp1 and exp2
    static shared_ptr<ExpC> getCmpResult(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op);
    // Get shared_ptr<ExpC> by casting exp from srcType to dstType
    static shared_ptr<ExpC> getCastResult(shared_ptr<STypeC> dstStype, shared_ptr<STypeC> expStype);
    // Get shared_ptr<ExpC> from a function call
    static shared_ptr<ExpC> getCallResult(shared_ptr<FuncIdC> funcIdStype, shared_ptr<STypeC> argsStype);
    // Get shared_ptr<ExpC> from variable ID
    static shared_ptr<ExpC> loadIdValue(shared_ptr<IdC> idSymbol);
    // Get shared_ptr<ExpC> from string literal
    static shared_ptr<ExpC> loadStringLiteralAddr(string literal);
};

class IdC : public STypeC {
    string name;
    string type;

   public:
    Offset offset;

    IdC(const string &varName, const string &type);
    const string &getName() const;
    virtual const string &getType() const;
    void setOffset(Offset offset);
    Offset getOffset() const;
};

class FuncIdC : public IdC {
    vector<string> argTypes;
    string retType;

   public:
    FuncIdC(const string &name, const string &type, const vector<shared_ptr<IdC>> &formals);
    const vector<string> &getArgTypes() const;
    vector<string> &getArgTypes();
    const string &getType() const;
    // Create FuncIdC with opening a scope
    static shared_ptr<FuncIdC> startFuncIdWithScope(const string &name, const string &type, const vector<shared_ptr<IdC>> &formals);
    static void endFuncIdScope();
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
bool isImpliedCastAllowed(shared_ptr<STypeC> rawExp1, shared_ptr<STypeC> rawExp2);
bool areStrTypesCompatible(const string &typeStr1, const string &typeStr2);
void verifyBoolType(shared_ptr<STypeC> exp);
string typeNameToLlvmType(const string &typeName);

#define YYSTYPE STypePtr
#define NEW(x, y) (std::shared_ptr<x>(new x y))
#define NEWSTD(x) (std::shared_ptr<StdType<x>>(new StdType<x>(x())))
#define NEWSTD_V(x, y) (std::shared_ptr<StdType<x>>(new StdType<x>(x y)))
#define STYPE2STD(t, x) (dynamic_pointer_cast<StdType<t>>(x)->getValue())
#define DC(t, x) (dynamic_pointer_cast<t>(x))
#define VECS(x) STYPE2STD(vector<string>, x)

#endif