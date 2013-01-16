
#include "cot/AllPasses.h"
#include "cot/ModuloScheduling.h"
#include "cot/FileParser.h"

#include "llvm/Support/raw_ostream.h"
#include "llvm/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/Instructions.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/DerivedTypes.h"
#include "llvm/Type.h"

#include "llvm/Module.h"
#include "llvm/Function.h"
#include "llvm/PassManager.h"
#include "llvm/Analysis/Verifier.h"
#include "llvm/Assembly/PrintModulePass.h"

#include "llvm/ADT/StringRef.h"

#include <map>
#include <vector>

using namespace cot;

char ModuloScheduling::ID = 0;

bool ModuloScheduling::doInitialization(llvm::Loop *L, llvm::LPPassManager &LPM){
  return false;   // Program not modified
}

bool ModuloScheduling::runOnLoop(llvm::Loop *L, llvm::LPPassManager &LPM){
 
  // Ignore nested loop
  int depth = L->getLoopDepth();
  if(depth > 1)
    return false;     // Program not modified

  // Init all the vars ---
  // REAL VARS
  delta = 0;
  
  FileParser &fp = getAnalysis<FileParser>();
  Architecture *currentArchitecture = fp.getArchitecture();
  
  std::vector<llvm::Instruction *> instructions;


  // TEMP VARS
  blocksCount = 0;
  instructionsCount = 0;

  // llvm::Instruction *tempInstr

  // controlla che la ind var sia incrementata di 1 (canonica)
  // loopBody: dev'essere un unico BB
  // scarta la induction variable: splitta in due il BB  e lasciala fuori

  // lavora sull'unico BB: chiama funzione schedule()
  const std::vector<llvm::BasicBlock *> blocks = L->getBlocks();

  for(unsigned i = 0; i < blocks.size(); ++i) {
    blocksCount += 1;

    llvm::BasicBlock *currentBlock = blocks[i];

    llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop> LIB = llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>();
    LIB.changeLoopFor(currentBlock, L);
    if (!LIB.isLoopHeader(currentBlock)) {
    // Extract the instructions from the basic block
      for(llvm::BasicBlock::iterator istr = *currentBlock->begin(), 
                                     end = *currentBlock->end(); 
                                     istr != end; 
                                     ++istr) {
          // instructionsCount += 1;
          instructions.push_back(istr);
      }
      // instructions = schedule(instructions);
      // createNewBlock(currentBlock, instructions, i);
    }
    // createNewBlock(currentBlock, instructions, i);
  }

  // rifondi con il BB della ind var

  // Apply the algorithm
  //instructions = schedule(instructions);
  // llvm::BasicBlock *newBlock = createNewBlock(currentBlock, instructions);
  // deleteOldBlock();

  // Set the global variable, so it's accessible by the print method 
  scheduledInstructions = instructions;
  architecture = currentArchitecture;

  return true;   // Program modified
}

void ModuloScheduling::createNewBlock(llvm::BasicBlock *CB, std::vector<llvm::Instruction *> instructions)
{
  llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry");
  llvm::IRBuilder<> builder(llvm::getGlobalContext());
  builder.SetInsertPoint(CB);

  // llvm::iplist<llvm::NodeTy, llvm::Traits> il = CB->getInstList();
  // llvm::Instruction *pi = llvm::dyn_cast<llvm::Instruction>(CB->getFirstNonPHI());
  // llvm::Instruction *newInst = new llvm::Instruction();
  // CB->getInstList().insert(pi, newInst);
  // llvm::BasicBlock::iterator ii(instToReplace);
  // const llvm::Type * Int32Type = llvm::IntegerType::getInt32Ty(llvm::getGlobalContext());
  // llvm::ReplaceInstWithInst(instToReplace->getParent()->getInstList(), ii, new llvm::AllocaInst(llvm::PointerType::getUnqual(Int32Type), 0, "ptrToReplacedInt"));

  // llvm::BasicBlock* BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), ("bb"));

  // // //CB = &BB;
  // u_int i = 0;
  // u_int tot = instructions.size() - 1;
  // for (i = 0; i < instructions.size() / 2; ++i) {
  //   // llvm::AllocationInst *AI = llvm::dyn_cast<llvm::AllocationInst>(instructions[i]);
  //     llvm::Instruction *Inst = llvm::dyn_cast<llvm::Instruction>(instructions[i]);

  //     // CB->getInstList()
  // }
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

  OS << "=======-------=======\n";
  std::vector<Instruction> A = architecture->getAllArch();
  unsigned i = 0;
  while (i < A.size()) {
    OS << "Conf " << (i + 1) << ":\n";
    OS << "\tInstr:\t" << A[i].getInstruction() << "\n";
    OS << "\tUnit:\t" << A[i].getUnit() << "\n";
    OS << "\tCycle:\t" << A[i].getCycle() << "\n";
    ++i;
  }
  OS << "=======-------=======\n\n";
  OS << "dataDependenceBoundEstimator = " << dataDependenceBoundEstimator() << "\n";
  OS << "resourcesBoundEstimator --- \n" << resourcesBoundEstimator(OS);
}





