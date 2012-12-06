
#ifndef COT_MODULOSCHEDULING_H
#define COT_MODULOSCHEDULING_H

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/raw_ostream.h"

namespace cot {

  // All LoopPass execute on each loop in the function independent of all of the other loops in the function. 
  // LoopPass processes loops in loop nest order such that outer most loop is processed last.
  // LoopPass subclasses are allowed to update loop nest using LPPassManager interface. 
  // Implementing a loop pass is usually straightforward. 
  // LoopPass's may overload three virtual methods to do their work. 
  // All these methods should return true if they modified the program, or false if they didn't.
  class ModuloScheduling : public llvm::LoopPass {
  public:
    static char ID;

  public:
    ModuloScheduling() : llvm::LoopPass(ID) { }

  public:

    // The doInitialization method is designed to do simple initialization type of stuff that does not depend on the functions being processed. 
    // The doInitialization method call is not scheduled to overlap with any other pass executions (thus it should be very fast). 
    // LPPassManager interface should be used to access Function or Module level analysis information.
    virtual bool doInitialization(llvm::Loop *L, llvm::LPPassManager &LPM);

    // The runOnLoop method must be implemented by your subclass to do the transformation or analysis work of your pass. 
    // As usual, a true value should be returned if the function is modified. 
    // LPPassManager interface should be used to update loop nest.
    virtual bool runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM);

    // The doFinalization method is an infrequently used method that is called when the pass framework has finished calling runOnLoop for every loop in the program being compiled.
    virtual bool doFinalization();

    // Not sure if it's used
    virtual const char *getPassName() const {
      return "Modulo Scheduling";
    }

    virtual void print(llvm::raw_ostream &OS, const llvm::Module *Mod) const;

  public:
   // Other methods?

  private:
    // This is the information computed by the analysis.
    unsigned blocksCount;
    unsigned instructionsCount;
    std::vector<std::string *> instructions;

  };

} // End cot namespace.

#endif // COT_MODULOSCHEDULING_H
