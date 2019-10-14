/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#ifndef PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTRAMONOPROBLEMPLUGIN_H_
#define PHASAR_PHASARLLVM_PLUGINS_INTERFACES_MONO_INTRAMONOPROBLEMPLUGIN_H_

#include <map>
#include <memory>
#include <string>

#include <phasar_plugins_export.h>

namespace psr {

class IntraMonoProblemPlugin {};

extern "C" std::unique_ptr<IntraMonoProblemPlugin> makeIntraMonoProblemPlugin();

extern PHASAR_PLUGINS_EXPORT std::map<std::string, std::unique_ptr<IntraMonoProblemPlugin> (*)()>
    IntraMonoProblemPluginFactory;

} // namespace psr

#endif
