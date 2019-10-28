// $Id: congen.cc 4154 2012-01-05 00:45:39Z flaterco $

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

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <Congen>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <iostream>
#include <algorithm>
#include <cstring>

#ifdef HAVE_LIBTCD
#include "tcd.h"
#endif


#if Congen_interfaceRevision != 0
#error trying to compile with an incompatible version of libcongen
#endif


static void printSpeed (const Congen::Constituent &c) {
  printf ("%-27s %11.7f\n", c.name.c_str(), c.speed);
}


static void printArgs (const Congen::Constituent &c, uint_fast16_t numYears) {
  printf ("%s\n", c.name.c_str());
  uint_fast8_t colctr (0);
  for (uint_fast16_t i(0); i<numYears; ++i) {
    if (colctr)
      putchar (' ');
    printf ("%s", Congen::normalize(c.(Vₒ+u)[i],2).c_str());
    if (++colctr == 10) {
      putchar ('\n');
      colctr = 0;
    }
  }
  if (colctr)
    putchar ('\n');
}


static void printNods (const Congen::Constituent &c, uint_fast16_t numYears) {
  printf ("%s\n", c.name.c_str());
  uint_fast8_t colctr (0);
  for (uint_fast16_t i(0); i<numYears; ++i) {
    if (colctr)
      putchar (' ');
    printf ("%6.4f", c.f[i]);
    if (++colctr == 10) {
      putchar ('\n');
      colctr = 0;
    }
  }
  if (colctr)
    putchar ('\n');
}


static Congen::year_t getYear (char const * const arg) {
  assert (arg);
  Congen::year_t y;
  if (sscanf (arg, "%" SCNu16, &y) != 1) {
    fprintf (stderr, "Bad year: %s\n", arg);
    exit (-1);
  }
  if (y < 1 || y > 4000) {
    fprintf (stderr, "Year out of range: %s\n", arg);
    exit (-1);
  }
  return y;
}


