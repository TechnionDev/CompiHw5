#ifndef RALLOC_H_
#define RALLOC_H_

#include <string>

// Singleton class for dynamically allocating LLVM registers to be used by the code synthesizer
class Ralloc {
   private:
    int nextReg;
    // Constructor
    Ralloc();
    Ralloc(const Ralloc &) = delete;

   public:
    // Get the singleton instance
    static Ralloc &instance();
    // Get the next available register
    std::string getNextReg();
    // Get the next available variable name
    std::string getNextVarName();
};

#endif