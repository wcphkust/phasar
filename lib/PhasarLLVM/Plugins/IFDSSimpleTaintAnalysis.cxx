/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PluginTest.cpp
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#include <iostream>

#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Value.h>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>

#include "IFDSSimpleTaintAnalysis.h"
using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSSimpleTaintAnalysis(LLVMBasedICFG &I, vector<string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSSimpleTaintAnalysis(I, EntryPoints));
}

__attribute__((constructor)) void init() {
  cout << "init - IFDSSimpleTaintAnalysis\n";
  IFDSTabulationProblemPluginFactory["ifds_testplugin"] =
      &makeIFDSSimpleTaintAnalysis;
}

__attribute__((destructor)) void fini() {
  cout << "fini - IFDSSimpleTaintAnalysis\n";
}

const FlowFact *IFDSSimpleTaintAnalysis::createZeroValue() {
  // create a special value to represent the zero value!
  return new FlowFactWrapper<const llvm::Value *>(LLVMZeroValue::getInstance());
}

bool IFDSSimpleTaintAnalysis::isZeroValue(const FlowFact *d) const {
  const FlowFactWrapper<const llvm::Value *> *d1 =
      static_cast<const FlowFactWrapper<const llvm::Value *> *>(d);
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d1->get());
}

IFDSSimpleTaintAnalysis::IFDSSimpleTaintAnalysis(LLVMBasedICFG &I,
                                                 vector<string> EntryPoints)
    : IFDSTabulationProblemPlugin(I, createZeroValue(), EntryPoints) {}

void IFDSSimpleTaintAnalysis::printDataFlowFact(std::ostream &os,
                                                const FlowFact *d) const {
  const FlowFactWrapper<const llvm::Value *> *d1 =
      static_cast<const FlowFactWrapper<const llvm::Value *> *>(d);
  os << llvmIRToString(d1->get());
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSSimpleTaintAnalysis::getNormalFlowFunction(const llvm::Instruction *curr,
                                               const llvm::Instruction *succ) {
  cout << "IFDSSimpleTaintAnalysis::getNormalFlowFunction()\n";
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    struct TFF : FlowFunction<const FlowFact *> {
      const llvm::StoreInst *Store;
      TFF(const llvm::StoreInst *S) : Store(S) {}
      // TODO:Pointer clean up
      set<const FlowFact *> computeTargets(const FlowFact *src) override {
        const FlowFactWrapper<const llvm::Value *> *wrapper =
            static_cast<const FlowFactWrapper<const llvm::Value *> *>(src);
        if (Store->getValueOperand() == wrapper->get()) {
          return {new FlowFactWrapper<const llvm::Value *>(
                      Store->getValueOperand()),
                  new FlowFactWrapper<const llvm::Value *>(
                      Store->getPointerOperand())};
        } else {
          return {src};
        }
      }
    };
    return make_shared<TFF>(Store);
  }
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    struct TFF : FlowFunction<const FlowFact *> {
      const llvm::LoadInst *Load;
      TFF(const llvm::LoadInst *L) : Load(L) {}
      // TODO:Pointer clean up
      set<const FlowFact *> computeTargets(const FlowFact *src) override {
        const FlowFactWrapper<const llvm::Value *> *wrapper =
            static_cast<const FlowFactWrapper<const llvm::Value *> *>(src);
        if (Load->getPointerOperand() == wrapper->get()) {
          return {new FlowFactWrapper<const llvm::Value *>(Load), src};
        } else {
          return {src};
        }
      }
    };
    return make_shared<TFF>(Load);
  }
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSSimpleTaintAnalysis::getCallFlowFunction(const llvm::Instruction *callStmt,
                                             const llvm::Function *destMthd) {
  cout << "IFDSSimpleTaintAnalysis::getCallFlowFunction()\n";
  // Do not follow calls to sources and sinks and handle their special
  // semantics in getCallToRetFlowFunction()
  if (destMthd->getName() == "taint" || destMthd->getName() == "leak") {
    return KillAll<const FlowFact *>::getInstance();
  }
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSSimpleTaintAnalysis::getRetFlowFunction(const llvm::Instruction *callSite,
                                            const llvm::Function *calleeMthd,
                                            const llvm::Instruction *exitStmt,
                                            const llvm::Instruction *retSite) {
  cout << "IFDSSimpleTaintAnalysis::getRetFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSSimpleTaintAnalysis::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    set<const llvm::Function *> callees) {
  cout << "IFDSSimpleTaintAnalysis::getCallToRetFlowFunction()\n";
  for (auto callee : callees) {
    if (callee->getName() == "taint") {
      return make_shared<Gen<const FlowFact *>>(
          new FlowFactWrapper<const llvm::Value *>(callSite), zeroValue());
    }
    if (callee->getName() == "leak") {
      struct TFF : FlowFunction<const FlowFact *> {
        llvm::ImmutableCallSite CS;
        TFF(const llvm::Instruction *callSite) : CS(callSite) {}
        set<const FlowFact *> computeTargets(const FlowFact *src) override {
          for (auto &arg : CS.args()) {
            if (static_cast<const FlowFactWrapper<const llvm::Value *> *>(src)
                    ->get() == arg) {
              cout << "IFDSSimpleTaintAnalysis ==> found leak at instruction: "
                   << llvmIRToString(CS.getInstruction()) << '\n';
            }
          }
          return {src};
        }
      };
      return make_shared<TFF>(callSite);
    }
  }
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSSimpleTaintAnalysis::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  cout << "IFDSSimpleTaintAnalysis::getSummaryFlowFunction()\n";
  return nullptr;
}

map<const llvm::Instruction *, set<const FlowFact *>>
IFDSSimpleTaintAnalysis::initialSeeds() {
  cout << "IFDSSimpleTaintAnalysis::initialSeeds()\n";
  map<const llvm::Instruction *, set<const FlowFact *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const FlowFact *>({zeroValue()})));
  }
  return SeedMap;
}

} // namespace psr
