#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/AliasSetTracker.h"
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

  Value* convertSizeBetweenTypes(IRBuilder<> builder, Type* ptr1, Type* ptr2, Value* ptr1Size, LLVMContext &C) {
    if (PointerType *p1t = dyn_cast<PointerType>(ptr1)) {
      if (PointerType *p2t = dyn_cast<PointerType>(ptr2)) {
	auto elemFrom = p1t->getElementType();
	auto elemTo = p2t->getElementType();
	int fromSize = elemFrom->getPrimitiveSizeInBits();
	int toSize = elemTo->getPrimitiveSizeInBits();
	if (!fromSize || !toSize) {
	  return nullptr;
	} else {
	  errs() << "From Size = " << fromSize << "\n";
	  errs() << "To Size = " << toSize << "\n";
	  Value* fromVal = ConstantInt::get(IntegerType::get(C, 64), fromSize);
	  Value* toVal = ConstantInt::get(IntegerType::get(C, 64), toSize);
	  auto mul = builder.CreateMul(ptr1Size, fromVal);
	  return builder.CreateUDiv(mul, toVal);
	  //	  auto mul = BinaryOperator::Create(Instruction::BinaryOps::Mul, ptr1Size, fromVal);
	  //	  return BinaryOperator::Create(Instruction::BinaryOps::UDiv, mul, toVal);
	}
      }
    }
    return nullptr;
  }

  struct GepPass : public FunctionPass {
    static char ID;
    GepPass() : FunctionPass(ID) {}

    /*    void getAnalysisUsage(AnalysisUsage &AU) const {
      AU.setPreservesAll();
      AU.addRequired<AAResultsWrapperPass>();
      } */
 
    virtual bool runOnFunction(Function &F) {
      errs() << "fname: " << F.getName() << "\n";
      std::vector<GetElementPtrInst*> geps;
      std::map<Value*, Value*> allocSizes;
      for (auto &basicblock : F) {
	for (auto &inst : basicblock) {
	  if(GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(&inst)) {
	    geps.push_back(gep);
	  } else if (AllocaInst *ai = dyn_cast<AllocaInst>(&inst)) {
	    allocSizes[ai] = ai->getArraySize();
	  } else if (CallInst *call = dyn_cast<CallInst>(&inst)) {
	    auto f = call->getCalledFunction();
	    if (f) {
	      auto fname = f->getName();
	      if (fname.compare("malloc") == 0) {
		allocSizes[call] = call->getArgOperand(0);
	      }
	    }
	  } else if (BitCastInst *bc = dyn_cast<BitCastInst>(&inst)) {	    
	    if (bc->getNumOperands() == 1) {
	      auto size = allocSizes.find(bc->getOperand(0));
	      if (size != allocSizes.end()) {
		IRBuilder<> bcBuild(bc);
		auto newSize = convertSizeBetweenTypes(bcBuild,
				size->first->getType(), bc->getType(), size->second, F.getContext());
		if (newSize) {
		  allocSizes[bc] = newSize;
		}
	      }
	    } 
	  }
	}
      }
      bool changed = false;
      int changedCount = 0;
      int notChangedCount = 0;
      int skipped = 0;
      int checked = 0;
      for (auto &gep : geps) {
	Type* srcType = gep->getPointerOperandType();
	Value* pointerOp = gep->getPointerOperand();
	IRBuilder<> builder(gep);
	Value* checkCondition = nullptr;
	for ( auto &op : gep->indices()) {
	  //insert instructions to calculate maximum legal offset for source type
	  if (ArrayType* arrType = dyn_cast<ArrayType>(srcType)) {
	    Value* maxIdxEx = ConstantInt::getSigned(op->getType(), arrType->getNumElements());
	    Value* minIdxIn = ConstantInt::getSigned(op->getType(), 0);
	    Value* cond = insertCondition(builder, gep, op, minIdxIn, maxIdxEx);
	    if (checkCondition) {
	      checkCondition = builder.CreateAnd(checkCondition, cond);
	    } else {
	      checkCondition = cond;
	    }
	    srcType = arrType->getElementType();
	  } else if (StructType* structType = dyn_cast<StructType>(srcType)) {
	    ConstantInt* offsetVal = dyn_cast<ConstantInt>(op);
	    assert(offsetVal && "Struct offsets should all be i64 constants");
	    srcType = structType->getElementType(offsetVal->getSExtValue());
	  } else if (PointerType* ptrType = dyn_cast<PointerType>(srcType)) {
	    assert(op == *gep->indices().begin() && "Should not find pointer ops after first op");
	    auto size = allocSizes.find(pointerOp);
	    if (size != allocSizes.end()) {
	      Value* minIdxIn = ConstantInt::getSigned(op->getType(), 0);
	      Value* cond = insertCondition(builder, gep, op, minIdxIn, size->second);
	      if (checkCondition) {
		checkCondition = builder.CreateAnd(checkCondition, cond);
	      } else {
		checkCondition = cond;
	      }
	      checked += 1;
	    } else {
	      skipped += 1;
	    }
	    srcType = ptrType->getElementType();
	  } else if (VectorType* vType = dyn_cast<VectorType>(srcType)) {
	    //	    errs() << "Found vector type\n";
	    assert(false && "GEP Safety doesn't check vector refs");
	    srcType = vType->getElementType();
	  } else {
	    llvm_unreachable("No other types should be used for gep pointers");
	  }
	}
	if (checkCondition) {
	  insertExitBranch(builder, gep, checkCondition);
	  changed = true;
	  changedCount += 1;
	} else {
	  notChangedCount += 1;
	}
      }
      errs() << "Pointers Skipped: " << skipped << "\n";
      errs() << "Pointers Checked: " << checked << "\n";
      errs() << "Geps Modified: " << changedCount << "\n";
      errs() << "Geps Skipped: " << notChangedCount << "\n";
      return changed;
    }
  };
}

char GepPass::ID = 0;

// Register the pass so `opt -gepsafe` runs it.
static RegisterPass<GepPass> X("gepsafe", "check for unsafe uses of getelemptr and insert guards");
