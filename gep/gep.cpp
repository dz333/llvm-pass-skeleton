#include "llvm/Pass.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <cassert>

using namespace llvm;

namespace {

  void printType(std::string msg, Type* t) {
    errs() << msg;
    t->print(errs(), true, true);
    errs() << "\n";
  }

  Value* insertCondition(IRBuilder<> builder, GetElementPtrInst* gep, Value* idx, Value* minInclusive, Value* maxExclusive) {
    Value* lessMax = builder.CreateICmpSLT(idx, maxExclusive);
    Value* geMin = builder.CreateICmpSGE(minInclusive, idx);
    return builder.CreateAnd(lessMax, geMin);
  }

  void insertExitBranch(IRBuilder<> builder, GetElementPtrInst* gep, Value* safe) {
    //now either continue to gep instruction OR call exit(1) if not safe;
    Value* notsafe = builder.CreateNot(safe);
    SplitBlockAndInsertIfThen(notsafe, gep, true);
  }

  struct GepPass : public FunctionPass {
    static char ID;
    GepPass() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F) {
      std::vector<GetElementPtrInst*> geps;
      for (auto &basicblock : F) {
	for (auto &inst : basicblock) {
	  if(GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&inst)) {
	    geps.push_back(gep);
	  }
	}
      }
      bool changed = false;
      for (auto &gep : geps) {
	Type* sourceType = gep->getSourceElementType();
	Value* pointerOp = gep->getPointerOperand();
	IRBuilder<> builder(gep);
	Value* checkCondition = nullptr;
	for ( auto &op : gep->indices()) {
	  //insert instructions to calculate maximum legal offset for source type
	  printType("\tOp Type: ",op->getType());
	  if (ArrayType* arrType = dyn_cast<ArrayType>(sourceType)) {
	    printType("\tArray Source Type: ", arrType);
	    errs() << "\t\tWith " << arrType->getNumElements() << " elements \n";
	    //arrType->getNumElements() - 1 is max index
	    Value* maxIdxEx = ConstantInt::getSigned(op->getType(), arrType->getNumElements());
	    Value* minIdxIn = ConstantInt::getSigned(op->getType(), 0);
	    Value* cond = insertCondition(builder, gep, op, minIdxIn, maxIdxEx);
	    if (checkCondition) {
	      checkCondition = builder.CreateAnd(checkCondition, cond);
	    } else {
	      checkCondition = cond;
	    }
	    changed = true;
	  }
	  else if (StructType* structType = dyn_cast<StructType>(sourceType)) {
	    printType("\tStruct Source Type: ", structType);
	  }
	}
	if (checkCondition) {
	  insertExitBranch(builder, gep, checkCondition);
	}
      }
      return changed;
    }
  };
}

char GepPass::ID = 0;

// Register the pass so `opt -gepsafe` runs it.
static RegisterPass<GepPass> X("gepsafe", "check for unsafe uses of getelemptr and insert guards");
