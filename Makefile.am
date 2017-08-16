SUBDIRS = man src
EXTRA_DIST = LICENSE NEWS.adoc README.adoc \
             pkg.w32/config.make.in pkg.w32/cp-with-libs pkg.w32/GNUmakefile

if ASCIIDOC
CLEANFILES = *.html
%.html: %.adoc
	TZ=UTC asciidoc $<

html: README.html NEWS.html
endif
