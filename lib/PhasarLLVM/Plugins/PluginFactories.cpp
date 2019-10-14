/******************************************************************************
 * Copyright (c) 2017 Philipp Schubert.
 * All rights reserved. This program and the accompanying materials are made
 * available under the terms of LICENSE.txt.
 *
 * Contributors:
 *     Philipp Schubert and others
 *****************************************************************************/

#include <phasar/PhasarLLVM/Plugins/PluginFactories.h>
using namespace std;
using namespace psr;

namespace psr {

// Maps for registering the plugins
PHASAR_PLUGINS_EXPORT map<string, unique_ptr<IFDSTabulationProblemPlugin> (*)(LLVMBasedICFG &,
                                                        vector<string>)>
    IFDSTabulationProblemPluginFactory;

PHASAR_PLUGINS_EXPORT map<string,
    unique_ptr<IDETabulationProblemPlugin> (*)(LLVMBasedICFG &, vector<string>)>
    IDETabulationProblemPluginFactory;

PHASAR_PLUGINS_EXPORT map<string, unique_ptr<IntraMonoProblemPlugin> (*)()>
    IntraMonoProblemPluginFactory;

PHASAR_PLUGINS_EXPORT map<string, unique_ptr<InterMonoProblemPlugin> (*)()>
    InterMonoProblemPluginFactory;

PHASAR_PLUGINS_EXPORT map<string,
    unique_ptr<ICFGPlugin> (*)(ProjectIRDB &, const vector<string> EntryPoints)>
    ICFGPluginFactory;

} // namespace psr
