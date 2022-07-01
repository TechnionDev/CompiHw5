#include "stypes.hpp"

#include "bp.hpp"
#include "hw3_output.hpp"
#include "parser.tab.hpp"
#include "ralloc.hpp"
#include "symbolTable.hpp"

using namespace output;

extern SymbolTable symbolTable;

template <typename Base, typename T>
inline bool instanceof (const T *ptr) {
    return dynamic_cast<const Base *>(ptr) != nullptr;
}

STypeC::STypeC(SymbolType symType) : symType(symType) {}

RetTypeNameC::RetTypeNameC(const string &type) : STypeC(STRetType), type(verifyRetTypeName(type)) {}

VarTypeNameC::VarTypeNameC(const string &type) : RetTypeNameC(verifyVarTypeName(type)) {}

ExpC::ExpC(const string &type, const string &regOrImmStr) : STypeC(STExpression), type(verifyValTypeName(type)), registerOrImmediate(regOrImmStr) {
    if (regOrImmStr[0] != '%' and type == "BYTE" and stoi(regOrImmStr) > 255) {
        errorByteTooLarge(yylineno, regOrImmStr);
    }

    if (type != "SC_BOOL" and regOrImmStr == "") {
        throw Exception("Only SC_BOOL can have no register");
    }

    // auto &buffer = CodeBuffer::instance();
    // this->expStartLabel = buffer.genLabel(type + "Start");
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

ShortCircuitBool::ShortCircuitBool(const AddressList &trueList, const AddressList &falseList)
    : ExpC("SC_BOOL", ""), boolTrueList(trueList), boolFalseList(falseList) {}

ShortCircuitBool::ShortCircuitBool(shared_ptr<ExpC> boolExp) : ShortCircuitBool(AddressList(), AddressList()) {
    if (boolExp == nullptr) {
        throw Exception("Can't convert nullptr ExpC to ShortCircuitBool");
    } else if (not boolExp->isBool()) {
        throw Exception("Can't convert non BOOL ExpC to ShortCircuitBool");
    }
    auto &buffer = CodeBuffer::instance();
    int instrAddr = buffer.emit("br i1 " + boolExp->getRegOrImmResult() + ", label @, label @");
    this->boolTrueList.push_back(make_pair(instrAddr, FIRST));
    this->boolFalseList.push_back(make_pair(instrAddr, SECOND));
}

AddressList &ShortCircuitBool::getFalseList() {
    // this->ensureTrueFalseList();
    return this->boolFalseList;
}

AddressList &ShortCircuitBool::getTrueList() {
    // this->ensureTrueFalseList();
    return this->boolTrueList;
}

/* Assures that the expression has a register with the result.
 *   To assure even short-circuit bool expressions (that don't have reg) are being assigned with result reg properly.
 */
string ExpC::getRegOrImmResult() {
    if (instanceof <ShortCircuitBool>(this)) {
        throw Exception("Trying to get RegOrImm for ShortCircuitBool");
    }
    if (this->registerOrImmediate == "") {
        throw Exception("ExpC without register or immediate");
    }
    return this->registerOrImmediate;
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
    string resultReg = ralloc.getNextReg("getBinOpResult");
    string exp1Reg = exp1->getRegOrImmResult();
    string exp2Reg = exp2->getRegOrImmResult();

    if (not isImpliedCastAllowed(stype1, stype2)) {
        errorMismatch(yylineno);
    }
    if (exp1->isInt() or exp2->isInt()) {
        if (exp1->isByte()) {
            string resultReg = ralloc.getNextReg("binOpResExp1");
            codeBuffer.emit(resultReg + " = zext i8 " + exp1Reg + " to i32");
            exp1Reg = resultReg;
        } else if (exp2->isByte()) {
            string resultReg = ralloc.getNextReg("binOpResExp1");
            codeBuffer.emit(resultReg + " = zext i8 " + exp2Reg + " to i32");
            exp2Reg = resultReg;
        }
        resultSizeof = "i32";
        resultType = "INT";
        // BYTE is upcasted to INT
        divOp = "sdiv";
    } else {
        resultSizeof = "i8";
        resultType = "BYTE";
        divOp = "udiv";
    }

    int instAddr, instAddr2;
    string ifShouldErrorDivBy0;
    string labelDivBy0;
    string labelNotDivBy0;

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
            ifShouldErrorDivBy0 = ralloc.getNextReg("divBy0icmp");

            codeBuffer.emit(ifShouldErrorDivBy0 + " = icmp eq " + typeNameToLlvmType(exp2->getType()) + " " + exp2->getRegOrImmResult() + ", 0");
            instAddr = codeBuffer.emit("br i1 " + ifShouldErrorDivBy0 + ", label @, label @");
            labelDivBy0 = codeBuffer.genLabel("labelDivBy0");
            codeBuffer.emit("call void @error_division_by_zero()");
            labelNotDivBy0 = codeBuffer.genLabel("labelNotDivBy0");

            codeBuffer.bpatch(make_pair(instAddr, FIRST), labelDivBy0);
            codeBuffer.bpatch(make_pair(instAddr, SECOND), labelNotDivBy0);
            opStr = divOp;
            break;
        default:
            errorMismatch(yylineno);
    }

    codeBuffer.emit(resultReg + " = " + opStr + " " + resultSizeof + " " + exp1Reg + ", " + exp2Reg);
    return shared_ptr<ExpC>(NEW(ExpC, (resultType, resultReg)));
}

