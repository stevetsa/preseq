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

#ifndef CONTINUED_FRACTION_HPP
#define CONTINUED_FRACTION_HPP

#include <fstream>
#include <iomanip>
#include <numeric>
#include <vector>
#include <complex>
#include <cassert>

struct ContinuedFraction {
  ContinuedFraction() {}
  ContinuedFraction(const std::vector<double> &ps_cf, 
		    const int di, const size_t dg);
  double operator()(const double val) const;
  void extrapolate_distinct(const std::vector<double> &counts_hist,
			    const double max_value, const double step_size,
			    std::vector<double> &estimates) const;
  double complex_deriv(const double val) const;
  bool is_valid() const {return !cf_coeffs.empty();}
  
  std::vector<double> ps_coeffs;
  std::vector<double> cf_coeffs;
  std::vector<double> offset_coeffs;
  int diagonal_idx;
  size_t degree;
};

std::ostream& 
operator<<(std::ostream& the_stream, const ContinuedFraction &cf);


class ContinuedFractionApproximation {
public:
  // Constructor
  ContinuedFractionApproximation(const int di, const size_t mt, 
				 const double ss, const double mv);
  
  ContinuedFraction
  optimal_continued_fraction(const std::vector<double> &counts_hist) const;
  
  double
  local_max(const ContinuedFraction &cf, const double upper_bound,
	    const double deriv_upper_bound) const;
  double 
  lowerbound_librarysize(const std::vector<double> &counts_hist,
			 const double upper_bound) const;
  
private:

  int diagonal_idx; // the diagonal to work with for estimates
  size_t max_terms; // the maximum number of terms to try for a CF
  double step_size; // the step size to use when training
  double max_value; // the largest value to check when training
  
  double locate_zero_cf_deriv(const ContinuedFraction &cf, 
			      const double val, const double prev_val) const;
  static const size_t MIN_ALLOWED_DEGREE = 6;
};

#endif