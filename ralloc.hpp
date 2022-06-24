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
    static Ralloc &getInstance();
    // Get the next available register
    std::string getNextReg();
};

#endif