/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFACTWRAPPER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFACTWRAPPER_H_

#include <ostream>

#include <phasar/PhasarLLVM/IfdsIde/EdgeFact.h>

namespace psr {

template <typename T> class EdgeFactWrapper : public EdgeFact {
private:
  T fact;

public:
  EdgeFactWrapper(T f) : fact(f) {}

  ~EdgeFactWrapper() override = default;

  T get() { return fact; }

  void print(std::ostream &os) const override { os << fact << '\n'; }

  bool equal_to(const EdgeFact &EF) const override {
    // TODO provide better implementation
    return false;
  }

  bool less(const EdgeFact &EF) const override {
    // TODO provide better implementation
    return false;
  }
};

} // namespace psr

#endif
