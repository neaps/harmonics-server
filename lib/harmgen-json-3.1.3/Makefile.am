# $Id: Makefile.am 6755 2018-01-15 15:58:38Z flaterco $

bin_PROGRAMS           = harmgen
BUILT_SOURCES          = harmgen.cc
CLEANFILES             = harmgen.cc
dist_libexec_SCRIPTS   = harmgen.sh
dist_pkgdata_DATA      = congen_1yr.txt congen_5yrs.txt congen_9yrs.txt \
                         README
nodist_harmgen_SOURCES = harmgen.cc
EXTRA_DIST             = harmgen-orig.cc Bjoern_Brill.txt C_J_Read.txt
harmgen_LDADD          = -lcongen -lm

harmgen.cc: harmgen-orig.cc Makefile
	$(SED) "s|PathGoesHere|$(DESTDIR)$(libexecdir)|" "$(srcdir)/harmgen-orig.cc" > harmgen.cc
