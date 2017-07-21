SUBDIRS = man src
EXTRA_DIST = LICENCE NEWS.adoc README.adoc

if ASCIIDOC
%.html: %.adoc
	TZ=UTC asciidoc $<
endif