static void insertToListFromList(AddressList &listTo, AddressList &listFrom) {
    listTo.insert(listTo.end(), listFrom.begin(), listFrom.end());
}
// static void insertToListsFromLists(AddressList &aListTo, AddressList &aListFrom,
//                                    AddressList &bListTo, AddressList &bListFrom) {
//     aListTo.insert(aListTo.end(), aListFrom.begin(), aListFrom.end());
//     bListTo.insert(bListTo.end(), bListFrom.begin(), bListFrom.end());
// }

shared_ptr<ShortCircuitBool> ShortCircuitBool::evalBool(shared_ptr<STypeC> otherScExpStype, shared_ptr<STypeC> rightOperandStartStype, int op) {
    auto &buffer = CodeBuffer::instance();
    shared_ptr<ShortCircuitBool> otherScExp;
    string secondOperandStartLabel;

    AddressList trueList;
    AddressList falseList;
    if (op == OR) {
        {  // AND/OR Common lines
            otherScExp = assureScBool(otherScExpStype);
            secondOperandStartLabel = STYPE2STD(string, rightOperandStartStype);
        }
        buffer.bpatch(this->boolFalseList, secondOperandStartLabel);
        this->boolFalseList = otherScExp->boolFalseList;
        insertToListFromList(this->boolTrueList, otherScExp->boolTrueList);
        return shared_from_this();
    } else if (op == AND) {
        {  // AND/OR Common lines
            otherScExp = assureScBool(otherScExpStype);
            secondOperandStartLabel = STYPE2STD(string, rightOperandStartStype);
        }
        buffer.bpatch(this->boolTrueList, secondOperandStartLabel);
        insertToListFromList(this->boolFalseList, otherScExp->boolFalseList);
        this->boolTrueList = otherScExp->boolTrueList;
        return shared_from_this();
    } else if (op == NOT) {
        falseList = this->boolTrueList;
        trueList = this->boolFalseList;
        this->boolTrueList = trueList;
        this->boolFalseList = falseList;
        return shared_from_this();
    } else {
        errorMismatch(yylineno);
        throw Exception("Impossible to reach here");
    }
}

