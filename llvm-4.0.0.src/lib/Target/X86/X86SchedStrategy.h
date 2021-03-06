//===-- X86SchedStrategy.h - X86 Scheduler Strategy -*- C++ -*-------------===//
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

#ifndef LLVM_LIB_TARGET_X86_SCHEDSTRATEGY_H
#define LLVM_LIB_TARGET_X86_SCHEDSTRATEGY_H

#include "llvm/CodeGen/MachineScheduler.h"
#include "llvm/CodeGen/ScheduleDFS.h"
#include "X86InstrInfo.h"

namespace llvm {

class SIRegisterInfo;

class X86SchedStrategy : public GenericScheduler {
public:
  X86SchedStrategy(const MachineSchedContext *C);
  void initialize(ScheduleDAGMI *dag) override;
  SUnit *pickNode(bool &IsTopNode) override;

private:
  const SchedDFSResult *DFSResult;

  unsigned getDefReg(MachineInstr *MI);
  SUnit *chooseNewLoad(SUnit *Cand, SUnit *Curr);
};

} // End namespace llvm

#endif // LLVM_LIB_TARGET_X86_SCHEDSTRATEGY_H
