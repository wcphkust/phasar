/******************************************************************************
 * Copyright (c) 2020 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert, Linus Jungemann and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_MEMORYMANAGER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_MEMORYMANAGER_H_

#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"

// All singletons
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillAll.h>


namespace psr {

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class FlowEdgeFunctionCache;

template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class MemoryManager {

    friend class FlowEdgeFunctionCache<N, D, F, T, V, L, I>;

    // Caches for the flow functions
  std::map<std::tuple<N, N>, FlowFunction<D> *> NormalFlowFunctionCache;
  std::map<std::tuple<N, F>, FlowFunction<D> *> CallFlowFunctionCache;
  std::map<std::tuple<N, F, N, N>, FlowFunction<D> *> ReturnFlowFunctionCache;
  std::map<std::tuple<N, N, std::set<F>>, FlowFunction<D> *>
      CallToRetFlowFunctionCache;
  // Caches for the edge functions
  std::map<std::tuple<N, D, N, D>, EdgeFunction<L> *> NormalEdgeFunctionCache;
  std::map<std::tuple<N, D, F, D>, EdgeFunction<L> *> CallEdgeFunctionCache;
  std::map<std::tuple<N, F, N, D, N, D>, EdgeFunction<L> *>
      ReturnEdgeFunctionCache;
  std::map<std::tuple<N, D, N, D>, EdgeFunction<L> *>
      CallToRetEdgeFunctionCache;
  std::map<std::tuple<N, D, N, D>, EdgeFunction<L> *> SummaryEdgeFunctionCache;

  // Data for clean up
  std::unordered_set<EdgeFunction<L> *> managedEdgeFunctions;
  std::unordered_set<EdgeFunction<L> *> registeredEdgeFunctionSingletons = {
      EdgeIdentity<L>::getInstance()};
  std::unordered_set<FlowFunction<D> *> registeredFlowFunctionSingletons = {
      Identity<D>::getInstance(), KillAll<D>::getInstance()};

  std::unordered_set<FlowFunction<D> *> managedFlowFunctions;


    ~MemoryManager(){
// Freeing all Flow Functions that are no singletons
    std::cout << "Cache destructor\n";
    for (auto elem : NormalFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : ReturnFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallToRetFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second) && !managedFlowFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : managedFlowFunctions) {
      if (!registeredFlowFunctionSingletons.count(elem)) {
        delete elem;
      }
    }
    // Freeing all Edge Functions that are no singletons
    for (auto elem : NormalEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : ReturnEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallToRetEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : SummaryEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second) && !managedEdgeFunctions.count(elem.second)) {
        delete elem.second;
      }
    }
    // free additional edge functions
    for (auto elem : managedEdgeFunctions) {
      if (!registeredEdgeFunctionSingletons.count(elem)) {
        delete elem;
      }
    }
    }

    public:

    EdgeFunction<L> *manageEdgeFunction(EdgeFunction<L> *p) {
    managedEdgeFunctions.insert(p);
    return p;
  }

  FlowFunction<D> *manageFlowFunction(FlowFunction<D> *p){
    managedFlowFunctions.insert(p);
    return p;
  }

  void registerAsEdgeFunctionSingleton(std::set<EdgeFunction<L> *> s) {
    registeredEdgeFunctionSingletons.insert(s.begin(), s.end());
  }

  void registerAsFlowFunctionSingleton(std::set<FlowFunction<D> *> s) {
    registeredFlowFunctionSingletons.insert(s.begin(), s.end());
  }

};

} // namespace psr

#endif