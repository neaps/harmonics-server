// $Id: diff_congen_output.cc 4154 2012-01-05 00:45:39Z flaterco $

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

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <vector>
#include <math.h>
#include <valarray>
#include <assert.h>


static void check (std::istream &is) {
  if (is.fail()) {
    std::cerr << "Unexpected end of file or bad data\n";
    exit (-1);
  }
}


static std::string getHarmLine (std::istream &is) {
  std::string buf ("#");
  while (buf[0] == '#') {
    getline (is, buf);
    check (is);
  }
  return buf;
}


static unsigned getUnsigned (std::istream &is) {
  std::istringstream bufs (getHarmLine (is));
  unsigned numConst;
  bufs >> numConst;
  check (bufs);
  return numConst;
}


static void getSpeed (std::string &name, double &speed, std::istream &is) {
  std::istringstream bufs (getHarmLine (is));
  bufs >> name >> speed;
  check (bufs);
}


static std::valarray<double> getDoubles (unsigned numYears, std::istream &is) {
  std::valarray<double> args (numYears);
  (void) getHarmLine (is); // ignore name
  std::istringstream bufs (getHarmLine (is));
  for (unsigned i(0); i<numYears; ) {
    bufs >> args[i];
    if (bufs.fail()) {
      bufs.clear();
      bufs.str (getHarmLine (is));
    } else
      ++i;
  }
  return args;
}


int main (int argc, char **argv) {
  if (argc != 3) {
    std::cerr << "Usage: diff_congen_output file1.txt file2.txt\n"
      "\n"
      "    congen:  constituent generator.\n"
      "    Copyright (C) 1997  David Flater.\n"
      "\n"
      "    This program is free software: you can redistribute it and/or modify\n"
      "    it under the terms of the GNU General Public License as published by\n"
      "    the Free Software Foundation, either version 3 of the License, or\n"
      "    (at your option) any later version.\n"
      "\n"
      "    This program is distributed in the hope that it will be useful,\n"
      "    but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
      "    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
      "    GNU General Public License for more details.\n"
      "\n"
      "    You should have received a copy of the GNU General Public License\n"
      "    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n";
    exit (-1);
  }

  std::ifstream file1 (argv[1]), file2 (argv[2]);
  if (file1.fail()) {
    std::cerr << "Error trying to open " << argv[1] << std::endl;
    exit (-1);
  }
  if (file2.fail()) {
    std::cerr << "Error trying to open " << argv[2] << std::endl;
    exit (-1);
  }

  unsigned numConst1 (getUnsigned (file1));
  unsigned numConst2 (getUnsigned (file2));
  if (numConst1 != numConst2) {
    std::cerr << "Different number of constituents ("
              << numConst1 << " vs. " << numConst2 << ")\n";
    exit (-1);
  }

  std::vector<std::string> names (numConst1);
  for (unsigned i(0); i<numConst1; ++i) {
    std::string name1, name2;
    double speed1, speed2;
    getSpeed (name1, speed1, file1);
    getSpeed (name2, speed2, file2);
    if (name1 != name2) {
      std::cerr << "Constituent name mismatch ("
		<< name1 << " vs. " << name2 << ")\n";
      exit (-1);
    }
    names[i] = name1;
    double delta (fabs (speed1 - speed2));
    if (delta > 0)
      std::cout << "Speed of " << name1 << " mismatch (delta "
                << delta << ")\n";
  }

  unsigned startYear1 (getUnsigned (file1));
  unsigned startYear2 (getUnsigned (file2));
  if (startYear1 != startYear2) {
    std::cerr << "Different start year ("
              << startYear1 << " vs. " << startYear2 << ")\n";
    exit (-1);
  }

  unsigned numYears1 (getUnsigned (file1));
  unsigned numYears2 (getUnsigned (file2));
  if (numYears1 != numYears2) {
    std::cerr << "Different number of years ("
              << numYears1 << " vs. " << numYears2 << ")\n";
    exit (-1);
  }
  assert (numYears1 > 0);

  for (unsigned i(0); i<numConst1; ++i) {
    std::valarray<double> args1 (getDoubles (numYears1, file1));
    std::valarray<double> args2 (getDoubles (numYears1, file2));
    double maxDelta ((args1-args2).apply(fabs).max());
    if (maxDelta > .01001)
      std::cout << "Argument of " << names[i] << " mismatch (max delta "
                << maxDelta << ")\n";
  }

  (void) getHarmLine (file1);  // Ignore *END*
  (void) getHarmLine (file2);  // Ignore *END*
  (void) getHarmLine (file1);  // Ignore second year count
  (void) getHarmLine (file2);  // Ignore second year count

  for (unsigned i(0); i<numConst1; ++i) {
    std::valarray<double> args1 (getDoubles (numYears1, file1));
    std::valarray<double> args2 (getDoubles (numYears1, file2));
    double maxDelta ((args1-args2).apply(fabs).max());
    if (maxDelta > .0001001)
      std::cout << "Node factor of " << names[i] << " mismatch (max delta "
                << maxDelta << ")\n";
  }

  return 0;
}
