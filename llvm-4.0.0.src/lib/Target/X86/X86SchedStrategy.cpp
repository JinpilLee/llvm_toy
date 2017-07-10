//===-- X86SchedStrategy.cpp - X86 Scheduler Strategy ---------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This contains a MachineSchedStrategy implementation for X86
//
//===----------------------------------------------------------------------===//

#include "X86SchedStrategy.h"
#include <iostream>

#define DEBUG_TYPE "misched"

using namespace llvm;

X86SchedStrategy::X86SchedStrategy(const MachineSchedContext *C)
  : GenericScheduler(C) {
}

SUnit *X86SchedStrategy::pickNode(bool &IsTopNode) {
  if (!RegionPolicy.OnlyTopDown) {
    return GenericScheduler::pickNode(IsTopNode);
  }

  ReadyQueue &Q = Top.Available;
  for (SUnit *SU : Q) {
    if (SU != nullptr) {
      assert(SU->isInstr() && "Target SU has no MI!");
      MachineInstr *MI = SU->getInstr();
      if (MI->mayLoadOrStore()) {
        MI->dump();
        std::cout << "Node Num: " << SU->NodeNum << "\n";
        std::cout << "Queue ID: " << SU->NodeQueueId << "\n";
      }
    }
  }

  return GenericScheduler::pickNode(IsTopNode);
}
