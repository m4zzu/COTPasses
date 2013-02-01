
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
#include <string>
#include <math.h>

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
  architecture = fp.getArchitecture();
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
    }
  }

  // Apply the algorithm
  instructions = doScheduling(instructions);
  // llvm::BasicBlock *newBlock = createNewBlock(currentBlock, instructions);
  // deleteOldBlock();

  // Set the global variable, so it's accessible by the print method 
  scheduledInstructions = instructions;

  return true;   // Program modified
}

void ModuloScheduling::createNewBlock(llvm::BasicBlock *CB, std::vector<llvm::Instruction *> instructions)
{
  // llvm::BasicBlock *BB = llvm::BasicBlock::Create(llvm::getGlobalContext(), "entry");
  // llvm::IRBuilder<> builder(llvm::getGlobalContext());
  // builder.SetInsertPoint(CB);

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
  std::vector<Operand> A = architecture->getAllArch();
  unsigned i = 0;
  while (i < A.size()) {
    OS << "Conf " << (i + 1) << ":\n";
    OS << "\tInstr:\t" << A[i].getInstruction() << "\n";
    OS << "\tUnit:\t" << A[i].getUnit() << "\n";
    OS << "\tCycle:\t" << A[i].getCycle() << "\n";
    ++i;
  }
}

