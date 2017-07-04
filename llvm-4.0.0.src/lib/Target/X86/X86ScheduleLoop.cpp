//===-- X86ScheduleLoop.cpp - scheduling instructions in loops ------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Instruction Scheduling for OoO processors
// Main Target: KNL
//
//===----------------------------------------------------------------------===//

#include "X86.h"
#include "X86Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"

using namespace llvm;

#define DEBUG_TYPE "x86-schedule-loop"

namespace {
class ScheduleLoopPass : public MachineFunctionPass {
public:
  ScheduleLoopPass() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return "X86 Schedule Loops"; }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  static char ID;
};
char ScheduleLoopPass::ID = 0;
}

FunctionPass *llvm::createX86ScheduleLoop() { return new ScheduleLoopPass(); }

bool ScheduleLoopPass::runOnMachineFunction(MachineFunction &MF) {
  bool Changed = false;

  if (skipFunction(*MF.getFunction()))
    return false;

  // Process all basic blocks.
  for (auto &MBB : MF) {
    for (auto &MI : MBB) {
      if (MI.mayLoadOrStore()) {
        MI.dump();
      }
    }
  }

  return Changed;
}
