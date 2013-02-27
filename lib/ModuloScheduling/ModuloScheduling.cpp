
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

#include <stdlib.h>
#include <time.h>

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

  // Init the architecture
  FileParser &fp = getAnalysis<FileParser>();
  architecture = fp.getArchitecture();

  // Init the resource table
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

  // Retrieve blocks
  const std::vector<llvm::BasicBlock *> blocks = L->getBlocks();

  // Init here the instructions list, if you want to consider the induction variable in the loop body
  // std::vector<llvm::Instruction *> instructions;
  
  // For every block
  for(unsigned i = 0; i < blocks.size(); ++i) {

    llvm::BasicBlock *currentBlock = blocks[i];

    llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop> LIB = llvm::LoopInfoBase<llvm::BasicBlock, llvm::Loop>();
    LIB.changeLoopFor(currentBlock, L);
    if (!LIB.isLoopHeader(currentBlock)) {

       // Init here the instructions list, if you DON'T want to consider the induction variable in the loop body
      std::vector<llvm::Instruction *> instructions;

      // Extract the instructions from the basic block
      for(llvm::BasicBlock::iterator istr = *currentBlock->begin(), 
                                     end = *currentBlock->end(); 
                                     istr != end; 
                                     ++istr) {
          instructions.push_back(istr);
      }

      // Apply the algorithm here, if you DON'T want to consider the induction variable in the loop body
      instructions = doScheduling(instructions);
    }
  }

  // Apply the algorithm here, if you want to consider the induction variable in the loop body
  // instructions = doScheduling(instructions);


  // REMOVE THIS ------------------------------------------------------------------------------------------------------------------------------------
  // llvm::BasicBlock *newBlock = createNewBlock(currentBlock, instructions);
  // deleteOldBlock();

  // Set the global variable, so it's accessible by the print method 
  // scheduledInstructions = instructions;
  // REMOVE THIS ------------------------------------------------------------------------------------------------------------------------------------

  return true;   // Program modified
}



// REMOVE THIS ------------------------------------------------------------------------------------------------------------------------------------
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
// REMOVE THIS ------------------------------------------------------------------------------------------------------------------------------------



bool ModuloScheduling::doFinalization(){
  return false;   // Program not modified
}

void ModuloScheduling::print(llvm::raw_ostream &OS, 
                             const llvm::Module *Mod) const {
  if(!Mod)
    return;

  // REMOVE THIS ------------------------------------------------------------------------------------------------------------------------------------
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
  // REMOVE THIS ------------------------------------------------------------------------------------------------------------------------------------
}


