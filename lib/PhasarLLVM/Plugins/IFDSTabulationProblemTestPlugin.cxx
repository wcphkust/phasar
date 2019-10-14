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

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>

#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>

#include "IFDSTabulationProblemTestPlugin.h"
using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSTabulationProblemTestPlugin(LLVMBasedICFG &I,
                                    vector<string> EntryPoints) {
  return unique_ptr<IFDSTabulationProblemPlugin>(
      new IFDSTabulationProblemTestPlugin(I, EntryPoints));
}

__attribute__((constructor)) void init() {
  cout << "init - IFDSTabulationProblemTestPlugin\n";
  IFDSTabulationProblemPluginFactory["ifds_testplugin"] =
      &makeIFDSTabulationProblemTestPlugin;
}

__attribute__((destructor)) void fini() {
  cout << "fini - IFDSTabulationProblemTestPlugin\n";
}

IFDSTabulationProblemTestPlugin::IFDSTabulationProblemTestPlugin(
    LLVMBasedICFG &I, vector<string> EntryPoints)
    : IFDSTabulationProblemPlugin(I, createZeroValue(), EntryPoints) {}

const FlowFact *IFDSTabulationProblemTestPlugin::createZeroValue() {
  // create a special value to represent the zero value!
  return new FlowFactWrapper<const llvm::Value *>(LLVMZeroValue::getInstance());
}

bool IFDSTabulationProblemTestPlugin::isZeroValue(const FlowFact *d) const {
  const FlowFactWrapper<const llvm::Value *> *d1 =
      static_cast<const FlowFactWrapper<const llvm::Value *> *>(d);
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d1->get());
}

void IFDSTabulationProblemTestPlugin::printDataFlowFact(
    std::ostream &os, const FlowFact *d) const {
  const FlowFactWrapper<const llvm::Value *> *d1 =
      static_cast<const FlowFactWrapper<const llvm::Value *> *>(d);
  os << llvmIRToString(d1->get());
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSTabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction *curr, const llvm::Instruction *succ) {
  cout << "IFDSTabulationProblemTestPlugin::getNormalFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSTabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  cout << "IFDSTabulationProblemTestPlugin::getCallFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSTabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  cout << "IFDSTabulationProblemTestPlugin::getRetFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    set<const llvm::Function *> callees) {
  cout << "IFDSTabulationProblemTestPlugin::getCallToRetFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IFDSTabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  return nullptr;
}

map<const llvm::Instruction *, set<const FlowFact *>>
IFDSTabulationProblemTestPlugin::initialSeeds() {
  cout << "IFDSTabulationProblemTestPlugin::initialSeeds()\n";
  map<const llvm::Instruction *, set<const FlowFact *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const FlowFact *>({zeroValue()})));
  }
  return SeedMap;
}

} // namespace psr
