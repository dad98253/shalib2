




ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src


EXTRA_DIST = include \
	README.md \
	m4/Makefile.in
	

AC_CONFIG_AUX_DIR = build-aux

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status libtool


@GENERATE_CHANGELOG_RULES@
  dist-hook: dist-ChangeLog

distclean-local:
	@rm config.status configure config.log
	@rm Makefile
	@rm -r autom4te.cache/
	@rm aclocal.m4
	@rm Makefile.in
