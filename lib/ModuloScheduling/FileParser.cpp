
#include "cot/AllPasses.h"
#include "cot/FileParser.h"

#include "llvm/Function.h"
#include "llvm/Support/ErrorHandling.h"
#include "llvm/Support/raw_ostream.h"

#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/system_error.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/ADT/OwningPtr.h"
#include "llvm/ADT/StringRef.h"

using namespace cot;
using namespace llvm;

char FileParser::ID = 0;

bool FileParser::runOnModule(Module &Mod) {
  return  false;
}

void FileParser::initializePass() {
  OwningPtr<MemoryBuffer> fileConf;
  error_code err = MemoryBuffer::getFile("lib/ModuloScheduling/mips10000.cfg", fileConf);
  if (err != NULL && err.value() != 0) {
    return;
  }
  StringRef content = fileConf->getBuffer();
  std::pair<StringRef, StringRef> rows = content.split('\n');
  u_int i = 0;
  while (rows.first.size() != 0) {
    if (rows.first.front() != '#' && rows.first.front() != '\n') {
      std::pair<StringRef, StringRef> cells = rows.first.split(',');
      i = 0;
      std::string instr;
      std::string u;
      std::string c;
      while (cells.first.size() && i < 3) {
        if (i == 0)
          instr = cells.first;
        else if (i == 1)
          u = cells.first;
        else if (i == 2)
          c = cells.first;
        ++i;
        cells = cells.second.split(',');
      }
      architecture->addInstruction(instr, u, c);
    }
    rows = rows.second.split('\n');
  }
}

void FileParser::print(raw_ostream &OS,
                             const Module *Mod) const {
  if(!Mod)
    return;

  OS << "=======-------=======\n";
  std::vector<Instruction> A = architecture->getAllArch();
  u_int i = 0;
  while (i < A.size()) {
    OS << "Conf " << (i + 1) << ":\n";
    OS << "\tInstr:\t" << A[i].getInstruction() << "\n";
    OS << "\tUnit:\t" << A[i].getUnit() << "\n";
    OS << "\tCycle:\t" << A[i].getCycle() << "\n";
    ++i;
  }
  OS << "=======-------=======\n";

  OS << "mul Cycle: ";
  OS << architecture->getCycle("mul") << "\n";

  std::vector<std::string> c = architecture->getUnit("add");
  OS << "add Unit:\n";
  i = 0;
  while (i < c.size()) {
    OS << "\t" << c[i] << "\n";
  ++i;
  }
}

INITIALIZE_PASS(FileParser,
                "file-parser",
                "Parses architectural configuration file",
                true,
                true)
