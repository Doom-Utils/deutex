# Makefile of DeuTex/DeuSF
# by Per Allansson, Olivier Montanuy and André Majorel
#
# Platform-specific notes:
#
#   Unix    User targets should work on all Unices. Some _developer_
#           targets, however, require lynx, GCC or GNU tar and gzip.
#
#   DOS     "make [all]" should work with DJGPP and Cygwin. With other
#           compilers, use the batch files in dos\ instead.
#           "make install" will most likely not work ; install the
#           executables by hand.
#
#   Others  Don't know. Send feedback to the maintainer.
#

PREFIX=/usr/local

# Compiled by users
CFLAGS   = -O2
CC       = cc
LDFLAGS  =

# Compiled by developers
DCFLAGS  = -g -Wall -Wpointer-arith -Wstrict-prototypes
DCC      = gcc
DLDFLAGS = -g
#-lefence

#DEFINES = -DDT_ALPHA -DDT_PRIVATE

######### do not edit after this line #########
VERSION       = $$(cat VERSION)
DISTARC       = deutex-$(VERSION).tar.gz
DISTARCDOS    = deutex-$(VERSION).zip
DISTARCDOS8   = dtex$$(tr -cd '[0-9]' <VERSION).zip
BINZIP        = deutex-$(VERSION).bin.dos.zip
DISTDIR       = deutex-$(VERSION)
DISTDIRDOS    = dtex$$(tr -cd '[0-9]' <VERSION)
DISTFILES     = $(DOC_SRC) $(HEADERS) $(MISCFILES) $(MISCSRC)\
		$(SCRIPTS) $(SRC)
DISTFILESBIN  = $(MISCFILES) deutex.exe deusf.exe

DOC_SRC =\
	docsrc/README\
	docsrc/changes.html\
	docsrc/deutex.6\
	docsrc/hackers_guide.html\
	docsrc/readme.dos\
	docsrc/todo.html\

DDOCUNIXFILES =\
	unixtmp1/CHANGES\
	unixtmp1/COPYING\
	unixtmp1/COPYING.LIB\
	unixtmp1/INSTALL\
	unixtmp1/LICENSE\
	unixtmp1/README\
	unixtmp1/TODO\
	unixtmp1/deutex.6\
	unixtmp1/dtexman6.txt\

DDOCUNIX = unixtmp1 $(DDOCUNIXFILES)

DDOCDOSFILES =\
	dostmp1/changes.txt\
	dostmp1/copying\
	dostmp1/copying.lib\
	dostmp1/dtexman6.txt\
	dostmp1/license\
	dostmp1/manpage.txt\
	dostmp1/readme.txt\
	dostmp1/todo.txt\
	
DDOCDOS = dostmp1 $(DDOCDOSFILES)

HEADERS =\
	src/color.h\
	src/deutex.h\
	src/endianio.h\
	src/endianm.h\
	src/extract.h\
	src/gifcodec.h\
	src/ident.h\
	src/lists.h\
	src/merge.h\
	src/mkwad.h\
	src/picture.h\
	src/sound.h\
	src/text.h\
	src/texture.h\
	src/tools.h\
	src/usedidx.h\
	src/wadio.h\

MISCFILES =\
	VERSION\

# Only in source distributions
MISCSRC =\
	Makefile\
	dos/buildbc.bat\
	dos/buildmsc.bat\
	old/deusf.ide\
	old/deutex.ide\
	old/dos2unix.sh\
	old/save.bat\
	src/deusf.def\
	src/deusfos.def\
	src/deutex.def\
	src/deutexos.def\

SCRIPTS =\
	scripts/process\

SRC =\
	src/color.c\
	src/compose.c\
	src/deutex.c\
	src/endianio.c\
	src/endianm.c\
	src/extract.c\
	src/gifcodec.c\
	src/ident.c\
	src/listdir.c\
	src/lists.c\
	src/lzw.c\
	src/merge.c\
	src/mkwad.c\
	src/picture.c\
	src/sound.c\
	src/substit.c\
	src/text.c\
	src/texture.c\
	src/tools.c\
	src/usedidx.c\
	src/version.c\
	src/wadio.c\

