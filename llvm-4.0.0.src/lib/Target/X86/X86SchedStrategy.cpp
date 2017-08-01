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
#include <vector>

// FIXME for debug
#include <iostream>

#define DEBUG_TYPE "misched"

using namespace llvm;

X86SchedStrategy::X86SchedStrategy(const MachineSchedContext *C)
  : GenericScheduler(C) {
}

void X86SchedStrategy::initialize(ScheduleDAGMI *dag) {
  assert(dag->hasVRegLiveness() && "ILPScheduler needs vreg liveness");

  ScheduleDAGMILive *DAG = static_cast<ScheduleDAGMILive*>(dag);
  DAG->computeDFSResult();
  DFSResult = DAG->getDFSResult();

  GenericScheduler::initialize(dag);
}

SUnit *X86SchedStrategy::pickNode(bool &IsTopNode) {
  if (!RegionPolicy.OnlyTopDown) {
    return GenericScheduler::pickNode(IsTopNode);
  }

  SUnit *CandSU = nullptr;
  for (SUnit *CurrSU : Top.Available) {
    if (CurrSU != nullptr) { // FIXME requires this?
      MachineInstr *CurrMI = CurrSU->getInstr();
      if (CurrMI->mayLoad()) {
        CandSU = chooseNewLoad(CandSU, CurrSU);
      }
    }
  }

  if (CandSU == nullptr) return GenericScheduler::pickNode(IsTopNode);
  else {
    if (CandSU->isTopReady())    Top.removeReady(CandSU);
    if (CandSU->isBottomReady()) Bot.removeReady(CandSU);

    IsTopNode = true;
    return CandSU;
  }
}

SUnit *X86SchedStrategy::chooseNewLoad(SUnit *CandSU, SUnit *CurrSU) {
  if (CandSU == nullptr) return CurrSU;

  unsigned CurrSubTreeID =  DFSResult->getSubtreeID(CurrSU);
  unsigned CandSubTreeID =  DFSResult->getSubtreeID(CandSU);
  if (CurrSubTreeID < CandSubTreeID) return CurrSU;
  else if (CurrSubTreeID == CandSubTreeID) {
    if (CurrSU < CandSU) return CurrSU;
  }

  return CandSU;
}
