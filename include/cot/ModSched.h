
#ifndef COT_MODSCHED_H
#define COT_MODSCHED_H

#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Module.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Type.h"

namespace cot{

class ModSched : public llvm::ModulePass {
public:
  static char ID;

public:
  ModSched() : llvm::ModulePass(ID) { }

public:
  // This member function must implement the code of your pass.
  virtual bool runOnModule(llvm::Module &Mod);

  // The getAnalysisUsage allows to tell LLVM pass manager which analysis are
  // used by the pass. It is also used to declare which analysis are preserved
  // by the pass.
  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const;

  virtual const char *getPassName() const {
    return "Hello llvm";
  }

private:
  llvm::Function &GetHelloWorld(llvm::Module &Mod, bool &Built);
  void AddMain(llvm::Module &Mod, llvm::Function &HelloFun, bool &Modified);

  llvm::Function &GetPrintf(llvm::Module &Mod);
};

}

#endif // COT_MODSCHED_H