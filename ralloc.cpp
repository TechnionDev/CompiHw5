#include "ralloc.hpp"
using std::string;

Ralloc::Ralloc() : nextReg(0) {}

// Get the singleton object instance
Ralloc &Ralloc::instance() {
    static Ralloc instance;
    return instance;
}

// Get the next register
string Ralloc::getNextReg() {
    return "%" + std::to_string(nextReg++);
}

string Ralloc::getNextVarName() {
    return "@." + std::to_string(nextReg++);
}