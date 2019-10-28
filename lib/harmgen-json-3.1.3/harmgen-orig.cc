// $Id: harmgen-orig.cc 6979 2019-02-02 21:03:36Z flaterco $

/*  harmgen:  Derive harmonic constants from water level observations.
    Copyright (C) 1998  David Flater.

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
#include <string.h>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <algorithm>

#ifndef require
#define require(expr)         \
  {                           \
    bool requireExpr((expr)); \
    assert(requireExpr);      \
  }
#endif

#if Congen_interfaceRevision != 0
#error trying to compile with an incompatible version of libcongen
#endif

// In units 1.85, year means tropicalyear, and month means 1/12 of a
// tropicalyear.  To reproduce the results below, you need to say
// gregorianyear not year and (1/12) gregorianyear not month.
static const uint32_t avgSecPerYear(31556952);
static const double avgHoursPerYear(8765.82);
static const uint32_t avgSecPerMonth(2629746);

static const int64_t secPerDay(86400ll);
static const int64_t yearOneStart(-62135596800ll);

// 1970-01-01 00:00Z has the Julian day number 2440587.5 (as used by
// XTide in Timestamp.cc), but inserting J2440587.5 into a date column
// in PostgreSQL 8.2.5 yields 1969-12-31.  The value below causes the
// lastTimestamp / secPerDay + epochJulian calculation of
// last_date_on_station to yield the correct result in PostgreSQL.
static const int64_t epochJulian(2440588ll);

// Factor to convert degrees per hour to rotations per year.
static const double rotFactor(avgHoursPerYear / 360);

// Rayleigh criterion constant.  Several pairs of constituents that
// are commonly accepted for a 1-year record differ by .999953
// rotations per average Gregorian year.
static const double minRotations(.99);

// Amplitudes below this round to zero (== AMPLITUDE_EPSILON in tcd.h)
static const double amplitude_epsilon(.00005);

static bool sortBySpeed(const Congen::Constituent &x,
                        const Congen::Constituent &y)
{
  return (x.speed < y.speed);
}

// In:   fp is open, positioned at start
// Out:  fp is open, positioned at start
//       Other three parameters are set
static void findInputBounds(FILE *fp,
                            unsigned long &seriesLength,
                            int64_t &firstTimestamp,
                            int64_t &lastTimestamp)
{
  int64_t t;
  double h;
  seriesLength = 0;
  while (fscanf(fp, "%" SCNd64 " %lf", &t, &h) == 2)
  {
    if (seriesLength == 0)
    {
      firstTimestamp = lastTimestamp = t;
    }
    else
    {
      if (t < firstTimestamp)
        firstTimestamp = t;
      if (t > lastTimestamp)
        lastTimestamp = t;
    }
    ++seriesLength;
  }
  if (!feof(fp))
  {
    fprintf(stderr, "Error at line %lu of time series file\n", seriesLength + 1);
    exit(-1);
  }
  if (seriesLength == 0)
  {
    fprintf(stderr, "No data found in time series file\n");
    exit(-1);
  }
  rewind(fp);
}

// Duplicated from congen.cc in congen-1.6 (svn rev 2616 2007-08-16 00:53:06Z)
static int64_t startYear(Congen::year_t year)
{
  assert(year > 0);
  assert(year <= 4001);
  --year;
  return yearOneStart + year * 31536000ll + (year / 4 - year / 100 + year / 400) * secPerDay;
}

static Congen::year_t yearOfTimestamp(int64_t t)
{
  Congen::year_t year((t - yearOneStart) / avgSecPerYear + 1);
  if (t < startYear(year))
    --year;
  else if (t >= startYear(year + 1))
    ++year;
  assert(year > 0 && year < 4001);
  assert(startYear(year) <= t && t < startYear(year + 1));
  return year;
}

static void octfailed()
{
  fprintf(stderr, "Unexpected end of file in oct_output.  The process has failed.  Sorry.\n");
  exit(-1);
}

static void deleteConstituent(std::vector<Congen::Constituent> &constituents,
                              std::vector<double> &amp,
                              std::vector<double> &phase,
                              unsigned i)
{
  assert(i < constituents.size());
  amp.erase(amp.begin() + i);
  phase.erase(phase.begin() + i);
  constituents.erase(constituents.begin() + i);
}

static void usage()
{
  fprintf(stderr,
          "This is " PACKAGE_STRING "\n"
          "\n"
          "Usage: harmgen [--name \"Station name\"]\n"
          "               [--original_name \"Original station name\"]\n"
          "               [--station_id_context \"Organization assigning ID\"]\n"
          "               [--station_id \"ID\"]\n"
          "               [--coordinates N.NNNNN N.NNNNN]    -90..90 �N  -180..180 �E\n"
          "               [--timezone \"Zoneinfo time zone spec\"]\n"
          "               [--country \"Country\"]\n"
          "               [--units meters|feet|knots]\n"
          "               [--min_dir N]                       0..359 � true\n"
          "               [--max_dir N]                       0..359 � true\n"
          "               [--legalese \"1-line legal notice\"]\n"
          "               [--notes \"Warnings to users\"]\n"
          "               [--comments \"Info about this station\"]\n"
          "               [--source \"Harmgen using data from XYZ\"]\n"
          "               [--restriction \"Public domain\"]\n"
          "               [--xfields \"EtCetera:  Et cetera.\"]\n"
          "               [--datum \"Lowest Astronomical Tide\"]\n"
          "               [--datum_override N.NN]\n"
          "               [--maxconstituents N]\n"
          "               [--minamplitude N.NN]\n"
          "               [--force]\n"
          "               congen-input-file.txt\n"
          "               time-series-input-file.txt\n"
          "               output-file.sql\n"
          "\n"
          "    harmgen:  Derive harmonic constants from water level observations.\n"
          "    Copyright (C) 1998  David Flater.\n"
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
          "    along with this program.  If not, see <http://www.gnu.org/licenses/>.\n");
  exit(-1);
}

// Translate from C++ string to SQL string.
static const std::string quote(const std::string &s)
{
  std::string ret("DEFAULT");
  if (!s.empty())
  {
    ret = "'";
    for (unsigned i(0); i < s.length(); ++i)
      if (s.at(i) == '\'')
        ret += "''";
      else
        ret += s.at(i);
    ret += "'";
  }
  return ret;
}

// Used for latitude and longitude.
static const std::string quote(const double d, const bool notNull)
{
  assert(!notNull || (d >= -180 && d <= 180));
  std::string ret("DEFAULT");
  if (notNull)
  {
    char temp[11];
    sprintf(temp, "%.5f", d);
    ret = temp;
  }
  return ret;
}

// Used for current bearings (degrees true).
static const std::string quote(const unsigned u, const bool notNull)
{
  assert(!notNull || (u < 360));
  std::string ret("DEFAULT");
  if (notNull)
  {
    char temp[4];
    sprintf(temp, "%u", u);
    ret = temp;
  }
  return ret;
}

int main(int argc, char **argv)
{

  // Station defaults.
  std::string ds_name("New unnamed station from Harmgen");
  std::string ds_original_name;
  std::string ds_station_id_context;
  std::string ds_station_id;
  double ds_latitude;
  double ds_longitude;
  bool haveCoordinates(false);
  std::string ds_timezone(":UTC");
  std::string ds_country;
  std::string ds_units("meters");
  unsigned ds_min_dir;
  bool haveMinDir(false);
  unsigned ds_max_dir;
  bool haveMaxDir(false);
  std::string ds_legalese;
  std::string ds_notes;
  std::string ds_comments;
  std::string ds_source(PACKAGE_STRING);
  std::string ds_restriction("Do not distribute");
  std::string ds_xfields;
  std::string ds_datum("Unspecified");
  unsigned maxconstituents(0);
  double minamplitude(0);
  bool haveDatumOverride(false);
  double datumOverride(666);
  bool force(false);

  // Exceedingly verbose and tedious hard-coded parsing of command line.
  int argnum(1);
  for (; argnum < argc; ++argnum)
  {
    if (!strcmp(argv[argnum], "--name"))
    {
      if (++argnum >= argc)
        usage();
      ds_name = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--original_name"))
    {
      if (++argnum >= argc)
        usage();
      ds_original_name = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--station_id_context"))
    {
      if (++argnum >= argc)
        usage();
      ds_station_id_context = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--station_id"))
    {
      if (++argnum >= argc)
        usage();
      ds_station_id = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--coordinates"))
    {
      if (++argnum >= argc)
        usage();
      if (sscanf(argv[argnum], "%lf", &ds_latitude) != 1)
        usage();
      if (++argnum >= argc || ds_latitude < -90 || ds_latitude > 90)
        usage();
      if (sscanf(argv[argnum], "%lf", &ds_longitude) != 1)
        usage();
      if (ds_longitude < -180 || ds_longitude > 180)
        usage();
      haveCoordinates = true;
    }
    else if (!strcmp(argv[argnum], "--timezone"))
    {
      if (++argnum >= argc)
        usage();
      ds_timezone = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--country"))
    {
      if (++argnum >= argc)
        usage();
      ds_country = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--units"))
    {
      if (++argnum >= argc)
        usage();
      ds_units = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--min_dir"))
    {
      if (++argnum >= argc)
        usage();
      if (sscanf(argv[argnum], "%u", &ds_min_dir) != 1)
        usage();
      if (ds_min_dir >= 360)
        usage();
      haveMinDir = true;
    }
    else if (!strcmp(argv[argnum], "--max_dir"))
    {
      if (++argnum >= argc)
        usage();
      if (sscanf(argv[argnum], "%u", &ds_max_dir) != 1)
        usage();
      if (ds_max_dir >= 360)
        usage();
      haveMaxDir = true;
    }
    else if (!strcmp(argv[argnum], "--legalese"))
    {
      if (++argnum >= argc)
        usage();
      ds_legalese = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--notes"))
    {
      if (++argnum >= argc)
        usage();
      ds_notes = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--comments"))
    {
      if (++argnum >= argc)
        usage();
      ds_comments = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--source"))
    {
      if (++argnum >= argc)
        usage();
      ds_source = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--restriction"))
    {
      if (++argnum >= argc)
        usage();
      ds_restriction = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--xfields"))
    {
      if (++argnum >= argc)
        usage();
      ds_xfields = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--datum"))
    {
      if (++argnum >= argc)
        usage();
      ds_datum = argv[argnum];
    }
    else if (!strcmp(argv[argnum], "--datum_override"))
    {
      if (++argnum >= argc)
        usage();
      if (sscanf(argv[argnum], "%lf", &datumOverride) != 1)
        usage();
      haveDatumOverride = true;
    }
    else if (!strcmp(argv[argnum], "--maxconstituents"))
    {
      if (++argnum >= argc)
        usage();
      if (sscanf(argv[argnum], "%u", &maxconstituents) != 1)
        usage();
    }
    else if (!strcmp(argv[argnum], "--minamplitude"))
    {
      if (++argnum >= argc)
        usage();
      if (sscanf(argv[argnum], "%lf", &minamplitude) != 1)
        usage();
    }
    else if (!strcmp(argv[argnum], "--force"))
    {
      force = true;
    }
    else
      break;
  }
  if (argnum >= argc)
    usage();
  char const *const congenfname = argv[argnum];
  std::ifstream congenInputFile(congenfname);
  if (congenInputFile.fail())
  {
    perror(congenfname);
    exit(-1);
  }
  if (++argnum >= argc)
    usage();
  FILE *timeSeriesFile(fopen(argv[argnum], "r"));
  if (timeSeriesFile == NULL)
  {
    perror(argv[argnum]);
    exit(-1);
  }
  if (++argnum >= argc)
    usage();
  FILE *SQLfile(fopen(argv[argnum], "w"));
  if (SQLfile == NULL)
  {
    perror(argv[argnum]);
    exit(-1);
  }

  // Parse input files.
  unsigned long seriesLength;
  int64_t firstTimestamp, lastTimestamp;
  findInputBounds(timeSeriesFile, seriesLength, firstTimestamp,
                  lastTimestamp);
  const Congen::year_t epochForSpeeds(1900),
      firstYear(yearOfTimestamp(firstTimestamp)),
      lastYear(yearOfTimestamp(lastTimestamp));
  std::vector<Congen::Constituent> constituents;
  unsigned lineno(Congen::parseLegacyInput(congenInputFile, firstYear,
                                           lastYear, epochForSpeeds,
                                           constituents));
  if (lineno)
  {
    fprintf(stderr, "Error on line %u of %s\n", lineno, congenfname);
    exit(-1);
  }
  if (constituents.empty())
  {
    fprintf(stderr, "No constituents found!\n");
    exit(-1);
  }
  unsigned origNumberOfConstituents(constituents.size());

  // Check if time series is long enough.
  double timeSeriesLengthInYears((double)(lastTimestamp - firstTimestamp) / avgSecPerYear);
  std::sort(constituents.begin(), constituents.end(), sortBySpeed);

  // Test 1:  is the record long enough to resolve the slowest constituent?
  if (constituents[0].speed * rotFactor * timeSeriesLengthInYears < minRotations)
  {
    fprintf(stderr, "The time series of length %f average Gregorian years\n"
                    "is too short to resolve %s (%.7f deg/hr, %f rotations/year)\n",
            timeSeriesLengthInYears,
            constituents[0].name.c_str(),
            constituents[0].speed,
            constituents[0].speed * rotFactor);
    exit(-1);
  }

  // Test 2:  is the record long enough to separate constituents from
  // each other?
  bool bad(false);
  for (unsigned i(1); i < constituents.size(); ++i)
  {
    const double rotationsPerYear((constituents[i].speed -
                                   constituents[i - 1].speed) *
                                  rotFactor);
    if (rotationsPerYear * timeSeriesLengthInYears < minRotations)
    {
      if (!bad)
      {
        fprintf(stderr, "The time series of length %f average Gregorian years\n"
                        "is too short to separate the following constituents from each other:\n",
                timeSeriesLengthInYears);
        bad = true;
      }
      fprintf(stderr, "  %s (%.7f deg/hr) and %s (%.7f deg/hr)\n    delta = %f rotations/year\n",
              constituents[i - 1].name.c_str(),
              constituents[i - 1].speed,
              constituents[i].name.c_str(),
              constituents[i].speed,
              rotationsPerYear);
    }
  }
  if (bad && !force)
    exit(-1);

  // Create input file for Octave.
  FILE *fpout;
  if ((fpout = fopen("oct_input", "w")) == NULL)
  {
    perror("oct_input");
    exit(-1);
  }

  // Write constituent speeds to oct_input.
  fprintf(fpout, "%u\n", constituents.size());
  for (std::vector<Congen::Constituent>::iterator it(constituents.begin());
       it != constituents.end();
       ++it)
    fprintf(fpout, "%.7f\n", it->speed);

  fprintf(fpout, "%lu\n", seriesLength);

  // Transfer the time series, adding years.
  for (unsigned long looper(0); looper < seriesLength; ++looper)
  {
    int64_t t;
    double h;
    require(fscanf(timeSeriesFile, "%" SCNd64 " %lf", &t, &h) == 2);
    fprintf(fpout, "%.16f %.16f %d\n",
            t / 3600., h, yearOfTimestamp(t) - firstYear + 1);
  }
  fclose(timeSeriesFile);

  // Copy out node factors.
  fprintf(fpout, "%d\n", lastYear - firstYear + 1);
  for (std::vector<Congen::Constituent>::iterator it(constituents.begin());
       it != constituents.end();
       ++it)
    for (Congen::year_t year(firstYear); year <= lastYear; ++year)
      fprintf(fpout, "%.4f ", it->f[year - firstYear]);
  fprintf(fpout, "\n");

  // The equilibrium arguments have to be adjusted so that time starts
  // at 1970-01-01 00:00 instead of the beginning of the current year.
  for (std::vector<Congen::Constituent>::iterator it(constituents.begin());
       it != constituents.end();
       ++it)
    for (Congen::year_t year(firstYear); year <= lastYear; ++year)
      fprintf(fpout, "%s ",
              Congen::normalize(it->equilibriumArgument[year - firstYear] -
                                    it->speed * (startYear(year) / 3600.),
                                16)
                  .c_str());
  fprintf(fpout, "\n");
  fclose(fpout);

  // Invoke Octave.  albatross is a trick to avoid people invoking
  // harmgen.sh directly by mistake.
  system("PathGoesHere/harmgen.sh albatross");

  // Parse Octave's output.
  FILE *octout;
  if ((octout = fopen("oct_output", "r")) == NULL)
  {
    perror("oct_output");
    exit(-1);
  }
  double z0;
  if (fscanf(octout, "%lf", &z0) != 1)
    octfailed();
  std::vector<double> amp(constituents.size());
  std::vector<double> phase(constituents.size());
  for (unsigned i(0); i < constituents.size(); ++i)
    if (fscanf(octout, "%lf %lf", &(amp[i]), &(phase[i])) != 2)
      octfailed();
  fclose(octout);

  // Delete constituents that round to zero.
  for (unsigned i(0); i < constituents.size();)
    if (amp[i] < amplitude_epsilon)
      deleteConstituent(constituents, amp, phase, i);
    else
      ++i;

  // If requested, limit the number of constituents.
  double total_dropped_amp(0), max_dropped_amp(0);
  if (maxconstituents)
  {
    while (constituents.size() > maxconstituents)
    {
      unsigned victim(min_element(amp.begin(), amp.end()) - amp.begin());
      total_dropped_amp += amp[victim];
      max_dropped_amp = amp[victim];
      deleteConstituent(constituents, amp, phase, victim);
    }
  }

  // If requested, throw out wimpy constituents.
  for (unsigned i(0); i < constituents.size();)
    if (amp[i] < minamplitude)
    {
      total_dropped_amp += amp[i];
      if (amp[i] > max_dropped_amp)
        max_dropped_amp = amp[i];
      deleteConstituent(constituents, amp, phase, i);
    }
    else
      ++i;

  if (constituents.empty())
  {
    fprintf(stderr, "Error:  All constituents were eliminated.\n");
    exit(-1);
  }

  // Add harmgen comments to user-specified comments.
  {
    std::string myc("Harmonic constants derived by " PACKAGE_STRING " ");
    time_t timep(time(NULL));
    struct tm *temptm(localtime(&timep));
    char tempchar[80];
    strftime(tempchar, 79, "%Y-%m-%d %H:%M %Z", temptm);
    myc += tempchar;
    myc += "\nusing ";
    sprintf(tempchar, "%lu", seriesLength);
    myc += tempchar;
    myc += " observations from ";
    // FIXME:  gmtime 32-bit epoch
    timep = firstTimestamp;
    temptm = gmtime(&timep);
    strftime(tempchar, 79, "%Y-%m-%d", temptm);
    myc += tempchar;
    myc += " to ";
    timep = lastTimestamp;
    temptm = gmtime(&timep);
    strftime(tempchar, 79, "%Y-%m-%d", temptm);
    myc += tempchar;
    myc += "\nnumber of constituents tried = ";
    sprintf(tempchar, "%u", origNumberOfConstituents);
    myc += tempchar;
    if (bad && force)
      myc += "\nwarnings about time series being too short were ignored";
    if (minamplitude > 0 || maxconstituents > 0)
    {
      if (minamplitude > 0)
      {
        myc += "\nminimum amplitude to retain constituent = ";
        sprintf(tempchar, "%.4f", minamplitude);
        myc += tempchar;
      }
      if (maxconstituents > 0)
      {
        myc += "\nmaximum number of constituents to retain = ";
        sprintf(tempchar, "%u", maxconstituents);
        myc += tempchar;
      }
      myc += '\n';
      if (total_dropped_amp > 0)
      {
        sprintf(tempchar, "max dropped amp %.4f, total %.4f", max_dropped_amp, total_dropped_amp);
        myc += tempchar;
      }
      else
        myc += "no loss";
    }
    if (ds_comments.empty())
      ds_comments = myc;
    else
      ds_comments = myc + "\n\n" + ds_comments;
  }

  fprintf(SQLfile, "{\n");
  for (unsigned i(0); i < constituents.size(); ++i)
  {
    fprintf(SQLfile, "\"%s\": {\"amplitude\": %.4f, \"phase\": %s}",
            constituents[i].name.c_str(), amp[i],
            Congen::normalize(phase[i], 2).c_str());
    if (i + 1 < constituents.size())
    {
      fprintf(SQLfile, ",\n");
    }
  }
  fprintf(SQLfile, "\n}\n");
  fclose(SQLfile);
  return 0;
}
