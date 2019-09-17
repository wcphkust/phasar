/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <limits>
#include <utility>

#include <phasar/PhasarLLVM/ControlFlow/LLVMBasedICFG.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFact.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFactWrapper.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/AllBottom.h>
#include <phasar/PhasarLLVM/IfdsIde/EdgeFunctions/EdgeIdentity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFactWrapper.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunction.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Gen.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenAll.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/GenIf.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Identity.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/Kill.h>
#include <phasar/PhasarLLVM/IfdsIde/FlowFunctions/KillAll.h>
#include <phasar/Utils/LLVMIRToSrc.h>
#include <phasar/Utils/Logger.h>

#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

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

const int64_t IDETabulationProblemTestPlugin::TOP =
    numeric_limits<int64_t>::min();

const int64_t IDETabulationProblemTestPlugin::BOTTOM =
    numeric_limits<int64_t>::max();

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
                << "IDELinearConstantAnalysis::getNormalFlowFunction()");
  // Check store instructions. Store instructions override previous value
  // of their pointer operand, i.e. kills previous fact (= pointer operand).
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    const llvm::Value *PointerOp = Store->getPointerOperand();
    const llvm::Value *ValueOp = Store->getValueOperand();
    // Case I: Storing a constant integer.
    if (llvm::isa<llvm::ConstantInt>(ValueOp)) {
      struct LCAFF : FlowFunction<const FlowFact *> {
        const llvm::Value *PointerOp;
        const FlowFact *ZeroValue;
        LCAFF(const llvm::Value *PointerOperand, const FlowFact *ZeroValue)
            : PointerOp(PointerOperand), ZeroValue(ZeroValue) {}
        set<const FlowFact *> computeTargets(const FlowFact *source) override {
          auto wrapper =
              static_cast<const FlowFactWrapper<const llvm::Value *> *>(source);
          if (wrapper->get() == PointerOp) {
            return {};
          } else if (source == ZeroValue) {
            return {source,
                    new FlowFactWrapper<const llvm::Value *>(PointerOp)};
          } else {
            return {source};
          }
        }
      };
      return make_shared<LCAFF>(PointerOp, zeroValue());
    }
    // Case II: Storing an integer typed value.
    if (ValueOp->getType()->isIntegerTy()) {
      struct LCAFF : FlowFunction<const FlowFact *> {
        const llvm::Value *PointerOp, *ValueOp;
        LCAFF(const llvm::Value *PointerOperand,
              const llvm::Value *ValueOperand)
            : PointerOp(PointerOperand), ValueOp(ValueOperand) {}
        set<const FlowFact *> computeTargets(const FlowFact *source) override {
          auto wrapper =
              static_cast<const FlowFactWrapper<const llvm::Value *> *>(source);
          if (wrapper->get() == PointerOp) {
            return {};
          } else if (wrapper->get() == ValueOp) {
            return {source,
                    new FlowFactWrapper<const llvm::Value *>(PointerOp)};
          } else {
            return {source};
          }
        }
      };
      return make_shared<LCAFF>(PointerOp, ValueOp);
    }
  }
  // check load instructions
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    // only consider i32 load
    if (Load->getPointerOperandType()->getPointerElementType()->isIntegerTy()) {
      return make_shared<GenIf<const FlowFact *>>(
          new FlowFactWrapper<const llvm::Value *>(Load), zeroValue(),
          [Load](const FlowFact *source) {
            auto wrapper =
                static_cast<const FlowFactWrapper<const llvm::Value *> *>(
                    source);
            return wrapper->get() == Load->getPointerOperand();
          });
    }
  }
  // check for binary operations: add, sub, mul, udiv/sdiv, urem/srem
  if (llvm::isa<llvm::BinaryOperator>(curr)) {
    auto lop = curr->getOperand(0);
    auto rop = curr->getOperand(1);
    return make_shared<GenIf<const FlowFact *>>(
        new FlowFactWrapper<const llvm::Value *>(curr), zeroValue(),
        [this, lop, rop](const FlowFact *source) {
          auto wrapper =
              static_cast<const FlowFactWrapper<const llvm::Value *> *>(source);
          return (source != zerovalue &&
                  ((lop == wrapper->get() &&
                    llvm::isa<llvm::ConstantInt>(rop)) ||
                   (rop == wrapper->get() &&
                    llvm::isa<llvm::ConstantInt>(lop)))) ||
                 (source == zerovalue && llvm::isa<llvm::ConstantInt>(lop) &&
                  llvm::isa<llvm::ConstantInt>(rop));
        });
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
  auto wrapper =
      static_cast<const FlowFactWrapper<const llvm::Value *> *>(succNode);
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
  // All_Bottom for zero value
  if (isZeroValue(currNode) && isZeroValue(succNode)) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Zero value.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    return make_shared<AllBottom<const EdgeFact *>>(
        new EdgeFactWrapper<int64_t>(IDETabulationProblemTestPlugin::BOTTOM));
  }
  // Check store instruction
  if (auto Store = llvm::dyn_cast<llvm::StoreInst>(curr)) {
    const llvm::Value *pointerOperand = Store->getPointerOperand();
    const llvm::Value *valueOperand = Store->getValueOperand();
    if (pointerOperand == wrapper->get()) {
      // Case I: Storing a constant integer.
      if (isZeroValue(currNode) && llvm::isa<llvm::ConstantInt>(valueOperand)) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Case: Storing constant integer.");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        auto CI = llvm::dyn_cast<llvm::ConstantInt>(valueOperand);
        auto IntConst = CI->getSExtValue();
        return make_shared<IDETabulationProblemTestPlugin::GenConstant>(
            IntConst);
      }
      // Case II: Storing an integer typed value.
      if (currNode != succNode && valueOperand->getType()->isIntegerTy()) {
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Case: Storing an integer typed value.");
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        return make_shared<IDETabulationProblemTestPlugin::LCAIdentity>();
      }
    }
  }

  // Check load instruction
  if (auto Load = llvm::dyn_cast<llvm::LoadInst>(curr)) {
    if (Load == wrapper->get()) {
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                    << "Case: Loading an integer typed value.");
      LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
      return make_shared<IDETabulationProblemTestPlugin::LCAIdentity>();
    }
  }
  // Check for binary operations add, sub, mul, udiv/sdiv and urem/srem
  if (curr == wrapper->get() && llvm::isa<llvm::BinaryOperator>(curr)) {
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Binary operation.");
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
    unsigned OP = curr->getOpcode();
    auto lop = curr->getOperand(0);
    auto rop = curr->getOperand(1);
    struct LCAEF : EdgeFunction<const EdgeFact *>,
                   enable_shared_from_this<LCAEF> {
      const unsigned EdgeFunctionID, Op;
      const llvm::Value *lop, *rop, *currNode;
      LCAEF(const unsigned Op, const llvm::Value *lop, const llvm::Value *rop,
            const llvm::Value *currNode)
          : EdgeFunctionID(++IDETabulationProblemTestPlugin::CurrBinary_Id),
            Op(Op), lop(lop), rop(rop), currNode(currNode) {}

      const EdgeFact *computeTarget(const EdgeFact *source) override {
        auto wrapper = static_cast<const EdgeFactWrapper<int64_t> *>(source);
        auto &lg = lg::get();
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Left Op   : " << llvmIRToString(lop));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Right Op  : " << llvmIRToString(rop));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG)
                      << "Curr Node : " << llvmIRToString(currNode));
        LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
        if (lop == currNode && llvm::isa<llvm::ConstantInt>(rop)) {
          auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
          int64_t i = wrapper->get();
          return IDETabulationProblemTestPlugin::executeBinOperation(
              Op, i, ric->getSExtValue());
        } else if (rop == currNode && llvm::isa<llvm::ConstantInt>(lop)) {
          auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
          int64_t i = wrapper->get();
          return IDETabulationProblemTestPlugin::executeBinOperation(
              Op, lic->getSExtValue(), i);
        } else if (LLVMZeroValue::getInstance()->isLLVMZeroValue(currNode) &&
                   llvm::isa<llvm::ConstantInt>(lop) &&
                   llvm::isa<llvm::ConstantInt>(rop)) {
          auto lic = llvm::dyn_cast<llvm::ConstantInt>(lop);
          auto ric = llvm::dyn_cast<llvm::ConstantInt>(rop);
          return IDETabulationProblemTestPlugin::executeBinOperation(
              Op, lic->getSExtValue(), ric->getSExtValue());
        }
        throw runtime_error(
            "Only linear constant propagation can be specified!");
      }

      shared_ptr<EdgeFunction<const EdgeFact *>> composeWith(
          shared_ptr<EdgeFunction<const EdgeFact *>> secondFunction) override {
        if (auto *EI = dynamic_cast<EdgeIdentity<const EdgeFact *> *>(
                secondFunction.get())) {
          return this->shared_from_this();
        }
        if (auto *LSVI = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
          return this->shared_from_this();
        }
        return make_shared<
            IDETabulationProblemTestPlugin::LCAEdgeFunctionComposer>(
            this->shared_from_this(), secondFunction);
      }

      shared_ptr<EdgeFunction<const EdgeFact *>> joinWith(
          shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) override {
        if (otherFunction.get() == this ||
            otherFunction->equal_to(this->shared_from_this())) {
          return this->shared_from_this();
        }
        if (auto *AT =
                dynamic_cast<AllTop<const EdgeFact *> *>(otherFunction.get())) {
          return this->shared_from_this();
        }
        return make_shared<AllBottom<const EdgeFact *>>(
            new EdgeFactWrapper<int64_t>(
                IDETabulationProblemTestPlugin::BOTTOM));
      }

      bool equal_to(
          shared_ptr<EdgeFunction<const EdgeFact *>> other) const override {
        // could be more precise - compare op,lop and rop?
        return this == other.get();
      }

      void print(ostream &OS, bool isForDebug = false) const override {
        OS << "Binary_" << EdgeFunctionID;
      }
    };
    auto wrapper =
        static_cast<const FlowFactWrapper<const llvm::Value *> *>(currNode);
    return make_shared<LCAEF>(OP, lop, rop, wrapper->get());
  }
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Case: Edge identity.");
  LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << ' ');
  return EdgeIdentity<const EdgeFact *>::getInstance();
}

