
#include "cot/AllPasses.h"
#include "cot/Architecture.h"
#include "llvm/Support/raw_ostream.h"

#include <vector>
#include <algorithm>

using namespace cot;

void Architecture::addOperand(std::string i, std::string u, std::string c) {
  Operand record;
  record.setOperand(i, u, atoi(c.c_str()));
  arch->push_back(record);
}

int Architecture::getCycle(std::string instruction) {
  for (std::vector<Operand>::iterator op = arch->begin();
                                      op != arch->end();
                                      ++op) {
    if (instruction.compare(op->getInstruction()) == 0)
      return op->getCycle();
  }
  return -1;
}

std::vector<std::string> Architecture::getUnit(std::string instruction) {
  std::vector<std::string> v;
  for (std::vector<Operand>::iterator op = arch->begin();
                                      op != arch->end();
                                      ++op) {
    if (op->getInstruction().compare(instruction) == 0)
      v.push_back(op->getUnit());
  }
  return v;
}

int Architecture::getNumberOfUnits(std::string instruction) {
  u_int tot = 0;
  for (std::vector<Operand>::iterator op = arch->begin();
                                      op != arch->end();
                                      ++op) {
    if (op->getInstruction().compare(instruction) == 0)
      ++tot;
  }
  return tot;
}

std::vector<std::string> Architecture::getSupportedOperand() {
  std::vector<std::string> supportedOp;
  for (std::vector<Operand>::iterator op = arch->begin();
                                      op != arch->end();
                                      ++op) {
    std::string opName = op->getInstruction();
    if (std::find(supportedOp.begin(), supportedOp.end(), opName) == supportedOp.end())
      supportedOp.push_back(opName);
  }
  return supportedOp;
}