std::vector<llvm::Instruction *> ModuloScheduling::doScheduling(std::vector<llvm::Instruction *> originalInstructions){

  llvm::errs() << "\n\n";
  llvm::errs() << "################################################################################\n";
  llvm::errs() << "####################            MODULO SCHEDULING           ####################\n";
  llvm::errs() << "################################################################################";
  llvm::errs() << "\n\n";

  // Declarations
  std::map<llvm::Instruction *, int> lastTime;
  std::map<llvm::Instruction *, int> schedTime;

  // Lower bound for delta
  int deltaMin = std::max(resourcesBoundEstimator(originalInstructions), dataDependenceBoundEstimator(originalInstructions));

  // Order instructions by a priority
  std::vector<llvm::Instruction *> instructions = prioritizeInstructions(originalInstructions);

  // Infinite loop: we have no upper bound for delta
  for(int delta = deltaMin; ; ++delta){

    // Init the number of attempts
    int budget = instructions.size() * 3;        

    // Init the resource table
    for (std::map<std::string, std::vector<llvm::Instruction *> >::iterator it = resourceTable.begin();
                                                              it != resourceTable.end();
                                                              ++it) {
      for (u_int i = 0; i < it->second.size(); ++i)
        resourceTable[it->first][i] = NULL;
      for (int i = it->second.size(); i < delta; ++i)
        resourceTable[it->first].push_back(NULL);
    }

    // Init the data structures
    lastTime.clear();
    schedTime.clear();
    for(unsigned i = 0; i < instructions.size(); ++i){
      lastTime.insert(std::pair<llvm::Instruction *, int> (instructions[i], 0));
      schedTime.insert(std::pair<llvm::Instruction *, int> (instructions[i], -1));
    }

    llvm::errs() << "\nSCHEDULING LOOP - delta = " << delta << ", budget = " << budget << " #######################################################################################################\n";
    
    // While we have attempts and some instructions are not scheduled, 
    // try to schedule them, decrementing the budget of attempts at every iteration
    for (llvm::Instruction* currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime);
                            budget > 0 && currentInstruction != NULL;
                            currentInstruction = findHighPriorityUnscheduledInstruction(instructions, schedTime), budget--) {

      llvm::errs() << "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n";
      llvm::errs() << "- Current instruction: " << (*currentInstruction) << "\n";

      // Find all the predecessors, using llvm::User methods (intra-loop body)
      std::set<llvm::Instruction *> predecessors = findPredecessors(instructions, currentInstruction);

      llvm::errs() << "- Predecessors:\n";
      for (std::set<llvm::Instruction *>::iterator firstP = predecessors.begin(), 
                                                   lastP = predecessors.end(); 
                                                   firstP != lastP; 
                                                   ++firstP){
        if(llvm::Instruction * currentP = llvm::dyn_cast<llvm::Instruction>(*firstP)) {
          llvm::errs() << "  " << (*currentP) << ", schedTime: " << schedTime[currentP] << "\n";
        }
      }

      // Init tMin
      int tMin = 0;
      // Find the most slow predecessor and update tMin
      for (std::set<llvm::Instruction *>::iterator firstP = predecessors.begin(), 
                                                   lastP = predecessors.end(); 
                                                   firstP != lastP; 
                                                   ++firstP){

        if(llvm::Instruction * currentP = llvm::dyn_cast<llvm::Instruction>(*firstP)){

          // If the predecessor is scheduled
          if(schedTime[currentP] != -1){
            // Calculate the earliest time the current instruction can be scheduled at
            int currentSchedTime = schedTime[currentP] + delay(currentP, currentInstruction, originalInstructions, delta, schedTime);
            tMin = std::max(tMin, currentSchedTime);
          }
        }
      }

      llvm::errs() << "Try to schedule:\n " << (*currentInstruction) << "\nbetween " << tMin << " and " << tMin + delta - 1 << ": ";
      
       // Try to schedule the current instruction between tMin and tMin + delta - 1
       for(int t = tMin; t <= tMin + delta - 1; ++t){
        if(schedTime[currentInstruction] == -1){

          // If the istruction can be scheduled, schedule it
          if(canBeScheduled(currentInstruction, t, delta)){
            schedule(currentInstruction, &schedTime, t, delta);

            llvm::errs() << " scheduled at time " << t << "\n";
            printResourceTable();
            printSchedTime(schedTime);
          }
        }
      }

      // If the instruction has not been scheduled, 
      // schedule it as soon as possible, resolving all the existing conflicts
      if(schedTime[currentInstruction] == -1){

        // Increase the last time of scheduling and use it, 
        // if it's greater than the earliest time of schedule
        int t = std::max(tMin, 1 + lastTime[currentInstruction]);
        schedule(currentInstruction, &schedTime, t, delta);

        llvm::errs() << " can't schedule in this interval => FORCE SCHEDULING at time: " << t << "\n";    
        printResourceTable();
        printSchedTime(schedTime);
      }

      // Remember the last schedule time of the current instruction,
      // as it may be useful for the next attempts in order to explore a better scheduling
      lastTime[currentInstruction] = schedTime[currentInstruction];

      // Find all the successors, using llvm::Value methods (intra-loop body)
      std::set<llvm::Instruction *> successors = findSuccessors(instructions, currentInstruction);

      llvm::errs() << "- Successors:\n";
      for (std::set<llvm::Instruction *>::iterator firstS = successors.begin(), 
                                                   lastS = successors.end(); 
                                                   firstS != lastS; 
                                                   ++firstS){
        if(llvm::Instruction * currentS = llvm::dyn_cast<llvm::Instruction>(*firstS)){
          llvm::errs() << "  " << (*currentS) << ", schedTime: " << schedTime[currentS] << "\n";
        }
      }

      // if successor of the current instruction is execute before its end, then unschedule it
      for (std::set<llvm::Instruction *>::iterator firstS = successors.begin(), 
                                                   lastS = successors.end(); 
                                                   firstS != lastS; 
                                                   ++firstS){
        if(llvm::Instruction * currentS = llvm::dyn_cast<llvm::Instruction>(*firstS)){

          // If the successor has been scheduled
          if(schedTime[currentS] != -1){
            if(schedTime[currentInstruction] + delay(currentInstruction, currentS, originalInstructions, delta, schedTime) > schedTime[currentS])
              unschedule(currentS, &schedTime);

              printResourceTable();
              printSchedTime(schedTime);
          }
        }
      }

      // Remove from the scheduling all the instructions (other than currentInstruction) involved in a resource conflict
      /* ALREADY DONE IN SCHEDULE(...)
      for(llvm::Instruction* conflictingInstruction = getFirstConflictingInstruction(currentInstruction, schedTime[currentInstruction]); 
          conflictingInstruction != NULL && conflictingInstruction != currentInstruction; 
          conflictingInstruction = getFirstConflictingInstruction(currentInstruction, t)){
        schedTime[conflictingInstruction] = -1;
      }
      */

    }


    // If all instructions have been scheduled, 
    if(scheduleCompleted(schedTime)){

      // Map with schedule time used as key
      std::map<int, llvm::Instruction *> schedTimeMap;

      for (std::map<llvm::Instruction *, int>::iterator iter = schedTime.begin();
                                                        iter != schedTime.end(); 
                                                        ++iter){
        schedTimeMap.insert(std::pair<int, llvm::Instruction *> (iter->second, iter->first));
      }

      for (std::map<int, llvm::Instruction *>::iterator iter = schedTimeMap.begin();
                                                        iter != schedTimeMap.end();
                                                        ++iter){
        scheduledInstructions.push_back(iter->second);
      }
      
      return scheduledInstructions;
    }
  }
}


