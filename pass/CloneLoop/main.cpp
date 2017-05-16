#include "llvm/Pass.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopIterator.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instruction.h"
#include "llvm/IR/Constants.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <iostream>
#include <vector>
#include <map>

#include "boost/graph/adjacency_list.hpp"
#include "boost/graph/graph_utility.hpp"
#include "boost/graph/topological_sort.hpp"

using namespace llvm;

class TreeNode;

std::vector<TreeNode *> RootNodes;
std::map<Instruction *, TreeNode *> TreeCreationCache;

typedef
boost::adjacency_list<boost::listS, boost::vecS,
                      boost::directedS> Graph;

typedef
std::map<Instruction *, Graph::vertex_descriptor> VerDesc;
typedef
std::map<Graph::vertex_descriptor, Instruction *> InstrMap;

class TreeNode {
public:
  TreeNode(Instruction *I, VerDesc &D, Graph &G) {
    if (StoreInst *SI = dyn_cast<StoreInst>(I)) {
      Latency = 11;
    }
    else if (LoadInst *LI = dyn_cast<LoadInst>(I)) {
      Latency = 11;
    }
    else {
      Latency = 9;
    }

    for (User::op_iterator OpIter = I->op_begin(), OpEnd = I->op_end();
         OpIter != OpEnd; ++OpIter) {
      Instruction *OpInstr = dyn_cast<Instruction>(*OpIter);
      if (OpInstr) {    
        if (OpInstr->getOpcode() == Instruction::PHI) continue;

        TreeNode *SubNode;
        auto SubNodeIter = TreeCreationCache.find(OpInstr);
        if (SubNodeIter == TreeCreationCache.end()) {
          TreeCreationCache[OpInstr] = SubNode = new TreeNode(OpInstr, D, G);
        }
        else {
          SubNode = SubNodeIter->second;
        }

        add_edge(D[OpInstr], D[I], G);
      }
    }
  }

  int Latency;
};

class FuncBlockCount : public FunctionPass {
public:
  static char ID;

  FuncBlockCount() : FunctionPass(ID) {
  }

  virtual bool runOnFunction(Function &F);
  virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

bool FuncBlockCount::runOnFunction(Function &F) {
  auto *LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto *DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  int count = 0;
  Graph G;
  VerDesc D;
  InstrMap IM;

  errs() << "===== ORIGINAL LOOP ==========\n";

  for (Loop *L : *LI) {
    LoopBlocksDFS DFS(L);
    DFS.perform(LI);
    for (BasicBlock *BB : make_range(DFS.beginRPO(), DFS.endRPO())) {
      for (auto &I : *BB) {
        errs() << count << ": " << I << "\n";
        count++;
        auto V = add_vertex(G);
        D[&I] = V;
        IM[V] = &I;
      }
    }

    for (BasicBlock *BB : make_range(DFS.beginRPO(), DFS.endRPO()))
      for (auto &I : *BB) {
        Value *Val = dyn_cast<Value>(&I);
        if (I.isTerminator()) {
        }
        else if (Val->getNumUses() == 0) {
          TreeNode *Node = new TreeNode(&I, D, G);
          TreeCreationCache[&I] = Node;
          RootNodes.push_back(Node);
        }
      }

    for (BasicBlock *BB : make_range(DFS.beginRPO(), DFS.endRPO()))
      for (auto &I : *BB) {
        int OpLat = 0;
        for (User::op_iterator OpIter = I.op_begin(), OpEnd = I.op_end();
           OpIter != OpEnd; ++OpIter) {
          Instruction *OpInstr = dyn_cast<Instruction>(*OpIter);
          TreeNode *Node = TreeCreationCache[OpInstr];
          if (Node) {
            int Lat = Node->Latency;
            if (Lat > OpLat) {
              OpLat = Lat;
            }
          }
        }

        TreeNode *Node =  TreeCreationCache[&I];
        if (Node)
          Node->Latency += OpLat;
      }
  }

  errs() << "===== NODE GRAPH ==========\n";
  boost::print_graph(G, std::cerr);

  std::vector<Graph::vertex_descriptor> Result;
  boost::topological_sort(G, std::back_inserter(Result));

  errs() << "===== NODE VALUE ==========\n";
  for (std::vector<Graph::vertex_descriptor>::iterator Iter = Result.begin();
       Iter != Result.end(); ++Iter) {
    Instruction *Instr = IM[*Iter];
    int Lat;
    TreeNode *Node = TreeCreationCache[Instr];
    if (Node) {
      Lat = Node->Latency;
    }
    else {
      Lat = 9999999;
    }
    errs() << Lat << "\t" << *Instr << "\n";
  }

  for (Loop *L : *LI) {
    LoopBlocksDFS DFS(L);
    DFS.perform(LI);
    for (BasicBlock *BB : make_range(DFS.beginRPO(), DFS.endRPO())) {
      Instruction *SplitInstr = nullptr;
      for (auto &I : *BB) {
        int Lat;
        TreeNode *Node = TreeCreationCache[&I];
        if (Node) {
          Lat = Node->Latency;
          if ((Lat > 50) && (Lat < 60)) {
            SplitInstr = &I;
            MDNode *MD = MDNode::get(F.getContext(),
                                     ArrayRef<Metadata *>(ConstantAsMetadata::get(
                                     ConstantInt::get(Type::getInt64Ty(F.getContext()), 0))));
            I.setMetadata("split.instr", MD);
          }
        }
      }

      if (SplitInstr)
        SplitBlock(BB, SplitInstr, DT, LI);
    }
  }

  errs() << "===== SPLITTED BASIC BLOCK ==========\n";
  for (Loop *L : *LI) {
    LoopBlocksDFS DFS(L);
    DFS.perform(LI);
    for (BasicBlock *BB : make_range(DFS.beginRPO(), DFS.endRPO()))
      BB->dump();
  }

  errs() << "===== SEARCH METADATA ==========\n";
  for (Loop *L : *LI) {
    LoopBlocksDFS DFS(L);
    DFS.perform(LI);
    for (BasicBlock *BB : make_range(DFS.beginRPO(), DFS.endRPO())) {
      for (auto &I : *BB) {
        if (MDNode *MD = I.getMetadata("split.instr"))
          errs() << "split point: " << I << "\n";
      }
    }
  }

  return false;
}

void FuncBlockCount::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<LoopInfoWrapperPass>();
  AU.addRequired<DominatorTreeWrapperPass>();
}

char FuncBlockCount::ID = 0;
static RegisterPass<FuncBlockCount> X("funcpass", "Function Block Count",
                                      false, false);
