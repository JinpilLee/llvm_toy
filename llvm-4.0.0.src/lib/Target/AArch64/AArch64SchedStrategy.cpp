//===-- AArch64SchedStrategy.cpp - AArch64 Scheduler Strategy -------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
/// \file
/// This contains a MachineSchedStrategy implementation for AArch64
//
//===----------------------------------------------------------------------===//

#include "AArch64SchedStrategy.h"
#include <iostream>
#include <vector>

#define DEBUG_TYPE "misched"

extern std::vector<unsigned> MemInstrRegVector;

using namespace llvm;

AArch64SchedStrategy::AArch64SchedStrategy(const MachineSchedContext *C)
  : GenericScheduler(C) {
}

void AArch64SchedStrategy::initialize(ScheduleDAGMI *dag) {
  assert(dag->hasVRegLiveness() && "ILPScheduler needs vreg liveness");

  ScheduleDAGMILive *DAG = static_cast<ScheduleDAGMILive*>(dag);
  DAG->computeDFSResult();
  DFSResult = DAG->getDFSResult();

  GenericScheduler::initialize(dag);
}

SUnit *AArch64SchedStrategy::pickNode(bool &IsTopNode) {
  if (!RegionPolicy.OnlyTopDown) {
    return GenericScheduler::pickNode(IsTopNode);
  }

  SUnit *CandSU = nullptr;
  for (SUnit *CurrSU : Top.Available) {
    if (CurrSU != nullptr) { // FIXME requires this?
      MachineInstr *CurrMI = CurrSU->getInstr();
      if (hasHighPriority(CurrMI)) {
        CandSU = chooseNewCand(CandSU, CurrSU);
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

bool AArch64SchedStrategy::hasHighPriority(MachineInstr *MI) {
  if (MI->mayLoad()) {
    return true;
  }

  if (MI->getNumOperands() > 0) {
    MachineOperand &MO = MI->getOperand(0);
    if (MO.isReg() && MO.isDef()) {
      std::vector<unsigned>::iterator VecIter =
        find(MemInstrRegVector.begin(),
             MemInstrRegVector.end(), MO.getReg());

      if (VecIter != MemInstrRegVector.end()) {
        return true;
      }
    }
  }

  return false;
}

SUnit *AArch64SchedStrategy::chooseNewCand(SUnit *CandSU, SUnit *CurrSU) {
  if (CandSU == nullptr) {
    return CurrSU;
  }

  MachineInstr *CandMI = CandSU->getInstr();
  MachineInstr *CurrMI = CurrSU->getInstr();

  // address calculation has higher priority
  // FIXME should look dependencies,
  // there may be a load which does not requires address calculation
  if (!CandMI->mayLoad() && CurrMI->mayLoad()) {
    return CandSU;
  }
  else if (CandMI->mayLoad() && !CurrMI->mayLoad()) {
    return CurrSU;
  }

  // CandSU and CurrSU are equivalent, unrolled instructions
  unsigned SubTreeID =  DFSResult->getSubtreeID(CurrSU);
  if (SubTreeID < DFSResult->getSubtreeID(CandSU)) {
    return CurrSU;
  }
  else if (SubTreeID == DFSResult->getSubtreeID(CandSU)) {
    MachineInstr *CandMI = CandSU->getInstr();
    if (CurrMI->getOperand(0).getReg() <
        CandMI->getOperand(0).getReg()) {
      return CurrSU;
    }
  }

  return CandSU;
}
