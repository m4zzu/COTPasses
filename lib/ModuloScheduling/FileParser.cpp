
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

#if !defined(FILE_CONF)
  #define FILE_CONF "lib/ModuloScheduling/mips10000.cfg"
#endif

using namespace cot;

char FileParser::ID = 0;

bool FileParser::runOnModule(llvm::Module &Mod) {
  return  false;
}

void FileParser::initializePass() {
  architecture = new Architecture();
  llvm::OwningPtr<llvm::MemoryBuffer> fileConf;
  llvm::error_code err = llvm::MemoryBuffer::getFile(FILE_CONF, fileConf);
  if (err != NULL && err.value() != 0) {
    return;
  }
  llvm::StringRef content = fileConf->getBuffer();
  std::pair<llvm::StringRef, llvm::StringRef> rows = content.split('\n');
  u_int i = 0;
  while (rows.first.size() != 0) {
    if (rows.first.front() != '#' && rows.first.front() != '\n') {
      std::pair<llvm::StringRef, llvm::StringRef> cells = rows.first.split(',');
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

void FileParser::print(llvm::raw_ostream &OS,
                             const llvm::Module *Mod) const {
  if(!Mod)
    return;

  OS << "=======-------=======\n";
  std::vector<Operand> A = architecture->getAllArch();
  u_int i = 0;
  while (i < A.size()) {
    OS << "Conf " << (i + 1) << ":\n";
    OS << "  Instr:  " << A[i].getInstruction() << "\n";
    OS << "  Unit:   " << A[i].getUnit() << "\n";
    OS << "  Cycle:  " << A[i].getCycle() << "\n";
    ++i;
  }
  OS << "=======-------=======\n";

}

using namespace llvm;

INITIALIZE_PASS(FileParser,
                "file-parser",
                "Parses architectural configuration file",
                true,
                true)
