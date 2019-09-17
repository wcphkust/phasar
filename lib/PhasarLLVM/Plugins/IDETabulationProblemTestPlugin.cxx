/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <iostream>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFact.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFactWrapper.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFactWrapper.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>

#include "IDETabulationProblemTestPlugin.h"
using namespace std;
using namespace psr;

namespace psr {

unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemTestPlugin(LLVMBasedICFG &I,
                                   vector<string> EntryPoints) {
  return unique_ptr<IDETabulationProblemPlugin>(
      new IDETabulationProblemTestPlugin(I, EntryPoints));
}

__attribute__((constructor)) void init() {
  cout << "init - IDETabulationProblemTestPlugin\n";
  IDETabulationProblemPluginFactory["ide_testplugin"] =
      &makeIDETabulationProblemTestPlugin;
}

__attribute__((destructor)) void fini() {
  cout << "fini - IDETabulationProblemTestPlugin\n";
}

// Initialize debug counter for edge functions
unsigned IDETabulationProblemTestPlugin::CurrGenConstant_Id = 0;
unsigned IDETabulationProblemTestPlugin::CurrLCAID_Id = 0;
unsigned IDETabulationProblemTestPlugin::CurrBinary_Id = 0;

// const IDETabulationProblemTestPlugin::v_t IDETabulationProblemTestPlugin::TOP
// =
//     numeric_limits<IDETabulationProblemTestPlugin::v_t>::min();

// const IDETabulationProblemTestPlugin::v_t
// IDETabulationProblemTestPlugin::BOTTOM =
//     numeric_limits<IDETabulationProblemTestPlugin::v_t>::max();

IDETabulationProblemTestPlugin::IDETabulationProblemTestPlugin(
    LLVMBasedICFG &I, vector<string> EntryPoints)
    : IDETabulationProblemPlugin(I, createZeroValue(), EntryPoints) {}

IDETabulationProblemTestPlugin::~IDETabulationProblemTestPlugin() {
  IDETabulationProblemTestPlugin::CurrGenConstant_Id = 0;
  IDETabulationProblemTestPlugin::CurrLCAID_Id = 0;
  IDETabulationProblemTestPlugin::CurrBinary_Id = 0;
}

const FlowFact *IDETabulationProblemTestPlugin::createZeroValue() {
  // create a special value to represent the zero value!
  return new FlowFactWrapper<const llvm::Value *>(LLVMZeroValue::getInstance());
}

bool IDETabulationProblemTestPlugin::isZeroValue(const FlowFact *d) const {
  const FlowFactWrapper<const llvm::Value *> *d1 =
      static_cast<const FlowFactWrapper<const llvm::Value *> *>(d);
  return LLVMZeroValue::getInstance()->isLLVMZeroValue(d1->get());
}

shared_ptr<FlowFunction<const FlowFact *>>
IDETabulationProblemTestPlugin::getNormalFlowFunction(
    const llvm::Instruction *curr, const llvm::Instruction *succ) {
  cout << "IDETabulationProblemTestPlugin::getNormalFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IDETabulationProblemTestPlugin::getCallFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  cout << "IDETabulationProblemTestPlugin::getCallFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IDETabulationProblemTestPlugin::getRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMthd,
    const llvm::Instruction *exitStmt, const llvm::Instruction *retSite) {
  cout << "IDETabulationProblemTestPlugin::getRetFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IDETabulationProblemTestPlugin::getCallToRetFlowFunction(
    const llvm::Instruction *callSite, const llvm::Instruction *retSite,
    set<const llvm::Function *> callees) {
  cout << "IDETabulationProblemTestPlugin::getCallToRetFlowFunction()\n";
  return Identity<const FlowFact *>::getInstance();
}

shared_ptr<FlowFunction<const FlowFact *>>
IDETabulationProblemTestPlugin::getSummaryFlowFunction(
    const llvm::Instruction *callStmt, const llvm::Function *destMthd) {
  cout << "IDETabulationProblemTestPlugin::getSummaryFlowFunction()\n";
  return nullptr;
}

map<const llvm::Instruction *, set<const FlowFact *>>
IDETabulationProblemTestPlugin::initialSeeds() {
  cout << "IDETabulationProblemTestPlugin::initialSeeds()\n";
  map<const llvm::Instruction *, set<const FlowFact *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    SeedMap.insert(std::make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                                  set<const FlowFact *>({zeroValue()})));
  }
  return SeedMap;
}

} // namespace psr