std::vector<llvm::Instruction *> ModuloScheduling::schedule(Architecture* architecture, std::vector<llvm::Instruction *> instructions){

  // Declarations
  std::map<llvm::Instruction *, int> lastTime;
  std::map<llvm::Instruction *, int> schedTime;

  // Lower bound for delta
  int deltaMin = 0; // std::max(resourcesBoundEstimator(), dataDependenceBoundEstimator());

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

      // std::vector<llvm::Instruction *> successors = findSuccessors(currentInstruction, instructions);
      std::vector<llvm::Instruction *> successors;

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


int ModuloScheduling::resourcesBoundEstimator(llvm::raw_ostream &OS) const{

  // Get all the operands supported by the architecture
  std::vector<Instruction> operands = architecture->getAllArch();

  for (std::vector<Instruction>::iterator i = operands.begin(); i != operands.end(); ++i)
  {
    OS << "Operand: " << i->getInstruction() << "\n";
  }


  /* PARAM: architecture
  creo una mappa: istr - contatore
  for(tutte le istruzioni){
    se è presente nella mappa
      incremento
    altrimenti
      aggiungo: istr - 1
  }

  delta = 0;
  for(sulla mappa){
    numCicliIstr = arch.getCycle(chiave della mappa);
    deltaTemp = roundUp(numCicliIstr*mappa.get(chiave)/getNumberOfUnits(chiave));
    if(deltaTemp > delta)
      delta = deltaTemp;
  }
  return delta;
  */
  return 2;
}



int ModuloScheduling::dataDependenceBoundEstimator() const{

  // Create a map: {instruction, isVisited}
  std::map<llvm::Instruction *, bool> instructionsMap;
  for(unsigned j = 0; j < scheduledInstructions.size(); ++j) {
    instructionsMap.insert(std::pair<llvm::Instruction *, int> (scheduledInstructions[j], false));
  }

  int finalBound = 0;
  int tempBound = 0;

  // For all the instructions
  for(unsigned i = 0; i < scheduledInstructions.size(); ++i) {

    tempBound = 0;

    // Skip "phi" function: they will not be scheduled on the pipeline
    if(!llvm::StringRef("phi").equals(scheduledInstructions[i]->getOpcodeName())){

      // Recursively find definitions of the operands of the instruction
      tempBound = findDefRecursive(instructionsMap, scheduledInstructions[i], 0);
    }

    if(tempBound > finalBound)
      finalBound = tempBound;
  }

  return finalBound;  
}

int ModuloScheduling::findDefRecursive(std::map<llvm::Instruction *, bool> instructionsMap, llvm::Instruction * currentI, int offset) const{
  
  /*
  if(!llvm::StringRef("phi").equals(currentI->getOpcodeName())){
    // Print to screen
    for (int i = 0; i <= offset; ++i)
      OS << "  ";

    OS << offset << " - " << currentI->getOpcodeName() << "\n";
  }
  */

  // Set the current instruction as visited
  instructionsMap[currentI] = true;

  // Final and temp offset
  int finalOffset = 0;
  int currentOffset = 0;

  // For all the definitions
  for (llvm::User::op_iterator i = currentI->op_begin(), e = currentI->op_end(); i != e; ++i) {

    // Reset the current offset 
    currentOffset = 0;

    // For all the instruction defining one of the current instruction's operators
    if(llvm::Instruction * definerI = llvm::dyn_cast<llvm::Instruction>(*i)){
          
      // If the definer instruction hasn't been visited
      if(instructionsMap[definerI] == false){

        if(llvm::StringRef("phi").equals(definerI->getOpcodeName()))
          currentOffset = findDefRecursive(instructionsMap, definerI, offset);      // Find definers recursively, without incrementing the offset (ignore "phi" functions)     
        else
          currentOffset = findDefRecursive(instructionsMap, definerI, offset + 1);  // Find definers recursively, incrementing the offset    
      }else{
        currentOffset = offset + 1;
      }
    }
    
    if(currentOffset > finalOffset)
      finalOffset = currentOffset;      // Update the offset
    
  }

  return finalOffset;
}



std::vector<llvm::Instruction *> ModuloScheduling::prioritizeInstructions(std::vector<llvm::Instruction *> instructions){
  // No heuristic has been implemented to sort the instructions. 
  return instructions;
}

llvm::Instruction* ModuloScheduling::findHighPriorityUnscheduledInstruction(std::vector<llvm::Instruction *> instructions, std::map<llvm::Instruction *, int>  schedTime){

  for (unsigned i = 0; i < instructions.size(); ++i) {
    llvm::Instruction* currentInstr = instructions[i];
    if(schedTime[currentInstr] == -1)
      return currentInstr;           // Unscheduled instruction found
  }

  return NULL;                       // NO unscheduled instruction found
}

std::vector<llvm::Instruction *> ModuloScheduling::findPredecessors(llvm::Instruction * h, std::vector<llvm::Instruction *> instructions) {
  /*
  // Find all the predecessors of an instruction
  // --- NO!!
  iteratore sulle instructions;
  vettore pred;
  while(iteratore è diverso dalla instrPassata){
    pred.push_back(instrCorrente);
  }
  return pred;
  */
  std::vector<llvm::Instruction *> pred;
  for (std::vector<llvm::Instruction *>::iterator instr = instructions.begin();
                                                instr != instructions.end();
                                                ++instr) {
    if ((*instr) == h)
      break;
    pred.push_back(*instr);
  }
  return pred;
}

std::vector<llvm::Instruction *> ModuloScheduling::findSuccessors(llvm::Instruction * h, std::vector<llvm::Instruction *> instructions) {
  /*
  // Find all the successors of an instruction
  // --- NO!!
  iteratore sulle instructions;
  vettore succ;
  while(iteratore è diverso dalla instrPassata){
    // Skip
  }

  while(finisce vettore){
    succ.push_back(instrCorrente);
  }
  return succ;
  */
  bool flag = 0;
  std::vector<llvm::Instruction *> succ;
  for (std::vector<llvm::Instruction *>::iterator istr = instructions.begin();
                                                istr != instructions.end();
                                                ++istr) {
    if (flag)
      succ.push_back(*istr);
    if ((*istr) == h)
      flag = 1;
  }
  return succ;
}

int ModuloScheduling::delay(llvm::Instruction * firstInstruction, llvm::Instruction * secondInstruction, std::vector<llvm::Instruction *> instructions){
  /* CHECK THE APPEL BOOK!
  */
  return 2;
}

llvm::Instruction* ModuloScheduling::getFirstConflictingInstruction(llvm::Instruction * currentInstruction, std::vector<llvm::Instruction *> instructions){
  /* Find resource conflicts
  */
  return NULL;
}

bool ModuloScheduling::scheduleCompleted(std::map<llvm::Instruction *, int> schedTime){
  for(std::map<llvm::Instruction *, int>::iterator iter = schedTime.begin(); 
          iter != schedTime.end(); 
          ++iter){
        if(iter->second == -1)
          return false;
  }
  return true;
}

void ModuloScheduling::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<FileParser>();
  AU.setPreservesAll();
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
