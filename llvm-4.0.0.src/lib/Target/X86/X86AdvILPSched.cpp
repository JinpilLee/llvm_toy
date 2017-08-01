//===-- X86AdvILPSched.cpp - X86 Scheduler Strategy -----------------------===//
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

#include "X86AdvILPSched.h"

// FIXME for debug
#include <iostream>

#define DEBUG_TYPE "misched"

using namespace llvm;

void X86AdvILPSched::initialize(ScheduleDAGMI *dag) {
  assert(dag->hasVRegLiveness() && "ILPScheduler needs vreg liveness");
  DAG = static_cast<ScheduleDAGMILive *>(dag);
  DAG->computeDFSResult();
  Cmp.DFSResult = DAG->getDFSResult();
  Cmp.ScheduledTrees = &DAG->getScheduledTrees();
  ReadyQ.clear();

  XII = static_cast<const X86InstrInfo *>(DAG->TII);
}

void X86AdvILPSched::registerRoots() {
  // Restore the heap in ReadyQ with the updated DFS results.
  std::make_heap(ReadyQ.begin(), ReadyQ.end(), Cmp);
}

SUnit *X86AdvILPSched::pickNode(bool &IsTopNode) {
    if (ReadyQ.empty()) return nullptr;
    std::pop_heap(ReadyQ.begin(), ReadyQ.end(), Cmp);
    SUnit *SU = ReadyQ.back();
    ReadyQ.pop_back();
    IsTopNode = false;
    DEBUG(dbgs() << "Pick node " << "SU(" << SU->NodeNum << ") "
          << " ILP: " << DAG->getDFSResult()->getILP(SU)
          << " Tree: " << DAG->getDFSResult()->getSubtreeID(SU) << " @"
          << DAG->getDFSResult()->getSubtreeLevel(
            DAG->getDFSResult()->getSubtreeID(SU)) << '\n'
          << "Scheduling " << *SU->getInstr());
    return SU;
}

void X86AdvILPSched::scheduleTree(unsigned SubtreeID) {
  std::make_heap(ReadyQ.begin(), ReadyQ.end(), Cmp);
}

void X86AdvILPSched::releaseBottomNode(SUnit *SU) {
  ReadyQ.push_back(SU);
  std::push_heap(ReadyQ.begin(), ReadyQ.end(), Cmp);
}
