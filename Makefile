# Makefile of DeuTex/DeuSF
# by Per Allansson, Olivier Montanuy and André Majorel
#
# Platform-specific notes:
#
#   Unix    User targets should work on all Unices. Some _developer_
#           targets, however, require lynx, GCC or GNU tar and gzip.
#
#   DOS     "make [all]" should work with any C compiler.
#           "make install" will most likely not work. You'll have to
#           install the executables by hand.
#
#   Others  Don't know. Send feedback to the maintainer.
#

PREFIX=/usr/local

# Compiled by users
CFLAGS   = -O2
CC       = cc
LDFLAGS  =

# Compiled by developers
DCFLAGS  = -g -Wall
DCC      = gcc
DLDFLAGS = -g

#DEFINES = -DDT_ALPHA

######### do not edit after this line #########
DISTARC    = deutex-$$(cat VERSION).tar.gz	# "deutex-4.0.0.tar.gz"
DISTARCDOS = dtex$$(tr -cd '[0-9]' <VERSION).zip  # "dtex400.zip"
DISTDIR    = deutex-$$(cat VERSION)		# "deutex-4.0.0"
DISTDIRDOS = dtex$$(tr -cd '[0-9]' <VERSION)	# "dtex400"
DISTFILES  = $(DOC) $(DOC_SRC) $(HEADERS) $(MISCFILES) $(SCRIPTS) $(SRC)
DOC_SRC = \
	docsrc/README\
	docsrc/changes.html\
	docsrc/deutex.6\
	docsrc/dtexman6.txt\
	docsrc/hackers_guide.html\
	docsrc/todo.html\

DOC = \
	CHANGES\
	README\
	TODO\
	deusf.6\
	deutex.6\

HEADERS = \
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
	src/wadio.h\

MISCFILES = \
	COPYING\
	COPYING.LIB\
	INSTALL\
	LICENSE\
	Makefile\
	VERSION\
	dos/buildbc.bat\
	dos/buildmsc.bat\
	old/deusf.ide\
	old/deutex.ide\
	old/dos2unix.sh\
	old/readme.txt\
	old/save.bat\
	src/deusf.def\
	src/deusfos.def\
	src/deutex.def\
	src/deutexos.def\

SCRIPTS = \
	scripts/process\

SRC = \
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
	src/version.c\
	src/wadio.c\

OBJSF   = $(SRC:.c=.os)
OBJTEX  = $(SRC:.c=.ot)
DOBJSF  = $(SRC:.c=.osd)
DOBJTEX = $(SRC:.c=.otd)

.SUFFIXES: .os .os~ .ot .ot~ .osd .osd~ .otd .otd~ $(SUFFIXES)

.c.ot:
	$(CC) $(CFLAGS) $(DEFINES) -DDeuTex -o $@ -c $<

.c.os:
	$(CC) $(CFLAGS) $(DEFINES) -DDeuSF  -o $@ -c $<

.c.otd:
	$(DCC) $(DCFLAGS) $(DEFINES) -DDeuTex -o $@ -c $<

.c.osd:
	$(DCC) $(DCFLAGS) $(DEFINES) -DDeuSF  -o $@ -c $<

all: deutex deusf doc

deutex: $(OBJTEX) .deutex
	$(CC) $(LDFLAGS) -o deutex $(OBJTEX) -lm

deusf: $(OBJSF) .deusf
	$(CC) $(LDFLAGS) -o deusf $(OBJSF) -lm

dall: ddeutex ddeusf

ddt: ddeutex

dds: ddeusf


ddeutex: $(DOBJTEX)
	$(DCC) $(DLDFLAGS) -o deutex $(DOBJTEX) -lm
	(sleep 1; touch .deutex) &  # Force next "make deutex" to relink

ddeusf: $(DOBJSF)
	$(DCC) $(DLDFLAGS) -o deusf $(DOBJSF) -lm
	(sleep 1; touch .deusf) &  # Force next "make deusf" to relink

