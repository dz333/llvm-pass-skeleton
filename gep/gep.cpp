#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

namespace {
  struct GepPass : public FunctionPass {
    static char ID;
    GepPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      for (auto &basicblock : F) {
	for (auto &inst : basicblock) {
	  GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&inst);
	  if (gep) {
	    errs() << "I saw a getelemptr: " << gep->getAddressSpace() << " \n";
	  }
	}
      }
      return false;
    }
  };
}

char GepPass::ID = 0;

// Register the pass so `opt -gepsafe` runs it.
static RegisterPass<GepPass> X("gepsafe", "check for unsafe uses of getelemptr and insert guards");
