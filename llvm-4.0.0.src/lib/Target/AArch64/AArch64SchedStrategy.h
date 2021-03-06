//===-- AArch64SchedStrategy.h - AArch64 Scheduler Strategy -*- C++ -*-------------===//
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

#ifndef LLVM_LIB_TARGET_AARCH64_SCHEDSTRATEGY_H
#define LLVM_LIB_TARGET_AARCH64_SCHEDSTRATEGY_H

#include "llvm/CodeGen/MachineScheduler.h"
#include "llvm/CodeGen/ScheduleDFS.h"

namespace llvm {

class SIRegisterInfo;

class AArch64SchedStrategy : public GenericScheduler {
public:
  AArch64SchedStrategy(const MachineSchedContext *C);
  void initialize(ScheduleDAGMI *dag) override;
  SUnit *pickNode(bool &IsTopNode) override;

private:
  const SchedDFSResult *DFSResult;

  bool hasHighPriority(MachineInstr *MI);
  SUnit *chooseNewCand(SUnit *Cand, SUnit *Curr);
};

} // End namespace llvm

#endif // LLVM_LIB_TARGET_AARCH64_SCHEDSTRATEGY_H