int ModuloScheduling::resourcesBoundEstimator(std::vector<llvm::Instruction *> instructions) {

  llvm::errs() << "\nENTERED IN: resourcesBoundEstimator -------------------------------------------\n";

  // Get all the operands supported by the architecture
  std::vector<std::string> operands = architecture->getSupportedOperand();
  std::map<std::string, int> instructionsMap;
  std::map<std::string, int> cyclePerUnit;
  int finalBound = 0, tempBound = 0, numOfCycles = 0;

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
    numOfCycles = architecture->getCycle(record->first);
    std::map<std::string, int>::iterator it = instructionsMap.find(record->first);
    if (it != instructionsMap.end() && numOfCycles > 0) {

      // Estimate the bound of the current operand
      tempBound = ceil(numOfCycles * it->second / architecture->getNumberOfUnits(record->first));

      llvm::errs() << record->first << ": " << numOfCycles << "*" << it->second << "/" << architecture->getNumberOfUnits(record->first) << " = " << tempBound << "\n";

      // Group by unit and sum tempBound
      std::vector<std::string> units = architecture->getUnit(record->first);
      for (std::vector<std::string>::iterator unit = units.begin();
                                                unit != units.end();
                                                ++unit) {
        if (cyclePerUnit.find(*unit) == cyclePerUnit.end())
          cyclePerUnit.insert(std::pair<std::string, int> (*unit, 0));
        cyclePerUnit[*unit] += tempBound;
      }
      // finalBound = std::max(tempBound, finalBound);
    }
  }

  for (std::map<std::string, int>::iterator i = cyclePerUnit.begin(); i != cyclePerUnit.end(); ++i)
    finalBound = std::max((*i).second, finalBound);

