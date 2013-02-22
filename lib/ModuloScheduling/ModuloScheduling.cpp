
#include "cot/AllPasses.h"
#include "cot/ModuloScheduling.h"
#include "cot/FileParser.h"

// #include "llvm/Support/raw_ostream.h"
// #include "llvm/Function.h"
// #include "llvm/Support/ErrorHandling.h"
// #include "llvm/PassAnalysisSupport.h"
#include "llvm/Instructions.h"
// #include "llvm/Analysis/LoopInfo.h"
#include "llvm/LLVMContext.h"
#include "llvm/Support/IRBuilder.h"
// #include "llvm/Transforms/Utils/BasicBlockUtils.h"
// #include "llvm/DerivedTypes.h"
// #include "llvm/Type.h"

#include "llvm/Module.h"
// #include "llvm/Function.h"
// #include "llvm/PassManager.h"
// #include "llvm/Analysis/Verifier.h"
// #include "llvm/Assembly/PrintModulePass.h"

// #include "llvm/ADT/StringRef.h"

// #include <map>
// #include <vector>
// #include <string>
// #include <math.h>

// #include <stdlib.h>
// #include <time.h>

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
  FileParser &fp = getAnalysis<FileParser>();
  architecture = fp.getArchitecture();
  std::vector<llvm::Instruction *> instructions;
  std::vector<Operand> ops = architecture->getAllArch();
  for (std::vector<Operand>::iterator op = ops.begin();
                                      op != ops.end();
                                      ++op) {
    std::string unit = op->getUnit();
    if (resourceTable.find(unit) == resourceTable.end()) {
      std::vector<llvm::Instruction *> vec;
      resourceTable.insert(std::pair<std::string, std::vector<llvm::Instruction *> > (unit, vec));
    }
  }

  // TEMP VARS
  blocksCount = 0;
  instructionsCount = 0;

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
      instructions.clear();
      // Extract the instructions from the basic block
      for(llvm::BasicBlock::iterator istr = *currentBlock->begin(), 
                                     end = *currentBlock->end(); 
                                     istr != end; 
                                     ++istr) {
          instructionsCount += 1;
          instructions.push_back(istr);
      }
    }
  }

  // Apply the algorithm
  instructions = doScheduling(instructions);
  // createNewBlock(currentBlock, instructions);
  // createNewBlock(currentBlock, instructions);
  // deleteOldBlock();

  // Set the global variable, so it's accessible by the print method 
  scheduledInstructions = instructions;
  
  return true;   // Program modified
}


