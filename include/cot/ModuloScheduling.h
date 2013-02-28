
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
    virtual void printResourceTable();
    virtual void printSchedTime(std::map<llvm::Instruction *, int> schedTime);


  public:
   // Other methods?

  private:
    // This is the information computed by the analysis.
    std::vector<llvm::Instruction *> scheduledInstructions;
    Architecture *architecture;
    std::map<std::string, std::vector<llvm::Instruction *> > resourceTable;

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
    virtual int findDefRecursive(llvm::Instruction * startingI, std::map<llvm::Instruction *, bool> instructionsMap, llvm::Instruction * istr, int offset) const;
    
    // This method is used to sort the instructions using a particular criterion
    virtual std::vector<llvm::Instruction *> prioritizeInstructions(std::vector<llvm::Instruction *> instructions);

    // The following method is used to get the unscheduled instruction with the higher priority
    virtual llvm::Instruction* findHighPriorityUnscheduledInstruction(std::vector<llvm::Instruction *> instructions, std::map<llvm::Instruction *, int>  schedTime);

    // The following method is used to get the instructions that produce the values of the current instruction's operands
    virtual std::set<llvm::Instruction *> findPredecessors(std::vector<llvm::Instruction *> instructions, llvm::Instruction * currentI);

    // The following method is used to get the instructions that use the value of the current instruction's result
    virtual std::set<llvm::Instruction *> findSuccessors(std::vector<llvm::Instruction *> instructions, llvm::Instruction * currentI);

    // The delay method, implemented as definded in the book
    virtual int delay(llvm::Instruction * firstInstruction, llvm::Instruction * secondInstruction, std::vector<llvm::Instruction *> instructions, int delta, std::map<llvm::Instruction *, int> schedTime);

    // This method is used to find the first instruction conflicting with the one passed
    virtual llvm::Instruction* getFirstConflictingInstruction(llvm::Instruction * currentInstruction, int t, std::string schedulingUnit, int delta);

    // This method returns true if an allowed schedule exists for the current instruction
    virtual bool canBeScheduled(llvm::Instruction * currentInstruction, int t, int delta);

    // This method returns true if all instructions have been scheduled
    virtual bool scheduleCompleted(std::map<llvm::Instruction *, int> schedTime);

    // The following method schedule the instruction, removing (greedy) conflicting instructions if an admissible schedule doesn't exist
    virtual void schedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime, int t, int delta);

    // The following method unschedule an instruction
    virtual void unschedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime);

    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

  };

} // End cot namespace.

#endif // COT_MODULOSCHEDULING_H
