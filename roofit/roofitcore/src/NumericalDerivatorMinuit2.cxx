// @(#)root/mathcore:$Id$
// Authors: L. Moneta, J.T. Offermann, E.G.P. Bos    2013-2018
//
/**********************************************************************
 *                                                                    *
 * Copyright (c) 2013 , LCG ROOT MathLib Team                         *
 *                                                                    *
 **********************************************************************/
/*
 * NumericalDerivatorMinuit2.cxx
 *
 *  Original version (NumericalDerivator) created on: Aug 14, 2013
 *      Authors: L. Moneta, J. T. Offermann
 *  Modified version (NumericalDerivatorMinuit2) created on: Sep 27, 2017
 *      Author: E. G. P. Bos
 *
 *      NumericalDerivator was essentially a slightly modified copy of code
 *      written by M. Winkler, F. James, L. Moneta, and A. Zsenei for Minuit2,
 *      Copyright (c) 2005 LCG ROOT Math team, CERN/PH-SFT. Original version:
 *      https://github.com/lmoneta/root/blob/lvmini/math/mathcore/src/NumericalDerivator.cxx
 *
 *      This class attempts to more closely follow the Minuit2 implementation.
 *      Modified things (w.r.t. NumericalDerivator) are indicated by MODIFIED.
 */

#include "NumericalDerivatorMinuit2.h"
#include <cmath>
#include <algorithm>
#include <Math/IFunction.h>
#include <iostream>
#include <TMath.h>
#include <cassert>
#include "Fit/ParameterSettings.h"

#include <Math/Minimizer.h> // needed here because in Fitter is only a forward declaration

#include <RooMsgService.h>