std::shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::getCallEdgeFunction(
    const llvm::Instruction *callStmt, const FlowFact *srcNode,
    const llvm::Function *destinationMethod, const FlowFact *destNode) {
  return EdgeIdentity<const EdgeFact *>::getInstance();
}

std::shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::getReturnEdgeFunction(
    const llvm::Instruction *callSite, const llvm::Function *calleeMethod,
    const llvm::Instruction *exitStmt, const FlowFact *exitNode,
    const llvm::Instruction *reSite, const FlowFact *retNode) {
  return EdgeIdentity<const EdgeFact *>::getInstance();
}

std::shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::getCallToRetEdgeFunction(
    const llvm::Instruction *callSite, const FlowFact *callNode,
    const llvm::Instruction *retSite, const FlowFact *retSiteNode,
    std::set<const llvm::Function *> callees) {
  return EdgeIdentity<const EdgeFact *>::getInstance();
}

std::shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::getSummaryEdgeFunction(
    const llvm::Instruction *curr, const FlowFact *currNode,
    const llvm::Instruction *succ, const FlowFact *succNode) {
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
  // Check commandline arguments, e.g. argc, and generate all integer
  // typed arguments.
  map<const llvm::Instruction *, set<const FlowFact *>> SeedMap;
  for (auto &EntryPoint : EntryPoints) {
    if (EntryPoint == "main") {
      set<const FlowFact *> CmdArgs;
      for (auto &Arg : icfg.getMethod(EntryPoint)->args()) {
        if (Arg.getType()->isIntegerTy()) {
          CmdArgs.insert(new FlowFactWrapper<const llvm::Value *>(&Arg));
        }
      }
      CmdArgs.insert(zeroValue());
      SeedMap.insert(
          make_pair(&icfg.getMethod(EntryPoint)->front().front(), CmdArgs));
    } else {
      SeedMap.insert(make_pair(&icfg.getMethod(EntryPoint)->front().front(),
                               set<const FlowFact *>({zeroValue()})));
    }
  }
  return SeedMap;
}

