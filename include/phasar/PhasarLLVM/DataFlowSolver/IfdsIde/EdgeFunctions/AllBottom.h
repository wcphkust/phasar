/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

/*
 * AllBottom.h
 *
 *  Created on: 04.08.2016
 *      Author: pdschbrt
 */

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_ALLBOTTOM_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFUNCTIONS_ALLBOTTOM_H_

#include <iostream> // std::cerr
#include <ostream>
#include <string>

#include "phasar/PhasarLLVM/DataFlowSolver/IfdsIde/EdgeFunction.h"

namespace psr {

template <typename L> class EdgeIdentity;
template <typename L> class AllTop;

template <typename L> class AllBottom : public EdgeFunction<L> {
private:
  const L bottomElement;

public:
  AllBottom(L bottomElement) : bottomElement(bottomElement) {}

  ~AllBottom() override = default;

  L computeTarget(L source) override { return bottomElement; }

  EdgeFunction<L> *composeWith(EdgeFunction<L> *secondFunction) override {
    if (AllBottom<L> *ab = dynamic_cast<AllBottom<L> *>(secondFunction)) {
      return this;
    }
    if (EdgeIdentity<L> *ei = dynamic_cast<EdgeIdentity<L> *>(secondFunction)) {
      return this;
    }
    return secondFunction->composeWith(this);
  }

  EdgeFunction<L> *joinWith(EdgeFunction<L> *otherFunction) override {
    if (otherFunction == this || otherFunction->equal_to(this))
      return this;
    if (AllTop<L> *alltop = dynamic_cast<AllTop<L> *>(otherFunction))
      return this;
    if (EdgeIdentity<L> *ei = dynamic_cast<EdgeIdentity<L> *>(otherFunction))
      return this;
    return this;
  }

  bool equal_to(EdgeFunction<L> *other) const override {
    if (AllBottom<L> *allbottom = dynamic_cast<AllBottom<L> *>(other)) {
      return (allbottom->bottomElement == bottomElement);
    }
    return false;
  }

  void print(std::ostream &OS, bool isForDebug = false) const override {
    OS << "AllBottom";
  }
};

} // namespace psr

#endif
