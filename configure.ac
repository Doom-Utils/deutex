AC_INIT([DeuTex], [4.5.0-alpha])
AM_INIT_AUTOMAKE([foreign])
AC_PROG_CC
AC_PROG_CC_C99

AM_SILENT_RULES([yes])

PKG_CHECK_MODULES([PNG], [libpng16])

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
AC_OUTPUT
