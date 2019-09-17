/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * IFDSExtendedTabulationProblemPlugin.h
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IFDSTABULATIONPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_IFDSIDE_IFDSTABULATIONPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <phasar/PhasarLLVM/IfdsIde/DefaultIFDSTabulationProblem.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/Utils/LLVMShorthands.h>

namespace llvm {
class Function;
class Instruction;
} // namespace llvm

namespace psr {

class LLVMBasedICFG;

class IFDSTabulationProblemPlugin
    : public DefaultIFDSTabulationProblem<
          const llvm::Instruction *, const FlowFact *, const llvm::Function *,
          LLVMBasedICFG &> {
protected:
  std::vector<std::string> EntryPoints;

public:
  IFDSTabulationProblemPlugin(LLVMBasedICFG &ICFG, const FlowFact *zeroValue,
                              std::vector<std::string> EntryPoints = {"main"})
      : DefaultIFDSTabulationProblem<const llvm::Instruction *,
                                     const FlowFact *, const llvm::Function *,
                                     LLVMBasedICFG &>(ICFG),
        EntryPoints(EntryPoints) {
    DefaultIFDSTabulationProblem::zerovalue = zeroValue;
  }
  ~IFDSTabulationProblemPlugin() override = default;

  void printNode(std::ostream &os, const llvm::Instruction *n) const override {
    os << llvmIRToString(n);
  }

  void printMethod(std::ostream &os, const llvm::Function *m) const override {
    os << m->getName().str();
  }
};

extern std::map<std::string,
                std::unique_ptr<IFDSTabulationProblemPlugin> (*)(
                    LLVMBasedICFG &I, std::vector<std::string> EntryPoints)>
    IFDSTabulationProblemPluginFactory;

} // namespace psr

#endif
