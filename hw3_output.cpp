#include "hw3_output.hpp"
#include "parser.tab.hpp"

#include <stdlib.h>

#include <iostream>
#include <sstream>

#define DEBUG_PREFIX "; DEBUG: "

using namespace std;

void output::endScope() {
    cout << DEBUG_PREFIX " ---end scope---" << endl;
}

void output::printID(const string& id, int offset, const string& type) {
    // cout << DEBUG_PREFIX << id << " " << type << " " << offset << endl;
}

string typeListToString(const std::vector<string>& argTypes) {
    stringstream res;
    res << "(";
    for (int i = argTypes.size() - 1; i >= 0; --i) {
        res << argTypes[i];
        if (i > 0)
            res << ",";
    }
    res << ")";
    return res.str();
}

string valueListsToString(const std::vector<string>& values) {
    stringstream res;
    res << "{";
    for (int i = values.size() - 1; i >= 0; --i) {
        res << values[i];
        if (i > 0)
            res << ",";
    }
    res << "}";
    return res.str();
}

string output::makeFunctionType(const string& retType, std::vector<string>& argTypes) {
    stringstream res;
    res << typeListToString(argTypes) << "->" << retType;
    return res.str();
}

void output::errorLex(int lineno) {
    cout << "line " << lineno << ":"
         << " lexical error" << endl;

    ::exit(0);
}

void output::errorSyn(int lineno) {
    cout << "line " << lineno << ":"
         << " syntax error" << endl;

    ::exit(0);
}

void output::errorUndef(int lineno, const string& id) {
    cout << "line " << lineno << ":"
         << " variable " << id << " is not defined" << endl;

    ::exit(0);
}

void output::errorDef(int lineno, const string& id) {
    cout << "line " << lineno << ":"
         << " identifier " << id << " is already defined" << endl;

    ::exit(0);
}

void output::errorUndefFunc(int lineno, const string& id) {
    cout << "line " << lineno << ":"
         << " function " << id << " is not defined" << endl;

    ::exit(0);
}

void output::errorMismatch(int lineno) {
    cout << "line " << lineno << ":"
         << " type mismatch" << endl;

    ::exit(0);
}

void output::errorPrototypeMismatch(int lineno, const string& id, std::vector<string>& argTypes) {
    cout << "line " << lineno << ": prototype mismatch, function " << id << " expects arguments " << typeListToString(argTypes) << endl;

    ::exit(0);
}

void output::errorUnexpectedBreak(int lineno) {
    cout << "line " << lineno << ":"
         << " unexpected break statement" << endl;

    ::exit(0);
}

void output::errorUnexpectedContinue(int lineno) {
    cout << "line " << lineno << ":"
         << " unexpected continue statement" << endl;

    ::exit(0);
}

void output::errorMainMissing() {
    cout << "Program has no 'void main()' function" << endl;

    ::exit(0);
}

void output::errorByteTooLarge(int lineno, const string& value) {
    cout << "line " << lineno << ": byte value " << value << " out of range" << endl;

    ::exit(0);
}

int yyerror(const char* message) {
    output::errorSyn(yylineno);

    ::exit(0);
}
