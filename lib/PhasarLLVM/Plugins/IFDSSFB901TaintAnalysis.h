/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * PluginTest.h
 *
 *  Created on: 14.06.2017
 *      Author: philipp
 */

#ifndef SRC_ANALYSIS_PLUGINS_IFDSSFB901TaintAnalysis_H_
#define SRC_ANALYSIS_PLUGINS_IFDSSFB901TaintAnalysis_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFactWrapper.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IFDSExtendedTabulationProblemPlugin.h>

namespace psr {

class IFDSSFB901TaintAnalysis : public IFDSTabulationProblemPlugin {
public:
  IFDSSFB901TaintAnalysis(LLVMBasedICFG &I,
                          std::vector<std::string> EntryPoints);
  ~IFDSSFB901TaintAnalysis() = default;

  const FlowFact *createZeroValue() override {
    // create a special value to represent the zero value!
    return new const FlowFactWrapper<const llvm::Value *>(
        LLVMZeroValue::getInstance());
  }

  bool isZeroValue(const FlowFact *d) const override {
    const FlowFactWrapper<const llvm::Value *> *d1 =
        dynamic_cast<const FlowFactWrapper<const llvm::Value *> *>(d);
    return LLVMZeroValue::getInstance()->isLLVMZeroValue(d1->get());
  }

  void printDataFlowFact(std::ostream &os, const FlowFact *d) const override {
    const FlowFactWrapper<const llvm::Value *> *d1 =
        dynamic_cast<const FlowFactWrapper<const llvm::Value *> *>(d);
    os << llvmIRToString(d1->get());
  }

  std::shared_ptr<FlowFunction<const FlowFact *>>
  getNormalFlowFunction(const llvm::Instruction *curr,
                        const llvm::Instruction *succ) override;

  std::shared_ptr<FlowFunction<const FlowFact *>>
  getCallFlowFunction(const llvm::Instruction *callStmt,
                      const llvm::Function *destMthd) override;

  std::shared_ptr<FlowFunction<const FlowFact *>>
  getRetFlowFunction(const llvm::Instruction *callSite,
                     const llvm::Function *calleeMthd,
                     const llvm::Instruction *exitStmt,
                     const llvm::Instruction *retSite) override;

  std::shared_ptr<FlowFunction<const FlowFact *>>
  getCallToRetFlowFunction(const llvm::Instruction *callSite,
                           const llvm::Instruction *retSite,
                           std::set<const llvm::Function *> callees) override;

  std::shared_ptr<FlowFunction<const FlowFact *>>
  getSummaryFlowFunction(const llvm::Instruction *callStmt,
                         const llvm::Function *destMthd) override;

  std::map<const llvm::Instruction *, std::set<const FlowFact *>>
  initialSeeds() override;
};

extern "C" std::unique_ptr<IFDSTabulationProblemPlugin>
makeIFDSSFB901TaintAnalysis(LLVMBasedICFG &I,
                            std::vector<std::string> EntryPoints);
} // namespace psr

#endif /* SRC_ANALYSIS_PLUGINS_IFDSSFB901TaintAnalysis_H_ */
