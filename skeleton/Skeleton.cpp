#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
using namespace llvm;

namespace {
  struct SkeletonPass : public FunctionPass {
    static char ID;
    SkeletonPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      errs() << "I saw a function called " << F.getName() << "!\n";
      bool changed = false;
      std::vector<Instruction*> toRemove;
      for (auto &basicblock : F) {
	for (auto &inst : basicblock) {
	  BinaryOperator *bop = dyn_cast<BinaryOperator>(&inst);
	  if (bop) {
	    Value *lhs = bop->getOperand(0);
	    Value *rhs = bop->getOperand(1);

	    IRBuilder<> builder(bop);
	    Value *mul = builder.CreateMul(lhs, rhs);

	    for (auto& use : bop->uses()) {
	      User* user = use.getUser();
	      user->setOperand(use.getOperandNo(), mul);
	    }
	    changed = true;
	    toRemove.push_back(&inst);
	  }
	}
      }
      for (auto &op : toRemove) {
	op->eraseFromParent();
      }
      return changed;
    }
  };
}

char SkeletonPass::ID = 0;

// Register the pass so `opt -skeleton` runs it.
static RegisterPass<SkeletonPass> X("skeleton", "a useless pass");
