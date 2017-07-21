if ASCIIDOC
man_MANS = @PACKAGE@.6
CLEANFILES = @PACKAGE@.6 @PACKAGE@.html

@PACKAGE@.6: deutex.txt
	a2x -f manpage deutex.txt

@PACKAGE@.html: deutex.txt
	TZ=UTC asciidoc deutex.txt
endif
