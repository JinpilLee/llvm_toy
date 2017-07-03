//===- VectorLoopSchedule.cpp - Instr Sched Pass for SIMD Loops -*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Instruction Scheduling Pass for SIMD Vectorized Loops
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_TRANSFORMS_VECTORIZE_VECTORLOOPSCHED_H
#define LLVM_TRANSFORMS_VECTORIZE_VECTORLOOPSCHED_H

#include "llvm/IR/PassManager.h"
namespace llvm {

class VectorLoopSchedulePass : public PassInfoMixin<VectorLoopSchedulePass> {
public:
  PreservedAnalyses run(Function &F, FunctionAnalysisManager &AM);
};
} // end namespace llvm

#endif // LLVM_TRANSFORMS_VECTORIZE_VECTORLOOPSCHED_H
