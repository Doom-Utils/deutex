SUBDIRS = man src
EXTRA_DIST = LICENSE NEWS.adoc README.adoc

if ASCIIDOC
%.html: %.adoc
	TZ=UTC asciidoc $<
endif
