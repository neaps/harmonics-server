#!/bin/sh
# 	$Id: harmgen.sh 5998 2015-06-04 23:10:18Z flaterco $	
#
# This script is meant to be invoked by the harmgen program, not run directly.
#
#   harmgen:  Derive harmonic constants from water level observations.
#   Copyright (C) 1998  David Flater.
#
#   This program is free software: you can redistribute it and/or modify
#   it under the terms of the GNU General Public License as published by
#   the Free Software Foundation, either version 3 of the License, or
#   (at your option) any later version.
# 
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#
# This is just a trick to avoid invoking harmgen.sh directly.
if [ "$1" != "albatross" ]; then
  echo "You probably meant to run the harmgen program, not the harmgen.sh script."
  exit 1
fi

# Setting octave as the pound-bang shell at the top did not work.
#
# Somewhere between Octave version 3.4.3 (worked) and 4.0.0 (did not work) it
# became necessary to specify -i here to make it not ignore the function
# definition.  Simply adding other code before the keyword function as
# suggested in the documentation did not help.
octave -i << \OCT_EOF

function harmgen

  # Parse input.  Note that Octave reads matrices column-major, and
  # that %f and %lf are exactly the same.
  inf = fopen ("oct_input", "r");
  Nc = fscanf (inf, "%u", 1)           # Number of constituents.
  p = fscanf (inf, "%f", [Nc,1])       # The speeds in degrees per hour.
  N = fscanf (inf, "%u", 1)            # Number of observations.
  ob = fscanf (inf, "%f", [3,N])       # N rows (cols) of time, height, year.
  # (year is index into node factors / equilibrium args arrays)
  Ny = fscanf (inf, "%u", 1)           # Number of years.
  nod = fscanf (inf, "%f", [Ny,Nc])    # Node factors.
  eqa = fscanf (inf, "%f", [Ny,Nc])    # Normalized equilibrium arguments.
  fclose (inf);

  # Convert to radians.
  p = p * pi / 180
  eqa = eqa * pi / 180

  # The following was translated by DWF from the APL function written
  # by Dr. Charles Read, and later extended by DWF using information
  # from Bjoern Brill.  There might be a quicker way to do this using
  # Octave's built-in least squares functions.

  # The commentary is lightly edited from Dr. Read's explanation of his APL.

  # t{b[2;]
  # t the times at which observations were made.
  t = ob(1,1:N)

  # h{b[1;]
  # h the list of observed heights.
  h = ob(2,1:N)

  # y, the year indices corresponding to t.
  y = ob(3,1:N)

  ## z{p J.X t
  ## z is a matrix containing all the products pt where p is a period and t
  ## one of the observation times.
  z = p * t

  # Factor in equilibrium arguments per Bjoern Brill.  These have been
  # normalized relative to the beginning of time (1970-01-01 00:00Z).
  for i = 1:N
    z(:,i) = z(:,i) + eqa(y(i),:).' ;
  endfor

  # Make sine and cosine arrays.
  cosz = cos(z)
  sinz = sin(z)

  # Factor in node factors per Bjoern Brill.
  for i = 1:N
    cosz(:,i) = cosz(:,i) .* nod(y(i),:).' ;
    sinz(:,i) = sinz(:,i) .* nod(y(i),:).' ;
  endfor

  # z{((1,Rh)R1),[1](2Oz),[1]1Oz
  # z becomes a matrix whose first row is a row of N 1's, then rows of cos pt_i
  # (i=1 to N) for the various p; then rows of sin pt_i.
  #
  # It's no longer quite pt_i since eqa and nod were factored in:
  #   c_w(t) = node_w(t) * cos(speed_w * t + equi_w(t))
  #   s_w(t) = node_w(t) * sin(speed_w * t + equi_w(t))
  z = [ones(1, N); cosz; sinz]

  # zt{\\bOz
  # Form the transpose of z.
  #
  # sprod{z+.Xzt
  # sprod is the matrix of scalar products of the "trial vectors" (1....1) (cos
  # p_1t_1.......cos p_1t_N) and so forth with each other.
  #
  # a{h+.Xzt+.X(L\b%sprod)
  # For an answer, you use h matrix multiplied by z transpose, matrix
  # multiplied by the inverse of the square matrix of inner products. h is a
  # row vector, so it counts as a 1 by N matrix.  zt is N by 2m+1, where m is
  # the number of periods; sprod is (2m+1 by N) times (N by 2m+1) so, 2m+1 by
  # 2m+1; so you get an answer which is 1 by 2m+1, a row of answer
  # coefficients.
  #
  # A literal translation of all that would be:
  #   zt = z.'
  #   sprod = z * zt
  #   a = h * zt * inv(sprod)
  # But I think this is equivalent.
  a = h / z

  # Split out datum, sines, and cosines.
  z0 = a(1)
  cw = a(2:Nc+1)
  sw = a(Nc+2:2*Nc+1)

  # Convert to amplitudes and phases using equations from "Computer
  # Applications to Tides in the National Ocean Survey."
  amp = sqrt (cw .* cw + sw .* sw)
  phase = atan2 (sw, cw) * 180 / pi

  # Drop off the output for harmgen.c to pick up.
  outf = fopen ("oct_output", "w");
  fprintf (outf, "%.16f\n", z0);
  for i = 1:Nc
    fprintf (outf, "%.16f %.16f\n", amp(i), phase(i));
  endfor
  fclose (outf);

endfunction

# This is the "main program" of the Octave script.

# Turning off verbose output saves lots of CPU.
if (exist ("OCTAVE_VERSION") == 5)
  # 2.9.x and later
  silent_functions(1)
else
  # 2.1.73 and earlier
  silent_functions=1
endif

harmgen                # Do the math.

OCT_EOF
