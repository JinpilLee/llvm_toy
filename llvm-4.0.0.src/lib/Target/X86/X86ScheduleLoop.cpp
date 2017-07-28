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
#include "llvm/CodeGen/MachineLoopInfo.h"
#include "llvm/CodeGen/Passes.h"
#include "llvm/IR/Function.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Target/TargetInstrInfo.h"
#include <map>

using namespace llvm;

typedef std::map<unsigned, MachineInstr *> RegInstrMap;

// FIXME ad-hoc impl
std::vector<MachineInstr *> X86SchedHighPriorInstrVector;

#define DEBUG_TYPE "x86-schedule-loop"

namespace {
class ScheduleLoopPass : public MachineFunctionPass {
public:
  ScheduleLoopPass() : MachineFunctionPass(ID) {}

  StringRef getPassName() const override { return "X86 Schedule Loops"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfo>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  void mapDef(RegInstrMap &DefMap, MachineInstr &MI);
  void addInstrRec(RegInstrMap &DefMap, MachineInstr *MI);
  bool processLoop(MachineLoop *L);

  static char ID;
};

char ScheduleLoopPass::ID = 0;
}

FunctionPass *llvm::createX86ScheduleLoop() { return new ScheduleLoopPass(); }

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

void ScheduleLoopPass::mapDef(RegInstrMap &DefMap, MachineInstr &MI) {
  for (unsigned i = 0; i < MI.getNumOperands(); i++) {
    MachineOperand &MO = MI.getOperand(i);
    if (MO.isReg() && MO.isDef()) {
      DefMap[MO.getReg()] = &MI;
    }
  }
}

void ScheduleLoopPass::addInstrRec(RegInstrMap &DefMap, MachineInstr *MI) {
  X86SchedHighPriorInstrVector.push_back(MI);
  for (unsigned i = 0; i < MI->getNumOperands(); i++) {
    MachineOperand &MO = MI->getOperand(i);
    if (MO.isReg() && !MO.isDef()) {
      RegInstrMap::iterator Iter = DefMap.find(MO.getReg());
      if (Iter != DefMap.end()) {
        addInstrRec(DefMap, Iter->second);
      }
    }
  }
}

bool ScheduleLoopPass::processLoop(MachineLoop *L) {
  RegInstrMap DefMap;
  for (auto &MBB : L->blocks()) {
    for (auto &MI : *MBB) {
      mapDef(DefMap, MI);

      if (MI.mayLoad()) {
        addInstrRec(DefMap, &MI);
      }
    }
  }

  return false;
}