.deutex:
	touch .deutex

.deusf:
	touch .deusf

install:
	install -p -m 0755 deutex   $(PREFIX)/bin
	install -p -m 0755 deusf    $(PREFIX)/bin
	install -p -m 0644 deutex.6 $(PREFIX)/man/man6
	install -p -m 0644 deusf.6  $(PREFIX)/man/man6

src/version.c: VERSION
	printf "const char deutex_version[] = \"%s\";\n" $$(cat VERSION) >$@

strip: deutex deusf
	strip deutex
	strip deusf

doc: $(DOC)

CHANGES: docsrc/changes.htm* VERSION
	echo 'THIS IS A GENERATED FILE -- DO NOT EDIT !' >$@
	echo 'Edit docsrc/changes.html instead.' >>$@
	lynx -dump $< >>$@

README: docsrc/README VERSION scripts/process
	scripts/process $< $(SRC_NON_GEN) >$@

TODO: docsrc/todo.htm* VERSION
	echo 'THIS IS A GENERATED FILE -- DO NOT EDIT !' >$@
	echo 'Edit docsrc/todo.html instead.' >>$@
	lynx -dump $< >>$@

deutex.6: docsrc/deutex.6 VERSION scripts/process
	scripts/process $< $(SRC_NON_GEN) >$@

deusf.6: deutex.6
	@if [ ! -e $@ ]; then ln -s $< $@; fi
	@if [ ! -L $@ ]; then echo "Can't overwrite $@"; false; fi

clean:
	rm -f $(OBJTEX) $(OBJSF) $(DOBJTEX) $(DOBJSF) deutex deusf
	if [ -f deutex.exe ]; then rm deutex.exe; fi
	if [ -f deusf.exe ]; then rm deusf.exe; fi

# dist - make the distribution archive for Unix (.tar.gz)
dist: $(DISTFILES)
	mkdir -p $(DISTDIR)
	cp -dpP $(DISTFILES) $(DISTDIR)
	tar -zcf $(DISTARC) $(DISTDIR)
	rm -rf $(DISTDIR)

# distdos - make the distribution archive DOS (.zip, 8+3)
distdos: $(DISTFILES)
	mkdir -p $(DISTDIRDOS)
	cp -dpP $(DISTFILES) $(DISTDIRDOS)
	if [ -e $(DISTARCDOS) ]; then rm $(DISTARCDOS); fi
	zip -D -X -9 -r $(DISTARCDOS) $(DISTDIRDOS)
	rm -rf $(DISTDIRDOS)
	printf 'DeuTex %s\nhttp://www.teaser.fr/~amajorel/deutex/'\
	  "$$(cat VERSION)" | zip -z $(DISTARCDOS)

# save - your daily backup
save:
	tar -zcvf ../deutex-$$(date '+%Y%m%d').tar.gz\
	  --exclude "*~" --exclude "*.o"\
	  --exclude "*.os" --exclude "*.ot" .

help:
	@echo "Targets for end users:"
	@echo "  make [all]    Build DeuTex, DeuSF and the doc"
	@echo "  make install  Install DeuTex, DeuSF and the doc"
	@echo
	@echo "Targets for developers:"
	@echo "  make doc      Just the doc"
	@echo "  make dall     Same as ddeutex + ddeusf"
	@echo "  make ddt      Same as ddeutex"
	@echo "  make dds      Same as ddeusf"
	@echo "  make ddeutex  Debug version of DeuTex -> ./deutex"
	@echo "  make ddeusf   Debug version of DeuSF  -> ./deusf"
	@echo "  make dist     Distribution archive    -> ./deutex-VERSION.tar.gz"
	@echo "  make distdos  Distribution archive    -> ./dtexVERSION.zip"
	@echo "  make save     Backup archive          -> ../deutex-YYYYMMDD.tar.gz"
	@echo "  make strip    Strip ./deutex and ./deusf"
	@echo "  make clean    Remove executables and object files"