/*
  std::map<std::string, int> instructionsPerUnit;
  std::map<std::string, int> maxCyclesPerUnit;

  int finalBound = 0, tempBound = 0, cycles = 0;
  std::vector<int> v = [0 ,0 ];


  // For all the instructions to be scheduled
  for (std::vector<llvm::Instruction *>::iterator istr = instructions.begin();
                                                  istr != instructions.end();
                                                  ++istr) {

    // Get suitable units
    std::vector<std::string> suitableUnits = architecture->getUnit((*istr)->getOpcodeName());

    // For every suitable unit
    for (std::vector<std::string>::iterator unit = suitableUnits.begin(); 
                                            unit != suitableUnits.end(); 
                                            ++unit){

      // Add the unit to the list of available units (if not present)
      if (instructionsPerUnit.find(*unit) == instructionsPerUnit.end()){
        instructionsPerUnit.insert(std::pair<std::string, int>(*unit, 0));
        maxCyclesPerUnit.insert(std::pair<std::string, int>(*unit, 0));
      }

      // Increment the instructions count for that unit
      ++instructionsPerUnit[*unit];

      // Set the actual max number of cycles found
      maxCyclesPerUnit[*unit] = std::max(maxCyclesPerUnit[*unit], architecture->getCycle((*istr)->getOpcodeName()));
    }
  }

  // For every available unit
  for (std::map<std::string, int>::iterator unit = instructionsPerUnit.begin();
                                            unit != instructionsPerUnit.end();
                                            ++unit) {
    cycles = maxCyclesPerUnit[*unit];
    if (cycles > 0) {

      // Estimate the bound of the current operand
      tempBound = ceil(cycles * unit->second / architecture->getNumberOfUnits(unit->first));    // WTF??

      llvm::errs() << unit->first << ": " << cycles << "*" << it->second << "/" << architecture->getNumberOfUnits(unit->first) << " = " << tempBound << "\n";

      finalBound = std::max(tempBound, finalBound);
    }
  }
  */

  llvm::errs() << "Lower bound estimation: " << finalBound << "\n";

  llvm::errs() << "EXITING: resourcesBoundEstimator ----------------------------------------------\n";

  return finalBound;
}


int ModuloScheduling::dataDependenceBoundEstimator(std::vector<llvm::Instruction *> instructions) {

  llvm::errs() << "\nENTERED IN: dataDependenceBoundEstimator -----------------------------------------\n";

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

      llvm::errs() << "-------------------------------------------------------------\n";

      // Recursively find definitions of the operands of the instruction
      tempBound = findDefRecursive(instructions[i], instructionsMap, instructions[i], 0);

    }

    finalBound = std::max(tempBound, finalBound);
  }

  llvm::errs() << "Lower bound estimation: " << finalBound << "\n";

  llvm::errs() << "EXITING: dataDependenceBoundEstimator -----------------------------------------\n";

  return finalBound;
}


int ModuloScheduling::findDefRecursive(llvm::Instruction * startingI, std::map<llvm::Instruction *, bool> instructionsMap, llvm::Instruction * currentI, int offset) const{
  
  // Print to screen
  for (int i = 0; i <= offset; ++i)
    llvm::errs() << "  ";
  
  if(!llvm::StringRef("phi").equals(currentI->getOpcodeName())){
    llvm::errs() << offset << " -" << (*currentI) << "\n";
  }else{
    llvm::errs() << "PHI -" << (*currentI) << "\n";
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
          currentOffset = findDefRecursive(startingI, instructionsMap, definerI, offset);      // Find definers recursively, without incrementing the offset (ignore "phi" functions)
        }else{
          currentOffset = findDefRecursive(startingI, instructionsMap, definerI, offset + architecture->getCycle(definerI->getOpcodeName()));  // Find definers recursively, incrementing the offset
        }
      }else{

        // If the current instruction is the starting instruction
        if(definerI == startingI){
          // A cycle in the DDG has been found: set a valid currentOffset
          currentOffset = offset + architecture->getCycle(definerI->getOpcodeName());

          llvm::errs() << "===> LOOP FOUND ON THE DDG - Lenght: " << currentOffset << "\n";

        }else{
          // A cycle in the DDG has been found, but it doesn't involve the starting instruction: set an invalid currentOffset
          currentOffset = 0;
        }
      }
    }

    finalOffset = std::max(currentOffset, finalOffset);
  }

  return finalOffset;
}