void ModuloScheduling::createNewBlock(llvm::BasicBlock *CB, std::vector<llvm::Instruction *> instructions)
{
  for (std::vector<llvm::Instruction *>::iterator ii = instructions.begin();
                                                  ii != instructions.end();
                                                  ++ii) {
    llvm::errs() << "~~~\t" << (*ii)->getOpcodeName() << "\t~~~\n";
    (*ii)->eraseFromParent();
  }

  llvm::BasicBlock::iterator istr = *CB->begin();
  llvm::Instruction *pi = istr;
  u_int tot = resourceTable["ALU1"].size();
  for (u_int i = 0; i < tot - 1; ++i) {
    llvm::Instruction *is = resourceTable["ALU1"][i];
    if (is != NULL) {
      llvm::errs() << "Clone: " << is->getOpcodeName() << "\n";
      llvm::Instruction *newInst = is->clone();
      CB->getInstList().insert(pi, newInst);
    }
  }

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
  int deltaMin = std::max(resourcesBoundEstimator(instructions), dataDependenceBoundEstimator(instructions));

  // Order instructions by a priority
  instructions = prioritizeInstructions(instructions);

  // Infinite loop: we have no upper bound for delta
  for(int delta = deltaMin; ; ++delta){

    int budget = instructions.size() * 3;                // Init the number of attempts

    // Init the resource table
    for (std::map<std::string, std::vector<llvm::Instruction *> >::iterator it = resourceTable.begin();
                                                              it != resourceTable.end();
                                                              ++it) {
      for (int i = it->second.size(); i < delta; ++i)
        resourceTable[it->first].push_back(NULL);
    }

    // Init the data structures
    for(unsigned i = 0; i < instructions.size(); ++i){
      lastTime.insert(std::pair<llvm::Instruction *, int> (instructions[i], 0));
      schedTime.insert(std::pair<llvm::Instruction *, int> (instructions[i], -1));
    }

    llvm::errs() <<  "\nSCHEDULING LOOP - delta = " << delta << ", budget = " << budget << " ---------------------------------\n";
    // While we have attempts and some instructions are not scheduled, 
    // try to schedule them, decrementing the budget of attempts at every iteration
    for(llvm::Instruction* currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime);
        budget > 0 && currentInstruction != NULL;
        currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime), budget--) {

      llvm::errs() <<  "\n@@\n- Current instruction: " << (*currentInstruction) << "\n";

      // Find all the predecessors, using llvm::User methods (intra-loop body)
      std::set<llvm::Instruction *> predecessors = findPredecessors(instructions, currentInstruction);

      llvm::errs() <<  "- Predecessors:\n";
      for (std::set<llvm::Instruction *>::iterator firstP = predecessors.begin(), 
                                                   lastP = predecessors.end(); 
                                                   firstP != lastP; 
                                                   ++firstP){
        if(llvm::Instruction * currentP = llvm::dyn_cast<llvm::Instruction>(*firstP)) {
          llvm::errs() <<  "  " << (*currentP) << ", schedTime: " << schedTime[currentP] << "\n";
        }
      }

      int tMin = 0;

      // Find the most slow predecessor and update tMin
      for (std::set<llvm::Instruction *>::iterator firstP = predecessors.begin(), 
                                                   lastP = predecessors.end(); 
                                                   firstP != lastP; 
                                                   ++firstP){

        if(llvm::Instruction * currentP = llvm::dyn_cast<llvm::Instruction>(*firstP)){

          // If the predecessor is scheduled
          if(schedTime[currentP] != -1){
            int currentSchedTime = schedTime[currentP] + delay(currentP, currentInstruction, instructions, delta);
            tMin = std::max(tMin, currentSchedTime);
          }
        }
      }

      llvm::errs() <<  "Try to schedule: " << (*currentInstruction) <<"\n";
      // Try to schedule h between tMin and tMin + delta - 1
       for(int t = tMin; t < tMin + delta - 1; ++t){
        if(schedTime[currentInstruction] == -1){

          // If the istruction can be scheduled
          if(canBeScheduled(currentInstruction, t, delta)){
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
      std::set<llvm::Instruction *> successors = findSuccessors(instructions, currentInstruction);

      llvm::errs() <<  "- Successors:\n";
      for (std::set<llvm::Instruction *>::iterator firstS = successors.begin(), 
                                                   lastS = successors.end(); 
                                                   firstS != lastS; 
                                                   ++firstS){
        if(llvm::Instruction * currentS = llvm::dyn_cast<llvm::Instruction>(*firstS)){
          llvm::errs() <<  "  " << (*currentS) << ", schedTime: " << schedTime[currentS] << "\n";
        }
      }

      // // if successor of h is execute before the end of h, then unschedule it
      // for (std::set<llvm::Instruction *>::iterator firstS = successors.begin(), 
      //                                              lastS = successors.end(); 
      //                                              firstS != lastS; 
      //                                              ++firstS){
      //   if(llvm::Instruction * currentS = llvm::dyn_cast<llvm::Instruction>(*firstS)){
      //     if(schedTime[currentS] != -1){
      //       if(schedTime[currentInstruction] + delay(currentInstruction, currentS, instructions, delta) > schedTime[currentS])
      //         unschedule(currentInstruction, &schedTime);
      //     }
      //   }
      // }

      // for debug!
      // if (budget <= 26) {
      //   std::vector<llvm::Instruction *> v;
      //   return v;
      // }

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
  
  return scheduledInstructions;
}


int ModuloScheduling::resourcesBoundEstimator(std::vector<llvm::Instruction *> instructions) {

  llvm::errs() <<  "ENTERED IN: resourcesBoundEstimator -------------------------------------------\n";

  // Get all the operands supported by the architecture
  std::vector<std::string> operands = architecture->getSupportedOperand();
  std::map<std::string, int> instructionsMap;
  int finalBound = 0, tempBound = 0, numCicliIstr = 0;

  // Add all the operand available on the architecture in the new local map
  for (std::vector<std::string>::iterator op = operands.begin();
                                          op != operands.end();
                                          ++op){
    instructionsMap.insert(std::pair<std::string, int>(*op, 0));
  }

  // For all the instructions to be scheduled
  for (std::vector<llvm::Instruction *>::iterator istr = instructions.begin();
                                                  istr != instructions.end();
                                                  ++istr) {
    std::string op = (*istr)->getOpcodeName();

    if (instructionsMap.find(op) != instructionsMap.end())
      ++instructionsMap[op];
    else
      instructionsMap.insert(std::pair<std::string, int>(op, 1));       // Add the new operand: it will be considered for the bound estimation as well
  }

  // For every operand
  for (std::map<std::string, int>::iterator record = instructionsMap.begin();
                                            record != instructionsMap.end();
                                            ++record) {
    numCicliIstr = architecture->getCycle(record->first);
    std::map<std::string, int>::iterator it = instructionsMap.find(record->first);
    if (it != instructionsMap.end() && numCicliIstr > 0) {

      // Estimate the bound of the current operand
      tempBound = ceil(numCicliIstr * it->second / architecture->getNumberOfUnits(record->first));
      finalBound = std::max(tempBound, finalBound);
    }

    llvm::errs() <<  record->first << ": " << it->second << " occurrencies\n";
  }

  llvm::errs() <<  "Lower bound estimation: " << finalBound << "\n";

  llvm::errs() <<  "EXITING: resourcesBoundEstimator ----------------------------------------------\n";

  return finalBound;
}


int ModuloScheduling::dataDependenceBoundEstimator(std::vector<llvm::Instruction *> instructions) {

  llvm::errs() <<  "ENTERED IN: dataDependenceBoundEstimator -----------------------------------------\n";

  // Create a map: {instruction, isVisited}
  // Init isVisited to false
  std::map<llvm::Instruction *, bool> instructionsMap;
  for(unsigned j = 0; j < instructions.size(); ++j) {
    instructionsMap.insert(std::pair<llvm::Instruction *, int> (instructions[j], false));
  }

  int finalBound = 0;
  int tempBound = 0;

  // For all the instructions
  for(unsigned i = 0; i < instructions.size(); ++i) {

    tempBound = 0;

    // Skip "phi" function: they will not be scheduled on the pipeline
    if(!llvm::StringRef("phi").equals(instructions[i]->getOpcodeName())){

      llvm::errs() <<  "-------------------------------------------------------------\n";

      // Recursively find definitions of the operands of the instruction
      tempBound = findDefRecursive(instructionsMap, instructions[i], 0);
    }

    finalBound = std::max(tempBound, finalBound);
  }

  llvm::errs() <<  "Lower bound estimation: " << finalBound << "\n";

  llvm::errs() <<  "EXITING: dataDependenceBoundEstimator -----------------------------------------\n";

  return finalBound;
}


int ModuloScheduling::findDefRecursive(std::map<llvm::Instruction *, bool> instructionsMap, llvm::Instruction * currentI, int offset) const{
  
  // Print to screen
  for (int i = 0; i <= offset; ++i)
    llvm::errs() <<  "  ";
  
  if(!llvm::StringRef("phi").equals(currentI->getOpcodeName())){
    llvm::errs() <<  offset << " -" << (*currentI) << "\n";
  }else{
    llvm::errs() <<  "PHI -" << (*currentI) << "\n";
  }

  // Set the current instruction as visited
  instructionsMap[currentI] = true;

  // Final and temp offset
  int finalOffset = 0;
  int currentOffset = 0;

  // For all the instruction defining one of the current instruction's operators
  for (llvm::User::op_iterator i = currentI->op_begin(), e = currentI->op_end(); i != e; ++i) {

    // Reset the current offset 
    currentOffset = 0;

    // Dynamic cast
    if(llvm::Instruction * definerI = llvm::dyn_cast<llvm::Instruction>(*i)){
          
      // If the definer instruction hasn't been visited
      if(instructionsMap[definerI] == false){

        if(llvm::StringRef("phi").equals(definerI->getOpcodeName())){
          currentOffset = findDefRecursive(instructionsMap, definerI, offset);      // Find definers recursively, without incrementing the offset (ignore "phi" functions)
        }else{
          currentOffset = findDefRecursive(instructionsMap, definerI, offset + architecture->getCycle(definerI->getOpcodeName()));  // Find definers recursively, incrementing the offset
        }
      }else{
        currentOffset = offset;
      }
    }

    finalOffset = std::max(currentOffset, finalOffset);
  }

  return finalOffset;
}

std::vector<llvm::Instruction *> ModuloScheduling::prioritizeInstructions(std::vector<llvm::Instruction *> instructions){
  llvm::errs() <<  "ENTERED IN: prioritizeInstructions --------------------------------------------\n";
  llvm::errs() <<  "No heuristic has been implemented to sort the instructions\n";
  llvm::errs() <<  "EXITING: prioritizeInstructions -----------------------------------------------\n";
  
  // No heuristic has been implemented to sort the instructions. 
  return instructions;
}

llvm::Instruction* ModuloScheduling::findHighPriorityUnscheduledInstruction(std::vector<llvm::Instruction *> instructions, std::map<llvm::Instruction *, int>  schedTime){
  for (u_int i = 0; i < instructions.size(); ++i) {
    llvm::Instruction* currentInstr = instructions[i];
    if(schedTime[currentInstr] == -1){
      return currentInstr;           // Unscheduled instruction found
    }
  }
  return NULL;                       // NO unscheduled instruction found
  
  // if (scheduleCompleted(schedTime))
  //   return NULL;
  // // take one at random
  // srand(time(NULL));
  // llvm::Instruction* currentInstr;
  // u_int cont = 0;
  // do {
  //   u_int idx = rand() % instructions.size() + 1;
  //   currentInstr = instructions[idx];
  //   ++cont;
  // }while (schedTime[currentInstr] != -1 && cont < instructions.size());
  // if (schedTime[currentInstr] == -1)
  //   return currentInstr;

  // for (u_int i = 0; i < instructions.size(); ++i) {
  //   currentInstr = instructions[i];
  //   if(schedTime[currentInstr] == -1){
  //     return currentInstr;           // Unscheduled instruction found
  //   }
  // }
  // return NULL;
}

std::set<llvm::Instruction *> ModuloScheduling::findPredecessors(std::vector<llvm::Instruction *> instructions, llvm::Instruction * currentI) {

  // Init the vector of predecessors
  std::set<llvm::Instruction *> pred;

  // For all the instruction defining one of the current instruction's operators
  for (llvm::User::op_iterator i = currentI->op_begin(), e = currentI->op_end(); i != e; ++i) {

    // Try to cast
    if(llvm::Instruction * definerI = llvm::dyn_cast<llvm::Instruction>(*i)){
      
      // If it's a phi instruction
      if(llvm::StringRef("phi").equals(definerI->getOpcodeName())){

        // Find its predecessors
        std::set<llvm::Instruction *> recursivePred = findPredecessors(instructions, definerI);

        // Add them in the vector of predecessors, if it's not present
        for (std::set<llvm::Instruction *>::iterator first = recursivePred.begin(), 
                                                     last = recursivePred.end(); 
                                                     first != last; 
                                                     ++first){
          // Try to cast
          if(llvm::Instruction * currentP = llvm::dyn_cast<llvm::Instruction>(*first))
            // Add it in the vector of predecessors, if it's not present, it's different from itself and it's in the loop body
            if(pred.find(currentP) == pred.end() && currentP != currentI && std::find(instructions.begin(), instructions.end(), currentP) != instructions.end()){
              pred.insert(currentP);
            }
        }

      }else{
        // Add it to the predecessors, if it's not present, it's different from itself and it's in the loop body
        if(pred.find(definerI) == pred.end() && definerI != currentI && std::find(instructions.begin(), instructions.end(), definerI) != instructions.end()){
          pred.insert(definerI);
        }
      }
    }
  }

  return pred;
}

std::set<llvm::Instruction *> ModuloScheduling::findSuccessors(std::vector<llvm::Instruction *> instructions, llvm::Instruction * currentI) {

  // Init the vector of successors
  std::set<llvm::Instruction *> succ;

  // For all the instruction using the current instruction's result
  for (llvm::value_use_iterator<llvm::User> i = currentI->use_begin(), e = currentI->use_end(); i != e; ++i) {

    // Try to cast
    if(llvm::Instruction * userI = llvm::dyn_cast<llvm::Instruction>(*i)){
      
      // If it's a phi instruction
      if(llvm::StringRef("phi").equals(userI->getOpcodeName())){

        // Find its successors
        std::set<llvm::Instruction *> recursiveSucc = findSuccessors(instructions, userI);

        // Add them in the vector of successors, if they're not present
        for (std::set<llvm::Instruction *>::iterator first = recursiveSucc.begin(), 
                                                     last = recursiveSucc.end(); 
                                                     first != last; 
                                                     ++first){
          // Try to cast
          if(llvm::Instruction * currentS = llvm::dyn_cast<llvm::Instruction>(*first))
            // Add it to the successors, if it's not present, it's different from itself and it's in the loop body
            if(succ.find(currentS) == succ.end() && currentS != currentI && std::find(instructions.begin(), instructions.end(), currentS) != instructions.end())
              succ.insert(currentS);              
        }

      }else{
        // Add it to the successors, if it's not present, it's different from itself and it's in the loop body
        if(succ.find(userI) == succ.end() && userI != currentI && std::find(instructions.begin(), instructions.end(), userI) != instructions.end())
          succ.insert(userI);
      }
    }
  }

  return succ;
}

int ModuloScheduling::delay(llvm::Instruction * firstInstruction, llvm::Instruction * secondInstruction, std::vector<llvm::Instruction *> instructions, int delta){
  llvm::errs() <<  "ENTERED IN: delay --------------------------------------------\n";
  llvm::errs() <<  "- firstInstruction: " << (*firstInstruction) << "\n";
  llvm::errs() <<  "- secondInstruction: " << (*secondInstruction) << "\n";
  llvm::errs() <<  "- delta: " << delta << "\n";

  int delay = 0;
  int latency = architecture->getCycle(secondInstruction->getOpcodeName());
  int k = 0;

  for (llvm::User::op_iterator use = secondInstruction->op_begin();
                               use != secondInstruction->op_end();
                               ++use) {
    if(llvm::Instruction *useInstruction = llvm::dyn_cast<llvm::Instruction>(*use)) {
      if (useInstruction == firstInstruction) {
        for (std::vector<llvm::Instruction *>::iterator instr = instructions.begin();
                                                        instr != instructions.end();
                                                        ++instr) {
          if (*instr == secondInstruction) {
            k = 1;
            break;
          }
          if (*instr == firstInstruction)
            break;
        }
      }
    }
  }
  delay = latency - k * delta;
  llvm::errs() <<  "- latency:" << latency << "\n";
  llvm::errs() <<  "- k:" << k << "\n";

  llvm::errs() <<  "- Output delay:" << delay << "\n";
  llvm::errs() <<  "EXITING: delay -----------------------------------------------\n";

  return delay;
}

/*
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
*/

void ModuloScheduling::printResourceTable() {
  std::vector<cot::Operand> ops = architecture->getAllArch();
  int tot = resourceTable[ops[0].getUnit()].size();
  std::vector<std::string> done;
  llvm::errs() <<  "--------------- RESOURCE TABLE ---------------\n";
  llvm::errs() <<  "\t";
  for (int i = 0; i < tot; ++i)
    llvm::errs() <<  "  " << (i + 1) << "  \t";
  llvm::errs() <<  "\n";
  for (std::vector<cot::Operand>::iterator op = ops.begin(); op != ops.end(); ++op) {
    if (std::find(done.begin(), done.end(), op->getUnit()) == done.end()) {
      done.push_back(op->getUnit());
      llvm::errs() <<  op->getUnit() <<"\t";
      for (int i = 0; i < tot; ++i)
        if (resourceTable[op->getUnit()][i] != NULL) {
          llvm::errs() <<  resourceTable[op->getUnit()][i]->getOpcodeName() << "\t";
        } else {
          llvm::errs() <<  "nop\t";
        }
      llvm::errs() <<  "\n";
    }
  }
  llvm::errs() <<  "--------------- ~~~~~~~~~~~~~~ ---------------\n";
}

bool ModuloScheduling::canBeScheduled(llvm::Instruction * currentInstruction, int t, int delta) {
  std::vector<std::string> units = architecture->getUnit(currentInstruction->getOpcodeName());
  int latency = architecture->getCycle(currentInstruction->getOpcodeName());
  bool canBeScheduled = true;

  for (std::vector<std::string>::iterator unit = units.begin();
                                          unit != units.end();
                                          ++unit) {
    canBeScheduled = true;
    for (int i = t; i < (t + latency); ++i) {
      if (resourceTable[*unit][t % delta] != NULL) {
        canBeScheduled = false;
        break;
      }
    }
    if (canBeScheduled == true)
      return canBeScheduled;
  }
  return canBeScheduled;
}

llvm::Instruction* ModuloScheduling::getFirstConflictingInstruction(llvm::Instruction * currentInstruction, int t, std::string schedulingUnit, int delta) {
  
  llvm::errs() <<  "ENTERED IN: getFirstConflictingInstruction --------------------------------------------\n";

  // NOTE: in SSA form, we only have conflicts on resources

  // Get all the units available to execute the instruction
  std::vector<std::string> units = architecture->getUnit(currentInstruction->getOpcodeName());
  int latency = architecture->getCycle(currentInstruction->getOpcodeName());

  // for every t check if there is a conflict
  for (int i = t; i < (t + latency); ++i) {
    if (resourceTable[schedulingUnit][t % delta] != NULL) {
      // return the conflict instruction
      llvm::errs() <<  "conflict: " << resourceTable[schedulingUnit][t % delta] << "\n";
      return resourceTable[schedulingUnit][t % delta];
    }
  }

  // // For every unit, check if there's a conflicting instruction
  // llvm::Instruction * conflictingInstruction = NULL;
  // for (std::vector<std::string>::iterator unit = units.begin();
  //                                         unit != units.end();
  //                                         ++unit) {
  //   if (resourceTable.find(*unit) != resourceTable.end()) {

  //     // t must be in [0, resourceTable[*unit].size = currentDelta]
  //     if (0 < (u_int)t && (u_int)t < resourceTable[*unit].size()) {

  //       // Return the instruction allocated on that unit in that moment, if any
  //       if (resourceTable[*unit][t] != NULL){
  //         llvm::errs() <<  "Found conflicting instruction:" << (*(resourceTable[*unit][t])) << "\n";
  //         llvm::errs() <<  "EXITING: getFirstConflictingInstruction -----------------------------------------------\n";
  //         return resourceTable[*unit][t];
  //       }
  //     }
  //   }
  // }

  llvm::errs() <<  "No conflicting instruction found\n";
  llvm::errs() <<  "EXITING: getFirstConflictingInstruction -----------------------------------------------\n";
  return NULL;
}

bool ModuloScheduling::scheduleCompleted(std::map<llvm::Instruction *, int> schedTime){

  llvm::errs() <<  "ENTERED IN: scheduleCompleted --------------------------------------------\n";

  // The scheduling is completed if all the schedTimes are assigned (= different from -1)
  for(std::map<llvm::Instruction *, int>::iterator iter = schedTime.begin(); 
          iter != schedTime.end(); 
          ++iter){
        if(iter->second == -1){
          llvm::errs() <<  "At least one instruction has not been scheduled\n";
          llvm::errs() <<  "EXITING: scheduleCompleted -----------------------------------------------\n";
          return false;
        }
  }
  llvm::errs() <<  "All instructions scheduled\n";
  llvm::errs() <<  "EXITING: scheduleCompleted -----------------------------------------------\n";
  return true;
}

void ModuloScheduling::schedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime, int t, int delta) {

  // delta must be greater than zero.
  // if (delta == 0)
  //  return;

  int latency = architecture->getCycle(currentI->getOpcodeName());
  std::vector<std::string> units = architecture->getUnit(currentI->getOpcodeName());

  std::string schedulingUnit;

  if(canBeScheduled(currentI, t, delta)){
    // Find the first unit where the instruction can be scheduled
    for (std::vector<std::string>::iterator unit = units.begin();
                                            unit != units.end();
                                            ++unit) {
      schedulingUnit = *unit;
      for (int i = t; i < (t + latency); ++i) {
        if (resourceTable[*unit][t % delta] != NULL) {
          schedulingUnit.clear();
          break;
        }
      }
      if (!schedulingUnit.empty())
        break;
    }
  }else{
    // Greedy: select the first unit
    schedulingUnit = units[0];

    // Unschedule all the conflicting instructionsMap
    for(llvm::Instruction * conflictingInstruction = getFirstConflictingInstruction(currentI, t, schedulingUnit, delta);
                                      conflictingInstruction != NULL;
                                      conflictingInstruction = getFirstConflictingInstruction(currentI, t, schedulingUnit, delta)){
      unschedule(conflictingInstruction, schedTime);
    }
  }

  // Schedule the current instruction at time t
  (*schedTime)[currentI] = t;
 
  // Update the resourceTable
  for (int i = t; i < (t + latency); ++i) {
    int index = i % delta;
    resourceTable[schedulingUnit][index] = currentI;
  }

  printResourceTable();
}

void ModuloScheduling::unschedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime){
  (*schedTime)[currentI] = -1;
  std::vector<std::string> units = architecture->getUnit(currentI->getOpcodeName());
  for (std::vector<std::string>::iterator unit = units.begin();
                                          unit != units.end();
                                         ++unit) {
    if (resourceTable.find(*unit) != resourceTable.end()) {
      for (u_int i = 0; i < resourceTable[*unit].size(); ++i) {
        if (resourceTable[*unit][i] == currentI)
          resourceTable[*unit][i] = NULL;
      }
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
                      false,
                      false)
INITIALIZE_PASS_END(ModuloScheduling,
                    "modulo-scheduling",
                    "Iterative Modulo Scheduling algorithm implementation",
                    false,
                    false)
