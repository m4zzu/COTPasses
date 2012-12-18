
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
  int depth = L->getLoopDepth();
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
    // createNewBlock(instructions);
    // deleteOldBlock();
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

  // Declarations
  std::map<llvm::Instruction *, int> lastTime;
  std::map<llvm::Instruction *, int> schedTime;

  // Lower bound for delta
  int deltaMin = std::max(resourcesBoundEstimator(), dataDependenceBoundEstimator());

  // Order instructions by a priority
  instructions = prioritizeInstructions(instructions);

  // Infinite loop: we have no upper bound for delta
  for(delta = deltaMin; ; ++delta){
    int budget = instructions.size()*3;           // Init the number of attempts

    // Init the data structures
    for(unsigned i = 0; i < instructions.size(); ++i){
      lastTime.insert(std::pair<llvm::Instruction *, int> (instructions[i], 0));
      schedTime.insert(std::pair<llvm::Instruction *, int> (instructions[i], -1));
    }

    llvm::Instruction* currentInstruction = NULL;

    // While we have attempts and some instructions are not scheduled
    for(currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime);
        budget > 0 && currentInstruction != NULL;
        currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime), budget--){
      
      std::vector<llvm::Instruction *> predecessors = findPredecessors(currentInstruction, instructions);
      int tMin = 0;

      // Find the most slow predecessor and update tMin
      for(unsigned j = 0; j < predecessors.size(); ++j){

        llvm::Instruction* currentPredecessor = predecessors[j];

        if(schedTime[currentPredecessor] != -1){
          int currentSchedTime = schedTime[currentPredecessor] + delay(currentPredecessor, currentInstruction, instructions);
          tMin = std::max(tMin, currentSchedTime);
        } 
      }

      // Try to schedule h between tMin and tMin + delta - 1
       for(int t = tMin; t < tMin + delta - 1; ++t){
        if(schedTime[currentInstruction] == -1){

          // If no conflicts
          if(getFirstConflictingInstruction(currentInstruction, instructions) == NULL){
            schedTime[currentInstruction] = t;
          }
        }
      }

      // If h not scheduled, schedule it
      if(schedTime[currentInstruction] == -1){
        schedTime[currentInstruction] = std::max(tMin, 1 + lastTime[currentInstruction]);
      }
      lastTime[currentInstruction] = schedTime[currentInstruction];

      std::vector<llvm::Instruction *> successors = findSuccessors(currentInstruction, instructions);

      // if successor of h is execute before the end of h, then unschedule it
      for(unsigned k = 0; k < successors.size(); ++k){

        llvm::Instruction* currentSuccessor = successors[k];

        if(schedTime[currentSuccessor] != -1){
          if(schedTime[currentInstruction] + delay(currentInstruction, currentSuccessor, instructions) > schedTime[currentSuccessor]){
            schedTime[currentSuccessor] = -1;
          }
        }
      }

      // Remove from the scheduling all the instructions (other than currentInstruction) involved in a resource conflict
      for(llvm::Instruction* conflictingInstruction = getFirstConflictingInstruction(currentInstruction, instructions); 
          conflictingInstruction != NULL && conflictingInstruction != currentInstruction; 
          conflictingInstruction = getFirstConflictingInstruction(currentInstruction, instructions)){
        schedTime[conflictingInstruction] = -1;
      }
    }

    // If all instructions are scheduled
    if(scheduleCompleted(schedTime)){
      
      // Map with schedule time used as key
      std::map<int, llvm::Instruction *> schedTimeMap;

      for(std::map<llvm::Instruction *, int>::iterator iter = schedTime.begin(); 
          iter != schedTime.end(); 
          ++iter){
        schedTimeMap.insert(std::pair<int, llvm::Instruction *> (iter->second, iter->first));
      }

      for(std::map<int, llvm::Instruction *>::iterator iter = schedTimeMap.begin(); 
          iter != schedTimeMap.end(); 
          ++iter){
        scheduledInstructions.push_back(iter->second);
      }

      return scheduledInstructions;
    }
  }
}


int ModuloScheduling::resourcesBoundEstimator(){
  /* PARAM: architecturalDescription
    for(any kind of functional unit){
      cyclesUsed = cyclesPerInstr*instructionsUsingIt;
      resourceBound = cyclesUsed/numberOfResourcesOfThisKind;
    }
    return resourcesBoundEstimator = max(all bounds found);
  */
  return 2;
}

int ModuloScheduling::dataDependenceBoundEstimator(){
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

llvm::Instruction* findHighPriorityUnscheduledInstruction(std::vector<llvm::Instruction *> instructions, std::map<llvm::Instruction *, int>  schedTime){

  for (unsigned i = 0; i < instructions.size(); ++i) {
    llvm::Instruction* currentInstr = instructions[i];
    if(schedTime[currentInstr] == -1)
      return currentInstr;           // Unscheduled instruction found
  }

  return NULL;                       // NO unscheduled instruction found
}

std::vector<llvm::Instruction *> findPredecessors(llvm::Instruction * h, std::vector<llvm::Instruction *> instructions){
  /* Find all the predecessors of an instruction
  */
  return instructions;
}

std::vector<llvm::Instruction *> findSuccessors(llvm::Instruction * h, std::vector<llvm::Instruction *> instructions){
  /* Find all the successors of an instruction
  */
  return instructions;
}

int delay(llvm::Instruction * firstInstruction, llvm::Instruction * secondInstruction, std::vector<llvm::Instruction *> instructions){
  /* CHECK THE APPEL BOOK!
  */
  return 2;
}

llvm::Instruction* getFirstConflictingInstruction(llvm::Instruction * currentInstruction, std::vector<llvm::Instruction *> instructions){
  /* Find resource conflicts
  */
  return NULL;
}

bool scheduleCompleted(std::map<llvm::Instruction *, int> schedTime){
  for(std::map<llvm::Instruction *, int>::iterator iter = schedTime.begin(); 
          iter != schedTime.end(); 
          ++iter){
        if(iter->second == -1)
          return false;
  }
  return true;
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
