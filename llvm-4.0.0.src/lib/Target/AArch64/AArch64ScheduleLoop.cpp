//===-- AArch64ScheduleLoop.cpp - scheduling instructions in loops --------===//
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

#include "AArch64.h"
#include "AArch64Subtarget.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include <vector>

std::vector<unsigned> MemInstrRegVector;

using namespace llvm;

#define DEBUG_TYPE "aarch64-schedule-loop"

namespace {
class ScheduleLoopPass : public MachineFunctionPass {
public:
  ScheduleLoopPass() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return "AArch64 Schedule Loops"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfo>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  bool processLoop(MachineLoop *L);

  static char ID;
};

char ScheduleLoopPass::ID = 0;
}

FunctionPass *llvm::createAArch64ScheduleLoop() { return new ScheduleLoopPass(); }

bool ScheduleLoopPass::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(*MF.getFunction()))
    return false;

  MachineLoopInfo *MLI = &getAnalysis<MachineLoopInfo>();
  SmallVector<MachineLoop *, 8> Worklist(MLI->begin(), MLI->end());
  SmallVector<MachineLoop *, 8> InnermostLoops;
  while (!Worklist.empty()) {
    MachineLoop *CurLoop = Worklist.pop_back_val();
    if (CurLoop->begin() == CurLoop->end())
      InnermostLoops.push_back(CurLoop);
    else
      Worklist.append(CurLoop->begin(), CurLoop->end());
  }

  bool Changed = false;
  for (auto &L : InnermostLoops) {
    Changed |= processLoop(L);
  }

  return Changed;
}

bool ScheduleLoopPass::processLoop(MachineLoop *L) {
  for (auto &MBB : L->blocks()) {
    for (auto &MI : *MBB) {
      if (MI.mayLoad()) {
        for (unsigned i = 0; i < MI.getNumOperands(); i++) {
          MachineOperand &MO = MI.getOperand(i);
          if (MO.isReg()) {
            MemInstrRegVector.push_back(MO.getReg());
          }
        }
      }
    }
  }

  return false;
}
