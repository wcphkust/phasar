/******************************************************************************
 * Copyright (c) 2019 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef SRC_ANALYSIS_PLUGINS_IDETABULATIONPROBLEMTESTPLUGIN_H_
#define SRC_ANALYSIS_PLUGINS_IDETABULATIONPROBLEMTESTPLUGIN_H_

#include <phasar/PhasarLLVM/IfdsIde/FlowFactWrapper.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h>

namespace psr {

class IDETabulationProblemTestPlugin : public IDETabulationProblemPlugin {
private:
  // For debug purpose only
  static unsigned CurrGenConstant_Id;
  static unsigned CurrLCAID_Id;
  static unsigned CurrBinary_Id;

  //   static const v_t TOP;
  //   static const v_t BOTTOM;

public:
  IDETabulationProblemTestPlugin(LLVMBasedICFG &I,
                                 std::vector<std::string> EntryPoints);

  ~IDETabulationProblemTestPlugin() override;

  const FlowFact *createZeroValue() override;

  bool isZeroValue(const FlowFact *d) const override;

  void printDataFlowFact(std::ostream &os, const FlowFact *d) const override;

  void printValue(std::ostream &os, const EdgeFact *v) const override;

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

  std::shared_ptr<EdgeFunction<const EdgeFact *>>
  getNormalEdgeFunction(const llvm::Instruction *curr, const FlowFact *currNode,
                        const llvm::Instruction *succ,
                        const FlowFact *succNode) override;

  std::shared_ptr<EdgeFunction<const EdgeFact *>>
  getCallEdgeFunction(const llvm::Instruction *callStmt,
                      const FlowFact *srcNode,
                      const llvm::Function *destinationMethod,
                      const FlowFact *destNode) override;

  std::shared_ptr<EdgeFunction<const EdgeFact *>> getReturnEdgeFunction(
      const llvm::Instruction *callSite, const llvm::Function *calleeMethod,
      const llvm::Instruction *exitStmt, const FlowFact *exitNode,
      const llvm::Instruction *reSite, const FlowFact *retNode) override;

  std::shared_ptr<EdgeFunction<const EdgeFact *>> getCallToRetEdgeFunction(
      const llvm::Instruction *callSite, const FlowFact *callNode,
      const llvm::Instruction *retSite, const FlowFact *retSiteNode,
      std::set<const llvm::Function *> callees) override;

  std::shared_ptr<EdgeFunction<const EdgeFact *>> getSummaryEdgeFunction(
      const llvm::Instruction *curr, const FlowFact *currNode,
      const llvm::Instruction *succ, const FlowFact *succNode) override;

  std::shared_ptr<EdgeFunction<const EdgeFact *>> allTopFunction() override;

  const EdgeFact *topElement() override;

  const EdgeFact *bottomElement() override;

  const EdgeFact *join(const EdgeFact *lhs, const EdgeFact *rhs) override;
};

extern "C" std::unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemTestPlugin(LLVMBasedICFG &I,
                                   std::vector<std::string> EntryPoints);

} // namespace psr

#endif /* SRC_ANALYSIS_PLUGINS_IDETABULATIONPROBLEMTESTPLUGIN_HH_ */
