TESTS_ENVIRONMENT = $(SHELL) $(top_srcdir)/tests/weston-plugin-test

export abs_builddir

AM_CFLAGS = $(GCC_CFLAGS)
AM_CPPFLAGS = -I$(top_srcdir)/src -DUNIT_TEST $(COMPOSITOR_CFLAGS)

noinst_PROGRAMS =		\
	test-homescreen		\
	test-client

check_LTLIBRARIES = $(TESTS)
check_PROGRAMS = test-homescreen test-client

AM_LDFLAGS = -module -avoid-version -rpath $(libdir) -lwayland-egl -lEGL -lGLESv2
AM_CFLAGS = $(wayland_ivi_client_inc) $(GCC_CFLAGS)

test_common_src = test-common.c test-common.h
test_wayland_client = -lwayland-client
wayland_ivi_client_lib = -lico-uxf-weston-plugin
wayland_ivi_client_inc = -I/usr/include/ico-uxf-weston-plugin

test_homescreen_SOURCES = test-homescreen.c $(test_common_src)
test_homescreen_LDADD = $(SIMPLE_CLIENT_LIBS) $(wayland_ivi_client_lib) $(test_wayland_client)

test_client_SOURCES = test-client.c $(test_common_src)
test_client_LDADD = $(SIMPLE_CLIENT_LIBS) $(wayland_ivi_client_lib) $(test_wayland_client)

EXTRA_DIST = input-controller-test

