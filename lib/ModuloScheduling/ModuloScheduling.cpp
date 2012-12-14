
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

  // se loop depth è > 1, termina!
  // controlla che la ind var sia incrementata di 1 (canonica)
  // loopBody: dev'essere un unico BB
  // scarta la induction variable: splitta in due il BB  e lasciala fuori

  // lavora sull'unico BB: chiama funzione schedule()

  // rifondi con il BB della ind var

  blocksCount = 0;
  instructionsCount = 0;

  const std::vector<llvm::BasicBlock *> blocks = L->getBlocks();

  for(unsigned i = 0; i < blocks.size(); ++i) {
    blocksCount += 1;

    llvm::BasicBlock *currentBlock = blocks[i];
    for(llvm::BasicBlock::iterator i = *currentBlock->begin(), 
                                   e = *currentBlock->end(); 
                                   i != e; 
                                   ++i) {
      instructionsCount += 1;
      opCode.push_back(i->getOpcodeName());
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
  u_int i = 0;
  while (i < opCode.size()) {
    OS << "..:: " << opCode[i] << "\n";
    ++i;
  }
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
