/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <algorithm>
#include <iostream>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFact.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFactWrapper.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctionComposer.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFactWrapper.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenIf.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillMultiple.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCallee.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/MapFactsToCaller.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMFlowFunctions/PropagateLoad.h>
#include <phasar/PhasarLLVM/IfdsIde/LLVMZeroValue.h>
#include <phasar/Utils/LLVMIRToSrc.h>
#include <phasar/Utils/LLVMShorthands.h>
#include <phasar/Utils/Logger.h>

#include <llvm/IR/CallSite.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

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
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IDETypeStateAnalysis::getNormalFlowFunction()");
  // Check if Alloca's type matches the target type. If so, generate from zero
  // value.
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (hasMatchingType(Alloca)) {
      return make_shared<Gen<const FlowFact *>>(
          new FlowFactWrapper<const llvm::Value *>(Alloca), zeroValue());
    }
  }
  // Check load instructions for target type. Generate from the loaded value and
  // kill the load instruction if it was generated previously (strong update!).
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (hasMatchingType(Load)) {
      struct TSFlowFunction : FlowFunction<const FlowFact *> {
        const llvm::LoadInst *Load;

        TSFlowFunction(const llvm::LoadInst *L) : Load(L) {}
        ~TSFlowFunction() override = default;
        set<const FlowFact *> computeTargets(const FlowFact *source) override {
          const FlowFactWrapper<const llvm::Value *> *wrapper =
              static_cast<const FlowFactWrapper<const llvm::Value *> *>(source);
          if (wrapper->get() == Load) {
            return {};
          }
          if (wrapper->get() == Load->getPointerOperand()) {
            return {source, new FlowFactWrapper<const llvm::Value *>(Load)};
          }
          return {source};
        }
      };
      return make_shared<TSFlowFunction>(Load);
    }
  }
  // Check store instructions for target type. Perform a strong update, i.e.
  // kill the alloca pointed to by the pointer-operand and all alloca's related
  // to the value-operand and then generate them from the value-operand.
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    if (hasMatchingType(Store)) {
      auto RelevantAliasesAndAllocas = getLocalAliasesAndAllocas(
          Store->getValueOperand(),
          curr->getParent()->getParent()->getName().str());

      struct TSFlowFunction : FlowFunction<const FlowFact *> {
        const llvm::StoreInst *Store;
        std::set<const FlowFact *> AliasesAndAllocas;
        TSFlowFunction(const llvm::StoreInst *S, std::set<const FlowFact *> AA)
            : Store(S), AliasesAndAllocas(AA) {}
        ~TSFlowFunction() override = default;
        set<const FlowFact *> computeTargets(const FlowFact *source) override {
          // We kill all relevant loacal aliases and alloca's
          const FlowFactWrapper<const llvm::Value *> *wrapper =
              static_cast<const FlowFactWrapper<const llvm::Value *> *>(source);
          if (wrapper->get() != Store->getValueOperand() &&
              AliasesAndAllocas.find(source) != AliasesAndAllocas.end()) {
            return {};
          }
          // Generate all local aliases and relevant alloca's from the stored
          // value
          if (wrapper->get() == Store->getValueOperand()) {
            AliasesAndAllocas.insert(source);
            return AliasesAndAllocas;
          }
          return {source};
        }
      };
      return make_shared<TSFlowFunction>(Store, RelevantAliasesAndAllocas);
    }
  }
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

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::getNormalEdgeFunction(
    const llvm::Instruction *curr, const FlowFact *currNode,
    const llvm::Instruction *succ, const FlowFact *succNode) {
  auto &lg = lg::get();
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "IDETabulationProblemTestPlugin::getNormalEdgeFunction()");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(N) Curr Inst : "
                << IDETabulationProblemTestPlugin::NtoString(curr));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(D) Curr Node :   "
                << IDETabulationProblemTestPlugin::DtoString(currNode));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(N) Succ Inst : "
                << IDETabulationProblemTestPlugin::NtoString(succ));
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "(D) Succ Node :   "
                << IDETabulationProblemTestPlugin::DtoString(succNode));
  // Set alloca instructions of target type to uninitialized.
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(curr)) {
    if (hasMatchingType(Alloca)) {
      const FlowFactWrapper<const llvm::Value *> *wrapper =
          static_cast<const FlowFactWrapper<const llvm::Value *> *>(succNode);
      if (currNode == zeroValue() && wrapper->get() == Alloca) {
        struct TSAllocaEF : public TSEdgeFunction {
          TSAllocaEF(const TypeStateDescription &tsd, const std::string &tok)
              : TSEdgeFunction(tsd, tok) {}

          const EdgeFact *computeTarget(const EdgeFact *source) override {
            CurrentState = TSD.uninit();
            return new EdgeFactWrapper<int>(CurrentState);
          }

          void print(std::ostream &OS, bool isForDebug = false) const override {
            OS << "TSAllocaEF(" << TSD.stateToString(CurrentState) << ")";
          }
        };
        return make_shared<TSAllocaEF>(TSD, "alloca instruction");
      }
    }
  }
  return EdgeIdentity<const EdgeFact *>::getInstance();
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

