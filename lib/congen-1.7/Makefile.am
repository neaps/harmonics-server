# $Id: Makefile.am 6745 2018-01-14 19:21:26Z flaterco $

# Case matters:
# Congen is the namespace / header file.
# congen is the application.

lib_LTLIBRARIES             = libcongen.la
nodist_libcongen_la_SOURCES = libcongen.cc
nodist_include_HEADERS      = Congen

bin_PROGRAMS                = congen diff_congen_output
nodist_congen_SOURCES       = congen.cc
congen_LDADD                = libcongen.la
if TCD_SWITCH
congen_LDADD               += -ltcd
endif
congen_LDADD               += -lm

diff_congen_output_SOURCES  = diff_congen_output.cc
diff_congen_output_LDADD    = -lm
dist_bin_SCRIPTS            = scripts/Compound2Basic.rb scripts/XDO2Basic.rb
dist_pkgdata_DATA           = congen_input.txt README

BUILT_SOURCES               = Congen libcongen.cc congen.cc
CLEANFILES                  = Congen libcongen.cc congen.cc
EXTRA_DIST                  = utf8-src/mangle.sed utf8-src/Congen \
                              utf8-src/libcongen.cc utf8-src/congen.cc
ACLOCAL_AMFLAGS = -I m4


MANGLE = $(SED) --file=$(srcdir)/utf8-src/mangle.sed

Congen: $(srcdir)/utf8-src/Congen $(srcdir)/utf8-src/mangle.sed
	$(MANGLE) $(srcdir)/utf8-src/Congen > Congen

libcongen.cc: $(srcdir)/utf8-src/libcongen.cc $(srcdir)/utf8-src/mangle.sed
	$(MANGLE) $(srcdir)/utf8-src/libcongen.cc > libcongen.cc

congen.cc: $(srcdir)/utf8-src/congen.cc $(srcdir)/utf8-src/mangle.sed
	$(MANGLE) $(srcdir)/utf8-src/congen.cc > congen.cc


# -version-info current:revision:age

# Shared lib versions are almost, but not quite, completely unlike
# release versions.  They are sufficiently unlike that if the two
# happen to be the same it will cause people to jump to the conclusion
# that you don't know what you are doing.

# If the library source code has changed at all since the last update,
# then increment revision (c:r:a becomes c:r+1:a).

# If any interfaces have been added, removed, or changed since the
# last update, increment current, and set revision to 0.

# If any interfaces have been added since the last public release,
# then increment age.

# If any interfaces have been removed since the last public release,
# then set age to 0.

# FIXME: Update libtool version info on each release
libcongen_la_LDFLAGS = -version-info 0:2:0 -lm