std::vector<llvm::Instruction *> ModuloScheduling::prioritizeInstructions(std::vector<llvm::Instruction *> instructions){

  // Uncomment these lines to reproduce the priority order of the example
  if(instructions.size() == 10){
    std::vector<llvm::Instruction *> prioritizedInstructions;
    prioritizedInstructions.push_back(instructions[2]);     // c
    prioritizedInstructions.push_back(instructions[3]);     // d
    prioritizedInstructions.push_back(instructions[4]);     // e
    prioritizedInstructions.push_back(instructions[0]);     // a
    prioritizedInstructions.push_back(instructions[1]);     // b
    prioritizedInstructions.push_back(instructions[5]);     // f
    prioritizedInstructions.push_back(instructions[8]);     // j
    prioritizedInstructions.push_back(instructions[6]);     // g
    prioritizedInstructions.push_back(instructions[7]);     // h
    instructions = prioritizedInstructions;
  }

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

int ModuloScheduling::delay(llvm::Instruction * firstInstruction, llvm::Instruction * secondInstruction, std::vector<llvm::Instruction *> originalInstructions, int delta, std::map<llvm::Instruction *, int> schedTime){
  llvm::errs() << "\nENTERED IN: delay --------------------------------------------\n";
  llvm::errs() << "- firstInstruction: " << (*firstInstruction) << "\n";
  llvm::errs() << "- secondInstruction: " << (*secondInstruction) << "\n";
  llvm::errs() << "- delta: " << delta << "\n";

  // Init all the local variables
  int delay = 0, k = 0;
  int latency = architecture->getCycle(secondInstruction->getOpcodeName());

  // OLD IMPLEMENTATION: CHECK PRECEDENCES ON THE OLD LIST OF INSTRUCTION
  for (std::vector<llvm::Instruction *>::iterator instr = originalInstructions.begin();
                                                        instr != originalInstructions.end();
                                                        ++instr) {
    if (*instr == secondInstruction) {
      k = 1;
      break;
    }
    if (*instr == firstInstruction){
      k = 0;
      break;
    }
  }
  

  // Find which instruction comes before
  // if(schedTime[firstInstruction] > schedTime[secondInstruction])
  //   k = 1;

  delay = latency - k * delta;
  llvm::errs() << "- latency:" << latency << "\n";
  llvm::errs() << "- k:" << k << "\n";

  llvm::errs() << "- Output delay:" << delay << "\n";
  llvm::errs() << "EXITING: delay -----------------------------------------------\n";

  return delay;
}


void ModuloScheduling::printResourceTable() {
  std::vector<cot::Operand> ops = architecture->getAllArch();
  int tot = resourceTable[ops[0].getUnit()].size();
  std::vector<std::string> done;
  llvm::errs() << "--------------- RESOURCE TABLE ---------------\n";

  // VERTICAL RAPRESENTATION

  for (std::vector<cot::Operand>::iterator op = ops.begin(); op != ops.end(); ++op) {
    if (std::find(done.begin(), done.end(), op->getUnit()) == done.end()) {
      llvm::errs() << op->getUnit() << "\t\t || \t\t";
      done.push_back(op->getUnit());
    }
  }
  llvm::errs() << "\n";

  // For all the rows
  for (int i = 0; i < tot; ++i){
    llvm::errs() << i << "\t\t || ";

    for (std::vector<std::string>::iterator it = done.begin(); it != done.end(); ++it){
      if(resourceTable[*it][i] != NULL)
        llvm::errs() << (*resourceTable[*it][i]) << "\t\t || ";
      else
        llvm::errs() << "nop" << "\t\t || ";
    }

    llvm::errs() << "\n";
  }
  llvm::errs() << "--------------- ~~~~~~~~~~~~~~ ---------------\n";

  done.clear();
  // HORIZONTAL RAPRESENTATION
  llvm::errs() << "--------------- RESOURCE TABLE ---------------\n";
  llvm::errs() << "\t";
  for (int i = 0; i < tot; ++i)
    llvm::errs() << "  " << (i) << "  \t";
  llvm::errs() << "\n";
  for (std::vector<cot::Operand>::iterator op = ops.begin(); op != ops.end(); ++op) {
    if (std::find(done.begin(), done.end(), op->getUnit()) == done.end()) {
      done.push_back(op->getUnit());
      llvm::errs() << op->getUnit() <<"\t";
      for (int i = 0; i < tot; ++i)
        if (resourceTable[op->getUnit()][i] != NULL){
          llvm::Instruction * currentI = resourceTable[op->getUnit()][i];
          llvm::errs() << currentI->getOpcodeName() << "\t";
        }else 
          llvm::errs() << "nop\t";
      llvm::errs() << "\n";
    }
  }
  
  llvm::errs() << "--------------- ~~~~~~~~~~~~~~ ---------------\n";
}


void ModuloScheduling::printSchedTime(std::map<llvm::Instruction *, int> schedTime) {

  llvm::errs() << "--------------- SCHED-TIME ---------------\n";
  // The scheduling is completed if all the schedTimes are assigned (= different from -1)
  for(std::map<llvm::Instruction *, int>::iterator iter = schedTime.begin(); 
          iter != schedTime.end(); 
          ++iter){

    llvm::errs() << iter->second << " : " << *(iter->first) << "\n"; 
  }
  llvm::errs() << "--------------- ~~~~~~~~~~~~~~ ---------------\n";
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
      if (resourceTable[*unit][i % delta] != NULL) {
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
  
  // NOTE: in SSA form, we only have conflicts on resources

  // Get all the units available to execute the instruction
  std::vector<std::string> units = architecture->getUnit(currentInstruction->getOpcodeName());
  int latency = architecture->getCycle(currentInstruction->getOpcodeName());

  // for every t check if there is a conflict
  for (int i = t; i < (t + latency); ++i) {
    if (resourceTable[schedulingUnit][i % delta] != NULL) {
      // return the conflict instruction
      llvm::errs() << "\n==> First conflicting instruction: " << (*(resourceTable[schedulingUnit][i % delta])) << "\n";
      return resourceTable[schedulingUnit][i % delta];
    }
  }

  llvm::errs() << "\n==> No conflicting instruction found\n";
  return NULL;
}

bool ModuloScheduling::scheduleCompleted(std::map<llvm::Instruction *, int> schedTime){

  llvm::errs() << "\nENTERED IN: scheduleCompleted --------------------------------------------\n";

  // The scheduling is completed if all the schedTimes are assigned (= different from -1)
  for(std::map<llvm::Instruction *, int>::iterator iter = schedTime.begin(); 
          iter != schedTime.end(); 
          ++iter){

        if(iter->second == -1){
          llvm::errs() << "At least one instruction has not been scheduled: " << (*(iter->first)) << "\n";
          llvm::errs() << "EXITING: scheduleCompleted -----------------------------------------------\n";
          return false;
        }
  }
  llvm::errs() << "All instructions scheduled\n";
  llvm::errs() << "EXITING: scheduleCompleted -----------------------------------------------\n";
  return true;
}

void ModuloScheduling::schedule(llvm::Instruction * currentI, std::map<llvm::Instruction *, int> * schedTime, int t, int delta) {

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
        if (resourceTable[*unit][i % delta] != NULL) {
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
  llvm::errs() << "The instruction:\n";
  llvm::errs() << (*currentI) << "\n";
  llvm::errs() << "has been unscheduled!\n\n";
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
