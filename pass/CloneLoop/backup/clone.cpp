#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/IR/Dominators.h"
//#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/Transforms/Utils/Cloning.h"

// FIXME undefvalue
#include "llvm/IR/Constants.h"

#include <iostream>
#include <vector>
#include <list>
#include <map>

using namespace llvm;

class LoopPartition {
public:
  LoopPartition(Loop *L, LoopInfo *LI, DominatorTree *DT, unsigned Index)
    : OrigLoop(L), LI(LI), DT(DT), Index(Index), ClonedLoop(nullptr) {
    LoopDomBB = OrigLoop->getLoopPreheader()->getSinglePredecessor();
    assert(LoopDomBB && "Preheader does not have a single predecessor.");
  }

  Loop *cloneLoop(BasicBlock *InsertPH) {
    ClonedLoop = cloneLoopWithPreheader(InsertPH, LoopDomBB, OrigLoop,
                                        VMap, Twine(".LC") + Twine(Index),
                                        LI, DT, ClonedLoopBlocks);

    BasicBlock *ExitBlock = OrigLoop->getExitBlock();
    assert(ExitBlock && "Target loop does not have a unique exit block.");

    VMap[ExitBlock] = InsertPH;
    remapInstructionsInBlocks(ClonedLoopBlocks, VMap);

    return ClonedLoop;
  }

  Loop *getClonedLoop() const {
    assert(ClonedLoop && "Loop is not cloned.");
    return ClonedLoop;
  }

private:
  LoopPartition() = delete;

  Loop *OrigLoop;
  BasicBlock *LoopDomBB;

  LoopInfo *LI;
  DominatorTree *DT;

  unsigned Index;

  Loop *ClonedLoop;
  SmallVector<BasicBlock *, 8> ClonedLoopBlocks;
  ValueToValueMapTy VMap;
};

class LoopSplit : public FunctionPass {
public:
  static char ID;

  LoopSplit() : FunctionPass(ID) {
  }

  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
  unsigned getNumLoopPartitions() const {
    return 2;
  }

  bool processLoop(Loop *L, LoopInfo *LT, DominatorTree *DT);
};

bool LoopSplit::processLoop(Loop *L, LoopInfo *LI, DominatorTree *DT) {
  assert(L->empty() && "Target loop is not innermost.");

  if (!L->getExitBlock()) return false;
  if (!L->isLoopSimplifyForm()) return false;

  BasicBlock *OrigPH = L->getLoopPreheader();
  if (!OrigPH->getSinglePredecessor() ||
      &*OrigPH->begin() != OrigPH->getTerminator())
    SplitBlock(OrigPH, OrigPH->getTerminator(), DT, LI);

  OrigPH = L->getLoopPreheader();
  BasicBlock *PredPH = OrigPH->getSinglePredecessor();
// FIXME needed?
  assert(PredPH && "Preheader does not have a single predecessor.");

// FIXME just for test
  std::vector<LoopPartition *> LoopPartitionVector;
  unsigned NumLoopPartitions = getNumLoopPartitions();
  for (unsigned i = 0; i < NumLoopPartitions; i++) {
    LoopPartitionVector.push_back(new LoopPartition(L, LI, DT, i));
  }

  BasicBlock *InsertPH = OrigPH;
  Loop *ClonedLoop;
  for (int i = NumLoopPartitions - 1; i >= 0;
       --i, InsertPH = ClonedLoop->getLoopPreheader()) {
    ClonedLoop = LoopPartitionVector[i]->cloneLoop(InsertPH);
  }

  PredPH->getTerminator()->replaceUsesOfWith(OrigPH, InsertPH);

  for (unsigned i = 0; i < NumLoopPartitions - 1; i++) {
    BasicBlock *CurrEX
      = LoopPartitionVector[i]->getClonedLoop()->getExitingBlock();
    BasicBlock *NextPH
      = LoopPartitionVector[i + 1]->getClonedLoop()->getLoopPreheader();
    DT->changeImmediateDominator(NextPH, CurrEX);
  }

  return true;
}

bool LoopSplit::runOnFunction(Function &F) {
  LoopInfo *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  DominatorTree *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  SmallVector<Loop *, 8> Worklist;
  for (Loop *TopLevelLoop : *LI)
    for (Loop *L : depth_first(TopLevelLoop))
      if (L->empty())
        Worklist.push_back(L);

  bool Changed = false;
  for (Loop *L : Worklist) {
    Changed |= processLoop(L, LI, DT);
  }

  return Changed;
}

void LoopSplit::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
}

char LoopSplit::ID = 0;
static RegisterPass<LoopSplit> X("split-loop", "Loop Split Pass",
                                 false, false);
