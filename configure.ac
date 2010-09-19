dnl  Copyright 2005 Red Hat, Inc.
dnl 
dnl  Permission to use, copy, modify, distribute, and sell this software and its
dnl  documentation for any purpose is hereby granted without fee, provided that
dnl  the above copyright notice appear in all copies and that both that
dnl  copyright notice and this permission notice appear in supporting
dnl  documentation, and that the name of Red Hat not be used in
dnl  advertising or publicity pertaining to distribution of the software without
dnl  specific, written prior permission.  Red Hat makes no
dnl  representations about the suitability of this software for any purpose.  It
dnl  is provided "as is" without express or implied warranty.
dnl 
dnl  RED HAT DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl  EVENT SHALL RED HAT BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl  PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

AC_PREREQ([2.57])
AC_INIT(twm,[1.0.4], [https://bugs.freedesktop.org/enter_bug.cgi?product=xorg],twm)
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

AM_CONFIG_HEADER(config.h)

# Require xorg-macros: XORG_DEFAULT_OPTIONS
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.3 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.3)
XORG_DEFAULT_OPTIONS

AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_YACC
AC_PATH_PROG([YACC_INST], $YACC)
AC_CHECK_FILE([$srcdir/gram.c], [],
              [test -z "$YACC_INST" && AC_MSG_ERROR([yacc not found - unable to compile gram.y])])
AC_PROG_LEX

AC_CHECK_FUNC([mkstemp], 
   AC_DEFINE(HAS_MKSTEMP,1,[Define to 1 if you have the `mkstemp' function.]))

# Initialise TWM_SOURCECODE_ID_STRING for preprocessor
AC_PATH_PROG([CAT], [cat], [:])
AC_PATH_PROG([LS], [ls], [:])
AC_PATH_PROG([GREP], [grep], [:])
AC_PATH_PROG([SORT], [sort], [:])
AC_PATH_PROG([SED], [sed], [:])
AC_CHECK_PROGS([MD5], [md5sum gmd5sum md5 digest], [:])
if test ":" != "${MD5}"
then
    if test "digest" = "${MD5}"
    then
        MD5="${MD5} -a md5"
    elif test "md5" = "${MD5}"
    then
        MD5="${MD5} -q"
    else
        MD5="${MD5} -t -"
    fi
fi
# Create ".git.head.sha1" by "post-checkout" or "post-commit" with "git log HEAD~1.. | sed -n -e '/^commit/s/^commit //p' > .git.head.sha1"
if test -s .git.head.sha1 -a ":" != "${CAT}"
then
    TWM_SOURCECODE_ID_STRING="GIT-`${CAT} .git.head.sha1`"
elif test ":" != "${MD5}" -a ":" != "${CAT}" -a ":" != "${LS}" -a ":" != "${GREP}" -a ":" != "${SORT}" -a ":" != "${SED}"
then
    createIDstring ()
    {
        ${CAT} `${LS} src/*.[[chyl]] | ${GREP} -v gram.[[ch]] | ${GREP} -v lex.c | ${SORT}` | ${MD5} | ${SED} -e 's/[[ ]]*[[-]]$//'
    }
    TWM_SOURCECODE_ID_STRING="MD5-`createIDstring`"
else
    TWM_SOURCECODE_ID_STRING="missing"
fi
TWM_CFLAGS="${TWM_CFLAGS} -DTWM_SOURCECODE_ID_STRING=\\\"${TWM_SOURCECODE_ID_STRING}\\\""

# Checks for pkg-config packages
PKG_CHECK_MODULES(TWM, x11 xext xt xmu ice sm)

# Xft library checks
AC_ARG_ENABLE(xft, AC_HELP_STRING([--disable-xft],[Turn off Xft support @<:@default=enabled@:>@]))
if test "no" != "$enable_xft"
then
    PKG_CHECK_EXISTS(xft,[PKG_CHECK_MODULES(xft, xft);xft_AUX_CFLAGS=-DTWM_USE_XFT])
    TWM_CFLAGS="${TWM_CFLAGS} ${xft_CFLAGS} ${xft_AUX_CFLAGS}"
    TWM_LIBS="${TWM_LIBS} ${xft_LIBS}"
fi

# Xinerama library checks
AC_ARG_ENABLE(xinerama, AC_HELP_STRING([--disable-xinerama],[Turn off Xinerama support @<:@default=enabled@:>@]))
if test "no" != "$enable_xinerama"
then
    PKG_CHECK_EXISTS(xinerama,[PKG_CHECK_MODULES(xinerama, xinerama);xinerama_AUX_CFLAGS=-DTWM_USE_XINERAMA])
    TWM_CFLAGS="${TWM_CFLAGS} ${xinerama_CFLAGS} ${xinerama_AUX_CFLAGS}"
    TWM_LIBS="${TWM_LIBS} ${xinerama_LIBS}"
fi

# Xrandr library checks
AC_ARG_ENABLE(xrandr, AC_HELP_STRING([--disable-xrandr],[Turn off Xrandr support @<:@default=enabled@:>@]))
if test "no" != "$enable_xrandr"
then
    PKG_CHECK_EXISTS(xrandr,[PKG_CHECK_MODULES(xrandr, xrandr);xrandr_AUX_CFLAGS=-DTWM_USE_XRANDR])
    TWM_CFLAGS="${TWM_CFLAGS} ${xrandr_CFLAGS} ${xrandr_AUX_CFLAGS}"
    TWM_LIBS="${TWM_LIBS} ${xrandr_LIBS}"
fi

# Xrender library checks
AC_ARG_ENABLE(xrender, AC_HELP_STRING([--disable-xrender],[Turn off Xrender support @<:@default=enabled@:>@]))
if test "no" != "$enable_xrender"
then
    PKG_CHECK_EXISTS(xrender,[PKG_CHECK_MODULES(xrender, xrender);xrender_AUX_CFLAGS=-DTWM_USE_RENDER])
    TWM_CFLAGS="${TWM_CFLAGS} ${xrender_CFLAGS} ${xrender_AUX_CFLAGS}"
    TWM_LIBS="${TWM_LIBS} ${xrender_LIBS}"
fi

# Xcomposite library checks
AC_ARG_ENABLE(xcomposite, AC_HELP_STRING([--disable-xcomposite],[Turn off Xcomposite support, assumes Xrender @<:@default=enabled@:>@]))
if test "no" != "$enable_xcomposite"
then
    PKG_CHECK_EXISTS(xcomposite,[PKG_CHECK_MODULES(xcomposite, xcomposite);xcomposite_AUX_CFLAGS=-DTWM_USE_COMPOSITE])
    TWM_CFLAGS="${TWM_CFLAGS} ${xcomposite_CFLAGS} ${xcomposite_AUX_CFLAGS}"
    TWM_LIBS="${TWM_LIBS} ${xcomposite_LIBS}"
fi

# Sloppy focus
AC_ARG_ENABLE(sloppyfocus, AC_HELP_STRING([--disable-sloppyfocus],[Turn off window sloppy focus support @<:@default=enabled@:>@]))
if test "no" != "$enable_sloppyfocus"
then
    TWM_CFLAGS="${TWM_CFLAGS} -DTWM_USE_SLOPPYFOCUS"
    echo "checking for SloppyFocus... enabled"
else
    echo "checking for SloppyFocus... disabled"
fi

# Dynamic, font-size based text items spacing
AC_ARG_ENABLE(fontspacing, AC_HELP_STRING([--disable-fontspacing],[Turn off font-size based text items spacing @<:@default=enabled@:>@]))
if test "no" != "$enable_fontspacing"
then
    TWM_CFLAGS="${TWM_CFLAGS} -DTWM_USE_SPACING"
    echo "checking for font-size based text spacing... enabled"
else
    echo "checking for font-size based text spacing... disabled"
fi

AC_CHECK_HEADER([pwd.h], [HAVE_PWD_H=-DHAVE_PWD_H], [])
TWM_CFLAGS="${TWM_CFLAGS} ${HAVE_PWD_H}"

AC_SUBST(TWM_CFLAGS)
AC_SUBST(TWM_LIBS)


AC_OUTPUT([Makefile
	   src/Makefile
	   man/Makefile])
