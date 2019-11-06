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
    Value* geMin = builder.CreateICmpSGE(idx, minInclusive);
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
      errs() << "fname: " << F.getName() << "\n";
      std::vector<GetElementPtrInst*> geps;
      std::map<Value*, Value*> allocSizes;
      for (auto &basicblock : F) {
	for (auto &inst : basicblock) {
	  if(GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&inst)) {
	    geps.push_back(gep);
	  } else if (AllocaInst *ai = dyn_cast<AllocaInst>(&inst)) {
	    printType("Found alloc a of result type: ", ai->getAllocatedType());
	    errs() << "\t alloc is static size: " << ai->isStaticAlloca() << "\n";
	    if (!ai->isStaticAlloca()) {
	      allocSizes[ai] = ai->getArraySize();
	    }
	  }
	}
      }
      bool changed = false;
      for (auto &gep : geps) {
	Type* srcType = gep->getPointerOperandType();
	Value* pointerOp = gep->getPointerOperand();
	IRBuilder<> builder(gep);
	Value* checkCondition = nullptr;
	for ( auto &op : gep->indices()) {
	  //insert instructions to calculate maximum legal offset for source type
	  if (ArrayType* arrType = dyn_cast<ArrayType>(srcType)) {
	    errs() << "Op is array of length: " << arrType->getNumElements() << "\n";
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
	    srcType = arrType->getElementType();
	  } else if (StructType* structType = dyn_cast<StructType>(srcType)) {
	    ConstantInt* offsetVal = dyn_cast<ConstantInt>(op);
	    errs() << "Found struct op\n";
	    assert(offsetVal && "Struct offsets should all be i64 constants");
	    srcType = structType->getElementType(offsetVal->getSExtValue());
	  } else if (PointerType* ptrType = dyn_cast<PointerType>(srcType)) {
	    assert(op == *gep->indices().begin() && "Should not find pointer ops after first op");
	    auto size = allocSizes.find(pointerOp);
	    if (size != allocSizes.end()) {
	      //insert size check
	      Value* minIdxIn = ConstantInt::getSigned(op->getType(), 0);
	      Value* cond = insertCondition(builder, gep, op, minIdxIn, size->second);
	      if (checkCondition) {
		checkCondition = builder.CreateAnd(checkCondition, cond);
	      } else {
		checkCondition = cond;
	      }
	      changed = true;
	    }
	    srcType = ptrType->getElementType();
	  } else if (VectorType* vType = dyn_cast<VectorType>(srcType)) {
	    errs() << "Found vector type\n";
	    assert(false && "GEP Safety doesn't check vector refs");
	    srcType = vType->getElementType();
	  } else {
	    llvm_unreachable("No other types should be used for gep pointers");
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
