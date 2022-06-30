#include "stypes.hpp"

#include "bp.hpp"
#include "hw3_output.hpp"
#include "parser.tab.hpp"
#include "ralloc.hpp"
#include "symbolTable.hpp"

using namespace output;

extern SymbolTable symbolTable;

STypeC::STypeC(SymbolType symType) : symType(symType) {}

RetTypeNameC::RetTypeNameC(const string &type) : STypeC(STRetType), type(verifyRetTypeName(type)) {}

VarTypeNameC::VarTypeNameC(const string &type) : RetTypeNameC(verifyVarTypeName(type)) {}

ExpC::ExpC(const string &type, const string &regOrImmStr) : STypeC(STExpression), type(verifyValTypeName(type)), registerOrImmediate(regOrImmStr) {
    // TODO: Handle strings
    if (regOrImmStr[0] != '%' and type == "BYTE" and stoi(regOrImmStr) > 255) {
        errorByteTooLarge(yylineno, regOrImmStr);
    }
    if (type == "BOOL") {
        // Gen start label
        auto &buffer = CodeBuffer::instance();
        this->expStartLabel = buffer.genLabel("ExpCBoolStart");
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

const AddressList &ExpC::getFalseList() const {
    return this->boolFalseList;
}

const AddressList &ExpC::getTrueList() const {
    return this->boolTrueList;
}

string ExpC::getExpStartLabel() const {
    return this->expStartLabel;
}

/* Assures that the expression has a register with the result.
 *   To assure even short-circuit bool expressions (that don't have reg) are being assigned with result reg properly.
 */
string ExpC::assureAndGetRegResultOfExpression() {
    if (this->registerOrImmediate != "") {
        return this->registerOrImmediate;
    }
    if (not this->isBool()) {
        throw "ExpC without register must be bool";
    }
    auto &ralloc = Ralloc::instance();
    auto &buffer = CodeBuffer::instance();

    // Create a new register for the result
    string trueReg = ralloc.getNextReg();
    string falseReg = ralloc.getNextReg();
    string resultReg = ralloc.getNextReg();
    AddressList resLabelsList;

    // Backpatch true and false lists
    string trueLabel = buffer.genLabel("assureRegTrue");
    resLabelsList.push_back(make_pair(buffer.emit("br label @"), FIRST));

    string falseLabel = buffer.genLabel("assureRegFalse");
    resLabelsList.push_back(make_pair(buffer.emit("br label @"), FIRST));
    buffer.bpatch(this->boolTrueList, trueLabel);
    buffer.bpatch(this->boolFalseList, falseLabel);
    // Use phi to merge true and false registers
    string phiLabel = buffer.genLabel("assureRegPhi");
    buffer.emit(resultReg + " = phi i1 [true, " + trueLabel + "], [false, " + falseLabel + "]");
    buffer.bpatch(resLabelsList, phiLabel);
    this->registerOrImmediate = resultReg;
    return resultReg;
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
            // todo: define global string "Error division by zero"
            // todo: emit llvm code to dynamically check exp2 value == 0
            opStr = divOp;
            break;
        default:
            errorMismatch(yylineno);
    }

    codeBuffer.emit(resultReg + " = " + opStr + " " + resultSizeof + " " + exp1->assureAndGetRegResultOfExpression() + ", " + exp2->assureAndGetRegResultOfExpression());
    return shared_ptr<ExpC>(NEW(ExpC, (resultType, resultReg)));
}

static void insertToListsFromLists(AddressList &aListFrom, AddressList &aListTo,
                                   AddressList &bListFrom, AddressList &bListTo) {
    aListTo.insert(aListTo.end(), aListFrom.begin(), aListFrom.end());
    bListTo.insert(bListTo.end(), bListFrom.begin(), bListFrom.end());
}

shared_ptr<ExpC> ExpC::evalBool(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op) {
    shared_ptr<ExpC> exp1 = DC(ExpC, stype1);
    shared_ptr<ExpC> exp2 = DC(ExpC, stype2);
    shared_ptr<ExpC> resultExp;
    Ralloc &ralloc = Ralloc::instance();
    CodeBuffer &codeBuffer = CodeBuffer::instance();

    // exp2 == null  <==>  op == NOT
    if (not exp1 or (op == NOT) != (exp2 == nullptr)) {
        throw "evalBool must get _Nonnull as first param and the second is _Nullable iff op == NOT";
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
    if (exp1->registerOrImmediate != "") {
        exp1StartLabel = codeBuffer.genLabel("evalBoolExp1Start");
        instAddr = codeBuffer.emit("br i1 " + exp1->registerOrImmediate + ", label @, label @");
        exp1FirstList = CodeBuffer::makelist(make_pair(instAddr, FIRST));
        exp1SecondList = CodeBuffer::makelist(make_pair(instAddr, SECOND));
    } else {
        exp1StartLabel = exp1->expStartLabel;
        exp1FirstList = exp1->boolTrueList;
        exp1SecondList = exp1->boolFalseList;
    }

    if (exp2) {
        if (exp2->registerOrImmediate != "") {
            exp2StartLabel = codeBuffer.genLabel("evalBoolExp2Start");
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
                throw "Unsupported operation to evalBool with nonnull exp2";
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
            throw "Unsupported operation to evalBool";
    }
    resultExp = NEW(ExpC, ("BOOL", ""));
    resultExp->boolFalseList = falseList;
    resultExp->boolTrueList = trueList;
    resultExp->expStartLabel = exp1StartLabel;
    return resultExp;
}

shared_ptr<ExpC> ExpC::getCmpResult(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op) {
    shared_ptr<ExpC> exp1 = DC(ExpC, stype1);
    shared_ptr<ExpC> exp2 = DC(ExpC, stype2);
    if (not exp1 or not exp2) {
        throw "getCmpResult must get _Nonnull expressions";
    }

    string cmpOpStr;
    string regSizeofDecorator;

    if (not isImpliedCastAllowed(stype1, stype2)) {
        errorMismatch(yylineno);
        // Warning supression: the prev line will exit
        return nullptr;
    }

    Ralloc &ralloc = Ralloc::instance();
    CodeBuffer &codeBuffer = CodeBuffer::instance();

    string exp1RegOrImm = exp1->assureAndGetRegResultOfExpression();
    string exp2RegOrImm = exp2->assureAndGetRegResultOfExpression();

    if (exp1->isInt() or exp2->isInt()) {
        regSizeofDecorator = " i32 ";
        if (exp1->isByte() and exp1RegOrImm[0] == '%') {
            exp1RegOrImm = ralloc.getNextReg();
            codeBuffer.emit(exp1RegOrImm + " = zext i8 to i32 " + exp1RegOrImm[0]);
        } else if (exp2->isByte() and exp2RegOrImm[0] == '%') {
            exp2RegOrImm = ralloc.getNextReg();
            codeBuffer.emit(exp2RegOrImm + " = zext i8 to i32 " + exp2RegOrImm[0]);
        }
    } else {
        regSizeofDecorator = " i8 ";
    }

    switch (op) {
        case EQOP:
            cmpOpStr = " eq ";
            break;
        case NEOP:
            cmpOpStr = " ne ";
            break;
        case GEOP:
            cmpOpStr = " sge ";
            break;
        case GTOP:
            cmpOpStr = " sgt ";
            break;
        case LEOP:
            cmpOpStr = " sle ";
            break;
        case LTOP:
            cmpOpStr = " slt ";
            break;
        default:
            throw "Unsupported operation to getCmpResult";
    }

    string resultReg = ralloc.getNextReg();
    codeBuffer.emit(resultReg + " = icmp " + cmpOpStr + regSizeofDecorator + exp1RegOrImm);
    return NEW(ExpC, ("BOOL", resultReg));
}

shared_ptr<ExpC> ExpC::getCastResult(shared_ptr<STypeC> dstStype, shared_ptr<STypeC> expStype) {
    shared_ptr<ExpC> exp = DC(ExpC, expStype);

    if (not exp) {
        throw "getCastResult must get _Nonnull ExpC";
    }

    shared_ptr<VarTypeNameC> dstType = DC(VarTypeNameC, dstStype);

    if (not dstType) {
        throw "getCastResult must get a _Nonnull VarTypeNameC";
    }

    if (not areStrTypesCompatible(exp->getType(), dstType->getTypeName()) and not areStrTypesCompatible(dstType->getTypeName(), exp->getType())) {
        errorMismatch(yylineno);
        // Warning supression: the prev line will exit
        return nullptr;
    }

    // Make sure there's a result register or immediate that holds the result of the expression
    exp->assureAndGetRegResultOfExpression();

    Ralloc &ralloc = Ralloc::instance();
    CodeBuffer &codeBuffer = CodeBuffer::instance();
    string resultReg = ralloc.getNextReg();

    if (exp->isInt() and dstType->getTypeName() == "BYTE") {
        codeBuffer.emit(resultReg + " = trunc i32 " + exp->assureAndGetRegResultOfExpression() + " to i8");
    } else if (exp->isByte() and dstType->getTypeName() == "INT") {
        codeBuffer.emit(resultReg + " = zext i8 " + exp->assureAndGetRegResultOfExpression() + " to i32");
    } else {
        codeBuffer.emit(resultReg + " = add " + typeNameToLlvmType(exp->getType()) + " " + exp->assureAndGetRegResultOfExpression() + ", 0");
    }
    return NEW(ExpC, (dstType->getTypeName(), resultReg));
}

shared_ptr<ExpC> ExpC::getCallResult(shared_ptr<FuncIdC> funcId, shared_ptr<STypeC> argsStype) {
    vector<shared_ptr<ExpC>> args = STYPE2STD(vector<shared_ptr<ExpC>>, argsStype);
    auto &ralloc = Ralloc::instance();
    auto &buffer = CodeBuffer::instance();
    string llvmRetType = typeNameToLlvmType(funcId->getType());
    string resultReg = ralloc.getNextReg();
    string resultAssignment = llvmRetType == "void" ? "" : (resultReg + " = ");
    string expListStr = "";
    auto &formalsTypes = funcId->getArgTypes();

    if (formalsTypes.size() != args.size()) {
        errorPrototypeMismatch(yylineno, funcId->getName(), formalsTypes);
    }

    for (int i = 0; i < args.size(); i++) {
        if (args[i]->getType() != formalsTypes[i]) {
            errorPrototypeMismatch(yylineno, funcId->getName(), formalsTypes);
        }

        expListStr += typeNameToLlvmType(args[i]->getType()) + " " + args[i]->assureAndGetRegResultOfExpression() + ", ";
    }

    if (expListStr.size() > 0) {
        expListStr.pop_back();
        expListStr.pop_back();
    }

    buffer.emit(resultAssignment + "call " + llvmRetType + " @" + funcId->getName() + "(" + expListStr + ")");

    // This is valid as long as it doesn't derive from the Exp rule
    if (llvmRetType == "void") {
        return nullptr;
    }

    shared_ptr<ExpC> resultExp = NEW(ExpC, (funcId->getType(), resultReg));

    return resultExp;
}

shared_ptr<ExpC> ExpC::loadIdValue(shared_ptr<IdC> idSymbol, string stackVariablesPtrReg) {
    CodeBuffer &codeBuffer = CodeBuffer::instance();
    Ralloc &ralloc = Ralloc::instance();

    string llvmType = typeNameToLlvmType(idSymbol->getType());
    string offsetReg = ralloc.getNextReg();
    string idAddrReg = ralloc.getNextReg();
    string expReg = ralloc.getNextReg();

    codeBuffer.emit(offsetReg + " = i32 " + std::to_string(idSymbol->getOffset()));
    codeBuffer.emit(idAddrReg + " = getelementptr i32, i32* " + stackVariablesPtrReg + ", i32 " + offsetReg);
    codeBuffer.emit(expReg + " = load " + llvmType + ", ptr " + idAddrReg);

    shared_ptr<ExpC> idValue = NEW(ExpC, (idSymbol->getType(), expReg));

    return idValue;
}

shared_ptr<ExpC> ExpC::loadStringLiteralAddr(string literal) {
    auto &ralloc = Ralloc::instance();
    auto &codeBuffer = CodeBuffer::instance();
    string strLiteralAutoGeneratedName = ralloc.getNextVarName();
    string resultReg = ralloc.getNextReg();
    int literalLength = literal.length() + 1;  // + 1 for '\0'

    codeBuffer.emitGlobal(strLiteralAutoGeneratedName + " = constant [" +
                          std::to_string(literalLength) + " x i8] c\"" + literal + "\\00\"");

    codeBuffer.emit(resultReg + " = getelementptr [4 x i8], [4 x i8]* " +
                    strLiteralAutoGeneratedName + ", i32 0, i32 0");

    return NEW(ExpC, ("STRING", resultReg));
}

const string &ExpC::getType() const { return type; }

IdC::IdC(const string &varName, const string &type) : STypeC(STId), name(varName), type(verifyVarTypeName(type)) {}

const string &IdC::getName() const {
    return this->name;
}

const string &IdC::getType() const {
    return this->type;
}

void IdC::setOffset(Offset newOffset) {
    this->offset = newOffset;
}

Offset IdC::getOffset() const {
    return this->offset;
}

const string &IdC::getRegisterName() const {
    return this->registerName;
}

void IdC::setRegisterName(string registerName) {
    this->registerName = registerName;
}

CallC::CallC(const string &type, const string &symbol)
    : STypeC(STCall), type(verifyRetTypeName(type)), symbol(symbol) {}

// Convert vector<shared_ptr<IdC>> to vector<string> of just the types
static vector<string> getTypesFromIds(const vector<shared_ptr<IdC>> &ids) {
    vector<string> types;

    for (auto id : ids) {
        types.push_back(id->getType());
    }

    return types;
}

FuncIdC::FuncIdC(const string &name, const string &type, const vector<shared_ptr<IdC>> &formals, bool isPredefined)
    : IdC(name, "BAD_VIRTUAL_CALL"),
      argTypes(getTypesFromIds(formals)),
      mapFormalNameToReg(),
      retType(verifyRetTypeName(type)) {
    CodeBuffer &buffer = CodeBuffer::instance();
    Ralloc &ralloc = Ralloc::instance();
    string retTypeStr = typeNameToLlvmType(this->retType);

    string formalsStr = "";
    string regName = "";

    for (auto &formal : formals) {
        regName = typeNameToLlvmType(formal->getType());
        formalsStr += regName + ", ";
        this->mapFormalNameToReg[formal->getName()] = regName;
    }

    // Take substring without last comma
    if (formalsStr.size() > 0) {
        formalsStr.pop_back();
        formalsStr.pop_back();
    }

    if (isPredefined) return;

    buffer.emit("define " + retTypeStr + " @" + name + "(" + formalsStr.substr() + ") {");
    // I need to fix the hilighting of rainbow brackets so: }
    // Allocate space for 50 variables on the stack
    symbolTable.stackVariablesPtrReg = ralloc.getNextReg();
    buffer.emit("\t" + symbolTable.stackVariablesPtrReg + " = alloca i32, i8 50");
}

shared_ptr<FuncIdC> FuncIdC::startFuncIdWithScope(const string &name, shared_ptr<RetTypeNameC> type, const vector<shared_ptr<IdC>> &formals) {
    auto funcId = NEW(FuncIdC, (name, type->getTypeName(), formals));
    symbolTable.addSymbol(funcId);

    symbolTable.addScope(formals.size());
    for (auto i = 0; i < formals.size(); i++) {
        symbolTable.addFormal(formals[i]);
    }
    symbolTable.retType = type;
    return funcId;
}

void FuncIdC::endFuncIdScope() {
    symbolTable.removeScope();
    string retTypeLlvm = typeNameToLlvmType(symbolTable.retType->getTypeName());
    string defaultRetVal = retTypeLlvm + (retTypeLlvm == "void" ? "" : " 0");
    symbolTable.retType = nullptr;
    auto &codeBuffer = CodeBuffer::instance();
    codeBuffer.emit("ret " + defaultRetVal);
    // To balance rainbow brackets {
    codeBuffer.emit("}");
    codeBuffer.emit("");
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

string typeNameToLlvmType(const string &typeName) {
    if (typeName == "VOID") {
        return "void";
    } else if (typeName == "INT") {
        return "i32";
    } else if (typeName == "BYTE") {
        return "i8";
    } else if (typeName == "BOOL") {
        return "i1";
    } else if (typeName == "STRING") {
        return "i8*";
    } else {
        throw "Unsupported return type";
    }
}

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

static shared_ptr<ExpC> lastBoolExpC = nullptr;

void saveBoolExpC(shared_ptr<STypeC> boolExpStype) {
    verifyBoolType(boolExpStype);
    auto exp = DC(ExpC, boolExpStype);

    assert(exp->isBool());

    lastBoolExpC = exp;
}

shared_ptr<ExpC> getLastBoolExpC() {
    auto boolExp = lastBoolExpC;
    lastBoolExpC = nullptr;
    return boolExp;
}

// Handle if/loops open/close

void handleIfStart(shared_ptr<STypeC> conditionStype) {
    verifyBoolType(conditionStype);
    shared_ptr<ExpC> condition = DC(ExpC, conditionStype);
    auto &buffer = CodeBuffer::instance();

    string trueLabel = buffer.genLabel("ifStart");
    buffer.bpatch(condition->getTrueList(), trueLabel);
}

shared_ptr<STypeC> handleIfEnd(shared_ptr<STypeC> conditionStype, bool hasElse) {
    verifyBoolType(conditionStype);
    shared_ptr<ExpC> condition = DC(ExpC, conditionStype);
    auto &buffer = CodeBuffer::instance();
    shared_ptr<STypeC> brEndElseInstrStype = nullptr;

    if (hasElse) {
        AddressIndPair brEndElseInstr = make_pair(buffer.emit("br label @"), FIRST);
        brEndElseInstrStype = NEWSTD_V(AddressIndPair, (brEndElseInstr));
    }

    string falseLabel = buffer.genLabel("ifEnd");
    buffer.bpatch(condition->getFalseList(), falseLabel);

    return brEndElseInstrStype;
}

void handleElseEnd(shared_ptr<STypeC> endIfListStype) {
    AddressIndPair &endIfInstr = STYPE2STD(AddressIndPair, endIfListStype);
    auto &buffer = CodeBuffer::instance();

    string elseEndLabel = buffer.genLabel("elseEnd");

    buffer.bpatch(buffer.makelist(endIfInstr), elseEndLabel);
}

void handleWhileStart(shared_ptr<STypeC> conditionStype) {
    handleIfStart(conditionStype);
    symbolTable.startLoop(DC(ExpC, conditionStype)->getExpStartLabel());
}

void handleWhileEnd(shared_ptr<STypeC> conditionStype) {
    verifyBoolType(conditionStype);
    shared_ptr<ExpC> condition = DC(ExpC, conditionStype);
    string startLabel = condition->getExpStartLabel();

    auto &buffer = CodeBuffer::instance();

    buffer.emit("br label " + condition->getExpStartLabel());

    /* `endLoop` must be called after we emit the br to jump to the beginning of the
     * condition (becuase endLoop will generate the endLabel to which we jump from the falseList and break statements)
     */
    symbolTable.endLoop(condition->getFalseList());
}
