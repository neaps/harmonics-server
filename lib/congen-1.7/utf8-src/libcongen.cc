// -*- c++ -*-
// $Id: libcongen.cc 4869 2013-02-18 16:14:59Z flaterco $

/*  congen:  constituent generator.
    Copyright (C) 1997  David Flater.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#define __STDC_LIMIT_MACROS
#include <inttypes.h>
#include <Congen>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <iostream>
#include <sstream>
#include <cstring>


#if Congen_interfaceRevision != 0
#error got the wrong version Congen header file
#endif


// Congen::time_t is seconds since 1970-01-01 00:00 GMT, no leap
// seconds, no Julian calendar conversion.

namespace Congen {
  typedef int64_t time_t;
}


// All angles are maintained as degrees and none are ever normalized
// by libcongen.  This raises the question whether we have enough
// significant digits to carry the extra baggage through all the
// calculations without loss of precision.  doubles provide 53 bits,
// or 15 good decimal digits, of precision (nearly 16).
//
//   .XXXXXXXXXXXXXXX
//
// Our worst case to begin with is the calculation of T (hour angle of
// mean sun) in V_terms.  T for startYear(4001) is 276255180, so we
// have used up 9 digits.
//
//   XXXXXXXXX.XXXXXX
//
// To generate constituents, this value will then be multiplied by a
// coefficient.  This coefficient never exceeds 10 by much, but we'll
// allow 2 whole digits to be safe.
//
//   XXXXXXXXXXX.XXXX
//
// At this point the numbers are summed and output is quoted to the
// hundredth of a degree.  We only need two good digits after the
// decimal; the two extras beyond that provide an adequate buffer
// against accumulated error.


static const double M_PI_180 (M_PI / 180);
static const double M_180_PI (180 / M_PI);
static const double daysPerJulianCentury_double    (36525.);
static const double hoursPerJulianCentury_double   (876600.);
static const double secondsPerJulianCentury_double (3155760000.);


static double sind (double degrees) {
  return sin (degrees * M_PI_180);
}


static double cosd (double degrees) {
  return cos (degrees * M_PI_180);
}


static double asind (double x) {
  assert (x >= -1 && x <= 1);
  return M_180_PI * asin(x);
}


static double acosd (double x) {
  assert (x >= -1 && x <= 1);
  return M_180_PI * acos(x);
}


static double atan2d (double y, double x) {
  return atan2(y,x) * M_180_PI;
}


static double tand (double degrees) {
  return tan(degrees * M_PI_180);
}


static double cotd (double degrees) {
  return 1 / tand(degrees);
}


// This is far simpler than the mktime workaround used in congen 1.5,
// but it will be useless if we end up needing equilibrium arguments
// or node factors for different intervals.

static Congen::time_t startYear (Congen::year_t year) {
  assert (year > 0);
  assert (year <= 4001);
  --year;
  return -62135596800ll + year * 31536000ll
    + (year/4 - year/100 + year/400) * 86400ll;
}


// Exact middle of year is 07-02 12:00 on non-leap years and 07-02
// 00:00 on leap years.  The discrepancies in node factors versus SP
// 98 and its supplement are not explained by using alternate dates
// for the middle of the year.

static Congen::time_t midYear (Congen::year_t year) {
  assert (year > 0);
  assert (year <= 4000);
  return (startYear(year) + startYear(year+1)) / 2;
}


// Convert time_t to Julian centuries (36525 days) since Table 1
// epoch.  This T in Table 1 is not the same as T the hour angle of
// mean sun.

static double Table_1_T (Congen::time_t t) {
  static const Congen::time_t epoch (-2209032000ll); // 1899-12-31 12:00 GMT
  return (t - epoch) / secondsPerJulianCentury_double;
}


// Calculate T (hour angle of mean sun), s, h, p, p₁, and c (constant
// value) in degrees, or their speeds in degrees per Julian century.
// s, h, p, and p₁ are per the formulas in SP 98 Table 1.  Although
// they are trivial cases, T and c are handled in the same pipeline to
// simplify the code:  One set of vector operations does everything
// for V.

static std::valarray<double> V_terms (Congen::time_t t,
                                      uint_fast8_t derivative) {
  static const std::valarray<double> coeff0 (   // T⁰ coefficients
    (const double[]) {0.,
                      270 + 26./60 + 14.72/3600,
                      279 + 41./60 + 48.04/3600,
                      334 + 19./60 + 40.87/3600,
                      281 + 13./60 + 15./3600,
                      1.}, Congen::numVterms);
  static const std::valarray<double> coeff1 (   // T¹ coefficients
    (const double[]) {daysPerJulianCentury_double*360,
                      1336*360 + 1108411.2/3600,
                      129602768.13/3600,
                      11*360 + 392515.94/3600,
                      6189.03/3600,
                      0.}, Congen::numVterms);
  static const std::valarray<double> coeff2 (   // T² coefficients
    (const double[]) {0.,
                      9.09/3600,
                      1.089/3600,
                      -37.24/3600,
                      1.63/3600,
                      0.}, Congen::numVterms);
  static const std::valarray<double> coeff3 (   // T³ coefficients
    (const double[]) {0.,
                      .0068/3600,
                      0.,
                      -.045/3600,
                      .012/3600,
                      0.}, Congen::numVterms);

  const double t1 (Table_1_T(t));
  const double t2 (t1 * t1);
  switch (derivative) {
  case 0:
    return coeff0 + t1*coeff1 + t2*coeff2 + t2*t1*coeff3;
  case 1:
    return coeff1 + 2*t1*coeff2 + 3*t2*coeff3;
  default:
    assert (0);
  }
}


// For u and node factor formulas, which use the middle of the year
// instead of the beginning, we need N, p, and p₁ per the formulas in
// SP 98 Table 1.

static std::valarray<double> midyear_terms (Congen::time_t t,
                                            uint_fast8_t derivative) {
  static const std::valarray<double> coeff0 (   // T⁰ coefficients
    (const double[]) {259 + 10./60 + 57.12/3600,
                      334 + 19./60 + 40.87/3600,
                      281 + 13./60 + 15./3600}, 3);
  static const std::valarray<double> coeff1 (   // T¹ coefficients
    (const double[]) {-(5*360 + 482912.63/3600),
                      11*360 + 392515.94/3600,
                      6189.03/3600}, 3);
  static const std::valarray<double> coeff2 (   // T² coefficients
    (const double[]) {7.58/3600,
                      -37.24/3600,
                      1.63/3600}, 3);
  static const std::valarray<double> coeff3 (   // T³ coefficients
    (const double[]) {.008/3600,
                      -.045/3600,
                      .012/3600}, 3);

  const double t1 (Table_1_T(t));
  const double t2 (t1 * t1);
  switch (derivative) {
  case 0:
    return coeff0 + t1*coeff1 + t2*coeff2 + t2*t1*coeff3;
  case 1:
    return coeff1 + 2*t1*coeff2 + 3*t2*coeff3;
  default:
    assert (0);
  }
}

static const uint_fast8_t midyear_N_index  = 0;
static const uint_fast8_t midyear_p_index  = 1;
static const uint_fast8_t midyear_p₁_index = 2;


// Obliquity of the ecliptic as of 1900-01-01, per SP 98 Table 1
// (changes very very slowly).  Using the value 23.452 instead of the
// degrees/minutes/seconds value does not noticeably improve the match
// between Congen::tables() and SP 98 tables.
static const double ω (23 + 27./60 + 8.26/3600);


// Inclination of moon's orbit to plane of ecliptic, per SP 98 Table 1
// (doesn't change).  Using the value 5.145 instead of the
// degrees/minutes/seconds value does not noticeably improve the match
// between Congen::tables() and SP 98 tables.
static const double i (5 + 8./60 + 43.3546/3600);


// SP 98 leaves the derivation of u terms as a spherical trigonometry
// exercise for the reader (see Figure 1).  Ed Wallner generously
// provided support functions to derive many pertinent values from N,
// which at the time I simply copied.  Now in comments below I attempt
// to explain why those functions make sense.

// In spherical trig tutorials, A B and C denote the angles of the
// spherical triangle and a b and c denote the opposite sides
// expressed as arc angles.  Unfortunately, Figure 1 follows no such
// convention to help you figure out what goes where.


// Spherical trig identity:
//   cos A       = -cos B cos C + sin B sin C cos a
// SP 98 Figure 1:
//   cos (180-I) = -cos ω cos i + sin ω sin i cos N
// Using symmetry identity cos(180-x) = -cos x :
//   cos I       =  cos ω cos i - sin ω sin i cos N

static double cos_I (double N) {
  static const double term1 (cosd(ω) * cosd(i));
  static const double term2 (sind(ω) * sind(i));
  return term1 - term2 * cosd(N);
}


// SP 98 writes:  "The angle I will be always positive and will vary
// from ω - i to ω + i" (i.e., between 18 and 29 degrees).  In this
// range, both sine and cosine are always positive, and we can get
// sine from cosine using a Pythagorean identity:
//   sin²A + cos²A = 1
//   sin A = sqrt (1 - cos²A)

static double sin_I (double N) {
  const double t (cos_I(N));
  return sqrt (1 - t*t);
}


// Spherical trig identity:
//   sin a / sin A = sin b / sin B = sin c / sin C
// SP 98 Figure 1:
//   sin N / sin (180-I) = sin ν / sin i
//   sin i * sin N / sin (180-I) = sin ν
// Using symmetry identity sin(180-x) = sin x :
//   sin ν = sin i * sin N / sin I

static double sin_ν (double N) {
  static const double multiplier (sind(i));
  return multiplier * sind(N) / sin_I(N);
}


// ν ranges between -13.02 and +13.02 degrees.  In this range, cosine
// is always positive, and we can use the Pythagorean identity to get
// cosine from sine.

static double cos_ν (double N) {
  const double t (sin_ν(N));
  return sqrt (1 - t*t);
}


// The side of interest in Figure 1 is unlabelled.  Since it is
// opposite ω we'll call it Ω.

// Spherical trig identity:
//   sin a / sin A = sin b / sin B = sin c / sin C
// SP 98 Figure 1:
//   sin N / sin (180-I) = sin Ω / sin ω
//   sin ω * sin N / sin (180-I) = sin Ω
// Using symmetry identity sin(180-x) = sin x :
//   sin Ω = sin ω * sin N / sin I

static double sin_Ω (double N) {
  static const double multiplier (sind(ω));
  return multiplier * sind(N) / sin_I(N);
}


// This one could be either positive or negative, so we won't use the
// Pythagorean identity.

// Spherical trig identity:
//   cos c = cos a cos b + sin a sin b cos C
// SP 98 Figure 1:
//   cos Ω = cos N cos ν + sin N sin ν cos ω

static double cos_Ω (double N) {
  static const double multiplier (cosd(ω));
  return cosd(N) * cos_ν(N) + sind(N) * sin_ν(N) * multiplier;
}


// SP 98 paragraph 24:  "The arc designated by ξ is equal to the side
// ☊♈ − side ☊A and is the longitude in the moon's orbit of the
// intersection A."  Side ☊♈ is known to us as N and side ☊A is known
// to us as Ω.

static double ξ (double N) {
  return N - atan2d (sin_Ω(N), cos_Ω(N));
}


// ν ranges between -13.02 and +13.02 degrees, so arc sine suffices.

static double ν (double N) {
  return asind(sin_ν(N));
}


// I ranges between 18 and 29 degrees, so arc cosine suffices.

static double I (double N) {
  return acosd(cos_I(N));
}


// Formula 224 from SP 98.  congen through version 1.5 used a more
// complicated formulation.

static double ν′ (double N) {
  const double multiplier (sind (I(N) * 2));
  return atan2d (multiplier * sin_ν(N), multiplier * cos_ν(N) + .3347);
}


// Formula 232 from SP 98.  congen through version 1.5 used a more
// complicated formulation.

static double 2ν″ (double N) {
  double term1 (sin_I(N)), term2 (ν(N)*2);
  term1 *= term1;
  return atan2d (term1 * sind(term2), term1 * cosd(term2) + .0727);
}


// Formula 191 from SP 98.
static double P (double p, double ξ) {
  return p - ξ;
}


// Term in argument of constituent M₁.  Formula 203 does not fully
// determine Q.  We use the arc tangent function of two variables
// instead of one to avoid putting Q in the wrong quadrant.
// Trig identity:  tan P = sin P / cos P

static double Q (double P) {
  return atan2d (.483 * sind(P), cosd(P));
}


// Formula 204 from SP 98.
static double Qᵤ (double P, double Q) {
  return P - Q;
}


// Formula 197 from SP 98.
static double Qₐ (double P) {
  return 1 / sqrt (2.31 + 1.435 * cosd(2*P));
}


// Formula 214 from SP 98.
static double R (double P, double I) {
  // Value of cotI2 is always positive; I ranges between 18 and 29 degrees.
  const double cotI2 (cotd (I/2));
  return atan2d (sind (2*P), cotI2*cotI2/6 - cosd(2*P));
}


// Formula 213 from SP 98.
static double Rₐ (double P, double I) {
  double term (tand (I/2));
  term *= term;
  return 1 / sqrt (1 - 12*term*cosd(2*P) + 36*term*term);
}


// Node factor formulas.


static double f73 (double I) {
  const double term (sind(I));
  return (2./3 - term*term) / .5021;
}


static double f74 (double I) {
  const double term (sind(I));
  return term*term / .1578;
}


static double f75 (double I) {
  const double term (cosd(I/2));
  return sind(I) * term*term / .38;
}


static double f76 (double I) {
  return sind (2*I) / .7214;
}


static double f77 (double I) {
  const double term (sind(I/2));
  return sind(I) * term*term / .0164;
}


static double f78 (double I) {
  double term (cosd(I/2));
  term *= term;
  return term*term / .9154;
}


static double f79 (double I) {
  const double term (sind(I));
  return term*term / .1565;
}


static double f144 (double I) {
  double sterm (sind(I/2));
  sterm *= sterm;
  const double cterm (cosd(I/2));
  return (1 - 10*sterm + 15*sterm*sterm) * cterm*cterm / .5873;
}


static double f149 (double I) {
  return pow(cosd(I/2),6) / .8758;
}


static double f206 (double I, double P) {
  return f75(I) / Qₐ(P);
}


static double f215 (double I, double P) {
  return f78(I) / Rₐ(P,I);
}


static double f227 (double I, double ν) {
  const double term (sind(2*I));
  return sqrt (.8965*term*term + .6001*term*cosd(ν) + .1006);
}


static double f235 (double I, double ν) {
  double term (sind(I));
  term *= term;
  return sqrt (19.0444*term*term + 2.7702*term*cosd(2*ν) + .0981);
}


// End of node factor formulas.


// Dispatch to specified formula.

static double f (uint_fast8_t f_formula, double I, double P, double ν) {
  switch (f_formula) {
  case 1:
    return 1;
  case 73:
    return f73 (I);
  case 74:
    return f74 (I);
  case 75:
    return f75 (I);
  case 76:
    return f76 (I);
  case 77:
    return f77 (I);
  case 78:
    return f78 (I);
  case 79:
    return f79 (I);
  case 144:
    return f144 (I);
  case 149:
    return f149 (I);
  case 206:
    return f206 (I, P);
  case 215:
    return f215 (I, P);
  case 227:
    return f227 (I, ν);
  case 235:
    return f235 (I, ν);
  default:
    assert (0);
  }
}


static void tab4row (Congen::year_t left_year, Congen::year_t right_year) {
  Congen::time_t t (startYear (left_year));
  std::valarray<double> terms (V_terms (t, 0));
  printf ("%4u│%s│%s│%s│%s│%s║",
    left_year,
    Congen::normalize(terms[Congen::s_index],2).c_str(),
    Congen::normalize(terms[Congen::p_index],2).c_str(),
    Congen::normalize(terms[Congen::h_index],2).c_str(),
    Congen::normalize(terms[Congen::p₁_index],2).c_str(),
    Congen::normalize(midyear_terms(t,0)[midyear_N_index],2).c_str());
  if (right_year) {
    t = startYear (right_year);
    terms = V_terms (t, 0);
    printf ("%4u│%s│%s│%s│%s│%s\n",
      right_year,
      Congen::normalize(terms[Congen::s_index],2).c_str(),
      Congen::normalize(terms[Congen::p_index],2).c_str(),
      Congen::normalize(terms[Congen::h_index],2).c_str(),
      Congen::normalize(terms[Congen::p₁_index],2).c_str(),
      Congen::normalize(midyear_terms(t,0)[midyear_N_index],2).c_str());
  } else
    printf ("    │      │      │      │      │\n");
}


static void tab4part (Congen::year_t year_in) {
  printf (
    "════╤══════╤══════╤══════╤══════╤══════╦════╤══════╤══════╤══════╤══════╤══════\n"
    "Year│  s   │  p   │  h   │  p₁  │  N   ║Year│  s   │  p   │  h   │  p₁  │  N\n"
    "────┼──────┼──────┼──────┼──────┼──────║────┼──────┼──────┼──────┼──────┼──────\n"
    "    │  °   │  °   │  °   │  °   │  °   ║    │  °   │  °   │  °   │  °   │  °\n");
  for (Congen::year_t year (year_in); year < year_in + 52; ++year) {
    Congen::year_t left (year), right (year + 52);
    if ((right >= 1900 && right <= 1903) || right > 2000)
      right = 0;
    tab4row (left, right);
    if (year == 1851 || year == 1951)
      printf ("────┴──────┴──────┴──────┴──────┴──────╨────┴──────┴──────┴──────┴──────┴──────\n");
    else if ((year + 1) % 4 == 0)
      printf ("    │      │      │      │      │      ║    │      │      │      │      │\n");
  }
}


static void tab14row (const std::string &name,
                      uint_fast8_t f_formula,
                      Congen::year_t y1) {
  printf ("%s", name.c_str());
  for (Congen::year_t y (y1); y < y1+10; ++y) {
    const Congen::time_t t (midYear (y));
    const std::valarray<double> midyear_terms_t (midyear_terms(t,0));
    const double N_t (midyear_terms_t[midyear_N_index]);
    const double p_t (midyear_terms_t[midyear_p_index]);
    const double I_t (I(N_t));
    const double ξ_t (ξ(N_t));
    const double ν_t (ν(N_t));
    const double P_t (P(p_t,ξ_t));
    printf ("│%5.3f",
      f(f_formula,I_t,P_t,ν_t));
  }
  printf ("\n");
}


// Do weird logarithm mangling for SP 98 Tables 7 and 9.
static double mangled_log10 (double x) {
  double l (log10(x));
  if (l < 0)
    l += 10;
  return l;
}


namespace Congen {


void interfaceRevision_0 () {}


// %f implements the weird "nearest even number" rounding behavior:
// n    -2½ -2¼ -2  -1¾ -1½ -1¼ -1  -0¾ -0½ -0¼  0   0¼  0½  0¾  1   1¼  1½  1¾  2   2¼  2½
// %f   -2  -2  -2  -2  -2  -1  -1  -1   0   0   0   0   0   1   1   1   2   2   2   2   2


std::string normalize (double degrees, uint_fast8_t decimals) {
  assert (decimals > 0 && decimals <= 20);
  degrees = fmod (degrees, 360);
  if (degrees < 0)
    degrees += 360;
  degrees = fabs(degrees); // Fix anomalous -0 when fmodding -360
  char temp[25];
  sprintf (temp, "%*.*f", 4+decimals, decimals, degrees);
  if (!strncmp (temp, "36", 2))
    temp[0] = temp[1] = ' ';
  return temp;
}


std::string snormalize (double degrees, uint_fast8_t decimals) {
  assert (decimals > 0 && decimals <= 20);
  degrees = fmod (degrees, 360);
  if (degrees <= -180)
    degrees += 360;
  else if (degrees > 180)
    degrees -= 360;
  char temp[26];
  sprintf (temp, "% *.*f", 5+decimals, decimals, degrees);
  if (!strncmp (temp, "-18", 3))
    temp[0] = ' ';
  return temp;
}


Constituent::Constituent ():
name("default"),
speed(0) {
}


Constituent::Constituent (uint16_t numYears):
name("zero"),
speed(0) {
  (Vₒ+u).resize (numYears, 0);
  f.resize (numYears, 1);
}


void Constituent::resize (uint16_t numYears) {
  name = "zero";
  speed = 0;
  (Vₒ+u).resize (numYears, 0);
  f.resize (numYears, 1);
}


Constituent &Constituent::operator= (const Constituent &x) {
  name = x.name;
  speed = x.speed;
  (Vₒ+u).resize (x.(Vₒ+u).size());
  (Vₒ+u) = x.(Vₒ+u);
  f.resize (x.f.size());
  f = x.f;
  return *this;
}


Constituent::Constituent (const Constituent &x):
name(x.name),
speed(x.speed) {
  (Vₒ+u).resize (x.(Vₒ+u).size());
  (Vₒ+u) = x.(Vₒ+u);
  f.resize (x.f.size());
  f = x.f;
}


Constituent &Constituent::operator+= (const Constituent &x) {
  assert ((Vₒ+u).size() == x.(Vₒ+u).size());
  assert (f.size() == x.f.size());
  speed += x.speed;
  (Vₒ+u) += x.(Vₒ+u);
  f *= x.f;
  name = "nameless";
  return *this;
}


Constituent &Constituent::operator*= (double x) {
  speed *= x;
  (Vₒ+u) *= x;
  f = pow (f, fabs(x));
  name = "nameless";
  return *this;
}


Constituent operator+ (const Constituent &x, const Constituent &y) {
  Constituent temp (x);
  temp += y;
  return temp;
}


Constituent operator* (double x, const Constituent &y) {
  Constituent temp (y);
  temp *= x;
  return temp;
}


Constituent operator* (const Constituent &y, double x) {
  Constituent temp (y);
  temp *= x;
  return temp;
}


Constituent::Constituent  (const std::string &name_in,
                           const std::valarray<double> &V_coefficients,
                           const std::valarray<double> &u_coefficients,
                           uint_fast8_t f_formula,
                           year_t firstYear,
                           year_t lastYear,
                           year_t epochForSpeed):
  name(name_in) {
  assert (lastYear >= firstYear);
  assert (firstYear > 0);
  assert (lastYear <= 4000);
  assert (epochForSpeed > 0);
  assert (epochForSpeed <= 4000);
  assert (V_coefficients.size() == numVterms);
  assert (u_coefficients.size() == numuterms);

  (Vₒ+u).resize (lastYear - firstYear + 1);
  f.resize (lastYear - firstYear + 1);

  {
    const std::valarray<double> speeds (V_terms(startYear(epochForSpeed),1));
    speed = ((speeds * V_coefficients).sum()
            // M₁ special case; see SP 98 paragraph 124.
            + speeds[p_index] * u_coefficients[Q_index])
            / hoursPerJulianCentury_double;
  }

  for (year_t y (firstYear); y<=lastYear; ++y) {
    const Congen::time_t t0 (startYear(y));
    const Congen::time_t t1 (midYear(y));
    std::valarray<double> u_terms (numuterms);

    const std::valarray<double> midyear_terms_t1 (midyear_terms(t1,0));
    const double N_t1 (midyear_terms_t1[midyear_N_index]);
    const double p_t1 (midyear_terms_t1[midyear_p_index]);
    const double I_t1 (I(N_t1));
    u_terms[ξ_index] = ξ(N_t1);
    u_terms[ν_index] = ν(N_t1);
    u_terms[ν′_index] = ν′(N_t1);
    u_terms[2ν″_index] = 2ν″(N_t1);
    const double P_t1 (P(p_t1,u_terms[ξ_index]));
    u_terms[Q_index] = Q(P_t1);
    u_terms[R_index] = R(P_t1,I_t1);
    u_terms[Qᵤ_index] = Qᵤ(P_t1,u_terms[Q_index]);

    (Vₒ+u)[y-firstYear] = (V_terms(t0,0) * V_coefficients).sum()
                              + (u_terms * u_coefficients).sum();
    f[y-firstYear] = ::f(f_formula,I_t1,P_t1,u_terms[ν_index]);
  }
}


Constituent::Constituent  (const std::string &name_in,
                           const std::valarray<double> &V_coefficients,
                           const std::vector<Satellite> &satellites,
                           year_t firstYear,
                           year_t lastYear,
                           year_t epochForSpeed):
  name(name_in) {
  assert (lastYear >= firstYear);
  assert (firstYear > 0);
  assert (lastYear <= 4000);
  assert (epochForSpeed > 0);
  assert (epochForSpeed <= 4000);
  assert (V_coefficients.size() == numVterms);

  (Vₒ+u).resize (lastYear - firstYear + 1);
  f.resize (lastYear - firstYear + 1);

  {
    const std::valarray<double> speeds (V_terms(startYear(epochForSpeed),1));
    // No Q special case here.
    speed = (speeds * V_coefficients).sum() / hoursPerJulianCentury_double;
  }

  for (year_t y (firstYear); y<=lastYear; ++y) {
    const Congen::time_t t0 (startYear(y));
    const Congen::time_t t1 (midYear(y));

    const std::valarray<double> midyear_terms_t1 (midyear_terms(t1,0));
    const double N_t1 (midyear_terms_t1[midyear_N_index]);
    const double p_t1 (midyear_terms_t1[midyear_p_index]);
    const double p₁_t1 (midyear_terms_t1[midyear_p₁_index]);

    // See Foreman p. 26.
    double cossum (1);
    double sinsum (0);
    for (std::vector<Satellite>::const_iterator it (satellites.begin());
         it != satellites.end();
         ++it) {
      double satangle (it->Δp*p_t1 + it->ΔN*N_t1 + it->Δp₁*p₁_t1 + it->α);
      cossum += it->r * cosd(satangle);
      sinsum += it->r * sind(satangle);
    }

    (Vₒ+u)[y-firstYear] = (V_terms(t0,0) * V_coefficients).sum()
                          + atan2d (sinsum, cossum);
    f[y-firstYear] = sqrt (sinsum*sinsum + cossum*cossum);
  }
}


Constituent::Constituent (const std::string &name_in,
                          const std::valarray<double> &coefficients,
                          year_t firstYear,
                          year_t lastYear,
                          year_t epochForSpeed) {
  static year_t firstYear_cached=0, lastYear_cached=0, epochForSpeed_cached=0;
  static std::vector<Constituent> terms (numCompoundBases);

  assert (lastYear >= firstYear);
  assert (firstYear > 0);
  assert (lastYear <= 4000);
  assert (epochForSpeed > 0);
  assert (epochForSpeed <= 4000);
  assert (coefficients.size() == numCompoundBases);

  if (firstYear != firstYear_cached ||
      lastYear != lastYear_cached ||
      epochForSpeed != epochForSpeed_cached) {
    firstYear_cached = firstYear;
    lastYear_cached = lastYear;
    epochForSpeed_cached = epochForSpeed;
    terms[O₁_index] = Congen::Constituent ("O₁",
      std::valarray<double> ((const double[]){1,-2,1,0,0,90},numVterms),
      std::valarray<double> ((const double[]){2,-1,0,0,0,0,0},numuterms),
      75, firstYear, lastYear, epochForSpeed);
    terms[K₁_index] = Congen::Constituent ("K₁",
      std::valarray<double> ((const double[]){1,0,1,0,0,-90},numVterms),
      std::valarray<double> ((const double[]){0,0,-1,0,0,0,0},numuterms),
      227, firstYear, lastYear, epochForSpeed);
    terms[P₁_index] = Congen::Constituent ("P₁",
      std::valarray<double> ((const double[]){1,0,-1,0,0,90},numVterms),
      std::valarray<double> ((const double[]){0,0,0,0,0,0,0},numuterms),
      1, firstYear, lastYear, epochForSpeed);
    terms[M₂_index] = Congen::Constituent ("M₂",
      std::valarray<double> ((const double[]){2,-2,2,0,0,0},numVterms),
      std::valarray<double> ((const double[]){2,-2,0,0,0,0,0},numuterms),
      78, firstYear, lastYear, epochForSpeed);
    terms[S₂_index] = Congen::Constituent ("S₂",
      std::valarray<double> ((const double[]){2,0,0,0,0,0},numVterms),
      std::valarray<double> ((const double[]){0,0,0,0,0,0,0},numuterms),
      1, firstYear, lastYear, epochForSpeed);
    terms[N₂_index] = Congen::Constituent ("N₂",
      std::valarray<double> ((const double[]){2,-3,2,1,0,0},numVterms),
      std::valarray<double> ((const double[]){2,-2,0,0,0,0,0},numuterms),
      78, firstYear, lastYear, epochForSpeed);
    terms[L₂_index] = Congen::Constituent ("L₂",
      std::valarray<double> ((const double[]){2,-1,2,-1,0,180},numVterms),
      std::valarray<double> ((const double[]){2,-2,0,0,0,-1,0},numuterms),
      215, firstYear, lastYear, epochForSpeed);
    terms[K₂_index] = Congen::Constituent ("K₂",
      std::valarray<double> ((const double[]){2,0,2,0,0,0},numVterms),
      std::valarray<double> ((const double[]){0,0,0,-1,0,0,0},numuterms),
      235, firstYear, lastYear, epochForSpeed);
    terms[Q₁_index] = Congen::Constituent ("Q₁",
      std::valarray<double> ((const double[]){1,-3,1,1,0,90},numVterms),
      std::valarray<double> ((const double[]){2,-1,0,0,0,0,0},numuterms),
      75, firstYear, lastYear, epochForSpeed);
    terms[ν₂_index] = Congen::Constituent ("ν₂",
      std::valarray<double> ((const double[]){2,-3,4,-1,0,0},numVterms),
      std::valarray<double> ((const double[]){2,-2,0,0,0,0,0},numuterms),
      78, firstYear, lastYear, epochForSpeed);
    terms[S₁_index] = Congen::Constituent ("S₁",
      std::valarray<double> ((const double[]){1,0,0,0,0,0},numVterms),
      std::valarray<double> ((const double[]){0,0,0,0,0,0,0},numuterms),
      1, firstYear, lastYear, epochForSpeed);
    terms[M₁_DUTCH_index] = Congen::Constituent ("M₁-DUTCH",
      std::valarray<double> ((const double[]){1,-1,1,1,0,-90},numVterms),
      std::valarray<double> ((const double[]){0,-1,0,0,0,0,-1},numuterms),
      206, firstYear, lastYear, epochForSpeed);
    terms[λ₂_index] = Congen::Constituent ("λ₂",
      std::valarray<double> ((const double[]){2,-1,0,1,0,180},numVterms),
      std::valarray<double> ((const double[]){2,-2,0,0,0,0,0},numuterms),
      78, firstYear, lastYear, epochForSpeed);
  }

  resize (lastYear-firstYear+1);
  for (uint_fast8_t i(0); i<numCompoundBases; ++i)
    operator+= (coefficients[i] * terms[i]);
  name = name_in;
}


unsigned parseLegacyInput (std::istream &is,
                           year_t firstYear,
                           year_t lastYear,
                           year_t epochForSpeed,
                           std::vector<Constituent> &constituents) {
  unsigned lineno (0);

  // Initial state and state after reading a normal line:
  //   good: 1 bad: 0 eof: 0 fail: 0
  // After reading a nonempty line terminated by EOF:
  //   good: 0 bad: 0 eof: 1 fail: 0
  // After reading an empty line terminated by EOF or trying to read
  // again after the previous case:
  //   good: 0 bad: 0 eof: 1 fail: 1

  // Initial state and after reading a valid field:
  //   good: 1 bad: 0 eof: 0 fail: 0
  // After reading an invalid field:
  //   good: 0 bad: 0 eof: 0 fail: 1
  // After reading a valid field terminated by EOF:
  //   good: 0 bad: 0 eof: 1 fail: 0
  // After trying to read again after that:
  //   good: 0 bad: 0 eof: 1 fail: 1
  // If the previous field had trailing whitespace but no valid field
  // actually follows it, you see the following:
  //   good: 1 bad: 0 eof: 0 fail: 0
  //   good: 0 bad: 0 eof: 1 fail: 1

  // DOA?
  if (!is.good())
    return 1;

  while (is.good()) {
    std::string buf;
    ++lineno;
    getline (is, buf);
    if (is.fail())
      return 0;
    if (buf.empty())
      continue;
    if (buf[0] == '#')
      continue;
    std::istringstream iss (buf);
    std::string name, kind;
    iss >> name >> kind;
    if (!iss.good())
      return lineno;
    if (kind == "Basic") {
      std::valarray<double> V_coefficients (0., numVterms);
      for (uint_fast8_t i(0); i<numVterms; ++i) {
        iss >> V_coefficients[i];
        if (!iss.good())
          return lineno;
      }
      std::valarray<double> u_coefficients (0., numuterms);
      for (uint_fast8_t i(0); i<6; ++i) {
        iss >> u_coefficients[i];
        if (!iss.good())
          return lineno;
      }
      unsigned f_formula;
      iss >> f_formula;
      if (iss.fail())
        return lineno;
      constituents.push_back (Constituent (name, V_coefficients,
        u_coefficients, f_formula, firstYear, lastYear, epochForSpeed));
    } else if (kind == "Doodson") {
      std::valarray<double> V_coefficients (0., numVterms);
      for (uint_fast8_t i(0); i<numVterms; ++i) {
        iss >> V_coefficients[i];
        if (!iss.good())
          return lineno;
      }
      unsigned numsats;
      iss >> numsats;
      if (iss.fail())
        return lineno;
      std::vector<Satellite> satellites;
      for (unsigned i(0); i<numsats; ) {
        if (iss.eof()) {
          ++lineno;
          getline (is, buf);
          if (is.fail())
            return lineno;
          iss.clear();
          iss.str (buf);
        }
        Satellite satellite;
        iss >> satellite.Δp;
        if (iss.fail()) {
          if (iss.eof())
            continue;
          else
            return lineno;
        }
        iss >> satellite.ΔN;
        if (!iss.good())
          return lineno;
        iss >> satellite.Δp₁;
        if (!iss.good())
          return lineno;
        iss >> satellite.α;
        if (!iss.good())
          return lineno;
        std::string tempr;
        iss >> tempr;
        if (iss.fail())
          return lineno;
        std::istringstream temprs (tempr);
        temprs >> satellite.r;
        if (temprs.fail())
          return lineno;
        ++i;
        if (tempr.find ('R') != std::string::npos)
          continue;
        satellite.ΔN = -satellite.ΔN;
        satellite.α *= 360;
        satellites.push_back (satellite);
      }
      constituents.push_back (Constituent (name, V_coefficients, satellites,
        firstYear, lastYear, epochForSpeed));
    } else if (kind == "Compound") {
      std::valarray<double> coefficients (0., numCompoundBases);
      for (uint_fast8_t i(0); i<numCompoundBases; ++i) {
        iss >> coefficients[i];
        if (iss.fail()) {
          coefficients[i] = 0;
          break;
        }
      }
      constituents.push_back (Constituent (name, coefficients, firstYear,
        lastYear, epochForSpeed));
    } else
      return lineno;
  }

  return 0;
}


void tables () {

  printf (
    "════════════════════════════════════════════════════════════════════════════════\n"
    "          MEAN LONGITUDE OF SOLAR AND LUNAR ELEMENTS FOR CENTURY YEARS\n"
    "────────────────────────────────────────┬───────┬───────┬───────┬───────┬───────\n"
    "                                        │       │ Solar │       │ Lunar │Moon's\n"
    "       Epoch, Gregorian calendar        │  Sun  │perigee│ Moon  │perigee│ node\n"
    "       Greenwich mean civil time        │   h   │   p₁  │   s   │   p   │   N\n"
    "────────────────────────────────────────┼───────┼───────┼───────┼───────┼───────\n"
    "                                        │   °   │   °   │   °   │   °   │   °\n");
  for (year_t year (1600); year <= 2000; year += 100) {
    const Congen::time_t t (startYear (year));
    std::valarray<double> V (V_terms (t, 0));
    printf ("%u, Jan. 1, 0 hour                    │%s│%s│%s│%s│%s\n",
            year,
            normalize(V[h_index],3).c_str(),
            normalize(V[p₁_index],3).c_str(),
            normalize(V[s_index],3).c_str(),
            normalize(V[p_index],3).c_str(),
            normalize(midyear_terms(t,0)[midyear_N_index],3).c_str());
  }
  printf ("════════════════════════════════════════╧═══════╧═══════╧═══════╧═══════╧═══════\n");

  printf ("\n"
    "RATE OF CHANGE IN MEAN LONGITUDE OF SOLAR AND LUNAR ELEMENTS (EPOCH, JAN. 1, 1900)\n"
    "                       ────────────────────┬─────────────\n"
    "                             Elements      │Per solar day\n"
    "                       ────────────────────┼─────────────\n"
    "                                           │       °\n");
  {
    const Congen::time_t t (startYear (1900));
    std::valarray<double> speeds (V_terms(t,1) / daysPerJulianCentury_double);
    printf ("                       Sun (h)             │  %10.7f\n",
      speeds[h_index]);
    printf ("                       Solar perigee (p₁)  │  %10.7f\n",
      speeds[p₁_index]);
    printf ("                                           │\n");
    printf ("                       Moon (s)            │  %10.7f\n",
      speeds[s_index]);
    printf ("                       Lunar perigee (p)   │  %10.7f\n",
      speeds[p_index]);
    printf ("                       Moon's node (N)     │  %10.7f\n",
      midyear_terms(t,1)[midyear_N_index]/daysPerJulianCentury_double);
    printf ("                       ════════════════════╧═════════════\n");
  }

  printf ("\n"
    "    Table 4.--Mean longitude of lunar and solar elements at Jan. 1, 0 hour,\n"
    "           Greenwich mean civil time, of each year from 1800 to 2000\n");
  tab4part (1800);
  printf ("\n"
    "    Table 4.--Mean longitude of lunar and solar elements at Jan. 1, 0 hour,\n"
    "      Greenwich mean civil time, of each year from 1800 to 2000--Continued\n");
  tab4part (1900);

  printf ("\n"
    "         Table 6.--Values of I, ν, ξ, ν′, and 2ν″ for each degree of N.\n"
    "═══╤══════╤══════╤══════╤══════╤═══════╦═══════╤══════╤══════╤══════╤══════╤═══\n"
    " N │   I  │   ν  │   ξ  │   ν′ │  2ν″  ║    I  │   ν  │   ξ  │   ν′ │  2ν″ │ N\n"
    "───┼──────┼──────┼──────┼──────┼───────║───────┼──────┼──────┼──────┼──────┼───\n"
    " ° │   °  │   °  │   °  │   °  │   °   ║    °  │   °  │   °  │   °  │   °  │ °\n");
  for (uint_fast16_t N(0); N<=180; ++N) {
    printf ("%3u│%s│%s│%s│%s│%s ║ %s│%s│%s│%s│%s│%3u\n",
      N,
      normalize(I(N),2).c_str(),
      normalize(ν(N),2).c_str(),
      normalize(ξ(N),2).c_str(),
      normalize(ν′(N),2).c_str(),
      normalize(2ν″(N),2).c_str(),
      normalize(I(360-N),2).c_str(),
      snormalize(ν(360-N),2).c_str()+1,
      snormalize(ξ(360-N),2).c_str()+1,
      snormalize(ν′(360-N),2).c_str()+1,
      snormalize(2ν″(360-N),2).c_str()+1,
      360-N);
    if (N % 3 == 0 && N < 180)
      printf ("   │      │      │      │      │       ║       │      │      │      │      │\n");
  }
  printf ("───┴──────┴──────┴──────┴──────┴───────╨───────┴──────┴──────┴──────┴──────┴───\n");

  printf ("\n"
    "                   Table 7.--Log Rₐ for amplitude of constituent L₂\n"
    "═══╤══════╤══════╤══════╤══════╤══════╤══════╤══════╤══════╤══════╤══════╤══════╤══════\n");
  printf ("P\\I");
  for (uint_fast8_t I(18); I<=29; ++I)
    printf ("│%4u  ", I);
  printf ("\n───");
  for (uint_fast8_t I(18); I<=29; ++I)
    printf ("┼──────");
  printf ("\n °");
  for (uint_fast8_t I(18); I<=29; ++I)
    printf (" │  °  ");
  printf ("\n");
  for (uint_fast16_t P(0); P<=360; P += 5) {
    printf ("%3u", P);
    for (uint_fast8_t I(18); I<=29; ++I)
      printf ("│%6.4f",
        mangled_log10(Rₐ(P,I)));
    printf ("\n");
  }
  printf ("───┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────┴──────\n");

  printf ("\n"
    "              Table 8.--Values of R for argument of constituent L₂\n"
    "  ═══╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════\n");
  printf ("  P\\I");
  for (uint_fast8_t I(18); I<=29; ++I)
    printf ("│%4u ", I);
  printf ("\n  ───");
  for (uint_fast8_t I(18); I<=29; ++I)
    printf ("┼─────");
  printf ("\n   °");
  for (uint_fast8_t I(18); I<=29; ++I)
    printf (" │  ° ");
  printf ("\n");
  for (uint_fast16_t P(0); P<=360; P += 5) {
    printf ("  %3u", P);
    for (uint_fast8_t I(18); I<=29; ++I)
      printf ("│%s", snormalize(R(P,I),1).c_str()+1);
    printf ("\n");
  }
  printf ("  ───┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────┴─────\n");

  printf ("\n"
    "                Table 9.--Log Qₐ for amplitude of constituent M₁\n"
    "               ═══╤═══════╦════╤═══════╦════╤═══════╦════╤══════\n"
    "                P │Log Qₐ ║  P │Log Qₐ ║  P │Log Qₐ ║  P │Log Qₐ\n"
    "               ───┼───────║────┼───────║────┼───────║────┼──────\n"
    "                ° │   °   ║  ° │   °   ║  ° │   °   ║  ° │   °\n");
  for (uint_fast16_t P(0); P<=90; ++P) {
    printf ("               %3u│%6.4f ║ %3u│%6.4f ║ %3u│%6.4f ║ %3u│%6.4f\n",
            P, mangled_log10(Qₐ(P)),
            180+P, mangled_log10(Qₐ(180+P)),
            180-P, mangled_log10(Qₐ(180-P)),
            360-P, mangled_log10(Qₐ(360-P)));
    if (P % 3 == 0 && P < 90)
      printf ("                  │       ║    │       ║    │       ║    │\n");
  }
  printf ("               ───┴───────╨────┴───────╨────┴───────╨────┴──────\n");

  printf ("\n"
    "             Table 10.--Values of Q for argument of constituent M₁\n"
    "═══╤═════╦═══╤═════╦═══╤═════╦═══╤═════╦═══╤═════╦═══╤═════╦═══╤═════╦═══╤═════\n"
    " P │  Q  ║ P │  Q  ║ P │  Q  ║ P │  Q  ║ P │  Q  ║ P │  Q  ║ P │  Q  ║ P │  Q\n"
    "───┼─────║───┼─────║───┼─────║───┼─────║───┼─────║───┼─────║───┼─────║───┼─────\n"
    " ° │  °  ║ ° │  °  ║ ° │  °  ║ ° │  °  ║ ° │  °  ║ ° │  °  ║ ° │  °  ║ ° │  °\n");
  for (uint_fast16_t P(0); P<=45; ++P) {
    printf ("%3u│%s║%3u│%s║%3u│%s║%3u│%s║%3u│%s║%3u│%s║%3u│%s║%3u│%s\n",
            P,     normalize(Q(P),1).c_str(),
            P+45,  normalize(Q(P+45),1).c_str(),
            P+90,  normalize(Q(P+90),1).c_str(),
            P+135, normalize(Q(P+135),1).c_str(),
            P+180, normalize(Q(P+180),1).c_str(),
            P+225, normalize(Q(P+225),1).c_str(),
            P+270, normalize(Q(P+270),1).c_str(),
            P+315, normalize(Q(P+315),1).c_str());
    if (P % 3 == 0 && P < 45)
      printf ("   │     ║   │     ║   │     ║   │     ║   │     ║   │     ║   │     ║   │\n");
  }
  printf ("───┴─────╨───┴─────╨───┴─────╨───┴─────╨───┴─────╨───┴─────╨───┴─────╨───┴─────\n");

  printf ("\n"
    "    Table 14.--Node factor f for middle of each year, 1850 to 1999\n"
    "    (Not all figures agree with SP 98 to the quoted precision)\n");
  for (year_t y1 (1850); y1 < 2000; y1 += 10) {
    printf ("═══════════╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════╤═════\n");
    printf ("Constituent");
    for (year_t y (y1); y < y1+10; ++y)
      printf ("│%5u", y);
    printf ("\n");
    printf ("───────────");
    for (year_t y (y1); y < y1+10; ++y)
      printf ("┼─────");
    printf ("\n");
    tab14row ("J₁         ", 76, y1);
    tab14row ("K₁         ", 227, y1);
    tab14row ("K₂         ", 235, y1);
    printf ("           │     │     │     │     │     │     │     │     │     │\n");
    tab14row ("L₂         ", 215, y1);
    tab14row ("M₁         ", 206, y1);
    printf ("           │     │     │     │     │     │     │     │     │     │\n");
    tab14row ("M₂         ", 78, y1);
    tab14row ("M₃         ", 149, y1);
    printf ("           │     │     │     │     │     │     │     │     │     │\n");
    tab14row ("O₁         ", 75, y1);
    tab14row ("OO₁        ", 77, y1);
    printf ("           │     │     │     │     │     │     │     │     │     │\n");
    tab14row ("Mf         ", 74, y1);
    tab14row ("Mm         ", 73, y1);
  }

}


void makeArrays (const std::vector<Constituent> &constituents,
                 char **&names,
                 double *&speeds,
                 float **&equilibriumArgs,
                 float **&nodeFactors) {
  unsigned numc (constituents.size());
  assert (numc);
  names = new char* [numc];
  assert (names);
  speeds = new double [numc];
  assert (speeds);
  equilibriumArgs = new float* [numc];
  assert (equilibriumArgs);
  nodeFactors = new float* [numc];
  assert (nodeFactors);
  unsigned numYears (constituents[0].f.size());
  assert (numYears);

  for (unsigned i(0); i<numc; ++i) {
    const Constituent &c (constituents[i]);
    names[i] = new char [c.name.size()+1];
    assert (names[i]);
    strcpy (names[i], c.name.c_str());
    assert (c.speed >= 0);
    assert (c.speed < 214.748);
    speeds[i] = c.speed;
    assert (numYears == c.(Vₒ+u).size());
    assert (numYears == c.f.size());
    equilibriumArgs[i] = new float [numYears];
    assert (equilibriumArgs[i]);
    nodeFactors[i] = new float [numYears];
    assert (nodeFactors[i]);
    for (unsigned j(0); j<numYears; ++j) {
      // Yes, this is inefficient, but getting normalize right once
      // was hard enough.  The boundary cases with that are insane.
      std::istringstream(normalize(c.(Vₒ+u)[j],2)) >> equilibriumArgs[i][j];
      nodeFactors[i][j] = c.f[j];
    }
  }
}


}
