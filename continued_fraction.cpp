/*    Copyright (C) 2011 University of Southern California and
 *                       Andrew D. Smith and Timothy Daley
 *
 *    Authors: Andrew D. Smith and Timothy Daley
 *
 *    This program is free software: you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation, either version 3 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "continued_fraction.hpp"
#include <smithlab_utils.hpp>

#include <vector>
#include <cmath>
#include <cassert>
#include <complex>

using std::vector;
using std::complex;
using std::real;
using std::imag;
using std::cerr;
using std::endl;
using std::min;

const double TOLERANCE = 1e-20;
const double DERIV_DELTA = 1e-8;

static double
get_rescale_value(const double numerator, const double denominator) {
  const double rescale_val = fabs(numerator) + fabs(denominator);
  if (rescale_val > 1.0/TOLERANCE)
    return 1.0/rescale_val;
  else if (rescale_val < TOLERANCE)
    return 1.0/rescale_val;
  return 1.0;
}

static double
get_rescale_value(const complex<double> numerator, const complex<double> denominator) {
  const double rescale_val = norm(numerator) + norm(denominator);
  if (rescale_val > 1.0/TOLERANCE)
    return 1.0/rescale_val;
  if (rescale_val < TOLERANCE)
    return 1.0/rescale_val;
  return 1.0;
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////
////  QUOTIENT DIFFERENCE ALGORITHMS
////

/* quotient-difference algorithm to compute continued fraction
   coefficients
*/ 
static void
quotdiff_algorithm(const vector<double> &ps_coeffs, vector<double> &cf_coeffs) {
  
  const size_t depth = ps_coeffs.size();
  vector< vector<double> > q_table(depth, vector<double>(depth, 0.0));
  vector< vector<double> > e_table(depth, vector<double>(depth, 0.0));

  for (size_t i = 0; i < q_table[1].size(); i++)
    q_table[1][i] = ps_coeffs[i + 1]/ps_coeffs[i];
  
  for (size_t j = 0; j < depth-1; j++)
    e_table[1][j] = q_table[1][j + 1] - q_table[1][j] + e_table[0][j + 1];
  
  for (size_t i = 2; i < depth; i++) {
    for (size_t j = 0; j < depth; j++)
      q_table[i][j] = q_table[i - 1][j + 1]*e_table[i - 1][j + 1]/e_table[i - 1][j];
    
    for (size_t j = 0; j < depth; j++)
      e_table[i][j] = q_table[i][j + 1] - q_table[i][j] + e_table[i - 1][j + 1];
  }
  
  cf_coeffs.push_back(ps_coeffs[0]);
  for (size_t i = 1; i < depth; ++i) {
    if (i % 2 == 0)
      cf_coeffs.push_back(-e_table[i/2][0]);
    else
      cf_coeffs.push_back(-q_table[(i + 1)/2][0]);
  }
}


// compute CF coeffs when upper_offset > 0
static void
quotdiff_above_diagonal(const vector<double> &coeffs, const size_t offset,
			vector<double> &offset_coeffs, vector<double> &cf_coeffs) { 
  //first offset coefficients set to first offset coeffs
  vector<double> holding_coeffs;
  for (size_t i = offset; i < coeffs.size(); i++)
    holding_coeffs.push_back(coeffs[i]);
  
  // qd to determine cf_coeffs
  quotdiff_algorithm(holding_coeffs, cf_coeffs);
  for (size_t i = 0; i < offset; i++)
    offset_coeffs.push_back(coeffs[i]);
}


// calculate CF coeffs when lower_offset > 0
static void
quotdiff_below_diagonal(const vector<double> &coeffs, const size_t offset, 
			vector<double> &offset_coeffs, vector<double> &cf_coeffs) {
  
  //need to work with reciprocal series g = 1/f, then invert
  vector<double> reciprocal_coeffs;
  reciprocal_coeffs.push_back(1.0/coeffs[0]);
  for (size_t i = 1; i < coeffs.size(); i++) {
    double holding_val = 0.0;
    for (size_t j = 0; j < i; ++j)
      holding_val += coeffs[i - j]*reciprocal_coeffs[j];
    reciprocal_coeffs.push_back(-holding_val/coeffs[0]);
  }
  
  //set offset_coeffs to 1st offset coeffs of 1/f 
  for (size_t i = 0; i < offset; i++)
    offset_coeffs.push_back(reciprocal_coeffs[i]);
  
  // qd to compute cf_coeffs using remaining coeffs
  vector<double> holding_coeffs;
  for (size_t i = offset; i < coeffs.size(); i++)
    holding_coeffs.push_back(reciprocal_coeffs[i]);
  
  quotdiff_algorithm(holding_coeffs, cf_coeffs);
}