std::vector<llvm::Instruction *> ModuloScheduling::doScheduling(std::vector<llvm::Instruction *> instructions){

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

    // While we have attempts and some instructions are not scheduled, 
    // try to schedule them, decrementing the budget of attempts at every iteration
    for(currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime);
        budget > 0 && currentInstruction != NULL;
        currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime), budget--){
      
      // Find all the predecessors, using llvm::User methods (intra-loop body)
      std::set<llvm::Instruction *> predecessors = findPredecessors(currentInstruction);
      int tMin = 0;

      // Find the most slow predecessor and update tMin
      for (std::set<llvm::Instruction *>::iterator firstP = predecessors.begin(), 
                                                   lastP = predecessors.end(); 
                                                   firstP != lastP; 
                                                   ++firstP){
        if(llvm::Instruction * currentP = llvm::dyn_cast<llvm::Instruction>(*firstP)){

          // If the predecessor is scheduled
          if(schedTime[currentP] != -1){
            int currentSchedTime = schedTime[currentP] + delay(currentP, currentInstruction, instructions);
            tMin = std::max(tMin, currentSchedTime);
          }
        }
      }

      // Try to schedule h between tMin and tMin + delta - 1
       for(int t = tMin; t < tMin + delta - 1; ++t){
        if(schedTime[currentInstruction] == -1){

          // If no conflicts on the resources
          if(getFirstConflictingInstruction(currentInstruction, t) == NULL){
            // Schedule the current instruction
            schedule(currentInstruction, &schedTime, t, delta);
          }
        }
      }

      // If h not scheduled
      if(schedTime[currentInstruction] == -1){

        // Schedule like there's no tomorrow!!
        schedule(currentInstruction, &schedTime, std::max(tMin, 1 + lastTime[currentInstruction]), delta);
      }

      // Update lastTime
      lastTime[currentInstruction] = schedTime[currentInstruction];

      // Find all the successors, using llvm::Value methods (intra-loop body)
      std::set<llvm::Instruction *> successors = findSuccessors(currentInstruction);

      // if successor of h is execute before the end of h, then unschedule it
      for (std::set<llvm::Instruction *>::iterator firstS = successors.begin(), 
                                                   lastS = successors.end(); 
                                                   firstS != lastS; 
                                                   ++firstS){
        if(llvm::Instruction * currentS = llvm::dyn_cast<llvm::Instruction>(*firstS)){
          if(schedTime[currentS] != -1){
            if(schedTime[currentInstruction] + delay(currentInstruction, currentS, instructions) > schedTime[currentS])
              unschedule(currentInstruction, &schedTime);
          }
        }
      }
      

      unschedule(currentInstruction, &schedTime);
      std::vector<llvm::Instruction *> v;
      return v;
      // Remove from the scheduling all the instructions (other than currentInstruction) involved in a resource conflict
      /* ALREADY DONE IN SCHEDULE(...)
      for(llvm::Instruction* conflictingInstruction = getFirstConflictingInstruction(currentInstruction, schedTime[currentInstruction]); 
          conflictingInstruction != NULL && conflictingInstruction != currentInstruction; 
          conflictingInstruction = getFirstConflictingInstruction(currentInstruction, t)){
        schedTime[conflictingInstruction] = -1;
      }
      */
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


int ModuloScheduling::resourcesBoundEstimator() {
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

  // Get all the operands supported by the architecture
  std::vector<std::string> operands = architecture->getSupportedOperand();
  std::map<std::string, int> instructionsMap;
  int delta = 0, deltaTemp = 0, numCicliIstr = 0;

  for (std::vector<std::string>::iterator op = operands.begin();
                                          op != operands.end();
                                          ++op)
    instructionsMap.insert(std::pair<std::string, int>(*op, 0));

  for (std::vector<llvm::Instruction *>::const_iterator istr = scheduledInstructions.begin();
                                                        istr != scheduledInstructions.end();
                                                        ++istr) {
    std::string op = (*istr)->getOpcodeName();
    if (instructionsMap.find(op) != instructionsMap.end())
      ++instructionsMap[op];
    else
      instructionsMap.insert(std::pair<std::string, int>(op, 1));
  }

  for (std::map<std::string, int>::iterator record = instructionsMap.begin();
                                            record != instructionsMap.end();
                                            ++record) {
    numCicliIstr = architecture->getCycle(record->first);
    std::map<std::string, int>::iterator it = instructionsMap.find(record->first);
    if (it != instructionsMap.end() && numCicliIstr > 0) {
      deltaTemp = ceil(numCicliIstr * it->second / architecture->getNumberOfUnits(record->first));
      if(deltaTemp > delta)
        delta = deltaTemp;
    }
  }

  return delta;
}



int ModuloScheduling::dataDependenceBoundEstimator() {
  // Create a map: {instruction, isVisited}
  // Init isVisited to false
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
          currentOffset = findDefRecursive(instructionsMap, definerI, offset + architecture->getCycle(currentI->getOpcodeName()));  // Find definers recursively, incrementing the offset    
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

std::set<llvm::Instruction *> ModuloScheduling::findPredecessors(llvm::Instruction * currentI) {

  /* Old implementation - Predecessors in the BB
  for (std::vector<llvm::Instruction *>::iterator instr = instructions.begin();
                                                  instr != instructions.end();
                                                  ++instr) {
    if ((*instr) == h)
      break;
    pred.push_back(*instr);
  }
  return pred;
  */

  // Init the vector of predecessors
  std::set<llvm::Instruction *> pred;

  // For all the instruction defining one of the current instruction's operators
  for (llvm::User::op_iterator i = currentI->op_begin(), e = currentI->op_end(); i != e; ++i) {

    // Try to cast
    if(llvm::Instruction * definerI = llvm::dyn_cast<llvm::Instruction>(*i)){
      
      // If it's a phi instruction
      if(llvm::StringRef("phi").equals(definerI->getOpcodeName())){

        // Find its predecessors
        std::set<llvm::Instruction *> recursivePred = findPredecessors(definerI);

        // Add them in the vector of predecessors, if it's not present
        for (std::set<llvm::Instruction *>::iterator first = recursivePred.begin(), 
                                                     last = recursivePred.end(); 
                                                     first != last; 
                                                     ++first){
          // Try to cast
          if(llvm::Instruction * currentP = llvm::dyn_cast<llvm::Instruction>(*first))
            if(pred.find(currentP) == pred.end())
              pred.insert(currentP);                      // Add it to the predecessors, if it's not present
        }

      }else{
        // Add it to the predecessors, if it's not present
        if(pred.find(definerI) == pred.end())
          pred.insert(definerI);
      }
    }
  }

  return pred;
}

std::set<llvm::Instruction *> ModuloScheduling::findSuccessors(llvm::Instruction * currentI) {

  /* Old implementation - Successors in the BB
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
  */

  // Init the vector of successors
  std::set<llvm::Instruction *> succ;

  // For all the instruction using the current instruction's result
  for (llvm::value_use_iterator<llvm::User> i = currentI->use_begin(), e = currentI->use_end(); i != e; ++i) {

    // Try to cast
    if(llvm::Instruction * userI = llvm::dyn_cast<llvm::Instruction>(*i)){
      
      // If it's a phi instruction
      if(llvm::StringRef("phi").equals(userI->getOpcodeName())){

        // Find its successors
        std::set<llvm::Instruction *> recursiveSucc = findSuccessors(userI);

        // Add them in the vector of successors, if they're not present
        for (std::set<llvm::Instruction *>::iterator first = recursiveSucc.begin(), 
                                                     last = recursiveSucc.end(); 
                                                     first != last; 
                                                     ++first){
          // Try to cast
          if(llvm::Instruction * currentS = llvm::dyn_cast<llvm::Instruction>(*first))
            if(succ.find(currentS) == succ.end())
              succ.insert(currentS);              // Add it to the successors, if it's not present
        }

      }else{
        // Add it to the successors, if it's not present
        if(succ.find(userI) == succ.end())
          succ.insert(userI);
      }
    }
  }

  return succ;
}

int ModuloScheduling::delay(llvm::Instruction * firstInstruction, llvm::Instruction * secondInstruction, std::vector<llvm::Instruction *> instructions){
  /* CHECK THE APPEL BOOK!
  */
  return 2;
}

bool ModuloScheduling::resourcesConflict(std::vector<std::string> a, std::vector<std::string> b) {
  for (std::vector<std::string>::iterator ua = a.begin();
                                         ua != a.end();
                                         ++ua) {
    for (std::vector<std::string>::iterator ub = b.begin();
                                            ub != b.end();
                                            ++ub){
      if (*ua == *ub)
        return true;
    }
  }
  return false;
}

llvm::Instruction* ModuloScheduling::getFirstConflictingInstruction(llvm::Instruction * currentInstruction, int t) {
  /* Find resource conflicts
  */
  /*
  bool flag = false;
  std::vector<std::string> unitsCurrent = architecture->getUnit(currentInstruction->getOpcodeName());
  for (std::vector<llvm::Instruction *>::iterator instr = instructions.begin();
                                                  instr != instructions.end();
                                                  ++instr) {
    if (flag) {
      std::vector<std::string> units = architecture->getUnit((*instr)->getOpcodeName());
      if (resourcesConflict(unitsCurrent, units))
        return *instr;
    }
    if (*instr == currentInstruction)
      flag = true;
  }
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

void ModuloScheduling::schedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime, int t, int delta) {
  
  while(getFirstConflictingInstruction(currentI, t) != NULL){
    unschedule(currentI, schedTime);
  }
  (*schedTime)[currentI] = t;
  
}

void ModuloScheduling::unschedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime){
  (*schedTime)[currentI] = -1;
  std::vector<std::string> units = architecture->getUnit(currentI->getOpcodeName());
  for (std::vector<std::string>::iterator unit = units.begin();
                                          unit != units.end();
                                         ++unit) {
    std::vector<llvm::Instruction *> unitTime = resourceTalbe[unit];
    if (unitTime != NULL) {
      
    }
  }
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
