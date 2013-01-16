
#include "cot/AllPasses.h"
#include "cot/Architecture.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <algorithm>

using namespace cot;

void Architecture::addInstruction(std::string i, std::string u, std::string c) {
  Operand record;
  record.setInstruction(i, u, atoi(c.c_str()));
  arch->push_back(record);
}

int Architecture::getCycle(std::string instruction) {
  u_int i = 0;
  while (i < arch->size()) {
    Operand instr = arch->at(i);
    if (instruction.compare(instr.getInstruction()) == 0)
      return instr.getCycle();
    ++i;
  }
  return -1;
}

std::vector<std::string> Architecture::getUnit(std::string instruction) {
  std::vector<std::string> v;
  u_int i = 0;
  while (i < arch->size()) {
    Operand instr = arch->at(i);
    if (instr.getInstruction().compare(instruction) == 0)
      v.push_back(instr.getUnit());
    ++i;
  }
  return v;
}

int Architecture::getNumberOfUnits(std::string instruction) {
  u_int tot = 0, i;
  for (i = 0; i < arch->size(); ++i) {
    Operand instr = arch->at(i);
    if (instr.getInstruction().compare(instruction) == 0)
      ++tot;
  }
  return tot;
}

std::vector<std::string> Architecture::getSupportedOperand() {
  std::vector<std::string> supportedOp;
  for (std::vector<Operand>::iterator op = arch->begin(); op != arch->end(); ++op) {
    std::string opName = op->getInstruction();
    if (std::find(supportedOp.begin(), supportedOp.end(), opName) == supportedOp.end())
      supportedOp.push_back(opName);
  }
  return supportedOp;
}