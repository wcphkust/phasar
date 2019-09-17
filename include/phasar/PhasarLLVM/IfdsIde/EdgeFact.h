/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_IFDSIDE_EDGEFACT_H_
#define PHASAR_PHASARLLVM_IFDSIDE_EDGEFACT_H_

#include <iosfwd>

namespace psr {

class EdgeFact {
public:
  virtual ~EdgeFact() = default;
  virtual void print(std::ostream &OS) const = 0;
  virtual bool equal_to(const EdgeFact &EF) const = 0;
  virtual bool less(const EdgeFact &EF) const = 0;
};

static inline std::ostream &operator<<(std::ostream &OS, const EdgeFact &E) {
  E.print(OS);
  return OS;
}

static inline bool operator==(const EdgeFact &E, const EdgeFact &G) {
  return E.equal_to(G);
}

static inline bool operator!=(const EdgeFact &E, const EdgeFact &G) {
  return !E.equal_to(G);
}

static inline bool operator<(const EdgeFact &E, const EdgeFact &G) {
  return E.less(G);
}

} // namespace psr

#endif
