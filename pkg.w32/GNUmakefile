include config.make

DOC_HTML_FILES = ../NEWS.html ../README.html ../man/$(PACKAGE).html
DOC_TXT_FILES = ../AUTHORS ../COPYING
ZIP=$(PACKAGE)-$(PACKAGE_VERSION)_w32.zip

all: $(ZIP)

clean:
	rm -rf staging $(ZIP)

$(ZIP): staging
	zip -Xjr $@ $</*

../NEWS.html: ../NEWS.adoc
	$(MAKE) -C .. NEWS.html

../README.html: ../README.adoc
	$(MAKE) -C .. README.html

../man/$(PACKAGE).html: ../man/deutex.txt
	$(MAKE) -C ../man $(PACKAGE).html

staging: $(DOC_HTML_FILES)
	mkdir $@
	./cp-with-libs --ldflags="$(LDFLAGS)" ../src/deutex.exe $@

	$(STRIP) $@/*.exe $@/*.dll

	for f in $(DOC_HTML_FILES); do cp $$f $@; done
	for f in $(DOC_TXT_FILES); do                                  \
		unix2dos --add-bom -n $$f $@/$$(basename $$f).txt;     \
	done