shared_ptr<ExpC> ExpC::getCmpResult(shared_ptr<STypeC> stype1, shared_ptr<STypeC> stype2, int op) {
    shared_ptr<ExpC> exp1 = DC(ExpC, stype1);
    shared_ptr<ExpC> exp2 = DC(ExpC, stype2);
    if (not exp1 or not exp2) {
        throw Exception("getCmpResult must get _Nonnull expressions");
    }

    string cmpOpStr;
    string regSizeofDecorator;

    if (not isImpliedCastAllowed(stype1, stype2)) {
        errorMismatch(yylineno);
        // Warning supression: the prev line will exit
        return nullptr;
    }

    Ralloc &ralloc = Ralloc::instance();
    CodeBuffer &buffer = CodeBuffer::instance();

    string resultReg = ralloc.getNextReg("cmpOpRes");

    string exp1RegOrImm = exp1->getRegOrImmResult();
    string exp2RegOrImm = exp2->getRegOrImmResult();

    shared_ptr<ExpC> resultExp = NEW(ExpC, ("BOOL", resultReg));

    if (exp1->isInt() or exp2->isInt()) {
        regSizeofDecorator = " i32 ";
        if (exp1->isByte() and exp1RegOrImm[0] == '%') {
            string newReg = ralloc.getNextReg("cmpOpRegOrImmExp1");
            buffer.emit(newReg + " = zext i8 " + exp1RegOrImm + " to i32");
            exp1RegOrImm = newReg;
        } else if (exp2->isByte() and exp2RegOrImm[0] == '%') {
            string newReg = ralloc.getNextReg("cmpOpRegOrImmExp2");
            buffer.emit(newReg + " = zext i8 " + exp2RegOrImm + " to i32");
            exp2RegOrImm = newReg;
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
            throw Exception("Unsupported operation to getCmpResult");
    }

    buffer.emit(resultReg + " = icmp " + cmpOpStr + regSizeofDecorator + exp1RegOrImm + ", " + exp2RegOrImm);
    return resultExp;
}

shared_ptr<ExpC> ExpC::getCastResult(shared_ptr<STypeC> dstStype, shared_ptr<STypeC> expStype) {
    shared_ptr<ExpC> exp = DC(ExpC, expStype);

    if (not exp) {
        throw Exception("getCastResult must get _Nonnull ExpC");
    }

    shared_ptr<VarTypeNameC> dstType = DC(VarTypeNameC, dstStype);

    if (not dstType) {
        throw Exception("getCastResult must get a _Nonnull VarTypeNameC");
    }

    if (not areStrTypesCompatible(exp->getType(), dstType->getTypeName()) and not areStrTypesCompatible(dstType->getTypeName(), exp->getType())) {
        errorMismatch(yylineno);
        // Warning supression: the prev line will exit
        return nullptr;
    }

    // Make sure there's a result register or immediate that holds the result of the expression
    exp->getRegOrImmResult();

    Ralloc &ralloc = Ralloc::instance();
    CodeBuffer &codeBuffer = CodeBuffer::instance();
    string resultReg = ralloc.getNextReg("castRes");

    shared_ptr<ExpC> resultExp = NEW(ExpC, (dstType->getTypeName(), resultReg));

    if (exp->isInt() and dstType->getTypeName() == "BYTE") {
        codeBuffer.emit(resultReg + " = trunc i32 " + exp->getRegOrImmResult() + " to i8");
    } else if (exp->isByte() and dstType->getTypeName() == "INT") {
        codeBuffer.emit(resultReg + " = zext i8 " + exp->getRegOrImmResult() + " to i32");
    } else {
        codeBuffer.emit(resultReg + " = add " + typeNameToLlvmType(exp->getType()) + " " + exp->getRegOrImmResult() + ", 0");
    }
    codeBuffer.emit("; DEBUG: got cast result (" + dstType->getTypeName() + ") from " + exp->getType());
    return resultExp;
}

shared_ptr<ExpC> ExpC::getCallResult(shared_ptr<FuncIdC> funcId, shared_ptr<STypeC> argsStype) {
    vector<shared_ptr<ExpC>> args = STYPE2STD(vector<shared_ptr<ExpC>>, argsStype);
    auto &ralloc = Ralloc::instance();
    auto &buffer = CodeBuffer::instance();
    string llvmRetType = typeNameToLlvmType(funcId->getType());
    string resultReg = ralloc.getNextReg("callRes_" + funcId->getName());
    string resultAssignment = llvmRetType == "void" ? "" : (resultReg + " = ");
    string expListStr = "";
    auto &formalsTypes = funcId->getArgTypes();

    if (formalsTypes.size() != args.size()) {
        errorPrototypeMismatch(yylineno, funcId->getName(), formalsTypes);
    }

    string argReg;

    for (int i = 0; i < args.size(); i++) {
        // Check type compatibility
        if (not areStrTypesCompatible(formalsTypes[i], args[i]->getType())) {
            errorMismatch(yylineno);
            // Warning supression: the prev line will exit
            return nullptr;
        }

        argReg = args[i]->getRegOrImmResult();
        if (args[i]->getType() != formalsTypes[i]) {
            // zext to passing the argument
            string zextExpReg = ralloc.getNextReg("zextCallFuncArg" + to_string(i) + "_");
            buffer.emit(zextExpReg + " = zext " + typeNameToLlvmType(args[i]->getType()) + " " + argReg + " to " + typeNameToLlvmType(formalsTypes[i]));
            argReg = zextExpReg;
        }

        expListStr += typeNameToLlvmType(formalsTypes[i]) + " " + argReg + ", ";
    }

    if (expListStr.size() > 0) {
        expListStr.pop_back();
        expListStr.pop_back();
    }

    shared_ptr<ExpC> resultExp = nullptr;

    // This is valid as long as it doesn't derive from the Exp rule
    if (funcId->getType() != "VOID") {
        resultExp = NEW(ExpC, (funcId->getType(), resultReg));
    }

    buffer.emit(resultAssignment + "call " + llvmRetType + " @" + funcId->getName() + "(" + expListStr + ")");

    return resultExp;
}

shared_ptr<ExpC> ExpC::loadIdValue(shared_ptr<IdC> idSymbol, string stackVariablesPtrReg) {
    CodeBuffer &codeBuffer = CodeBuffer::instance();
    Ralloc &ralloc = Ralloc::instance();

    if (idSymbol->getRegisterName() != "") {
        return NEW(ExpC, (idSymbol->getType(), idSymbol->getRegisterName()));
    }

    string llvmType = typeNameToLlvmType(idSymbol->getType());
    string offsetReg = ralloc.getNextReg("loadIdOffset");
    string idAddrReg = ralloc.getNextReg("loadIdIdAddr");
    string expReg = ralloc.getNextReg("idVal_" + idSymbol->getName());
    shared_ptr<ExpC> idValueExpC = NEW(ExpC, (idSymbol->getType(), expReg));

    codeBuffer.emit(offsetReg + " = add i32 0, " + std::to_string(idSymbol->getOffset()));
    codeBuffer.emit(idAddrReg + " = getelementptr i32, i32* " + stackVariablesPtrReg + ", i32 " + offsetReg);
    string idAddrRegCorrectSize = idAddrReg;

    if (llvmType != "i32") {
        idAddrRegCorrectSize = ralloc.getNextReg("idAddrCorrect");
        codeBuffer.emit(idAddrRegCorrectSize + " = bitcast i32* " + idAddrReg + " to " + llvmType + "*");
    }

    codeBuffer.emit(expReg + " = load " + llvmType + ", " + llvmType + "* " + idAddrRegCorrectSize);

    return idValueExpC;
}

shared_ptr<ExpC> ExpC::loadStringLiteralAddr(string literal) {
    literal = literal.substr(1, literal.length() - 2);
    auto &ralloc = Ralloc::instance();
    auto &codeBuffer = CodeBuffer::instance();
    string strLiteralAutoGeneratedName = ralloc.getNextVarName();
    string resultReg = ralloc.getNextReg("loadStringLiteralResult");
    int literalLength = literal.length() + 1;  // + 1 for '\0'
    string literalLenStr = std::to_string(literalLength);

    codeBuffer.emitGlobal(strLiteralAutoGeneratedName + " = constant [" +
                          literalLenStr + " x i8] c\"" + literal + "\\00\"");

    codeBuffer.emit(resultReg + " = getelementptr [" + literalLenStr + " x i8], [" + literalLenStr + " x i8]* " +
                    strLiteralAutoGeneratedName + ", i32 0, i32 0");

    return NEW(ExpC, ("STRING", resultReg));
}

shared_ptr<ExpC> ShortCircuitBool::finallizeToExpC() {
    if (this->boolTrueList.size() == 0 or this->boolFalseList.size() == 0) {
        throw Exception("Can't finallizeToExpC ShortCircuitBool expression when true/false list is empty");
    }
    // Convert
    auto &ralloc = Ralloc::instance();
    auto &buffer = CodeBuffer::instance();

    // Create a new register for the result
    string resultReg = ralloc.getNextReg("finallizedScBool");
    shared_ptr<ExpC> resultExp = NEW(ExpC, ("BOOL", resultReg));
    AddressList resLabelsList;

    // Backpatch true and false lists
    string trueLabel = buffer.genLabel("finallizeScBoolTrue");
    resLabelsList.push_back(make_pair(buffer.emit("br label @"), FIRST));

    string falseLabel = buffer.genLabel("finallizeScBoolFalse");
    resLabelsList.push_back(make_pair(buffer.emit("br label @"), FIRST));
    buffer.bpatch(this->boolTrueList, trueLabel);
    buffer.bpatch(this->boolFalseList, falseLabel);
    // Use phi to merge true and false registers
    string phiLabel = buffer.genLabel("finallizeScBoolPhi");
    buffer.emit(resultReg + " = phi i1 [true, %" + trueLabel + "], [false, %" + falseLabel + "]");
    buffer.bpatch(resLabelsList, phiLabel);
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
        regName = ralloc.getNextReg("formal_" + formal->getName());
        formalsStr += typeNameToLlvmType(formal->getType()) + " " + regName + ", ";
        this->mapFormalNameToReg[formal->getName()] = regName;
        formal->setRegisterName(regName);
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
    symbolTable.stackVariablesPtrReg = ralloc.getNextReg("FuncIdCStackVarPtrReg");
    buffer.emit("\t" + symbolTable.stackVariablesPtrReg + " = alloca i32, i32 50");
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

static vector<shared_ptr<ExpC>> &assureExpCList(shared_ptr<STypeC> listStype) {
    if (listStype == nullptr) {
        throw Exception("nullptr can't be assured as ExpCList");
    }

    shared_ptr<StdType<vector<shared_ptr<ExpC>>>> expList = dynamic_pointer_cast<StdType<vector<shared_ptr<ExpC>>>>(listStype);
    if (expList == nullptr) {
        throw Exception("Expected list of expressions");
    }
    return expList->getValue();
}

shared_ptr<ExpC> assureExpC(shared_ptr<STypeC> expStype) {
    if (expStype == nullptr) {
        throw Exception("nullptr can't be assured as ExpC");
    }

    auto exp = DC(ExpC, expStype);
    if (expStype == nullptr) {
        throw Exception("Failed to assure ExpC from Stype");
    }
    if (exp->getType() == "SC_BOOL") {
        throw Exception("Got ExpC which is actually SC_BOOL. You probably don't want to use it like that");
    }

    return exp;
}

shared_ptr<STypeC> handleExpList(shared_ptr<STypeC> expStype, shared_ptr<STypeC> listStype) {
    if (listStype == nullptr) {
        listStype = NEWSTD(vector<shared_ptr<ExpC>>);
    }
    auto &list = assureExpCList(listStype);
    auto exp = assureExpC(expStype);
    list.push_back(exp);
    return listStype;
}

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
        throw Exception("Unsupported return type");
    }
}

