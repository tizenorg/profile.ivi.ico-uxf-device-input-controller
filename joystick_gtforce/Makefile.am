export abs_builddir

wayland_client_lib = -lwayland-client
wayland_ivi_client_lib = -lico-uxf-weston-plugin
wayland_ivi_client_inc = -I/usr/include/ico-uxf-weston-plugin

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src $(wayland_ivi_client_inc) $(GLIB_CFLAGS) $(COMPOSITOR_CFLAGS)

bin_PROGRAMS =		\
	ico_ictl-joystick_gtforce

check_LTLIBRARIES = $(TESTS)
check_PROGRAMS = ico_ictl-joystick

AM_LDFLAGS = -module -avoid-version -rpath $(libdir) $(GLIB_LIBS)

ico_ictl_joystick_gtforce_SOURCES = \
	ico_ictl-joystick.c		\
	ico_ictl-wayland.c		\
	dbg_curtime.c
ico_ictl_joystick_gtforce_LDADD = $(SIMPLE_CLIENT_LIBS) $(wayland_ivi_client_lib) $(wayland_client_lib)

