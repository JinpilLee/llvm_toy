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
    if (CurrSU != nullptr) {
      MachineInstr *CurrMI = CurrSU->getInstr();
      if (CurrMI->mayLoad()) {
        if (CandSU == nullptr) {
          CandSU = CurrSU;
          continue;
        }

        unsigned SubTreeID =  DFSResult->getSubtreeID(CurrSU);
        if (SubTreeID < DFSResult->getSubtreeID(CandSU)) {
          CandSU = CurrSU;
          continue;
        }
        else if (SubTreeID == DFSResult->getSubtreeID(CandSU)) {
          MachineInstr *CandMI = CandSU->getInstr();
          if (CurrMI->getOperand(0).getReg() <
              CandMI->getOperand(0).getReg()) {
            CandSU = CurrSU;
            continue;
          }
        }
      }
    }
  }

  if (CandSU == nullptr) return GenericScheduler::pickNode(IsTopNode);
  else {
    // FIXME for debug
    //MachineInstr *CandMI = CandSU->getInstr();
    //CandMI->dump();

    if (CandSU->isTopReady()) {
      Top.removeReady(CandSU);
    }

    if (CandSU->isBottomReady()) {
      Bot.removeReady(CandSU);
    }

    IsTopNode = true;
    return CandSU;
  }
}