OBJSF   = $(SRC:.c=.os)
OBJTEX  = $(SRC:.c=.ot)
DOBJSF  = $(SRC:.c=.osd)
DOBJTEX = $(SRC:.c=.otd)

.SUFFIXES: .os .os~ .ot .ot~ .osd .osd~ .otd .otd~ $(SUFFIXES)

.c.ot: src/deutex.h
	$(CC) $(CFLAGS) $(DEFINES) -DDeuTex -o $@ -c $<

.c.os: src/deutex.h
	$(CC) $(CFLAGS) $(DEFINES) -DDeuSF  -o $@ -c $<

.c.otd: src/deutex.h
	$(DCC) $(DCFLAGS) $(DEFINES) -DDeuTex -o $@ -c $<

.c.osd: src/deutex.h
	$(DCC) $(DCFLAGS) $(DEFINES) -DDeuSF  -o $@ -c $<

all: deutex deusf

deutex: $(OBJTEX) tmp/_deutex
	$(CC) $(LDFLAGS) -o deutex $(OBJTEX) -lm

deusf: $(OBJSF) tmp/_deusf
	$(CC) $(LDFLAGS) -o deusf $(OBJSF) -lm

dall: ddeutex ddeusf

ddt: ddeutex

dds: ddeusf


ddeutex: $(DOBJTEX)
	$(DCC) $(DLDFLAGS) -lm -o deutex $(DOBJTEX) 
	@# Force next "make deutex" to relink
	(sleep 1; mkdir -p tmp; touch tmp/_deutex) &

ddeusf: $(DOBJSF)
	$(DCC) $(DLDFLAGS) -lm -o deusf $(DOBJSF) 
	@# Force next "make deusf" to relink
	(sleep 1; mkdir -p tmp; touch tmp/_deusf) &

tmp/_deutex:
	-mkdir tmp
	touch $@

tmp/_deusf:
	-mkdir tmp
	touch $@

install:
	install -p -m 0755 deutex   $(PREFIX)/bin
	install -p -m 0755 deusf    $(PREFIX)/bin
	install -p -m 0644 deutex.6 $(PREFIX)/man/man6
	ln -sf deutex.6             $(PREFIX)/man/man6/deusf.6

src/version.c: VERSION
	printf "const char deutex_version[] = \"%s\";\n" $(VERSION) >$@

strip: deutex deusf
	strip deutex
	strip deusf

doc: $(DDOCUNIX)

unixtmp1:
	mkdir -p $@

unixtmp1/CHANGES: docsrc/changes.htm* VERSION
	echo 'THIS IS A GENERATED FILE -- DO NOT EDIT !' >$@
	echo 'Edit docsrc/changes.html instead.' >>$@
	lynx -dump $< >>$@

unixtmp1/COPYING: docsrc/COPYING
	cp -p $< $@

unixtmp1/COPYING.LIB: docsrc/COPYING.LIB
	cp -p $< $@

unixtmp1/INSTALL: docsrc/INSTALL VERSION scripts/process
	scripts/process $< >$@

unixtmp1/LICENSE: docsrc/LICENSE VERSION scripts/process
	scripts/process $< >$@

unixtmp1/README: docsrc/README VERSION scripts/process
	scripts/process $< >$@

unixtmp1/TODO: docsrc/todo.htm* VERSION
	echo 'THIS IS A GENERATED FILE -- DO NOT EDIT !' >$@
	echo 'Edit docsrc/todo.html instead.' >>$@
	lynx -dump $< >>$@

unixtmp1/deutex.6: docsrc/deutex.6 VERSION scripts/process
	scripts/process $< >$@

unixtmp1/dtexman6.txt: docsrc/dtexman6.txt
	cp -p $< $@

dostmp1:
	mkdir -p $@

dostmp1/changes.txt: unixtmp1/CHANGES
	todos <$< >$@
	touch -r $< $@

dostmp1/copying: unixtmp1/COPYING
	todos <$< >$@;
	touch -r $< $@

dostmp1/copying.lib: unixtmp1/COPYING.LIB
	todos <$< >$@
	touch -r $< $@

dostmp1/dtexman6.txt: unixtmp1/dtexman6.txt
	todos <$< >$@
	touch -r $< $@