const string &verifyAllTypeNames(const string &type) {
    if (type == "INT" or type == "BOOL" or type == "SC_BOOL" or type == "BYTE" or type == "VOID" or type == "STRING" or type == "BAD_VIRTUAL_CALL") {
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

static shared_ptr<ShortCircuitBool> lastScBool = nullptr;

shared_ptr<ShortCircuitBool> saveScBool(shared_ptr<STypeC> scBoolStype) {
    lastScBool = assureScBool(scBoolStype);
    return lastScBool;
}

shared_ptr<ShortCircuitBool> getLastScBool() {
    auto boolExp = lastScBool;
    lastScBool = nullptr;
    return boolExp;
}

// Handle if/loops open/close

shared_ptr<ShortCircuitBool> assureScBool(shared_ptr<STypeC> scBoolStype) {
    if (scBoolStype == nullptr) {
        throw Exception("nullptr can't be assured as ShortCircuitBool");
    }
    auto scBool = DC(ShortCircuitBool, scBoolStype);
    if (scBool == nullptr) {
        throw Exception("Can't assure ScBool");
    }

    return scBool;
}

void handleIfStart(shared_ptr<STypeC> scBoolStype) {
    auto scBool = assureScBool(scBoolStype);
    auto &buffer = CodeBuffer::instance();
    auto trueList = scBool->getTrueList();
    string trueLabel = buffer.genLabel("ifStatementStart");
    buffer.bpatch(trueList, trueLabel);
}

shared_ptr<STypeC> handleIfEnd(shared_ptr<STypeC> scBoolStype, bool hasElse) {
    auto scBool = assureScBool(scBoolStype);
    auto &buffer = CodeBuffer::instance();
    shared_ptr<STypeC> brEndElseInstrStype = nullptr;

    if (hasElse) {
        AddressIndPair brEndElseInstr = make_pair(buffer.emit("br label @"), FIRST);
        brEndElseInstrStype = NEWSTD_V(AddressIndPair, (brEndElseInstr));
    }

    auto falseList = scBool->getFalseList();

    string falseLabel = buffer.genLabel("ifEnd");
    buffer.bpatch(falseList, falseLabel);

    return brEndElseInstrStype;
}

void handleElseEnd(shared_ptr<STypeC> endIfListStype) {
    AddressIndPair &endIfInstr = STYPE2STD(AddressIndPair, endIfListStype);
    auto &buffer = CodeBuffer::instance();

    string elseEndLabel = buffer.genLabel("elseEnd");

    buffer.bpatch(endIfInstr, elseEndLabel);
}

void handleWhileStart(shared_ptr<STypeC> scBoolStype, string startLabel) {
    handleIfStart(scBoolStype);
    symbolTable.startLoop(startLabel);
}

void handleWhileEnd(shared_ptr<STypeC> scBoolStype) {
    auto scBool = assureScBool(scBoolStype);

    symbolTable.addContinue();

    /* `endLoop` must be called after we emit the br to jump to the beginning of the
     * condition (becuase endLoop will generate the endLabel to which we jump from the falseList and break statements)
     */
    symbolTable.endLoop(scBool->getFalseList());
}