namespace RooFit {

NumericalDerivatorMinuit2::NumericalDerivatorMinuit2(bool always_exactly_mimic_minuit2)
   : _always_exactly_mimic_minuit2(always_exactly_mimic_minuit2)
{
}

NumericalDerivatorMinuit2::NumericalDerivatorMinuit2(double step_tolerance, double grad_tolerance, unsigned int ncycles,
                                                     double error_level, bool always_exactly_mimic_minuit2)
   : fStepTolerance(step_tolerance), fGradTolerance(grad_tolerance), fNCycles(ncycles), Up(error_level),
     _always_exactly_mimic_minuit2(always_exactly_mimic_minuit2)
{
}

// deep copy constructor
NumericalDerivatorMinuit2::NumericalDerivatorMinuit2(const RooFit::NumericalDerivatorMinuit2 &other)
   : fStepTolerance(other.fStepTolerance), fGradTolerance(other.fGradTolerance), fNCycles(other.fNCycles), Up(other.Up),
     fVal(other.fVal), vx(other.vx), vx_external(other.vx_external), dfmin(other.dfmin), vrysml(other.vrysml),
     precision(other.precision), _always_exactly_mimic_minuit2(other._always_exactly_mimic_minuit2),
     vx_fVal_cache(other.vx_fVal_cache)
{
}

void NumericalDerivatorMinuit2::SetStepTolerance(double value)
{
   fStepTolerance = value;
}

void NumericalDerivatorMinuit2::SetGradTolerance(double value)
{
   fGradTolerance = value;
}

void NumericalDerivatorMinuit2::SetNCycles(int value)
{
   fNCycles = value;
}

NumericalDerivatorMinuit2::~NumericalDerivatorMinuit2()
{
   // TODO Auto-generated destructor stub
}

// This function sets internal state based on input parameters. This state
// setup is used in the actual (partial) derivative calculations.
void NumericalDerivatorMinuit2::setup_differentiate(const ROOT::Math::IBaseFunctionMultiDim *function, const double *cx,
                                                    const std::vector<ROOT::Fit::ParameterSettings> &parameters)
{
   assert(function != nullptr && "function is a nullptr");

   if (vx.size() != function->NDim()) {
      vx.resize(function->NDim());
   }
   if (vx_external.size() != function->NDim()) {
      vx_external.resize(function->NDim());
   }
   if (vx_fVal_cache.size() != function->NDim()) {
      vx_fVal_cache.resize(function->NDim());
   }
   std::copy(cx, cx + function->NDim(), vx.data());

   // convert to Minuit external parameters
   for (unsigned i = 0; i < function->NDim(); i++) {
      vx_external[i] = Int2ext(parameters[i], vx[i]);
   }

   if (vx != vx_fVal_cache) {
      vx_fVal_cache = vx;
      fVal = (*function)(vx_external.data()); // value of function at given points
   }

   dfmin = 8. * precision.Eps2() * (std::abs(fVal) + Up);
   vrysml = 8. * precision.Eps() * precision.Eps();
}

MinuitDerivatorElement
NumericalDerivatorMinuit2::partial_derivative(const ROOT::Math::IBaseFunctionMultiDim *function, const double *x,
                                              const std::vector<ROOT::Fit::ParameterSettings> &parameters,
                                              unsigned int i_component, MinuitDerivatorElement previous)
{
   setup_differentiate(function, x, parameters);
   return fast_partial_derivative(function, parameters, i_component, previous);
}

// leaves the parameter setup to the caller
MinuitDerivatorElement NumericalDerivatorMinuit2::fast_partial_derivative(const ROOT::Math::IBaseFunctionMultiDim *function,
                                                        const std::vector<ROOT::Fit::ParameterSettings> &parameters,
                                                        unsigned int ix, const MinuitDerivatorElement& previous)
{
   MinuitDerivatorElement deriv {previous};

   double xtf = vx[ix];
   double epspri = precision.Eps2() + std::abs(deriv.derivative * precision.Eps2());
   double step_old = 0.;

   for (unsigned int j = 0; j < fNCycles; ++j) {
      double optstp = std::sqrt(dfmin / (std::abs(deriv.second_derivative) + epspri));
      double step = std::max(optstp, std::abs(0.1 * deriv.step_size));

      if (parameters[ix].IsBound()) {
         if (step > 0.5)
            step = 0.5;
      }

      double stpmax = 10. * std::abs(deriv.step_size);
      if (step > stpmax)
         step = stpmax;

      double stpmin = std::max(vrysml, 8. * std::abs(precision.Eps2() * vx[ix]));
      if (step < stpmin)
         step = stpmin;
      if (std::abs((step - step_old) / step) < fStepTolerance) {
         break;
      }
      deriv.step_size = step;
      step_old = step;
      vx[ix] = xtf + step;
      vx_external[ix] = Int2ext(parameters[ix], vx[ix]);
      double fs1 = (*function)(vx_external.data());

      vx[ix] = xtf - step;
      vx_external[ix] = Int2ext(parameters[ix], vx[ix]);
      double fs2 = (*function)(vx_external.data());

      vx[ix] = xtf;
      vx_external[ix] = Int2ext(parameters[ix], vx[ix]);

      double fGrd_old = deriv.derivative;
      deriv.derivative = 0.5 * (fs1 - fs2) / step;

      deriv.second_derivative = (fs1 + fs2 - 2. * fVal) / step / step;

      if (std::abs(fGrd_old - deriv.derivative) / (std::abs(deriv.derivative) + dfmin / step) < fGradTolerance) {
         break;
      }
   }
   return deriv;
}

MinuitDerivatorElement
NumericalDerivatorMinuit2::operator()(const ROOT::Math::IBaseFunctionMultiDim *function, const double *x,
                                      const std::vector<ROOT::Fit::ParameterSettings> &parameters,
                                      unsigned int i_component, const MinuitDerivatorElement& previous)
{
   return partial_derivative(function, x, parameters, i_component, previous);
}

std::vector<MinuitDerivatorElement>
NumericalDerivatorMinuit2::Differentiate(const ROOT::Math::IBaseFunctionMultiDim *function, const double *cx,
                                         const std::vector<ROOT::Fit::ParameterSettings> &parameters,
                                         const std::vector<MinuitDerivatorElement>& previous_gradient)
{
   setup_differentiate(function, cx, parameters);

   std::vector<MinuitDerivatorElement> gradient;
   gradient.reserve(function->NDim());

   for (unsigned int ix = 0; ix < function->NDim(); ++ix) {
      gradient.emplace_back(fast_partial_derivative(function, parameters, ix, previous_gradient[ix]));
   }

   return gradient;
}


double NumericalDerivatorMinuit2::Int2ext(const ROOT::Fit::ParameterSettings &parameter, double val) const
{
   // return external value from internal value for parameter i
   if (parameter.IsBound()) {
      if (parameter.IsDoubleBound()) {
         return fDoubleLimTrafo.Int2ext(val, parameter.UpperLimit(), parameter.LowerLimit());
      } else if (parameter.HasUpperLimit() && !parameter.HasLowerLimit()) {
         return fUpperLimTrafo.Int2ext(val, parameter.UpperLimit());
      } else {
         return fLowerLimTrafo.Int2ext(val, parameter.LowerLimit());
      }
   }

   return val;
}

double NumericalDerivatorMinuit2::Ext2int(const ROOT::Fit::ParameterSettings &parameter, double val) const
{
   // return the internal value for parameter i with external value val

   if (parameter.IsBound()) {
      if (parameter.IsDoubleBound()) {
         return fDoubleLimTrafo.Ext2int(val, parameter.UpperLimit(), parameter.LowerLimit(), precision);
      } else if (parameter.HasUpperLimit() && !parameter.HasLowerLimit()) {
         return fUpperLimTrafo.Ext2int(val, parameter.UpperLimit(), precision);
      } else {
         return fLowerLimTrafo.Ext2int(val, parameter.LowerLimit(), precision);
      }
   }

   return val;
}

double NumericalDerivatorMinuit2::DInt2Ext(const ROOT::Fit::ParameterSettings &parameter, double val) const
{
   // return the derivative of the int->ext transformation: dPext(i) / dPint(i)
   // for the parameter i with value val

   double dd = 1.;
   if (parameter.IsBound()) {
      if (parameter.IsDoubleBound()) {
         dd = fDoubleLimTrafo.DInt2Ext(val, parameter.UpperLimit(), parameter.LowerLimit());
      } else if (parameter.HasUpperLimit() && !parameter.HasLowerLimit()) {
         dd = fUpperLimTrafo.DInt2Ext(val, parameter.UpperLimit());
      } else {
         dd = fLowerLimTrafo.DInt2Ext(val, parameter.LowerLimit());
      }
   }

   return dd;
}

// MODIFIED:
// This function was not implemented as in Minuit2. Now it copies the behavior
// of InitialGradientCalculator. See https://github.com/roofit-dev/root/issues/10
void NumericalDerivatorMinuit2::SetInitialGradient(const ROOT::Math::IBaseFunctionMultiDim *function,
                                                   const std::vector<ROOT::Fit::ParameterSettings> &parameters,
                                                   std::vector<MinuitDerivatorElement>& gradient)
{
   // set an initial gradient using some given steps
   // (used in the first iteration)

   assert(function != nullptr && "function is a nullptr");

   double eps2 = precision.Eps2();

   unsigned ix = 0;
   for (auto parameter = parameters.begin(); parameter != parameters.end(); ++parameter, ++ix) {
      // What Minuit2 calls "Error" is stepsize on the ROOT side.
      double werr = parameter->StepSize();

      // Actually, sav in Minuit2 is the external parameter value, so that is
      // what we called var before and var is unnecessary here.
      double sav = parameter->Value();

      // However, we do need var below, so let's calculate it using Ext2int:
      double var = Ext2int(*parameter, sav);

      if (_always_exactly_mimic_minuit2) {
         // this transformation can lose a few bits, but Minuit2 does it too
         sav = Int2ext(*parameter, var);
      }

      double sav2 = sav + werr;

      if (parameter->HasUpperLimit() && sav2 > parameter->UpperLimit()) {
         sav2 = parameter->UpperLimit();
      }

      double var2 = Ext2int(*parameter, sav2);
      double vplu = var2 - var;

      sav2 = sav - werr;

      if (parameter->HasLowerLimit() && sav2 < parameter->LowerLimit()) {
         sav2 = parameter->LowerLimit();
      }

      var2 = Ext2int(*parameter, sav2);
      double vmin = var2 - var;
      double gsmin = 8. * eps2 * (fabs(var) + eps2);
      // protect against very small step sizes which can cause dirin to zero and then nan values in grd
      double dirin = std::max(0.5 * (fabs(vplu) + fabs(vmin)), gsmin);
      double g2 = 2.0 * Up / (dirin * dirin);
      double gstep = std::max(gsmin, 0.1 * dirin);
      double grd = g2 * dirin;

      if (parameter->IsBound()) {
         if (gstep > 0.5)
            gstep = 0.5;
      }

      gradient[ix].derivative = grd;
      gradient[ix].second_derivative = g2;
      gradient[ix].step_size = gstep;
   }
}

bool NumericalDerivatorMinuit2::always_exactly_mimic_minuit2() const
{
   return _always_exactly_mimic_minuit2;
};

void NumericalDerivatorMinuit2::set_always_exactly_mimic_minuit2(bool flag)
{
   _always_exactly_mimic_minuit2 = flag;
}

void NumericalDerivatorMinuit2::set_step_tolerance(double step_tolerance)
{
   fStepTolerance = step_tolerance;
}
void NumericalDerivatorMinuit2::set_grad_tolerance(double grad_tolerance)
{
   fGradTolerance = grad_tolerance;
}
void NumericalDerivatorMinuit2::set_ncycles(unsigned int ncycles)
{
   fNCycles = ncycles;
}
void NumericalDerivatorMinuit2::set_error_level(double error_level)
{
   Up = error_level;
}

std::ostream& operator<<(std::ostream& out, const MinuitDerivatorElement value){
   return out << "(derivative: " << value.derivative << ", second_derivative: " << value.second_derivative << ", step_size: " << value.step_size << ")";
}

} // namespace RooFit
