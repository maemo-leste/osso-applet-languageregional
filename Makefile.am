# XXX/FIXME/TODO: honour debian cflags too
CFLAGS = -Wall -Werror -Wextra -std=gnu99 -O2 -ggdb -g
# CFLAGS += $(CFLAGS) -ggdb -g

# XXX: better names for the binaries
bin_PROGRAMS = setlocale

setlocale_CFLAGS = $(CFLAGS) $(GLIB_CFLAGS)
setlocale_LDADD = $(GLIB_LIBS)
setlocale_SOURCES = setlocale.c

hildoncontrolpanelpluginlib_LTLIBRARIES = libcplanguageregional.la

libcplanguageregional_la_SOURCES = libcplanguageregional.c
libcplanguageregional_la_CFLAGS = $(CFLAGS) $(LIBHILDON_CFLAGS) $(LIBDBUS_CFLAGS)
libcplanguageregional_la_LIBADD = $(LIBHILDON_LIBS) $(LIBDBUS_LIBS)
libcplanguageregional_la_LDFLAGS = -Wl,--as-needed -module -avoid-version

hildoncontrolpaneldesktopentry_DATA = cplanguageregional.desktop

sudoersdir = $(sysconfdir)/sudoers.d/
sudoers_DATA = osso-applet-languageregional.sudoers

EXTRA_DIST = $(hildoncontrolpaneldesktopentry_DATA)		\
             osso-applet-languageregional.sudoers

install-data-hook:
	@(cd $(DESTDIR)$(hildoncontrolpanelplugindir) && $(RM) \
		$(hildoncontrolpanelpluginlib_LTLIBRARIES))
