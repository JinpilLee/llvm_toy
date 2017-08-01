//===-- X86AdvILPSched.h - X86 Scheduler Strategy -*- C++ -*---------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_LIB_TARGET_X86_ADVILPSCHED_H
#define LLVM_LIB_TARGET_X86_ADVILPSCHED_H

#include "llvm/CodeGen/MachineScheduler.h"
#include "llvm/CodeGen/ScheduleDFS.h"
#include "X86InstrInfo.h"
#include <vector>

namespace llvm {

class SchedOrder {
public:
  const SchedDFSResult *DFSResult;
  const BitVector *ScheduledTrees;

  SchedOrder()
    : DFSResult(nullptr), ScheduledTrees(nullptr) {}

  /// \brief Apply a less-than relation on node priority.
  ///
  /// (Return true if A comes after B in the Q.)
  bool operator()(const SUnit *A, const SUnit *B) const {
    unsigned SchedTreeA = DFSResult->getSubtreeID(A);
    unsigned SchedTreeB = DFSResult->getSubtreeID(B);
    if (SchedTreeA != SchedTreeB) {
      // Unscheduled trees have lower priority.
      if (ScheduledTrees->test(SchedTreeA) !=
          ScheduledTrees->test(SchedTreeB))
        return ScheduledTrees->test(SchedTreeB);

      // Trees with shallower connections have have lower priority.
      if (DFSResult->getSubtreeLevel(SchedTreeA)
          != DFSResult->getSubtreeLevel(SchedTreeB)) {
        return DFSResult->getSubtreeLevel(SchedTreeA)
          < DFSResult->getSubtreeLevel(SchedTreeB);
      }
    }
    
    if ((DFSResult->getILP(A) <= DFSResult->getILP(B)) &&
        (DFSResult->getILP(A) >= DFSResult->getILP(B))) {
      if (SchedTreeA == SchedTreeB) {
        return A < B;
      }
        
      return SchedTreeA < SchedTreeB;
    }
    else {
      return DFSResult->getILP(A) < DFSResult->getILP(B);
    }
  }
};

class X86AdvILPSched : public MachineSchedStrategy {
public:
  X86AdvILPSched(): DAG(nullptr), XII(nullptr) {}

  void initialize(ScheduleDAGMI *dag) override;
  void registerRoots() override;
  SUnit *pickNode(bool &IsTopNode) override;

  void scheduleTree(unsigned SubtreeID) override;
  void releaseBottomNode(SUnit *SU) override;

  void schedNode(SUnit *SU, bool IsTopNode) override {
    assert(!IsTopNode && "SchedDFSResult needs bottom-up");
  }

  // only called for top roots
  void releaseTopNode(SUnit *) override {}

private:
  ScheduleDAGMILive *DAG;
  SchedOrder Cmp;
  std::vector<SUnit *> ReadyQ;
  const X86InstrInfo *XII;
};

}

#endif // LLVM_LIB_TARGET_X86_ADVILPSCHED_H
