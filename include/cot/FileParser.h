
#ifndef COT_FILEPARSER_H
#define COT_FILEPARSER_H

#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "cot/Architecture.h"

namespace cot {

class FileParser : public llvm::ImmutablePass {
public:
  static char ID;
  Architecture *architecture;

public:
  FileParser() : llvm::ImmutablePass(ID) {
    architecture = new Architecture();
  }

public:
  virtual bool runOnModule(llvm::Module &Mod);
  virtual void print(llvm::raw_ostream &OS, const llvm::Module *Mod) const;
  virtual void initializePass();

};

} // End cot namespace.

#endif // COT_FILEPARSER_H
