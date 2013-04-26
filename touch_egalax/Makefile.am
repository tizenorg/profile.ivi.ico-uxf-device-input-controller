export abs_builddir

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src $(COMPOSITOR_CFLAGS)

bin_PROGRAMS =		\
	ico_ictl-touch_egalax	\
	ico_ictl-egalax_calibration

check_LTLIBRARIES = $(TESTS)
check_PROGRAMS = \
	ico_ictl-touch_egalax	\
	ico_ictl-egalax_calibration

AM_LDFLAGS = -module -avoid-version -rpath $(libdir)

ico_ictl_touch_egalax_SOURCES = \
	ico_ictl-touch_egalax.c
ico_ictl_touch_egalax_LDADD = $(SIMPLE_CLIENT_LIBS)

ico_ictl_egalax_calibration_SOURCES = \
	ico_ictl-egalax_calibration.c
ico_ictl_egalax_calibration_LDADD = $(SIMPLE_CLIENT_LIBS)

