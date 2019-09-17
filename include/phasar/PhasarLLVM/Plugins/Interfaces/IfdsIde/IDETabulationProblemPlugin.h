/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IDETABULATIONPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IDETABULATIONPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <phasar/PhasarLLVM/IfdsIde/DefaultIDETabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFact.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/Utils/LLVMShorthands.h>

namespace llvm {
class Function;
class Instruction;
class Value;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class IDETabulationProblemPlugin
    : public DefaultIDETabulationProblem<
          const llvm::Instruction *, const FlowFact *, const llvm::Function *,
          const EdgeFact *, LLVMBasedICFG &> {

protected:
  std::vector<std::string> EntryPoints;

public:
  IDETabulationProblemPlugin(LLVMBasedICFG &ICFG, const FlowFact *zeroValue,
                             std::vector<std::string> EntryPoints = {"main"})
      : DefaultIDETabulationProblem<const llvm::Instruction *, const FlowFact *,
                                    const llvm::Function *, const EdgeFact *,
                                    LLVMBasedICFG &>(ICFG),
        EntryPoints(EntryPoints) {
    IDETabulationProblemPlugin::zerovalue = zeroValue;
  }
  ~IDETabulationProblemPlugin() override = default;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override {
    os << llvmIRToString(n);
  }

  void printMethod(std::ostream &os, const llvm::Function *m) const override {
    os << m->getName().str();
  }
};

extern "C" std::unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemPlugin(LLVMBasedICFG &I,
                               std::vector<std::string> EntryPoints);

extern std::map<std::string,
                std::unique_ptr<IDETabulationProblemPlugin> (*)(
                    LLVMBasedICFG &I, std::vector<std::string> EntryPoints)>
    IDETabulationProblemPluginFactory;

} // namespace psr

#endif
