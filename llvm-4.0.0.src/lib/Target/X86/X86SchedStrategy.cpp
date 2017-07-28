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

// FIXME ad-hoc implementation
extern std::vector<MachineInstr *> X86SchedHighPriorInstrVector;

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
      if (hasHighPriority(CurrMI)) {
        CandSU = chooseNewCand(CandSU, CurrSU);
      }
    }
  }

  if (CandSU == nullptr) return GenericScheduler::pickNode(IsTopNode);
  else {
    // FIXME for debug
    MachineInstr *CandMI = CandSU->getInstr();
    CandMI->dump();
    std::cerr << "SubID: " << DFSResult->getSubtreeID(CandSU) << "\n";
    std::cerr << "Depth: " << CandSU->getDepth() << "\n";

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

bool X86SchedStrategy::hasHighPriority(MachineInstr *MI) {
  std::vector<MachineInstr *>::iterator VecIter =
    find(X86SchedHighPriorInstrVector.begin(),
         X86SchedHighPriorInstrVector.end(),
         MI);

  return VecIter != X86SchedHighPriorInstrVector.end();
}

unsigned X86SchedStrategy::getDefReg(MachineInstr *MI) {
  for (unsigned i = 0; i < MI->getNumOperands(); i++) {
    MachineOperand &MO = MI->getOperand(i);
    if (MO.isReg() && MO.isDef()) {
      return MO.getReg();
    }
  }

  std::cerr << "UNREACHABLE\n";
  return 0; // FIXME unreachable
}

SUnit *X86SchedStrategy::chooseNewCand(SUnit *CandSU, SUnit *CurrSU) {
  if (CandSU == nullptr) {
    return CurrSU;
  }

  unsigned SubTreeID =  DFSResult->getSubtreeID(CurrSU);
  if (SubTreeID < DFSResult->getSubtreeID(CandSU)) {
    return CurrSU;
  }
  else if (SubTreeID == DFSResult->getSubtreeID(CandSU)) {
    if (getDefReg(CurrSU->getInstr()) <
        getDefReg(CandSU->getInstr())) {
      return CurrSU;
    }
  }

  return CandSU;
}
