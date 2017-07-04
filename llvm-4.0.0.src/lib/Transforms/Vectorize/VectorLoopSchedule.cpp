//===- VectorLoopSchedule.cpp - Instr Schedule Pass for SIMD Loops --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Instruction Scheduling Pass for SIMD Vectorized Loops
//
//===----------------------------------------------------------------------===//

#include "llvm/Transforms/Vectorize/VectorLoopSchedule.h"
#include "llvm/ADT/DepthFirstIterator.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"

#define LDIST_NAME "vector-loop-schedule"
#define DEBUG_TYPE LDIST_NAME

using namespace llvm;

namespace {
static bool runImpl(Function &F, LoopInfo *LI) {
  SmallVector<Loop *, 8> Worklist;
  for (Loop *TopLevelLoop : *LI) {
    for (Loop *L : depth_first(TopLevelLoop)) {
      // only handles innermost loops.
      if (L->empty()) {
        Worklist.push_back(L);
      }
    }
  }

  bool Changed = false;
  for (Loop *L : Worklist) {
    LoopBlocksDFS DFS(L);
    DFS.perform(LI);
    int count = 0;
    for (BasicBlock *BB : make_range(DFS.beginRPO(), DFS.endRPO())) {
      for (auto &I : *BB) {
        errs() << count << ": " << I << "\n";
        count++;
      }
    }
  }

  return Changed;
}

class VectorLoopScheduleLegacy : public FunctionPass {
public:
  VectorLoopScheduleLegacy() : FunctionPass(ID) {
    initializeVectorLoopScheduleLegacyPass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override {
    if (skipFunction(F))
      return false;

    auto *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    return runImpl(F, LI);
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
  }

  static char ID;
};
} // anonymous namespace

PreservedAnalyses VectorLoopSchedulePass::run(Function &F,
                                              FunctionAnalysisManager &AM) {
  auto &LI = AM.getResult<LoopAnalysis>(F);
  bool Changed = runImpl(F, &LI);
  if (!Changed)
    return PreservedAnalyses::all();

  PreservedAnalyses PA;
  PA.preserve<LoopAnalysis>();
  return PA;
}

char VectorLoopScheduleLegacy::ID;
static const char ldist_name[] = "Vector Loop Schedule";

INITIALIZE_PASS_BEGIN(VectorLoopScheduleLegacy, LDIST_NAME, ldist_name, false,
                      false)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_END(VectorLoopScheduleLegacy, LDIST_NAME, ldist_name, false,
                    false)

namespace llvm {
Pass *createVectorLoopSchedulePass() {
  return new VectorLoopScheduleLegacy();
}
}
