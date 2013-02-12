
#ifndef COT_MODULOSCHEDULING_H
#define COT_MODULOSCHEDULING_H

#include "llvm/Analysis/LoopPass.h"
#include "llvm/Support/raw_ostream.h"
#include "cot/Architecture.h"

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
    // REAL VARS
    int delta;
    std::vector<llvm::Instruction *> scheduledInstructions;
    Architecture *architecture;
    std::map<std::string, std::vector<llvm::Instruction *> > resourceTable;

    // TEMP VARS
    int blocksCount;
    int instructionsCount;
    std::vector<const char *> opCode;

    // The doScheduling method implements the Modulo Scheduling algorithm as it's described in: 
    // "Modern Compiler Implementation in Java", 
    // chapter: "Pipelining and Scheduling",
    // author: Andrew W. Appel
    virtual std::vector<llvm::Instruction *> doScheduling(std::vector<llvm::Instruction *> instructions);

    // The following method is meant to estimate a lower bound for delta, 
    // considering the number of functional units and the operations in the loop body
    virtual int resourcesBoundEstimator(std::vector<llvm::Instruction *> instructions);

    // The following method is meant to estimate a lower bound for delta,
    // finding the longest cycle in the data-dependence graph 
    virtual int dataDependenceBoundEstimator(std::vector<llvm::Instruction *> instructions);

    // This is an auxiliary method, used by the previous to recursively traverse the tree of dependencies
    virtual int findDefRecursive(std::map<llvm::Instruction *, bool> instructionsMap, llvm::Instruction * istr, int offset) const;
    
    // This method is used to sort the instructions using a particular criterion
    virtual std::vector<llvm::Instruction *> prioritizeInstructions(std::vector<llvm::Instruction *> instructions);

    // The following method is used to get the unscheduled instruction with the higher priority
    virtual llvm::Instruction* findHighPriorityUnscheduledInstruction(std::vector<llvm::Instruction *> instructions, std::map<llvm::Instruction *, int>  schedTime);

    virtual std::set<llvm::Instruction *> findPredecessors(std::vector<llvm::Instruction *> instructions, llvm::Instruction * currentI);

    virtual std::set<llvm::Instruction *> findSuccessors(std::vector<llvm::Instruction *> instructions, llvm::Instruction * currentI);

    virtual int delay(llvm::Instruction * firstInstruction, llvm::Instruction * secondInstruction, std::vector<llvm::Instruction *> instructions, int delta);

    virtual llvm::Instruction* getFirstConflictingInstruction(llvm::Instruction * currentInstruction, int t, std::string schedulingUnit);
    virtual bool canBeScheduled(llvm::Instruction * currentInstruction, int t);
    // virtual bool resourcesConflict(std::vector<std::string> a, std::vector<std::string> b);

    virtual bool scheduleCompleted(std::map<llvm::Instruction *, int> schedTime);

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

    virtual void createNewBlock(llvm::BasicBlock *, std::vector<llvm::Instruction *>);

    virtual void schedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime, int t, int delta);

    virtual void unschedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime);

  };

} // End cot namespace.

#endif // COT_MODULOSCHEDULING_H
