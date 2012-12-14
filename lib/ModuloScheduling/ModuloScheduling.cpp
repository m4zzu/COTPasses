
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
 
  // Init all the vars ---
  // REAL VARS
  delta = 0;
  std::vector<llvm::Instruction *> instructions;

  // TEMP VARS
  blocksCount = 0;
  instructionsCount = 0;

  // Ignore nested loop
  unsigned depth = L->getLoopDepth();
  if(depth > 1)
    return false;     // Program not modified

  // llvm::Instruction *tempInstr

  // controlla che la ind var sia incrementata di 1 (canonica)
  // loopBody: dev'essere un unico BB
  // scarta la induction variable: splitta in due il BB  e lasciala fuori

  // lavora sull'unico BB: chiama funzione schedule()
  const std::vector<llvm::BasicBlock *> blocks = L->getBlocks();

  for(unsigned i = 0; i < blocks.size(); ++i) {
    blocksCount += 1;

    llvm::BasicBlock *currentBlock = blocks[i];
    // Extract the instructions from the basic block
    for(llvm::BasicBlock::iterator istr = *currentBlock->begin(), 
                                   end = *currentBlock->end(); 
                                   istr != end; 
                                   ++istr) {
        instructionsCount += 1;
        instructions.push_back(istr);
      }

    // Apply the algorithm
    instructions = schedule(instructions);
  }

  // rifondi con il BB della ind var

  // Set the global variable, so it's accessible by the print method 
  scheduledInstructions = instructions;

  return true;   // Program modified
}

bool ModuloScheduling::doFinalization(){
  return false;   // Program not modified
}

void ModuloScheduling::print(llvm::raw_ostream &OS, 
                             const llvm::Module *Mod) const {
  if(!Mod)
    return;

  OS << "delta: " << delta << "\n";
  OS << "blocks count: " << blocksCount << "\n";
  OS << "instructions count: " << instructionsCount << "\n";
  u_int i = 0;
  while (i < scheduledInstructions.size()) {
    OS << "..:: " << (scheduledInstructions[i])->getOpcodeName() << "\n";
    ++i;
  }
}


std::vector<llvm::Instruction *> ModuloScheduling::schedule(std::vector<llvm::Instruction *> instructions){

  // Estimate the lower bound for delta
  unsigned deltaMin = std::max(resourcesBoundEstimator(), dataDependenceBoundEstimator());

  // Order instructions by a priority
  instructions = prioritizeInstructions(instructions);

  delta = deltaMin;

/*
   // Infinite loop: we have no upper bound for delta
  for(delta = deltaMin;;){

    budget = n*3;           // Init the number of attempts

    // Init the data structures
    for(i = 1; i<= n; i++){
      lastTime[i] = 0;      
      schedTime[i] = null;    // è un vettore o una mappa
    }

    // While we have attempts and some instructions are not scheduled
    while(budget > 0 && existUnscheduledInstructions(???)){
      budget--;

      h = findHighPriorityUnscheduledInstruction(???);
      predecessorsOfH = findPredecessors(h, ???);
      tMin = 0;

      // Find the most slow predecessor and update tMin
      forEach(instruction p in predecessorsOfH){
        if(SchedTime[p] != null){
          tMin = max(tMin, SchedTime[p] + delay(p, h));
        }
      }

      // Try to schedule h between tMin and tMin + delta - 1
      for(t = tMin; t < tMin + delta - 1, t++){
        if(SchedTime[h] == null){
          if(noResourceConflicts(h, ???)){
            SchedTime[h] = t;
          }
        }
      }

      // If h not scheduled, schedule it
      if(SchedTime[h] == null){
        SchedTime[h] = max(tMin, 1 + LastTime[h]);
      }
      LastTime[h] = SchedTime[h];
    }

    successorsOfH = findSuccessors(h, ???);

    // if successor of h is execute before the end of h, then unschedule it
    forEach(instruction s in successorsOfH){
      if(SchedTime[s] != null){
        if(SchedTime[h] + delay(h, s) > SchedTime[s]){
          SchedTime[s] = null;
        }
      }
    }

    // Remove from the scheduling all the instructions (other than h) involved in a resource conflict
    for(s = getFirstInstructionConflicting(???); s != h ; s = getFirstInstructionConflicting(???)){
      SchedTime[s] = null;
    }

    if(all instructions are scheduled)
      // Skip register allocation
      return;
  }
*/
  return instructions;
}


unsigned ModuloScheduling::resourcesBoundEstimator(){
  /* PARAM: architecturalDescription
    for(any kind of functional unit){
      cyclesUsed = cyclesPerInstr*instructionsUsingIt;
      resourceBound = cyclesUsed/numberOfResourcesOfThisKind;
    }
    return resourcesBoundEstimator = max(all bounds found);
  */
  return 2;
}

unsigned ModuloScheduling::dataDependenceBoundEstimator(){
  /* PARAM: instructions
    retrieve the data-dependence graph;
    per ogni istr, abbiamo la lista degli usi = le var che usa quella istruzione
    oppure usa SCEV: "dammi l'espressione corrispondente alla b..."
    una delle foglie sarà una recursive scev
      se la trovi, vuol dire che c'è un loop!
    retrieve cycles in that graph;
    for(every cycle found){
      dataDependenceBound = latency of the chain;
  }
  return dataDependenceBoundEstimator = max(all bounds found);
  */
  return 3;
}

std::vector<llvm::Instruction *> ModuloScheduling::prioritizeInstructions(std::vector<llvm::Instruction *> instructions){
  // No heuristic has been implemented to sort the instructions. 
  return instructions;
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