dostmp1/license: unixtmp1/LICENSE
	todos <$< >$@
	touch -r $< $@

dostmp1/manpage.txt: unixtmp1/deutex.6
	nroff -man -Tlatin1 $< | ul -t dumb | todos >$@
	touch -r $< $@

dostmp1/readme.txt: docsrc/readme.dos VERSION scripts/process
	scripts/process $< | todos >$@

dostmp1/todo.txt: unixtmp1/TODO
	todos <$< >$@
	touch -r $< $@

clean:
	rm -f $(OBJTEX) $(OBJSF) $(DOBJTEX) $(DOBJSF) deutex deusf
	if [ -f deutex.exe ]; then rm deutex.exe; fi
	if [ -f deusf.exe ]; then rm deusf.exe; fi

# dist - make the distribution archive for Unix (.tar.gz)
dist: $(DISTFILES) $(DDOCUNIX)
	mkdir -p $(DISTDIR)
	cp -dpP $(DISTFILES) $(DISTDIR)
	cp -p $(DDOCUNIXFILES) $(DISTDIR)
	tar -zcf $(DISTARC) $(DISTDIR)
	rm -rf $(DISTDIR)

# distdos - make the distribution archive DOS (.zip, 8+3)
distdos: $(DISTFILES) $(DDOCDOS)
	mkdir -p $(DISTDIRDOS)
	cp -dpP $(DISTFILES) $(DISTDIRDOS)
	cp -p $(DDOCDOSFILES) $(DISTDIRDOS)
	if [ -e $(DISTARCDOS) ]; then rm $(DISTARCDOS); fi
	zip -D -X -9 -r $(DISTARCDOS) $(DISTDIRDOS)
	rm -rf $(DISTDIRDOS)
	printf 'DeuTex %s\nhttp://www.teaser.fr/~amajorel/deutex/'\
	  "$(VERSION)" | zip -z $(DISTARCDOS)

# distbindos - make the DOS binary distribution archive (.zip, 8+3)
TMP=tmpd
distbindos: $(DISTFILESBIN) $(DDOCDOS)
	mkdir -p $(TMP)
	cp -dpP $(DISTFILESBIN) $(TMP)
	cp -p $(DDOCDOSFILES) $(TMP)
	if [ -e $(BINZIP) ]; then rm $(BINZIP); fi
	export name=$$(pwd)/$(BINZIP); cd $(TMP); zip -D -X -9 -R $$name '*'
	rm -rf $(TMP)
	printf 'DeuTex %s\nhttp://www.teaser.fr/~amajorel/deutex/'\
	  "$(VERSION)" | zip -z $(BINZIP)

# save - your daily backup
save:
	tar -zcvf ../deutex-$$(date '+%Y%m%d').tar.gz\
	  --exclude dostmp1 --exclude unixtmp1\
	  --exclude "*~" --exclude "*.o"\
	  --exclude "*.os" --exclude "*.ot"\
	  --exclude "*.osd" --exclude "*.otd"\
	  --exclude "*.obj" .

# help - display list of interesting targets
help:
	@echo "Targets for end users:"
	@echo "  [all]       Build DeuTex and DeuSF"
	@echo "  install     Install DeuTex, DeuSF and the doc"
	@echo
	@echo "Targets for developers:"
	@echo "  doc         Just the doc"
	@echo "  dall        Alias for ddeutex + ddeusf"
	@echo "  ddt         Alias for ddeutex"
	@echo "  dds         Alias for ddeusf"
	@echo "  ddeutex     Debug version of DeuTex -> ./deutex"
	@echo "  ddeusf      Debug version of DeuSF  -> ./deusf"
	@echo "  dist        Source dist. (Unix)     -> ./deutex-VERSION.tar.gz"
	@echo "  distdos     Source dist. (DOS)      -> ./dtexVERSION.zip"
	@echo "  distbindos  Binary-only dist. (DOS) -> ./deutex-VERSION.bin.dos.zip"
	@echo "  save        Backup archive          -> ../deutex-YYYYMMDD.tar.gz"
	@echo "  strip       Strip ./deutex and ./deusf"
	@echo "  test        Run all tests (long)"
	@echo "  clean       Remove executables and object files"