ContinuedFraction::ContinuedFraction(const vector<double> &ps_cf, 
				     const int di, const size_t dg) :
  ps_coeffs(ps_cf), diagonal_idx(di), degree(dg) {

  if (diagonal_idx == 0)
    quotdiff_algorithm(ps_coeffs, cf_coeffs);
  else if (diagonal_idx > 0)
    quotdiff_above_diagonal(ps_coeffs, diagonal_idx, cf_coeffs, offset_coeffs);
  else // if(cont_frac_estimate.lower_offset > 0) {
    quotdiff_below_diagonal(ps_coeffs, -diagonal_idx, offset_coeffs, cf_coeffs);
  // notice the "-" above...
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////
////  FUNCTIONS TO EVALUATE CONTINUED FRACTIONS AT A POINT
////

/* evaluate CF when upper_offset > 0 using Euler's recursion
 */
static double
evaluate_above_diagonal(const vector<double> &cf_coeffs,
			const vector<double> &offset_coeffs,
			const double val, const size_t depth) {
  
  double current_num = 0.0;
  double prev_num1 = cf_coeffs[0];
  double prev_num2 = 0.0;
  
  double current_denom = 0.0;
  double prev_denom1 = 1.0;
  double prev_denom2 = 1.0; 
  
  for (size_t i = 1; i < depth; i++) {
    // initialize
    current_num = prev_num1 + cf_coeffs[i]*val*prev_num2;
    current_denom = prev_denom1 + cf_coeffs[i]*val*prev_denom2;
    
    prev_num2 = prev_num1;
    prev_num1 = current_num;
    
    prev_denom2= prev_denom1;
    prev_denom1 = current_denom;
    
    //rescale to avoid over- and underflow
    const double rescale_val = get_rescale_value(current_num, current_denom);
    
    current_num = current_num*rescale_val;
    current_denom = current_denom*rescale_val;
    
    prev_num1 = prev_num1*rescale_val;
    prev_num2 = prev_num2*rescale_val;

    prev_denom1 = prev_denom1*rescale_val;
    prev_denom2 = prev_denom2*rescale_val;
  }

  double offset_part = 0.0;
  for (size_t i = 0; i < min(offset_coeffs.size(), depth); i++)
    offset_part += offset_coeffs[i]*pow(val, i);
  
  return val*(offset_part + pow(val, min(offset_coeffs.size(), depth))*
	      current_num/current_denom);
} 


// calculate ContinuedFraction approx when lower_offdiag > 0
static double 
evaluate_below_diagonal(const vector<double> &cf_coeffs,
			const vector<double> &offset_coeffs,
			const double val, const size_t depth) {
  
  //initialize
  double current_num = 0.0;
  double prev_num1 = cf_coeffs[0];
  double prev_num2 = 0.0;

  double current_denom = 0.0;
  double prev_denom1 = 1.0;
  double prev_denom2 = 1.0; 

  for (size_t i = 1; i < depth; i++) {

    // recursion
    current_num = prev_num1 + cf_coeffs[i]*val*prev_num2;
    current_denom = prev_denom1 + cf_coeffs[i]*val*prev_denom2;

    prev_num2 = prev_num1;
    prev_num1 = current_num;

    prev_denom2= prev_denom1;
    prev_denom1 = current_denom;

    const double rescale_val = get_rescale_value(current_num, current_denom);

    current_num = current_num*rescale_val;
    current_denom = current_denom*rescale_val;

    prev_num1 = prev_num1*rescale_val;
    prev_num2 = prev_num2*rescale_val;
    
    prev_denom1 = prev_denom1*rescale_val;
    prev_denom2 = prev_denom2*rescale_val;
  }
  
  double offset_terms = 0.0;
  for (size_t i = 0; i < min(offset_coeffs.size(), depth); i++)
    offset_terms += offset_coeffs[i]*pow(val, i);
  
  // recall that if lower_offset > 0, we are working with 1/f, invert approx
  return val/(offset_terms + pow(val, min(offset_coeffs.size(), depth))*
	      current_num/current_denom);
}


// calculate ContinuedFraction approx when there is no offset
// uses euler's recursion
static double
evaluate_on_diagonal(const vector<double> &cf_coeffs, 
		     const double val, const size_t depth) {
  
  // initialize
  double current_num = 0.0;
  double prev_num1 = cf_coeffs[0];
  double prev_num2 = 0.0;

  double current_denom = 0.0;
  double prev_denom1 = 1.0;
  double prev_denom2 = 1.0; 

  for (size_t i = 1; i < depth; i++) {
    // recursion
    current_num = prev_num1 + cf_coeffs[i]*val*prev_num2;
    current_denom = prev_denom1 + cf_coeffs[i]*val*prev_denom2;

    prev_num2 = prev_num1;
    prev_num1 = current_num;

    prev_denom2= prev_denom1;
    prev_denom1 = current_denom;

    const double rescale_val = get_rescale_value(current_num, current_denom);
    
    current_num = current_num*rescale_val;
    current_denom = current_denom*rescale_val;
    
    prev_num1 = prev_num1*rescale_val;
    prev_num2 = prev_num2*rescale_val;

    prev_denom1 = prev_denom1*rescale_val;
    prev_denom2 = prev_denom2*rescale_val;
  }
  return val*current_num/current_denom;
}


// calculate cont_frac approx depending on offset
double
ContinuedFraction::operator()(const double val) const {
  if (diagonal_idx > 0)
    return evaluate_above_diagonal(cf_coeffs, offset_coeffs, val, degree);
  
  if (diagonal_idx < 0)
    return evaluate_below_diagonal(cf_coeffs, offset_coeffs, val, degree);
  
  return evaluate_on_diagonal(cf_coeffs, val, degree);
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//////
//////  COMPLEX NUMBER FUNCTIONS BELOW HERE
//////

// compute ContFrac_eval for complex values to compute deriv when no offset
static void
evaluate_complex_on_diagonal(const vector<double> &cf_coeffs,
			     const complex<double> perturbed_val,
			     const size_t depth, complex<double> &approx) {
  const complex<double> sqrt_neg1(0.0,1.0);
  if (norm(perturbed_val) == 0.0)
    approx = 0.0*sqrt_neg1;
  
  else {
    
    // Previous elements of the table to recursively fill it
    complex<double> current_num(0.0, 0.0);
    complex<double> prev_num1(cf_coeffs[0], 0.0), prev_num2(0.0, 0.0);
    complex<double> current_denom(0.0, 0.0);
    complex<double> prev_denom1(1.0, 0.0), prev_denom2(1.0, 0.0);
    
    for (size_t j = 1; j < depth; j++) {
      //euler's recursion
      complex<double> coeff(cf_coeffs[j], 0.0);
      current_num = prev_num1 + coeff*perturbed_val*prev_num2;
      current_denom = prev_denom1 + coeff*perturbed_val*prev_denom2;
      
      prev_num2 = prev_num1;
      prev_num1 = current_num;
      
      prev_denom2 = prev_denom1;
      prev_denom1 = current_denom;

      //rescale to avoid over- and underflow
      const double rescale_val = get_rescale_value(current_num, current_denom);
      
      current_num = current_num*rescale_val;
      current_denom = current_denom*rescale_val;
      
      prev_num1 = prev_num1*rescale_val;
      prev_num2 = prev_num2*rescale_val;
      
      prev_denom1 = prev_denom1*rescale_val;
      prev_denom2 = prev_denom2*rescale_val;
    }
    approx = perturbed_val*current_num/current_denom;
  }
}

// compute complex ContFrac_eval when above_diagonal > 0
static void
evaluate_complex_above_diagonal(const vector<double> &cf_coeffs,
				const vector<double> &offset_coeffs,
				const complex<double> perturbed_val,
				const size_t depth, complex<double> &approx) {
  
  const complex<double> sqrt_neg1(0.0,1.0);
  if (norm(perturbed_val) == 0.0)
    approx = 0.0*imag;
  
  else {
    //initialize
    complex<double> current_num(0.0, 0.0);
    complex<double> prev_num1(cf_coeffs[0], 0.0), prev_num2(0.0, 0.0);
    complex<double> current_denom(0.0, 0.0);
    complex<double> prev_denom1(1.0, 0.0), prev_denom2(1.0, 0.0);

    for (size_t j = 1; j < depth; j++) {
      
      //eulers recursion
      complex<double> coeff(cf_coeffs[j], 0.0);
      current_num = prev_num1 + coeff*perturbed_val*prev_num2;
      current_denom = prev_denom1 + coeff*perturbed_val*prev_denom2;
      
      prev_num2 = prev_num1;
      prev_num1 = current_num;
      
      prev_denom2 = prev_denom1;
      prev_denom1 = current_denom;

      //rescale to avoid over and underflow
      const double rescale_val = get_rescale_value(current_num, current_denom);
      
      current_num = current_num*rescale_val;
      current_denom = current_denom*rescale_val;
      
      prev_num1 = prev_num1*rescale_val;
      prev_num2 = prev_num2*rescale_val;
      
      prev_denom1 = prev_denom1*rescale_val;
      prev_denom2 = prev_denom2*rescale_val;
    }

    complex<double> offset_terms(0.0, 0.0);
    for (size_t i = 0; i < min(offset_coeffs.size(), depth); i++)
      offset_terms += offset_coeffs[i]*pow(perturbed_val, i);
    
    approx = perturbed_val*
      (offset_terms + pow(perturbed_val, min(offset_coeffs.size(), depth))*
       current_num/current_denom);
  }
} 

// compute cf approx when lower_offset > 0
static void
evaluate_complex_below_diagonal(const vector<double> &cf_coeffs,
				const vector<double> &offset_coeffs,
				const complex<double> perturbed_val,
				const size_t depth,
				complex<double> &approx) {
  const complex<double> sqrt_neg1(0.0,1.0);
  if (norm(perturbed_val) == 0.0)
    approx = 0.0*imag;
  else{
    // initialize
    complex<double> current_num(0.0, 0.0);
    complex<double> prev_num1(cf_coeffs[0], 0.0), prev_num2(0.0, 0.0);
    complex<double> current_denom(0.0, 0.0);
    complex<double> prev_denom1(1.0, 0.0), prev_denom2(1.0, 0.0);

    for (size_t j = 1; j < cf_coeffs.size(); j++) {
      
      // euler's recursion
      complex<double> coeff(cf_coeffs[j], 0.0);
      current_num = prev_num1 + coeff*perturbed_val*prev_num2;
      current_denom = prev_denom1 + coeff*perturbed_val*prev_denom2;
      
      prev_num2 = prev_num1;
      prev_num1 = current_num;
      
      prev_denom2 = prev_denom1;
      prev_denom1 = current_denom;

      //rescale to avoid over and underflow
      const double rescale_val = get_rescale_value(current_num, current_denom);
      
      current_num = current_num*rescale_val;
      current_denom = current_denom*rescale_val;
      
      prev_num1 = prev_num1*rescale_val;
      prev_num2 = prev_num2*rescale_val;
      
      prev_denom1 = prev_denom1*rescale_val;
      prev_denom2 = prev_denom2*rescale_val;
    }
    
    complex<double> offset_terms(0.0, 0.0);
    for (size_t i = 0; i < min(offset_coeffs.size(), depth); i++)
      offset_terms += offset_coeffs[i]*pow(perturbed_val, i);

    approx = perturbed_val/
      (offset_terms + pow(perturbed_val, min(offset_coeffs.size(), depth))*
       current_num/current_denom);
  }
} 

/* compute cf approx for complex depending on offset df/dx =
 * lim_{delta -> 0} Imag(f(val+i*delta))/delta
 */
double
ContinuedFraction::complex_deriv(const double val) const {
  vector<double> ContFracCoeffs(cf_coeffs);
  vector<double> ContFracOffCoeffs(offset_coeffs);
  
  const complex<double> sqrt_neg1(0.0,1.0);
  complex<double> df(0.0, 0.0);
  complex<double> value(val, 0.0);
  
  if (diagonal_idx == 0)
    evaluate_complex_on_diagonal(ContFracCoeffs, value + DERIV_DELTA*sqrt_neg1, degree, df);
  
  else if (diagonal_idx > 0)
    evaluate_complex_above_diagonal(ContFracCoeffs, ContFracOffCoeffs,
				    value + DERIV_DELTA*sqrt_neg1, degree, df);
  
  else if (diagonal_idx < 0)
    evaluate_complex_below_diagonal(ContFracCoeffs, ContFracOffCoeffs,
 				    value + DERIV_DELTA*sqrt_neg1, degree, df);
  
  return imag(df)/DERIV_DELTA;
}


std::ostream& 
operator<<(std::ostream& the_stream, const ContinuedFraction &cf) {
  using std::setw;
  using std::fixed;
  using std::setprecision;
  the_stream << "OFFSET_COEFFS" << '\n';
  for (size_t i = 0; i < cf.offset_coeffs.size(); ++i)
    the_stream << setw(12) << fixed << setprecision(2) << cf.offset_coeffs[i] << '\t'
	       << setw(12) << fixed << setprecision(2) << cf.ps_coeffs[i] << '\n';
  the_stream << "CF_COEFFS" << '\n';
  const size_t offset = cf.offset_coeffs.size();
  for (size_t i = 0; i < cf.cf_coeffs.size(); ++i)
    the_stream << setw(12) << fixed << setprecision(2) << cf.cf_coeffs[i] << '\t'
	       << setw(12) << fixed << setprecision(2) << cf.ps_coeffs[i + offset] << '\n';
  return the_stream;
}


// Extrapolates the curve, for given values (step & max) and numbers
// of terms
void
ContinuedFraction::extrapolate_distinct(const vector<double> &counts_hist,
					const double max_value, const double step_size,
					vector<double> &estimates) const {
  const double hist_sum = accumulate(counts_hist.begin(), counts_hist.end(), 0.0);
  estimates.clear();
  estimates.push_back(hist_sum);
  for (double t = step_size; t <= max_value; t += step_size)
    estimates.push_back(hist_sum + operator()(t));
}


////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
//////////////////
/////////////////
////////////////  CONTINUED FRACTION APPROXIMATION CLASS BELOW
///////////////
//////////////
/////////////
////////////


// calculate cf_coeffs depending on offset
ContinuedFractionApproximation::ContinuedFractionApproximation(const int di, const size_t mt, 
							       const double ss, const double mv) :
  diagonal_idx(di), max_terms(mt), step_size(ss), max_value(mv) {
  std::cerr << max_terms << std::endl;  
}


static inline double
movement(const double a, const double b) {
  return fabs((a - b)/std::max(a, b)); //delta
}

/* locate zero deriv by bisection to find local max within (prev_val,
   val)
*/ 
double
ContinuedFractionApproximation::locate_zero_cf_deriv(const ContinuedFraction &cf, 
						     const double val, 
						     const double prev_val) const {
  
  double val_low = prev_val;
  double deriv_low = cf.complex_deriv(val_low);
  
  double val_high = val;
  double deriv_high = cf.complex_deriv(val_high);
  
  double val_mid = (val - prev_val)/2.0;
  double deriv_mid = std::numeric_limits<double>::max();
  
  double diff = std::numeric_limits<double>::max();
  double prev_deriv = std::numeric_limits<double>::max();

  while (diff > TOLERANCE && movement(val_low, val_high) > TOLERANCE) {
    
    val_mid = (val_low + val_high)/2.0;
    deriv_mid = cf.complex_deriv(val_mid);
    
    if ((deriv_mid > 0 && deriv_low < 0) || (deriv_mid < 0 && deriv_low > 0))
      val_high = val_mid;
    else val_low = val_mid;
    
    deriv_low = cf.complex_deriv(val_low);
    deriv_high = cf.complex_deriv(val_high);
    
    diff = fabs((prev_deriv - deriv_mid)/prev_deriv);
    prev_deriv = deriv_mid;
  }
  
  return val_mid;
}


// search (min_val, max_val) for local max
// return location of local max
double
ContinuedFractionApproximation::local_max(const ContinuedFraction &cf,
					  const double upper_bound,
					  const double deriv_upper) const {
  double current_max = cf(0.0);
  for (double val = step_size; val <= max_value; val += step_size)
    current_max = std::max(current_max, cf(locate_zero_cf_deriv(cf, val, val - step_size)));
  return current_max;
}


/* Checks if estimates are stable (derivative large) for the
 * particular approximation (degrees of num and denom) at a specific
 * point
 */
static bool
check_estimates_stability(const vector<double> &estimates) {
  // make sure that the estimate is increasing in the time_step and
  // is below the initial distinct per step_size
  for (size_t i = 1; i < estimates.size(); ++i)
    if (estimates[i] < estimates[i - 1] ||
	(i >= 2 && (estimates[i] - estimates[i - 1] >
		    estimates[i - 1] - estimates[i - 2])))
      return false;
  return true;
}


/* Finds the optimal number of terms (i.e. degree, depth, etc.) of the
 * continued fraction by checking for stability of estimates at
 * specific points.
 */
ContinuedFraction
ContinuedFractionApproximation::optimal_continued_fraction(const vector<double> &counts_hist) const {
  
  // ensure that we will use an underestimate
  const size_t local_max_terms = max_terms - (max_terms % 2 == 1);
  
  // counts_sum = number of total captures
  double counts_sum  = 0.0;
  for(size_t i = 0; i < counts_hist.size(); i++)
    counts_sum += i*counts_hist[i];
  
  vector<double> ps_coeffs(local_max_terms, 0.0);
  for (size_t j = 0; j < local_max_terms; j++)
    ps_coeffs[j] = counts_hist[j + 1]*pow(-1, j + 2);

  for (size_t n_terms = local_max_terms; n_terms >= MIN_ALLOWED_DEGREE; n_terms -= 2) {
    // make a CF for this number of terms
    const ContinuedFraction cf(ps_coeffs, diagonal_idx, n_terms);
    
    // compute the estimates for the desired set of points
    vector<double> estimates;
    cf.extrapolate_distinct(counts_hist, max_value, step_size, estimates);
    
    // return the continued fraction if it is stable
    if (check_estimates_stability(estimates))
      return cf;
  }
  
  throw SMITHLABException("unable to fit continued fraction");
  
  // no stable continued fraction: return crap
  return ContinuedFraction();
}


/* library_yield = xp(x)/q(x), so if degree(q) > degree(p)+1, then
 * library_yield acts like 1/x^n for some n > 0 in the limit and
 * therefore goes to zero since it approximates the library yield in
 * the neighborhood of zero there is global max, so if we choose a
 * conservative approx, this is a lower bound on library_size
 */
double
ContinuedFractionApproximation::lowerbound_librarysize(const vector<double> &counts_hist,
						       const double upper_bound) const {
  
  // the derivative must always be less than the number of distinct
  // reads in the initial sample
  const double distinct_reads = 
    accumulate(counts_hist.begin(), counts_hist.end(), 0.0);
  
  // make sure we are using appropriate order estimate
  const size_t local_max_terms = max_terms - (max_terms % 2 == 1);
  
  vector<double> ps_coeffs(local_max_terms, 0.0);
  for (size_t j = 0; j < local_max_terms; j++)
    ps_coeffs[j] = counts_hist[j + 1]*pow(-1, j + 2);
  
  // iterate over max_terms to find largest local max as lower bound
  // theortically larger max_terms will be better approximations ==>
  // larger lower bounds
  double best = std::numeric_limits<double>::max();
  for (size_t n_terms = local_max_terms; n_terms > MIN_ALLOWED_DEGREE; n_terms -= 2) {
    // make a CF for this number of terms
    const ContinuedFraction cf(ps_coeffs, diagonal_idx, n_terms);
    const double candidate_best = local_max(cf, upper_bound, distinct_reads);
    best = std::min(best, candidate_best);
    std::cerr << n_terms << "\t" << candidate_best << std::endl;
  }
  return best;
}