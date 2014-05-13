AC_PREREQ([2.68])
AC_INIT([ico-uxf-device-input-controller],
        [0.9.03],
        [https://BUG-REPORT-ADDRESS])

AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([1.11 foreign no-dist-gzip dist-xz])

AM_SILENT_RULES([yes])

# Check for programs
AC_PROG_CC
AC_PROG_CXX
AC_PROG_SED

# Initialize libtool
LT_PREREQ([2.2])
LT_INIT([disable-static])

PKG_PROG_PKG_CONFIG()

AC_CHECK_HEADERS([execinfo.h])

AC_CHECK_FUNCS([mkostemp strchrnul])

AC_ARG_ENABLE(setuid-install, [  --enable-setuid-install],,
	      enable_setuid_install=yes)
AM_CONDITIONAL(ENABLE_SETUID_INSTALL, test x$enable_setuid_install = xyes)

PKG_PROG_PKG_CONFIG

PKG_CHECK_MODULES([GLIB], [glib-2.0])
PKG_CHECK_MODULES([EGL], [egl])

CFLAGS="$CFLAGS $EGL_CFLAGS"
AC_SUBST(CFLAGS)

SHARED_LIBS=
SHARED_CFLAGS=
AC_SUBST(SHARED_LIBS)
AC_SUBST(SHARED_CFLAGS)

COMPOSITOR_LIBS="$COMPOSITOR_LIBS"
COMPOSITOR_CFLAGS="$COMPOSITOR_CFLAGS"

AM_CONDITIONAL(ENABLE_DESKTOP_SHELL, true)

if test "x$GCC" = "xyes"; then
	my_common_gcc_flags="-Wall -Wextra -Wno-unused-parameter \
		-Wno-missing-field-initializers -g -fvisibility=hidden"
	GCC_CFLAGS="$my_common_gcc_flags \
		-Wstrict-prototypes -Wmissing-prototypes"
	GCC_CXXFLAGS="$my_common_gcc_flags"
fi
AC_SUBST(GCC_CFLAGS)
AC_SUBST(GCC_CXXFLAGS)

WAYLAND_SCANNER_RULES(['$(top_srcdir)/protocol'])

AC_CONFIG_FILES([Makefile
		 gtforce/Makefile
		 tests/Makefile])
AC_OUTPUT
