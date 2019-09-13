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
  // bool equal_to(const FlowFact &FF) const override {
  //   return fact == FF.get();
  // }
  // bool less(const FlowFact &FF) const override {
  //   return fact < FF.get();
  // }
};
} // namespace psr

#endif
