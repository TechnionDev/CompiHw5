#include "ralloc.hpp"
using std::string;

Ralloc::Ralloc() : nextReg(0) {}

// Get the singleton object instance
Ralloc &Ralloc::getInstance() {
    static Ralloc instance;
    return instance;
}

// Get the next register
string Ralloc::getNextReg() {
    return "%" + (nextReg++);
}
