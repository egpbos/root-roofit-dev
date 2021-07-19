// Author: Patrick Bos, Netherlands eScience Center / NIKHEF 2021

/*****************************************************************************
 * RooFit
 * Authors:                                                                  *
 *   WV, Wouter Verkerke, UC Santa Barbara, verkerke@slac.stanford.edu       *
 *   DK, David Kirkby,    UC Irvine,         dkirkby@uci.edu                 *
 *                                                                           *
 * Copyright (c) 2000-2021, Regents of the University of California          *
 *                          and Stanford University. All rights reserved.    *
 *                                                                           *
 * Redistribution and use in source and binary forms,                        *
 * with or without modification, are permitted according to the terms        *
 * listed in LICENSE (http://roofit.sourceforge.net/license.txt)             *
 *****************************************************************************/

#include <TestStatistics/optional_parameter_types.h>

namespace RooFit {
namespace TestStatistics {

ConstrainedParameters::ConstrainedParameters(const RooArgSet &parameters) : set(parameters) {}

ExternalConstraints::ExternalConstraints(const RooArgSet &constraints) : set(constraints) {}

GlobalObservables::GlobalObservables(const RooArgSet &observables) : set(observables) {}

}
}