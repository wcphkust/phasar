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

#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctionComposer.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFactWrapper.h>
#include <phasar/PhasarLLVM/IfdsIde/Problems/TypeStateDescriptions/TypeStateDescription.h>
#include <phasar/PhasarLLVM/Plugins/Interfaces/IfdsIde/IDETabulationProblemPlugin.h>

namespace psr {

class IDETabulationProblemTestPlugin : public IDETabulationProblemPlugin {
private:
  // For debug purpose only
  static unsigned CurrGenConstant_Id;
  static unsigned CurrLCAID_Id;
  static unsigned CurrBinary_Id;

  //   static const const EdgeFact * TOP;
  //   static const const EdgeFact * BOTTOM;
  const TypeStateDescription &TSD;
  std::map<const llvm::Value *, std::set<const llvm::Value *>> PointsToCache;
  std::map<const llvm::Value *, std::set<const llvm::Value *>>
      RelevantAllocaCache;

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

  /**
   * We have a lattice with BOTTOM representing all information
   * and TOP representing no information. The other lattice elements
   * are defined by the type state description, i.e. represented by the
   * states of the finite state machine.
   *
   * @note Only one-level lattice's are handled currently
   */
  const EdgeFact *join(const EdgeFact *lhs, const EdgeFact *rhs) override;

  /**
   * @brief Checks if the type machtes the type of interest.
   */
  bool hasMatchingType(const llvm::Value *V);

  std::set<const llvm::Value *>
  getLocalAliasesAndAllocas(const llvm::Value *V, const std::string &Fname);

  /**
   * @brief Returns all alloca's that are (indirect) aliases of V.
   *
   * Currently PhASAR's points-to information does not include alloca
   * instructions, since alloca instructions, i.e. memory locations, are of
   * type T* for a target type T. Thus they do not alias directly. Therefore,
   * for each alias of V we collect related alloca instructions by checking
   * load and store instructions for used alloca's.
   */
  std::set<const llvm::Value *>
  IDETabulationProblemTestPlugin::getRelevantAllocas(const llvm::Value *V);

  /**
   * @brief Returns whole-module aliases of V.
   *
   * This function retrieves whole-module points-to information. We store
   * already computed points-to information in a cache to prevent expensive
   * recomputation since the whole module points-to graph can be huge. This
   * might become unnecessary once PhASAR's PointsToGraph starts using a cache
   * itself.
   */
  std::set<const llvm::Value *>
  IDETabulationProblemTestPlugin::getWMPointsToSet(const llvm::Value *V);

  /**
   * @brief Provides whole module aliases and relevant alloca's of V.
   */
  std::set<const llvm::Value *>
  IDETabulationProblemTestPlugin::getWMAliasesAndAllocas(const llvm::Value *V);

  // customize the edge function composer
  class TSEdgeFunctionComposer : public EdgeFunctionComposer<const EdgeFact *> {
  private:
    const EdgeFact *botElement;

  public:
    TSEdgeFunctionComposer(std::shared_ptr<EdgeFunction<const EdgeFact *>> F,
                           std::shared_ptr<EdgeFunction<const EdgeFact *>> G,
                           const EdgeFact *bot)
        : EdgeFunctionComposer<const EdgeFact *>(F, G), botElement(bot){};
    std::shared_ptr<EdgeFunction<const EdgeFact *>> joinWith(
        std::shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) override;
  };

  class TSEdgeFunction : public EdgeFunction<const EdgeFact *>,
                         public std::enable_shared_from_this<TSEdgeFunction> {
  protected:
    const TypeStateDescription &TSD;
    // Do not use a reference here, since LLVM's StringRef's (obtained by str())
    // might turn to nullptr for whatever reason...
    const std::string Token;
    int CurrentState;

  public:
    TSEdgeFunction(const TypeStateDescription &tsd, const std::string tok)
        : TSD(tsd), Token(tok), CurrentState(TSD.top()){};

    const EdgeFact *computeTarget(const EdgeFact *source) override;

    std::shared_ptr<EdgeFunction<const EdgeFact *>>
    composeWith(std::shared_ptr<EdgeFunction<const EdgeFact *>> secondFunction)
        override;

    std::shared_ptr<EdgeFunction<const EdgeFact *>> joinWith(
        std::shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) override;

    bool equal_to(
        std::shared_ptr<EdgeFunction<const EdgeFact *>> other) const override;

    void print(std::ostream &OS, bool isForDebug = false) const override;
  };
};

extern "C" std::unique_ptr<IDETabulationProblemPlugin>
makeIDETabulationProblemTestPlugin(LLVMBasedICFG &I,
                                   std::vector<std::string> EntryPoints);

} // namespace psr

#endif /* SRC_ANALYSIS_PLUGINS_IDETABULATIONPROBLEMTESTPLUGIN_HH_ */
