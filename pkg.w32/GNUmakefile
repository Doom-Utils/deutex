include config.make

DOC_FILES = ../NEWS.html       \
            ../README.html     \
            ../man/deutex.html
DOC_TXT_FILES = ../AUTHORS ../COPYING
ZIP=$(PACKAGE)-$(PACKAGE_VERSION)_w32.zip

all: $(ZIP)

clean:
	rm -rf staging $(ZIP)

$(ZIP): staging
	zip -Xjr $@ $</*

staging:
	mkdir $@
	./cp-with-libs --ldflags="$(LDFLAGS)" ../src/deutex.exe $@

	$(STRIP) $@/*.exe $@/*.dll

	# Will require NEWS.html README.html and man/deutex.html to be
	# built.
	for f in $(DOC_FILES); do cp $$f $@; done
	for f in $(DOC_TXT_FILES); do                                  \
		unix2dos --add-bom -n $$f $@/$$(basename $$f).txt;     \
	done
