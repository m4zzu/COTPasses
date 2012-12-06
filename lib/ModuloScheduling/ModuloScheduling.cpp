
#include "cot/AllPasses.h"
#include "cot/ModuloScheduling.h"

#include "llvm/Function.h"
#include "llvm/Support/ErrorHandling.h"

using namespace cot;

char ModuloScheduling::ID = 0;

bool ModuloScheduling::doInitialization(llvm::Loop *L, llvm::LPPassManager &LPM){
  return false;   // Program not modified
}

bool ModuloScheduling::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
  // llvm::Instruction *tempInstr

  blocksCount = 0;
  instructionsCount = 0;

  const std::vector<llvm::BasicBlock *> blocks = L->getBlocks();

  for(unsigned i = 0; i < blocks.size(); ++i){
    blocksCount += 1;

    llvm::BasicBlock *currentBlock = blocks[i];
    for(llvm::BasicBlock::iterator i = *currentBlock->begin(), 
                                   e = *currentBlock->end(); 
                                   i != e; 
                                   ++i){
      instructionsCount += 1;

    }
  }

  return false;   // Program not modified
}

bool ModuloScheduling::doFinalization(){
  return false;   // Program not modified
}

void ModuloScheduling::print(llvm::raw_ostream &OS, 
                             const llvm::Module *Mod) const {
  if(!Mod)
    return;

  OS << "blocks count: " << blocksCount << "\n";
  OS << "instructions count: " << instructionsCount << "\n";

}


using namespace llvm;

INITIALIZE_PASS_BEGIN(ModuloScheduling,
                      "modulo-scheduling",
                      "Iterative Modulo Scheduling algorithm implementation",
                      true,
                      true)
INITIALIZE_PASS_END(ModuloScheduling,
                    "modulo-scheduling",
                    "Iterative Modulo Scheduling algorithm implementation",
                    true,
                    true)