bool IDETabulationProblemTestPlugin::hasMatchingType(const llvm::Value *V) {
  // General case
  if (V->getType()->isPointerTy()) {
    if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
            V->getType()->getPointerElementType())) {
      if (StructTy->getName().find(TSD.getTypeNameOfInterest()) !=
          llvm::StringRef::npos) {
        return true;
      }
    }
  }
  if (auto Alloca = llvm::dyn_cast<llvm::AllocaInst>(V)) {
    if (Alloca->getAllocatedType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Alloca->getAllocatedType()->getPointerElementType())) {
        if (StructTy->getName().find(TSD.getTypeNameOfInterest()) !=
            llvm::StringRef::npos) {
          return true;
        }
      }
    }
    return false;
  }
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(V)) {
    if (Load->getPointerOperand()
            ->getType()
            ->getPointerElementType()
            ->isPointerTy()) {
      if (auto StructTy =
              llvm::dyn_cast<llvm::StructType>(Load->getPointerOperand()
                                                   ->getType()
                                                   ->getPointerElementType()
                                                   ->getPointerElementType())) {
        if (StructTy->getName().find(TSD.getTypeNameOfInterest()) !=
            llvm::StringRef::npos) {
          return true;
        }
      }
    }
    return false;
  }
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(V)) {
    if (Store->getValueOperand()->getType()->isPointerTy()) {
      if (auto StructTy = llvm::dyn_cast<llvm::StructType>(
              Store->getValueOperand()->getType()->getPointerElementType())) {
        if (StructTy->getName().find(TSD.getTypeNameOfInterest()) !=
            llvm::StringRef::npos) {
          return true;
        }
      }
    }
    return false;
  }
  return false;
}

std::set<const llvm::Value *>
IDETabulationProblemTestPlugin::getLocalAliasesAndAllocas(
    const llvm::Value *V, const std::string &Fname) {
  std::set<const llvm::Value *> PointsToAndAllocas;
  std::set<const llvm::Value *> RelevantAllocas = getRelevantAllocas(V);
  std::set<const llvm::Value *> Aliases =
      irdb.getPointsToGraph(Fname)->getPointsToSet(V);
  for (auto Alias : Aliases) {
    if (hasMatchingType(Alias))
      PointsToAndAllocas.insert(Alias);
  }
  // PointsToAndAllocas.insert(Aliases.begin(), Aliases.end());
  PointsToAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return PointsToAndAllocas;
}

