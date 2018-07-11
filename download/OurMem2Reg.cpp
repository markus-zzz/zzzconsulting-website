#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/IteratedDominanceFrontier.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

#if 0
#define DEBUG_TYPE "hello"
STATISTIC(HelloCounter, "Counts number of functions greeted");
#endif

namespace {
  struct OurMemToReg : public FunctionPass {
    struct VariableInfo {
      VariableInfo(AllocaInst *AI) : Alloca(AI) {}
      AllocaInst *Alloca;
      SmallPtrSet<BasicBlock *, 32> DefBlocks;
      std::vector<Value*> DefStack;
    };

    std::vector<Instruction*> trashList;
    std::vector<VariableInfo*> variableInfos;
    DenseMap<Instruction*, VariableInfo*> instToVariableInfo;

    static char ID; // Pass identification, replacement for typeid

    OurMemToReg() : FunctionPass(ID) {}

    bool linkDefsAndUsesToVar(AllocaInst *alloca, VariableInfo *varInfo) {
      for (auto U : alloca->users()) {
        Instruction *UI;
        if ((UI = dyn_cast<LoadInst>(U))) {
          instToVariableInfo[UI] = varInfo;
        }
        else if ((UI = dyn_cast<StoreInst>(U))) {
          // Need to check that the U is actually address and not datum
          if (UI->getOperand(1) == alloca) {
            instToVariableInfo[UI] = varInfo;
            varInfo->DefBlocks.insert(UI->getParent());
          }
          else {
            return false;
          }
        }
        else {
          return false;
        }
      }
      return true;
    }

    void renameRecursive(DomTreeNode *DN) {
      BasicBlock &BB = *DN->getBlock();

      for (Instruction &II : BB) {
        Instruction *I = &II;
        VariableInfo *VI;
        if (isa<StoreInst>(I) && (VI = instToVariableInfo[I])) {
          VI->DefStack.push_back(I->getOperand(0));
        }
        else if (isa<LoadInst>(I) && (VI = instToVariableInfo[I])) {
          if (VI->DefStack.size() > 0) {
              I->replaceAllUsesWith(VI->DefStack.back());
          }
        }
        else if (isa<PHINode>(I) && (VI = instToVariableInfo[I])) {
          VI->DefStack.push_back(I);
        }
      }

      // For phi-nodes in successors of BB I->addIncomming(Value, BB)
      for (succ_iterator I = succ_begin(&BB), E = succ_end(&BB); I != E; ++I) {
        for (Instruction &II : **I) {
          PHINode *phi;
          VariableInfo *VI;
          if ((phi = dyn_cast<PHINode>(&II)) && (VI = instToVariableInfo[&II])) {
            phi->addIncoming(VI->DefStack.back(), &BB);
          }
        }
      }

      for (auto I : DN->getChildren()) {
        renameRecursive(I);
      }

      for (Instruction &II : BB) {
        Instruction *I = &II;
        VariableInfo *VI;
        if (isa<StoreInst>(I) && (VI = instToVariableInfo[I])) {
          VI->DefStack.pop_back();
          trashList.push_back(I);
        }
        else if (isa<PHINode>(I) && (VI = instToVariableInfo[I])) {
          VI->DefStack.pop_back();
        }
        else if (isa<LoadInst>(I) && (VI = instToVariableInfo[I])) {
          trashList.push_back(I);
        }
      }
    }

    bool runOnFunction(Function &F) override {

      // We need the iterated dominance frontier of defs to place phi-nodes
      DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
      ForwardIDFCalculator IDF(DT);

      // Find allocas and then link defs (stores) and uses (loads) to variables
      // (allocas)
      for (auto &I : F.getEntryBlock()) {
        AllocaInst *AI;
        if ((AI = dyn_cast<AllocaInst>(&I))) {
          VariableInfo *VI = new VariableInfo(AI);
          if (linkDefsAndUsesToVar(AI, VI))
            variableInfos.push_back(VI);
          else
            delete VI;
        }
      }

      // Insert phi-nodes in iterated dominance frontier of defs
      for (auto VI : variableInfos) {
        IDF.setDefiningBlocks(VI->DefBlocks);
        SmallVector<BasicBlock *, 32> PHIBlocks;
        IDF.calculate(PHIBlocks);

        for (auto PB : PHIBlocks) {
          Instruction *PN;
          PN = PHINode::Create(VI->Alloca->getAllocatedType(), 0, "", &PB->front());
          instToVariableInfo[PN] = VI;
        }
      }

      // Do renaming
      DomTreeNode *DN = DT.getNode(&F.getEntryBlock());
      renameRecursive(DN);

      // Remove trash
      for (auto trash : trashList) {
        trash->eraseFromParent();
      }

      for (auto trash : variableInfos) {
        trash->Alloca->eraseFromParent();
      }

      return true;
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override {
      AU.addRequired<DominatorTreeWrapperPass>();
      AU.setPreservesCFG();
    }
  };
}

char OurMemToReg::ID = 0;
static RegisterPass<OurMemToReg> Z("ourmem2reg", "Our mem2reg pass");

