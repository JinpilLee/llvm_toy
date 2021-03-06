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

#define FINDLDCHN_DESC "X86 Find Load Chain"
#define FINDLDCHN_NAME "x86-find-load-chain"

#define DEBUG_TYPE FINDLDCHN_NAME

namespace {
class X86FindLoadChain : public MachineFunctionPass {
public:
  static char ID;

  X86FindLoadChain() : MachineFunctionPass(ID) {
    initializeX86FindLoadChainPass(*PassRegistry::getPassRegistry());
  }

  StringRef getPassName() const override { return "X86 Find Load Chain"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<MachineLoopInfo>();
    MachineFunctionPass::getAnalysisUsage(AU);
  }

  bool runOnMachineFunction(MachineFunction &MF) override;

private:
  const X86InstrInfo *TII;

  void mapDef(RegInstrMap &DefMap, MachineInstr &MI);
  void addInstrRec(RegInstrMap &DefMap, MachineInstr *MI);
  bool processLoop(MachineLoop *L);
};

char X86FindLoadChain::ID = 0;
}

INITIALIZE_PASS(X86FindLoadChain, FINDLDCHN_NAME, FINDLDCHN_DESC, false, false)

char &llvm::X86FindLoadChainID = X86FindLoadChain::ID;
FunctionPass *llvm::createX86FindLoadChainPass() { return new X86FindLoadChain(); }

bool X86FindLoadChain::runOnMachineFunction(MachineFunction &MF) {
  if (skipFunction(*MF.getFunction()))
    return false;
  
  TII = MF.getSubtarget<X86Subtarget>().getInstrInfo();

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

void X86FindLoadChain::mapDef(RegInstrMap &DefMap, MachineInstr &MI) {
  for (unsigned i = 0; i < MI.getNumOperands(); i++) {
    MachineOperand &MO = MI.getOperand(i);
    if (MO.isReg() && MO.isDef() && !MO.isDead()) {
      DefMap[MO.getReg()] = &MI;
    }
  }
}

void X86FindLoadChain::addInstrRec(RegInstrMap &DefMap, MachineInstr *MI) {
  X86SchedHighPriorInstrVector.push_back(MI);
  for (unsigned i = 0; i < MI->getNumOperands(); i++) {
    MachineOperand &MO = MI->getOperand(i);
    if (MO.isReg() && MO.readsReg()) {
      RegInstrMap::iterator Iter = DefMap.find(MO.getReg());
      if (Iter != DefMap.end()) {
        MachineInstr *DefMI = Iter->second;
        if (DefMI != MI) addInstrRec(DefMap, DefMI);
      }
    }
  }
}

bool X86FindLoadChain::processLoop(MachineLoop *L) {
  RegInstrMap DefMap;
  for (auto &MBB : L->blocks()) {
    for (auto &MI : *MBB) {
      mapDef(DefMap, MI);

      if (MI.mayLoad() &&
          TII->isHighLatencyDef(MI.getOpcode())) {
        addInstrRec(DefMap, &MI);
      }
    }
  }

  return false;
}
