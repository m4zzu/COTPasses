
#ifndef COT_ARCHITECTURE_H
#define COT_ARCHITECTURE_H

namespace cot {

struct Instruction
{
  private:
    std::string instruction;
    std::string unit;
    int cycle;
  public:
    Instruction& setInstruction(std::string instr_str_ref, std::string unit_str_ref, int cycle_str_ref) {
      instruction = instr_str_ref;
      unit = unit_str_ref;
      cycle = cycle_str_ref;
      return *this;
    }
    std::string getInstruction() {return instruction;}
    std::string getUnit() {return unit;}
    int getCycle() {return cycle;}
};

class Architecture
{
public:
  Architecture() {
    arch = new std::vector<Instruction>;
  }
  ~Architecture() {
    delete arch;
  }

public:
  void addInstruction(std::string i, std::string u, std::string c);
  int getCycle(std::string instruction);
  std::vector<std::string> getUnit(std::string instruction);
  std::vector<Instruction> getAllArch() { return *arch; };
  int getNumberOfUnits(std::string instruction);

private:
  std::vector<Instruction> *arch;

};

}

#endif // COT_ARCHITECTURE_H
