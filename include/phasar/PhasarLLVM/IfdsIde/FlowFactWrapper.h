/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_FLOWFACTWRAPPER_H_
#define PHASAR_PHASARLLVM_IFDSIDE_FLOWFACTWRAPPER_H_

#include <ostream>

#include <phasar/PhasarLLVM/IfdsIde/FlowFact.h>

namespace psr {

template <typename T> class FlowFactWrapper : public FlowFact {
private:
  const T fact;

public:
  FlowFactWrapper(T f) : fact(f) {}
  virtual ~FlowFactWrapper() = default;
  T get() const { return fact; }
  void print(std::ostream &os) const override { os << fact << '\n'; }

  // need to use try because of reference typs
  bool equal_to(const FlowFact &FF) const override {
    try {
      auto FFW = dynamic_cast<const FlowFactWrapper<T> &>(FF);
      return fact == FFW.get();
    } catch (std::bad_cast exp) {
      return *this == FF;
    }
  }
  bool less(const FlowFact &FF) const override {
    try {
      auto FFW = dynamic_cast<const FlowFactWrapper<T> &>(FF);
      return fact < FFW.get();
    } catch (std::bad_cast exp) {
      return *this < FF;
    }
  }
};
} // namespace psr

#endif