std::set<const llvm::Value *>
IDETabulationProblemTestPlugin::getRelevantAllocas(const llvm::Value *V) {
  if (RelevantAllocaCache.find(V) != RelevantAllocaCache.end()) {
    return RelevantAllocaCache[V];
  } else {
    auto PointsToSet = getWMPointsToSet(V);
    std::set<const llvm::Value *> RelevantAllocas;
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                  << "Compute relevant alloca's of "
                  << IDETabulationProblemTestPlugin::DtoString(V));
    for (auto Alias : PointsToSet) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Alias: "
                    << IDETabulationProblemTestPlugin::DtoString(Alias));
      // Collect the pointer operand of a aliased load instruciton
      if (auto Load = llvm::dyn_cast<llvm::LoadInst>(Alias)) {
        if (hasMatchingType(Alias)) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << " -> Alloca: "
                        << IDETabulationProblemTestPlugin::DtoString(
                               Load->getPointerOperand()));
          RelevantAllocas.insert(Load->getPointerOperand());
        }
      } else {
        // For all other types of aliases, e.g. callsites, function arguments,
        // we check store instructions where thoses aliases are value operands.
        for (auto User : Alias->users()) {
          LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                        << "  User: "
                        << IDETabulationProblemTestPlugin::DtoString(User));
          if (auto Store = llvm::dyn_cast<llvm::StoreInst>(User)) {
            if (hasMatchingType(Store)) {
              LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                            << "    -> Alloca: "
                            << IDETabulationProblemTestPlugin::DtoString(
                                   Store->getPointerOperand()));
              RelevantAllocas.insert(Store->getPointerOperand());
            }
          }
        }
      }
    }
    for (auto Alias : PointsToSet) {
      RelevantAllocaCache[Alias] = RelevantAllocas;
    }
    return RelevantAllocas;
  }
}

std::set<const llvm::Value *>
IDETabulationProblemTestPlugin::getWMPointsToSet(const llvm::Value *V) {
  if (PointsToCache.find(V) != PointsToCache.end()) {
    return PointsToCache[V];
  } else {
    auto PointsToSet = icfg.getWholeModulePTG().getPointsToSet(V);
    for (auto Alias : PointsToSet) {
      if (hasMatchingType(Alias))
        PointsToCache[Alias] = PointsToSet;
    }
    return PointsToSet;
  }
}

std::set<const llvm::Value *>
IDETabulationProblemTestPlugin::getWMAliasesAndAllocas(const llvm::Value *V) {
  std::set<const llvm::Value *> PointsToAndAllocas;
  std::set<const llvm::Value *> RelevantAllocas = getRelevantAllocas(V);
  std::set<const llvm::Value *> Aliases = getWMPointsToSet(V);
  PointsToAndAllocas.insert(Aliases.begin(), Aliases.end());
  PointsToAndAllocas.insert(RelevantAllocas.begin(), RelevantAllocas.end());
  return PointsToAndAllocas;
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::TSEdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<const EdgeFact *> *>(otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<const EdgeFact *>>(botElement);
}

const EdgeFact *IDETabulationProblemTestPlugin::TSEdgeFunction::computeTarget(
    const EdgeFact *source) {
  auto &lg = lg::get();
  const EdgeFactWrapper<int> *wrapper =
      static_cast<const EdgeFactWrapper<int> *>(source);
  CurrentState = TSD.getNextState(Token, wrapper->get());
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                << "State machine transition: (" << Token << " , "
                << TSD.stateToString(wrapper->get()) << ") -> "
                << TSD.stateToString(CurrentState));
  return new EdgeFactWrapper<int>(CurrentState);
}

std::shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::TSEdgeFunction::composeWith(
    std::shared_ptr<EdgeFunction<const EdgeFact *>> secondFunction) {
  if (auto *EI = dynamic_cast<EdgeIdentity<const EdgeFact *> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  // TODO: Can we reduce the EF if composed with AllTop?
  return make_shared<TSEdgeFunctionComposer>(this->shared_from_this(),
                                             secondFunction, TSD.bottom());
}

std::shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::TSEdgeFunction::joinWith(
    std::shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<const EdgeFact *> *>(otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<const EdgeFact *>>(TSD.bottom());
}

bool IDETabulationProblemTestPlugin::TSEdgeFunction::equal_to(
    std::shared_ptr<EdgeFunction<const EdgeFact *>> other) const {
  if (auto *TSEF =
          dynamic_cast<IDETabulationProblemTestPlugin::TSEdgeFunction *>(
              other.get())) {
    return this->CurrentState == TSEF->CurrentState;
  }
  return this == other.get();
}

void IDETabulationProblemTestPlugin::TSEdgeFunction::print(
    ostream &OS, bool isForDebug) const {
  OS << "TSEdgeFunc(" << TSD.stateToString(CurrentState) << ")";
}

} // namespace psr
