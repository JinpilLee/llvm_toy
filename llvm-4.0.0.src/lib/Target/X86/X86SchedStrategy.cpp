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

#define DEBUG_TYPE "misched"

using namespace llvm;

X86SchedStrategy::X86SchedStrategy(const MachineSchedContext *C)
  : GenericScheduler(C) {
}

SUnit *X86SchedStrategy::pickNode(bool &IsTopNode) {
  return GenericScheduler::pickNode(IsTopNode);
}
