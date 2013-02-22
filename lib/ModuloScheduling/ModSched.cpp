
#include "cot/AllPasses.h"
#include "cot/ModuloScheduling.h"
#include "cot/ModSched.h"

using namespace cot;

char ModSched::ID = 0;

bool ModSched::runOnModule(llvm::Module &Mod) {
  bool HelloWorldBuilt, MainBuilt;

  llvm::Function &HelloWorld = GetHelloWorld(Mod, HelloWorldBuilt);
  AddMain(Mod, HelloWorld, MainBuilt);

  // This method must return true if the visited module was modified.
  return HelloWorldBuilt || MainBuilt;
}

void ModSched::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<ModuloScheduling>();
  AU.setPreservesAll();
}

llvm::Function &ModSched::GetHelloWorld(llvm::Module &Mod, bool &Modified) {
  llvm::Function *Fun = Mod.getFunction("hello_world");

  // Function already available.
  if(Fun && !Fun->isDeclaration()) {
    Modified = false;
    return *Fun;
  }

  // A context is used to track the compiler status -- it is required passing it
  // to function creating new LLVM objects in order to be inserted into compiler
  // state.
  llvm::LLVMContext &Ctx = Mod.getContext();

  llvm::Constant *FunSym;

  // No code associated to the function, first insert a declaration.
  FunSym = Mod.getOrInsertFunction("hello_world",
                                   llvm::Type::getVoidTy(Ctx),
                                   NULL);
  Fun = llvm::cast<llvm::Function>(FunSym);

  // Then build a basic block to hold instructions.
  llvm::BasicBlock *Entry = llvm::BasicBlock::Create(Ctx, "entry", Fun);

  llvm::GlobalVariable *Msg;
  llvm::Value *MsgPtr;

  // We want to generate 'printf("Hello, World!")'. First parameter is a
  // constant.
  llvm::Constant *MsgInit = llvm::ConstantArray::get(Ctx, "Hello, World!\n");

  // Put it into a global variable.
  Msg = new llvm::GlobalVariable(Mod,
                                 MsgInit->getType(),
                                 true,
                                 llvm::GlobalVariable::InternalLinkage,
                                 MsgInit,
                                 "hello_world.msg");

  // Actually, 'printf' printf first parameter is a pointer -- we need to get
  // its address. Get an offsets vector first.
  llvm::SmallVector<llvm::Value *, 2> Offsets;
  Offsets.assign(2, llvm::ConstantInt::get(llvm::Type::getInt32Ty(Ctx), 0));

  // Then, use it to perform some pointer arithmetics.
  MsgPtr = llvm::GetElementPtrInst::Create(Msg, Offsets, "", Entry);

  // Now we need the 'printf' function.
  llvm::Function &Printf = GetPrintf(Mod);

  // Call it, passing the string as the only argument.
  llvm::CallInst::Create(&Printf, MsgPtr, "", Entry);

  // A Basic block must always ends with a terminator instruction. In this case
  // a return.
  llvm::ReturnInst::Create(Ctx, Entry);

  Modified = true;
  return *Fun;
}

void ModSched::AddMain(llvm::Module &Mod,
                        llvm::Function &HelloFun,
                        bool &Modified) {
  llvm::Function *Fun = Mod.getFunction("main");

  // Function already available.
  if(Fun && !Fun->isDeclaration()) {
    Modified = false;
    return;
  }

  llvm::LLVMContext &Ctx = Mod.getContext();
  llvm::Type *VoidTy = llvm::Type::getVoidTy(Ctx);

  llvm::Constant *FunSym;

  // Main not inside the compilation unit -- add it.
  FunSym = Mod.getOrInsertFunction("main", VoidTy, NULL);
  Fun = llvm::cast<llvm::Function>(FunSym);

  // Make an entry block.
  llvm::BasicBlock *Entry = llvm::BasicBlock::Create(Ctx, "entry", Fun);

  // Call the greeter and return.
  llvm::CallInst::Create(&HelloFun, llvm::ArrayRef<llvm::Value *>(), "", Entry);
  llvm::ReturnInst::Create(Ctx, Entry);

  Modified = true;
}

// The signature of 'printf' is architecture specific -- use this member
// function to define signatures with respect to the architecture.
llvm::Function &ModSched::GetPrintf(llvm::Module &Mod) {
  llvm::LLVMContext &Ctx = Mod.getContext();
  llvm::FunctionType *PrintfTy;

#if defined(__i386__) || defined(__x86_64__)
  PrintfTy = llvm::FunctionType::get(llvm::Type::getInt32Ty(Ctx),
                                     llvm::Type::getInt8PtrTy(Ctx),
                                     true);
#else
#error "Architecture not supported"
#endif

  llvm::Constant *FunSym = Mod.getOrInsertFunction("printf", PrintfTy);

  return *llvm::cast<llvm::Function>(FunSym);
}

llvm::Pass *cot::CreateModSchedPass() { return new ModSched(); }

using namespace llvm;

INITIALIZE_PASS(ModSched,
                      "masd",
                      "modulo scheduling transform",
                      false,
                      false)