int main (int argc, char **argv) {

  bool ambitiousSpeeds (false), tables (false);
  Congen::year_t firstYear (1970), lastYear (2037);
#ifdef HAVE_LIBTCD
  char *tcdFileName (NULL);
#endif

  for (int argnum(1); argnum<argc; ++argnum) {
    if (!strcmp (argv[argnum], "-a0")) {
      fprintf (stderr,
        "-a0 is no longer supported; use -a1 (the default) instead.\n");
      exit (-1);
    } else if (!strcmp (argv[argnum], "-a1"))
      ambitiousSpeeds = false;
    else if (!strcmp (argv[argnum], "-a2"))
      ambitiousSpeeds = true;
    else if (!strcmp (argv[argnum], "-sp98test"))
      tables = true;
    else if (!strcmp (argv[argnum], "-b")) {
      ++argnum;
      if (argnum >= argc) {
        fprintf (stderr, "Year missing after -b\n");
        exit (-1);
      }
      firstYear = getYear (argv[argnum]);
    } else if (!strcmp (argv[argnum], "-e")) {
      ++argnum;
      if (argnum >= argc) {
        fprintf (stderr, "Year missing after -e\n");
        exit (-1);
      }
      lastYear = getYear (argv[argnum]);
    } else if (!strcmp (argv[argnum], "-tcd")) {
#ifdef HAVE_LIBTCD
      ++argnum;
      if (argnum >= argc) {
        fprintf (stderr, "File name missing after -tcd\n");
        exit (-1);
      }
      tcdFileName = argv[argnum];
#else
      fprintf (stderr,
        "Congen was built without TCD support.  To enable this option, you must\n"
	"install libtcd and recompile congen.\n");
      exit (-1);
#endif
    } else {
      fprintf (stderr, "Unrecognized command line option: %s\n"
        "Usage: congen [-b year] [-e year] [-a1|-a2] [-tcd filename] [-sp98test]\n"
        "              < congen_input.txt > output.txt\n"
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
	"    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n",
	argv[argnum]);
      exit (-1);
    }
  }

  if (lastYear < firstYear) {
    fprintf (stderr, "End year is before start year\n");
    exit (-1);
  }
  uint_fast16_t numYears (lastYear - firstYear + 1);

  if (tables) {
    Congen::tables();
    return 0;
  }

  Congen::year_t epochForSpeeds (1900);
  if (ambitiousSpeeds)
    epochForSpeeds = (firstYear + lastYear) / 2;

  std::vector<Congen::Constituent> constituents;
  unsigned lineno (Congen::parseLegacyInput (std::cin, firstYear, lastYear,
					     epochForSpeeds, constituents));
  if (lineno) {
    fprintf (stderr, "Error on input line %u\n", lineno);
    exit (-1);
  }

#ifdef HAVE_LIBTCD
  if (tcdFileName) {
    char **names;
    double *speeds;
    float **equilibriumArgs;
    float **nodeFactors;
    Congen::makeArrays (constituents, names, speeds, equilibriumArgs,
			nodeFactors);
    if (!create_tide_db (tcdFileName, constituents.size(), names, speeds,
			 firstYear, lastYear-firstYear+1, equilibriumArgs,
			 nodeFactors)) {
      fprintf (stderr, "create_tide_db returned nonspecific failure\n");
      exit (-1);
    }
    close_tide_db();
    return 0;
  }
#endif

  puts ("# ------------- Begin congen output -------------");
  puts ("#\n# Number of constituents");
  printf ("%u\n", constituents.size());
  puts ("#\n# Constituent speeds");
  puts ("# Format:  identifier [whitespace] speed [CR]");
  puts ("# Speed is in degrees per solar hour.");
  puts ("# Identifier is just a name for the constituent.  They are for");
  puts ("# readability only; XTide assumes that the constituents will be listed");
  puts ("# in the same order throughout this file.");
  for_each (constituents.begin(), constituents.end(), printSpeed);
  puts ("#\n# Starting year for equilibrium arguments and node factors");
  printf ("%u\n", firstYear);
  puts ("#\n\
# The following table gives equilibrium arguments for each year that\n\
# we can predict tides for.  The equilibrium argument is in degrees for\n\
# the meridian of Greenwich, at the beginning of each year.\n\
#\n\
# First line:  how many years in this table [CR]\n\
# Remainder of table:  identifier [whitespace] arg [whitespace] arg...\n\
# Carriage returns inside the table will be ignored.\n\
#\n\
# The identifiers are for readability only; XTide assumes that they\n\
# are in the same order as defined above.\n\
#\n\
# DO NOT PUT COMMENT LINES INSIDE THE FOLLOWING TABLE.\n\
# DO NOT REMOVE THE \"*END*\" AT THE END.");
  printf ("%u\n", numYears);
  for (std::vector<Congen::Constituent>::iterator it (constituents.begin());
       it != constituents.end();
       ++it)
    printArgs (*it, numYears);
  puts ("*END*\n#\n\
# Now come the node factors for the middle of each year that we can\n\
# predict tides for.\n\
#\n\
# First line:  how many years in this table [CR]\n\
# Remainder of table:  identifier [whitespace] factor [whitespace] factor...\n\
# Carriage returns inside the table will be ignored.\n\
#\n\
# The identifiers are for readability only; XTide assumes that they\n\
# are in the same order as defined above.\n\
#\n\
# DO NOT PUT COMMENT LINES INSIDE THE FOLLOWING TABLE.\n\
# DO NOT REMOVE THE \"*END*\" AT THE END.");
  printf ("%u\n", numYears);
  for (std::vector<Congen::Constituent>::iterator it (constituents.begin());
       it != constituents.end();
       ++it)
    printNods (*it, numYears);
  puts ("*END*\n#\n# ------------- End congen output -------------");

  return 0;
}
