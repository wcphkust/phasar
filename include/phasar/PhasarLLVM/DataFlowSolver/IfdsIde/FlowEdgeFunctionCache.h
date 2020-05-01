/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWEDGEFUNCTIONCACHE_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWEDGEFUNCTIONCACHE_H_

#include <map>
#include <set>
#include <tuple>
#include <unordered_set>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/IDETabulationProblem.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/ZeroedFlowFunction.h"
#include "phasar/Utils/Logger.h"
#include "phasar/Utils/PAMMMacros.h"

// All singletons
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/EdgeIdentity.h>

#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/DataFlowSolver/IfdsIde/FlowFunctions/KillAll.h>

namespace psr {

/**
 * This class caches flow and edge functions to avoid their reconstruction.
 * When a flow or edge function must be applied to multiple times, a cached
 * version is used if existend, otherwise a new one is created and inserted
 * into the cache.
 */
template <typename N, typename D, typename F, typename T, typename V,
          typename L, typename I>
class FlowEdgeFunctionCache {
private:
  IDETabulationProblem<N, D, F, T, V, L, I> &problem;
  // Auto add zero
  bool autoAddZero;
  D zeroValue;
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

public:
  // Ctor allows access to the IDEProblem in order to get access to flow and
  // edge function factory functions.
  FlowEdgeFunctionCache(IDETabulationProblem<N, D, F, T, V, L, I> &problem)
      : problem(problem),
        autoAddZero(problem.getIFDSIDESolverConfig().autoAddZero()),
        zeroValue(problem.getZeroValue()) {
    PAMM_GET_INSTANCE;
    REG_COUNTER("Normal-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Normal-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call flow functions
    REG_COUNTER("Call-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Call-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for return flow functions
    REG_COUNTER("Return-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Return-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call to return flow functions
    REG_COUNTER("CallToRet-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("CallToRet-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the summary flow functions
    // REG_COUNTER("Summary-FF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    // REG_COUNTER("Summary-FF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the normal edge functions
    REG_COUNTER("Normal-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Normal-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call edge functions
    REG_COUNTER("Call-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Call-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the return edge functions
    REG_COUNTER("Return-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Return-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the call to return edge functions
    REG_COUNTER("CallToRet-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("CallToRet-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);
    // Counters for the summary edge functions
    REG_COUNTER("Summary-EF Construction", 0, PAMM_SEVERITY_LEVEL::Full);
    REG_COUNTER("Summary-EF Cache Hit", 0, PAMM_SEVERITY_LEVEL::Full);

    // register Singletons of Problem
    registerAsEdgeFunctionSingleton(
        problem.getRegisteredEdgeFunctionSingleton());
    registerAsFlowFunctionSingleton(
        problem.getRegisteredFlowFunctionSingleton());
  }

  ~FlowEdgeFunctionCache() {
    // Freeing all Flow Functions that are no singletons
    for (auto elem : NormalFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : ReturnFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallToRetFlowFunctionCache) {
      if (!registeredFlowFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    // Freeing all Edge Functions that are no singletons
    for (auto elem : NormalEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : ReturnEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : CallToRetEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second)) {
        delete elem.second;
      }
    }
    for (auto elem : SummaryEdgeFunctionCache) {
      if (!registeredEdgeFunctionSingletons.count(elem.second)) {
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

  FlowEdgeFunctionCache(const FlowEdgeFunctionCache &FEFC) = default;

  FlowEdgeFunctionCache(FlowEdgeFunctionCache &&FEFC) = default;

  EdgeFunction<L> *manageEdgeFunction(EdgeFunction<L> *p) {
    managedEdgeFunctions.insert(p);
    return p;
  }

  void registerAsEdgeFunctionSingleton(std::set<EdgeFunction<L> *> s) {
    registeredEdgeFunctionSingletons.insert(s.begin(), s.end());
  }

  void registerAsFlowFunctionSingleton(std::set<FlowFunction<D> *> s) {
    registeredFlowFunctionSingletons.insert(s.begin(), s.end());
  }

  FlowFunction<D> *getNormalFlowFunction(N curr, N succ) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Normal flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Curr Inst : " << problem.NtoString(curr));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Succ Inst : " << problem.NtoString(succ));
    auto key = std::tie(curr, succ);
    if (NormalFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Normal-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return NormalFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero)
                    ? new ZeroedFlowFunction<D>(
                          problem.getNormalFlowFunction(curr, succ), zeroValue)
                    : problem.getNormalFlowFunction(curr, succ);
      NormalFlowFunctionCache.insert(make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunction<D> *getCallFlowFunction(N callStmt, F destFun) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Call flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Dest Fun : " << problem.FtoString(destFun));
    auto key = std::tie(callStmt, destFun);
    if (CallFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Call-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff =
          (autoAddZero)
              ? new ZeroedFlowFunction<D>(
                    problem.getCallFlowFunction(callStmt, destFun), zeroValue)
              : problem.getCallFlowFunction(callStmt, destFun);
      CallFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunction<D> *getRetFlowFunction(N callSite, F calleeFun, N exitStmt,
                                      N retSite) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Return flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Callee    : " << problem.FtoString(calleeFun));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Exit Stmt : " << problem.NtoString(exitStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    auto key = std::tie(callSite, calleeFun, exitStmt, retSite);
    if (ReturnFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("Return-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return ReturnFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff = (autoAddZero) ? new ZeroedFlowFunction<D>(
                                    problem.getRetFlowFunction(
                                        callSite, calleeFun, exitStmt, retSite),
                                    zeroValue)
                              : problem.getRetFlowFunction(callSite, calleeFun,
                                                           exitStmt, retSite);
      ReturnFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunction<D> *getCallToRetFlowFunction(N callSite, N retSite,
                                            std::set<F> callees) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Call-to-Return flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "(F) Callee's  : ");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "  " << problem.FtoString(callee));
    }
    auto key = std::tie(callSite, retSite, callees);
    if (CallToRetFlowFunctionCache.count(key)) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      INC_COUNTER("CallToRet-FF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      return CallToRetFlowFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ff =
          (autoAddZero)
              ? new ZeroedFlowFunction<D>(problem.getCallToRetFlowFunction(
                                              callSite, retSite, callees),
                                          zeroValue)
              : problem.getCallToRetFlowFunction(callSite, retSite, callees);
      CallToRetFlowFunctionCache.insert(std::make_pair(key, ff));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Flow function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ff;
    }
  }

  FlowFunction<D> *getSummaryFlowFunction(N callStmt, F destFun) {
    // PAMM_GET_INSTANCE;
    // INC_COUNTER("Summary-FF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Summary flow function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Dest Mthd : " << problem.FtoString(destFun));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    auto ff = problem.getSummaryFlowFunction(callStmt, destFun);
    return ff;
  }

  EdgeFunction<L> *getNormalEdgeFunction(N curr, D currNode, N succ,
                                         D succNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Normal edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Curr Inst : " << problem.NtoString(curr));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Curr Node : " << problem.DtoString(currNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Succ Inst : " << problem.NtoString(succ));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Succ Node : " << problem.DtoString(succNode));
    auto key = std::tie(curr, currNode, succ, succNode);
    if (NormalEdgeFunctionCache.count(key)) {
      INC_COUNTER("Normal-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return NormalEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Normal-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getNormalEdgeFunction(curr, currNode, succ, succNode);
      NormalEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunction<L> *getCallEdgeFunction(N callStmt, D srcNode,
                                       F destinationFunction, D destNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Call edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Stmt : " << problem.NtoString(callStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Src Node  : " << problem.DtoString(srcNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Dest Fun : "
                  << problem.FtoString(destinationFunction));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Dest Node : " << problem.DtoString(destNode));
    auto key = std::tie(callStmt, srcNode, destinationFunction, destNode);
    if (CallEdgeFunctionCache.count(key)) {
      INC_COUNTER("Call-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return CallEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Call-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallEdgeFunction(callStmt, srcNode,
                                            destinationFunction, destNode);
      CallEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunction<L> *getReturnEdgeFunction(N callSite, F calleeFunction,
                                         N exitStmt, D exitNode, N reSite,
                                         D retNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Return edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(F) Callee    : " << problem.FtoString(calleeFunction));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Exit Stmt : " << problem.NtoString(exitStmt));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Exit Node : " << problem.DtoString(exitNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(reSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retNode));
    auto key =
        std::tie(callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
    if (ReturnEdgeFunctionCache.count(key)) {
      INC_COUNTER("Return-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ReturnEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Return-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getReturnEdgeFunction(
          callSite, calleeFunction, exitStmt, exitNode, reSite, retNode);
      ReturnEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunction<L> *getCallToRetEdgeFunction(N callSite, D callNode, N retSite,
                                            D retSiteNode,
                                            std::set<F> callees) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Call-to-Return edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Call Node : " << problem.DtoString(callNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retSiteNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << "(F) Callee's  : ");
    for (auto callee : callees) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "  " << problem.FtoString(callee));
    }
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (CallToRetEdgeFunctionCache.count(key)) {
      INC_COUNTER("CallToRet-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return CallToRetEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("CallToRet-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getCallToRetEdgeFunction(callSite, callNode, retSite,
                                                 retSiteNode, callees);
      CallToRetEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  EdgeFunction<L> *getSummaryEdgeFunction(N callSite, D callNode, N retSite,
                                          D retSiteNode) {
    PAMM_GET_INSTANCE;
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "Summary edge function factory call");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Call Site : " << problem.NtoString(callSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Call Node : " << problem.DtoString(callNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(N) Ret Site  : " << problem.NtoString(retSite));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                  << "(D) Ret Node  : " << problem.DtoString(retSiteNode));
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
    auto key = std::tie(callSite, callNode, retSite, retSiteNode);
    if (SummaryEdgeFunctionCache.count(key)) {
      INC_COUNTER("Summary-EF Cache Hit", 1, PAMM_SEVERITY_LEVEL::Full);
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function fetched from cache");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return SummaryEdgeFunctionCache.at(key);
    } else {
      INC_COUNTER("Summary-EF Construction", 1, PAMM_SEVERITY_LEVEL::Full);
      auto ef = problem.getSummaryEdgeFunction(callSite, callNode, retSite,
                                               retSiteNode);
      SummaryEdgeFunctionCache.insert(std::make_pair(key, ef));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG)
                    << "Edge function constructed");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), DEBUG) << ' ');
      return ef;
    }
  }

  void print() {
    if constexpr (PAMM_CURR_SEV_LEVEL >= PAMM_SEVERITY_LEVEL::Full) {
      PAMM_GET_INSTANCE;
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "=== Flow-Edge-Function Cache Statistics ===");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal-flow function cache hits: "
                    << GET_COUNTER("Normal-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal-flow function constructions: "
                    << GET_COUNTER("Normal-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-flow function cache hits: "
                    << GET_COUNTER("Call-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-flow function constructions: "
                    << GET_COUNTER("Call-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return-flow function cache hits: "
                    << GET_COUNTER("Return-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return-flow function constructions: "
                    << GET_COUNTER("Return-FF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return-flow function cache hits: "
                    << GET_COUNTER("CallToRet-FF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return-flow function constructions: "
                    << GET_COUNTER("CallToRet-FF Construction"));
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << "Summary-flow function
      // cache hits: "
      //                        << GET_COUNTER("Summary-FF Cache Hit"));
      // LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << "Summary-flow function
      // constructions: "
      //                         << GET_COUNTER("Summary-FF Construction"));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total flow function cache hits: "
          << GET_SUM_COUNT({"Normal-FF Cache Hit", "Call-FF Cache Hit",
                            "Return-FF Cache Hit", "CallToRet-FF Cache Hit"}));
      //"Summary-FF Cache Hit"});
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total flow function constructions: "
          << GET_SUM_COUNT({"Normal-FF Construction", "Call-FF Construction",
                            "Return-FF Construction",
                            "CallToRet-FF Construction" /*,
                "Summary-FF Construction"*/}));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO) << ' ');
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal edge function cache hits: "
                    << GET_COUNTER("Normal-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Normal edge function constructions: "
                    << GET_COUNTER("Normal-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call edge function cache hits: "
                    << GET_COUNTER("Call-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call edge function constructions: "
                    << GET_COUNTER("Call-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return edge function cache hits: "
                    << GET_COUNTER("Return-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Return edge function constructions: "
                    << GET_COUNTER("Return-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return edge function cache hits: "
                    << GET_COUNTER("CallToRet-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Call-to-Return edge function constructions: "
                    << GET_COUNTER("CallToRet-EF Construction"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Summary edge function cache hits: "
                    << GET_COUNTER("Summary-EF Cache Hit"));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "Summary edge function constructions: "
                    << GET_COUNTER("Summary-EF Construction"));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total edge function cache hits: "
          << GET_SUM_COUNT({"Normal-EF Cache Hit", "Call-EF Cache Hit",
                            "Return-EF Cache Hit", "CallToRet-EF Cache Hit",
                            "Summary-EF Cache Hit"}));
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Total edge function constructions: "
          << GET_SUM_COUNT({"Normal-EF Construction", "Call-EF Construction",
                            "Return-EF Construction",
                            "CallToRet-EF Construction",
                            "Summary-EF Construction"}));
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg::get(), INFO)
                    << "----------------------------------------------");
    } else {
      LOG_IF_ENABLE(
          BOOST_LOG_SEV(lg::get(), INFO)
          << "Cache statistics only recorded on PAMM severity level: Full.");
    }
  }
};

} // namespace psr

#endif