const EdgeFact *IDETabulationProblemTestPlugin::topElement() {
  return new EdgeFactWrapper<int64_t>(TOP);
}

const EdgeFact *IDETabulationProblemTestPlugin::bottomElement() {
  return new EdgeFactWrapper<int64_t>(BOTTOM);
}

const EdgeFact *IDETabulationProblemTestPlugin::join(const EdgeFact *lhs,
                                                     const EdgeFact *rhs) {
  auto wrapperlhs = static_cast<const EdgeFactWrapper<int64_t> *>(lhs);
  auto wrapperrhs = static_cast<const EdgeFactWrapper<int64_t> *>(rhs);
  if (wrapperlhs->get() == TOP && wrapperrhs->get() != BOTTOM) {
    return rhs;
  } else if (wrapperrhs->get() == TOP && wrapperlhs->get() != BOTTOM) {
    return lhs;
  } else if (rhs == lhs) {
    return rhs;
  } else {
    return new EdgeFactWrapper<int64_t>(BOTTOM);
  }
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::allTopFunction() {
  return make_shared<AllTop<const EdgeFact *>>(
      new EdgeFactWrapper<int64_t>(TOP));
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::LCAEdgeFunctionComposer::composeWith(
    shared_ptr<EdgeFunction<const EdgeFact *>> secondFunction) {
  if (auto *EI = dynamic_cast<EdgeIdentity<const EdgeFact *> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
    return this->shared_from_this();
  }
  return F->composeWith(G->composeWith(secondFunction));
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::LCAEdgeFunctionComposer::joinWith(
    shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<const EdgeFact *> *>(otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<const EdgeFact *>>(
      new EdgeFactWrapper<int64_t>(IDETabulationProblemTestPlugin::BOTTOM));
}

IDETabulationProblemTestPlugin::GenConstant::GenConstant(int64_t IntConst)
    : GenConstant_Id(++IDETabulationProblemTestPlugin::CurrGenConstant_Id),
      IntConst(IntConst) {}

const EdgeFact *IDETabulationProblemTestPlugin::GenConstant::computeTarget(
    const EdgeFact *source) {
  return new EdgeFactWrapper<int64_t>(IntConst);
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::GenConstant::composeWith(
    shared_ptr<EdgeFunction<const EdgeFact *>> secondFunction) {
  if (auto *EI = dynamic_cast<EdgeIdentity<const EdgeFact *> *>(
          secondFunction.get())) {
    return this->shared_from_this();
  }
  if (auto *LSVI = dynamic_cast<LCAIdentity *>(secondFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<IDETabulationProblemTestPlugin::LCAEdgeFunctionComposer>(
      this->shared_from_this(), secondFunction);
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::GenConstant::joinWith(
    shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<const EdgeFact *>>(
      new EdgeFactWrapper<int64_t>(IDETabulationProblemTestPlugin::BOTTOM));
}

bool IDETabulationProblemTestPlugin::GenConstant::equal_to(
    shared_ptr<EdgeFunction<const EdgeFact *>> other) const {
  if (auto *StC = dynamic_cast<IDETabulationProblemTestPlugin::GenConstant *>(
          other.get())) {
    return (StC->IntConst == this->IntConst);
  }
  return this == other.get();
}

void IDETabulationProblemTestPlugin::GenConstant::print(ostream &OS,
                                                        bool isForDebug) const {
  OS << "GenConstant_" << GenConstant_Id;
}

IDETabulationProblemTestPlugin::LCAIdentity::LCAIdentity()
    : LCAID_Id(++IDETabulationProblemTestPlugin::CurrLCAID_Id) {}

const EdgeFact *IDETabulationProblemTestPlugin::LCAIdentity::computeTarget(
    const EdgeFact *source) {
  return source;
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::LCAIdentity::composeWith(
    shared_ptr<EdgeFunction<const EdgeFact *>> secondFunction) {
  return secondFunction;
}

shared_ptr<EdgeFunction<const EdgeFact *>>
IDETabulationProblemTestPlugin::LCAIdentity::joinWith(
    shared_ptr<EdgeFunction<const EdgeFact *>> otherFunction) {
  if (otherFunction.get() == this ||
      otherFunction->equal_to(this->shared_from_this())) {
    return this->shared_from_this();
  }
  if (auto *AT =
          dynamic_cast<AllTop<const EdgeFact *> *>(otherFunction.get())) {
    return this->shared_from_this();
  }
  return make_shared<AllBottom<const EdgeFact *>>(
      new EdgeFactWrapper<int64_t>(IDETabulationProblemTestPlugin::BOTTOM));
}

bool IDETabulationProblemTestPlugin::LCAIdentity::equal_to(
    shared_ptr<EdgeFunction<const EdgeFact *>> other) const {
  return this == other.get();
}

void IDETabulationProblemTestPlugin::LCAIdentity::print(ostream &OS,
                                                        bool isForDebug) const {
  OS << "LCAIdentity_" << LCAID_Id;
}

const EdgeFact *IDETabulationProblemTestPlugin::executeBinOperation(
    const unsigned op, const int64_t lop, const int64_t rop) {
  // default initialize with BOTTOM (all information)
  int64_t res = numeric_limits<int64_t>::max();
  switch (op) {
  case llvm::Instruction::Add:
    res = lop + rop;
    break;

  case llvm::Instruction::Sub:
    res = lop - rop;
    break;

  case llvm::Instruction::Mul:
    res = lop * rop;
    break;

  case llvm::Instruction::UDiv:
  case llvm::Instruction::SDiv:
    res = lop / rop;
    break;

  case llvm::Instruction::URem:
  case llvm::Instruction::SRem:
    res = lop % rop;
    break;

  default:
    auto &lg = lg::get();
    LOG_IF_ENABLE(BOOST_LOG_SEV(lg, DEBUG) << "Operation not supported by "
                                              "IDELinearConstantAnalysis::"
                                              "executeBinOperation()");
    break;
  }
  return new EdgeFactWrapper<int64_t>(res);
}

} // namespace psr
