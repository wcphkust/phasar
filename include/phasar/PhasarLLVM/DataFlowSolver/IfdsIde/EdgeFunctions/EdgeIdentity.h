/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * EdgeIdentity.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_EDGEIDENTITY_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_EDGEIDENTITY_H_

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllTop.h"
#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunctions/AllBottom.h"
#include <iostream>
#include <memory>
#include <string>



namespace psr {

template <typename L> class EdgeFunction;

template <typename L> class EdgeIdentity : public EdgeFunction<L> {
private:
  EdgeIdentity() = default;

public:
  EdgeIdentity(const EdgeIdentity &ei) = delete;

  EdgeIdentity &operator=(const EdgeIdentity &ei) = delete;

  ~EdgeIdentity() override = default;

  L computeTarget(L source) override { return source; }

  EdgeFunction<L> *composeWith(EdgeFunction<L> *secondFunction) override {
    return secondFunction;
  }

  EdgeFunction<L> *joinWith(EdgeFunction<L> *otherFunction) override {
    if ((otherFunction == this) || otherFunction->equal_to(this))
      return this;
    if (AllBottom<L> *ab = dynamic_cast<AllBottom<L> *>(otherFunction))
      return otherFunction;
    if (AllTop<L> *at = dynamic_cast<AllTop<L> *>(otherFunction))
      return this;
    // do not know how to join; hence ask other function to decide on this
    return otherFunction->joinWith(this);
  }

  bool equal_to(EdgeFunction<L> *other) const override { return this == other; }

  static EdgeIdentity<L> *getInstance() {
    // implement singleton C++11 thread-safe (see Scott Meyers)
    static EdgeIdentity<L> *instance = new EdgeIdentity<L>();
    return instance;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "EdgeIdentity";
  }
};

} // namespace psr

#endif
