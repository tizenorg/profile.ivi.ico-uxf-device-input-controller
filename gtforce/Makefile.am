export abs_builddir

wayland_client_lib = -lwayland-client
wayland_ivi_client_lib = -lico-uxf-weston-plugin
wayland_ivi_client_inc = -I/usr/include/ico-uxf-weston-plugin

AM_CFLAGS = $(GCC_CFLAGS) -I/usr/include/ico-util
AM_CPPFLAGS = -I$(top_srcdir)/src $(wayland_ivi_client_inc) $(GLIB_CFLAGS) $(COMPOSITOR_CFLAGS)

bin_PROGRAMS =		\
	ico_dic-gtforce

check_LTLIBRARIES = $(TESTS)
check_PROGRAMS = ico_dic-gtforce

AM_LDFLAGS = -module -avoid-version -rpath $(libdir) $(GLIB_LIBS) -lico-util

ico_dic_gtforce_SOURCES = \
	ico_dic-gtforce.c		\
	ico_dic-wayland.c
ico_dic_gtforce_LDADD = $(SIMPLE_CLIENT_LIBS) $(wayland_ivi_client_lib) $(wayland_client_lib)
