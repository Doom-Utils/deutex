#! /bin/csh -f
#
# In case you don't have the dos2unix command:
# here is the fastest and simplest solution...
#
# 08-Mar-96, Jack Leunissen
#
foreach i ( *.[ch] makefile )
	tr -d '\015' < $i > tmp
	mv tmp $i
end
