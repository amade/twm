/*****************************************************************************/
/*

Copyright 1989, 1998  The Open Group

Permission to use, copy, modify, distribute, and sell this software and its
documentation for any purpose is hereby granted without fee, provided that
the above copyright notice appear in all copies and that both that
copyright notice and this permission notice appear in supporting
documentation.

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
OPEN GROUP BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of The Open Group shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from The Open Group.

*/
/**       Copyright 1988 by Evans & Sutherland Computer Corporation,        **/
/**                          Salt Lake City, Utah                           **/
/**                        Cambridge, Massachusetts                         **/
/**                                                                         **/
/**                           All Rights Reserved                           **/
/**                                                                         **/
/**    Permission to use, copy, modify, and distribute this software and    **/
/**    its documentation  for  any  purpose  and  without  fee is hereby    **/
/**    granted, provided that the above copyright notice appear  in  all    **/
/**    copies and that both  that  copyright  notice  and  this  permis-    **/
/**    sion  notice appear in supporting  documentation,  and  that  the    **/
/**    name of Evans & Sutherland not be used in advertising    **/
/**    in publicity pertaining to distribution of the  software  without    **/
/**    specific, written prior permission.                                  **/
/**                                                                         **/
/**    EVANS & SUTHERLAND DISCLAIMs ALL WARRANTIES WITH REGARD    **/
/**    TO THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES  OF  MERCHANT-    **/
/**    ABILITY  AND  FITNESS,  IN  NO  EVENT SHALL EVANS & SUTHERLAND    **/
/**    BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL  DAM-    **/
/**    AGES OR  ANY DAMAGES WHATSOEVER  RESULTING FROM LOSS OF USE, DATA    **/
/**    OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER    **/
/**    TORTIOUS ACTION, ARISING OUT OF OR IN  CONNECTION  WITH  THE  USE    **/
/**    OR PERFORMANCE OF THIS SOFTWARE.                                     **/
/*****************************************************************************/
/* $XFree86: xc/programs/twm/util.c,v 1.12 2002/09/19 13:22:05 tsi Exp $ */


/***********************************************************************
 *
 * $Xorg: util.c,v 1.5 2001/02/09 02:05:37 xorgcvs Exp $
 *
 * utility routines for twm
 *
 * 28-Oct-87 Thomas E. LaStrange	File created
 *
 ***********************************************************************/

#include "twm.h"
#include "util.h"
#include "gram.h"
#include "parse.h"
#include "screen.h"
#include <X11/Xos.h>
#include <X11/Xatom.h>
#include <stdio.h>
#include <ctype.h>
#include <X11/Xmu/Drawing.h>
#include <X11/Xmu/CharSet.h>
#ifdef TWM_USE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif

#define siconify_11x11_width 11
#define siconify_11x11_height 11
static unsigned char siconify_11x11_bits[] = {
   0xff, 0x07, 0x01, 0x04, 0x0d, 0x05, 0x9d, 0x05, 0xb9, 0x04, 0x51, 0x04,
   0xe9, 0x04, 0xcd, 0x05, 0x85, 0x05, 0x01, 0x04, 0xff, 0x07};
#define siconify_13x13_width 13
#define siconify_13x13_height 13
static unsigned char siconify_13x13_bits[] = {
   0xff, 0x1f, 0x01, 0x10, 0x0d, 0x14, 0x1d, 0x16, 0x39, 0x13, 0x71, 0x11,
   0xa1, 0x10, 0xd1, 0x11, 0x99, 0x13, 0x0d, 0x17, 0x05, 0x16, 0x01, 0x10,
   0xff, 0x1f};
#define siconify_15x15_width 15
#define siconify_15x15_height 15
static unsigned char siconify_15x15_bits[] = {
   0xff, 0x7f, 0x01, 0x40, 0x1d, 0x50, 0x3d, 0x58, 0x7d, 0x5c, 0xf9, 0x4e,
   0xf1, 0x46, 0x61, 0x43, 0xb1, 0x47, 0xb9, 0x4f, 0x1d, 0x5f, 0x0d, 0x5e,
   0x05, 0x5c, 0x01, 0x40, 0xff, 0x7f};
#define siconify_17x17_width 17
#define siconify_17x17_height 17
static unsigned char siconify_17x17_bits[] = {
   0xff, 0xff, 0x01, 0x01, 0x00, 0x01, 0x1d, 0x40, 0x01, 0x3d, 0x60, 0x01,
   0x7d, 0x70, 0x01, 0xf9, 0x38, 0x01, 0xf1, 0x1d, 0x01, 0xe1, 0x0d, 0x01,
   0xc1, 0x06, 0x01, 0x61, 0x0f, 0x01, 0x71, 0x1f, 0x01, 0x39, 0x3e, 0x01,
   0x1d, 0x7c, 0x01, 0x0d, 0x78, 0x01, 0x05, 0x70, 0x01, 0x01, 0x00, 0x01,
   0xff, 0xff, 0x01};
#define siconify_19x19_width 19
#define siconify_19x19_height 19
static unsigned char siconify_19x19_bits[] = {
   0xff, 0xff, 0x07, 0x01, 0x00, 0x04, 0x3d, 0x00, 0x05, 0x7d, 0x80, 0x05,
   0xfd, 0xc0, 0x05, 0xfd, 0xe1, 0x05, 0xf9, 0xf3, 0x04, 0xf1, 0x77, 0x04,
   0xe1, 0x3b, 0x04, 0xc1, 0x1d, 0x04, 0xe1, 0x3e, 0x04, 0x71, 0x7f, 0x04,
   0x79, 0xfe, 0x04, 0x3d, 0xfc, 0x05, 0x1d, 0xf8, 0x05, 0x0d, 0xf0, 0x05,
   0x05, 0xe0, 0x05, 0x01, 0x00, 0x04, 0xff, 0xff, 0x07};
#define siconify_21x21_width 21
#define siconify_21x21_height 21
static unsigned char siconify_21x21_bits[] = {
   0xff, 0xff, 0x1f, 0x01, 0x00, 0x10, 0x3d, 0x00, 0x14, 0x7d, 0x00, 0x16,
   0xfd, 0x00, 0x17, 0xfd, 0x81, 0x17, 0xf9, 0xc3, 0x13, 0xf1, 0xe7, 0x11,
   0xe1, 0xef, 0x10, 0xc1, 0x77, 0x10, 0x81, 0x3b, 0x10, 0xc1, 0x7d, 0x10,
   0xe1, 0xfe, 0x10, 0xf1, 0xfc, 0x11, 0x79, 0xf8, 0x13, 0x3d, 0xf0, 0x17,
   0x1d, 0xe0, 0x17, 0x0d, 0xc0, 0x17, 0x05, 0x80, 0x17, 0x01, 0x00, 0x10,
   0xff, 0xff, 0x1f};

static Pixmap CreateXLogoPixmap ( unsigned int *widthp, 
				  unsigned int *heightp );
static Pixmap CreateResizePixmap ( unsigned int *widthp, 
				   unsigned int *heightp );
static Pixmap CreateDotPixmap ( unsigned int *widthp, 
				unsigned int *heightp );
static Pixmap CreateQuestionPixmap ( unsigned int *widthp, 
				     unsigned int *heightp );
static Pixmap CreateMenuPixmap ( unsigned int *widthp, 
				 unsigned int *heightp );

int HotX, HotY;

/** 
 * move a window outline
 *
 *  \param root         window we are outlining
 *  \param x,y          upper left coordinate
 *  \param width,height size of the rectangle
 *  \param bw           border width of the frame
 *  \param th           title height
 */
void MoveOutline(Window root, int x, int y, int width, int height, int bw, int th)
{
    static int	lastx = 0;
    static int	lasty = 0;
    static int	lastWidth = 0;
    static int	lastHeight = 0;
    static int	lastBW = 0;
    static int	lastTH = 0;
    int		xl, xr, yt, yb, xinnerl, xinnerr, yinnert, yinnerb;
    int		xthird, ythird;
    XSegment	outline[18];
    register XSegment	*r;

    if (x == lastx && y == lasty && width == lastWidth && height == lastHeight
	&& lastBW == bw && th == lastTH)
	return;
    
    r = outline;

#define DRAWIT() \
    if (lastWidth || lastHeight)			\
    {							\
	xl = lastx;					\
	xr = lastx + lastWidth - 1;			\
	yt = lasty;					\
	yb = lasty + lastHeight - 1;			\
	xinnerl = xl + lastBW;				\
	xinnerr = xr - lastBW;				\
	yinnert = yt + lastTH + lastBW;			\
	yinnerb = yb - lastBW;				\
	xthird = (xinnerr - xinnerl) / 3;		\
	ythird = (yinnerb - yinnert) / 3;		\
							\
	r->x1 = xl;					\
	r->y1 = yt;					\
	r->x2 = xr;					\
	r->y2 = yt;					\
	r++;						\
							\
	r->x1 = xl;					\
	r->y1 = yb;					\
	r->x2 = xr;					\
	r->y2 = yb;					\
	r++;						\
							\
	r->x1 = xl;					\
	r->y1 = yt;					\
	r->x2 = xl;					\
	r->y2 = yb;					\
	r++;						\
							\
	r->x1 = xr;					\
	r->y1 = yt;					\
	r->x2 = xr;					\
	r->y2 = yb;					\
	r++;						\
							\
	r->x1 = xinnerl + xthird;			\
	r->y1 = yinnert;				\
	r->x2 = r->x1;					\
	r->y2 = yinnerb;				\
	r++;						\
							\
	r->x1 = xinnerl + (2 * xthird);			\
	r->y1 = yinnert;				\
	r->x2 = r->x1;					\
	r->y2 = yinnerb;				\
	r++;						\
							\
	r->x1 = xinnerl;				\
	r->y1 = yinnert + ythird;			\
	r->x2 = xinnerr;				\
	r->y2 = r->y1;					\
	r++;						\
							\
	r->x1 = xinnerl;				\
	r->y1 = yinnert + (2 * ythird);			\
	r->x2 = xinnerr;				\
	r->y2 = r->y1;					\
	r++;						\
							\
	if (lastTH != 0) {				\
	    r->x1 = xl;					\
	    r->y1 = yt + lastTH;			\
	    r->x2 = xr;					\
	    r->y2 = r->y1;				\
	    r++;					\
	}						\
    }

    /* undraw the old one, if any */
    DRAWIT ();

    lastx = x;
    lasty = y;
    lastWidth = width;
    lastHeight = height;
    lastBW = bw;
    lastTH = th;

    /* draw the new one, if any */
    DRAWIT ();

#undef DRAWIT


    if (r != outline)
    {
	XDrawSegments(dpy, root, Scr->DrawGC, outline, r - outline);
    }
}

/**
 * zoom in or out of an icon
 *
 *  \param wf window to zoom from
 *  \param wt window to zoom to
 */
void
Zoom(Window wf, Window wt)
{
    int fx, fy, tx, ty;			/* from, to */
    int fw, fh, tw, th;			/* from, to */
    long dx, dy, dw, dh;
    long z;
    int j;

    if (!Scr->DoZoom || Scr->ZoomCount < 1) return;

    if (wf == None || wt == None) return;

    XGetGeometry (dpy, wf, &JunkRoot, &fx, &fy, &fw, &fh, &JunkBW, &JunkDepth);
    XGetGeometry (dpy, wt, &JunkRoot, &tx, &ty, &tw, &th, &JunkBW, &JunkDepth);

    dx = ((long) (tx - fx));	/* going from -> to */
    dy = ((long) (ty - fy));	/* going from -> to */
    dw = ((long) (tw - fw));	/* going from -> to */
    dh = ((long) (th - fh));	/* going from -> to */
    z = (long) (Scr->ZoomCount + 1);

    for (j = 0; j < 2; j++) {
	long i;

	XDrawRectangle (dpy, Scr->Root, Scr->DrawGC, fx, fy, fw, fh);
	for (i = 1; i < z; i++) {
	    int x = fx + (int) ((dx * i) / z);
	    int y = fy + (int) ((dy * i) / z);
	    unsigned width = (unsigned) (((long) fw) + (dw * i) / z);
	    unsigned height = (unsigned) (((long) fh) + (dh * i) / z);
	
	    XDrawRectangle (dpy, Scr->Root, Scr->DrawGC,
			    x, y, width, height);
	    XFlush (dpy);
	    usleep (5000); /* 5 millisec sleep */
	}
	XDrawRectangle (dpy, Scr->Root, Scr->DrawGC, tx, ty, tw, th);
    }
}

/**
 * expand the tilde character to HOME if it is the first 
 * character of the filename
 *
 *	\return a pointer to the new name
 *
 *  \param name  the filename to expand
 */
char *
ExpandFilename(char *name)
{
    char *newname;

    if (name[0] != '~') return name;

    newname = (char *) malloc (HomeLen + strlen(name) + 2);
    if (!newname) {
	fprintf (stderr, 
		 "%s:  unable to allocate %ld bytes to expand filename %s/%s\n",
		 ProgramName, HomeLen + (unsigned long)strlen(name) + 2,
		 Home, &name[1]);
    } else {
	(void) sprintf (newname, "%s/%s", Home, name + (name[1] == '/' ? 2 : 1));
    }

    return newname;
}

/**
 * read in the bitmap file for the unknown icon
 *
 * \param name  the filename to read
 */
void
GetUnknownIcon(char *name)
{
    if ((Scr->UnknownPm = GetBitmap(name)) != None)
    {
	XGetGeometry(dpy, Scr->UnknownPm, &JunkRoot, &JunkX, &JunkY,
	    (unsigned int *)&Scr->UnknownWidth, (unsigned int *)&Scr->UnknownHeight, &JunkBW, &JunkDepth);
    }
}

/**
 *	FindBitmap - read in a bitmap file and return size
 *
 *  \return pixmap associated with bitmap
 *
 *  \param name          filename to read
 *  \param[out] widthp   pointer to width of bitmap
 *  \param[out] heightp	 pointer to height of bitmap
 */
Pixmap 
FindBitmap (char *name, unsigned *widthp, unsigned *heightp)
{
    char *bigname;
    Pixmap pm;

    if (!name) return None;

    /*
     * Names of the form :name refer to hardcoded images that are scaled to
     * look nice in title buttons.  Eventually, it would be nice to put in a
     * menu symbol as well....
     */
    if (name[0] == ':') {
	int i;
	static struct {
	    char *name;
	    Pixmap (*proc)(unsigned int *, unsigned int *);
	} pmtab[] = {
	    { TBPM_DOT,		CreateDotPixmap },
	    { TBPM_ICONIFY,	CreateDotPixmap },
	    { TBPM_RESIZE,	CreateResizePixmap },
	    { TBPM_XLOGO,	CreateXLogoPixmap },
	    { TBPM_DELETE,	CreateXLogoPixmap },
	    { TBPM_MENU,	CreateMenuPixmap },
	    { TBPM_QUESTION,	CreateQuestionPixmap },
	};
	
	for (i = 0; i < (sizeof pmtab)/(sizeof pmtab[0]); i++) {
	    if (XmuCompareISOLatin1 (pmtab[i].name, name) == 0)
	      return (*pmtab[i].proc) (widthp, heightp);
	}
	fprintf (stderr, "%s:  no such built-in bitmap \"%s\"\n",
		 ProgramName, name);
	return None;
    }

    /*
     * Generate a full pathname if any special prefix characters (such as ~)
     * are used.  If the bigname is different from name, bigname will need to
     * be freed.
     */
    bigname = ExpandFilename (name);
    if (!bigname) return None;

    /*
     * look along bitmapFilePath resource same as toolkit clients
     */
    pm = XmuLocateBitmapFile (ScreenOfDisplay(dpy, Scr->screen), bigname, NULL,
			      0, (int *)widthp, (int *)heightp, &HotX, &HotY);
    if (pm == None && Scr->IconDirectory && bigname[0] != '/') {
	if (bigname != name) free (bigname);
	/*
	 * Attempt to find icon in old IconDirectory (now obsolete)
	 */
	bigname = (char *) malloc (strlen(name) + strlen(Scr->IconDirectory) +
				   2);
	if (!bigname) {
	    fprintf (stderr,
		     "%s:  unable to allocate memory for \"%s/%s\"\n",
		     ProgramName, Scr->IconDirectory, name);
	    return None;
	}
	(void) sprintf (bigname, "%s/%s", Scr->IconDirectory, name);
	if (XReadBitmapFile (dpy, Scr->Root, bigname, widthp, heightp, &pm,
			     &HotX, &HotY) != BitmapSuccess) {
	    pm = None;
	}
    }
    if (bigname != name) free (bigname);
    if (pm == None) {
	fprintf (stderr, "%s:  unable to find bitmap \"%s\"\n", 
		 ProgramName, name);
    }

    return pm;
}

Pixmap 
GetBitmap (char *name)
{
    return FindBitmap (name, &JunkWidth, &JunkHeight);
}

void
InsertRGBColormap (Atom a, XStandardColormap *maps, int nmaps, Bool replace)
{
    StdCmap *sc = NULL;

    if (replace) {			/* locate existing entry */
	for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
	    if (sc->atom == a) break;
	}
    }

    if (!sc) {				/* no existing, allocate new */
	sc = (StdCmap *) malloc (sizeof (StdCmap));
	if (!sc) {
	    fprintf (stderr, "%s:  unable to allocate %ld bytes for StdCmap\n",
		     ProgramName, (unsigned long)sizeof (StdCmap));
	    return;
	}
    }

    if (replace) {			/* just update contents */
	if (sc->maps) XFree ((char *) maps);
	if (sc == Scr->StdCmapInfo.mru) Scr->StdCmapInfo.mru = NULL;
    } else {				/* else appending */
	sc->next = NULL;
	sc->atom = a;
	if (Scr->StdCmapInfo.tail) {
	    Scr->StdCmapInfo.tail->next = sc;
	} else {
	    Scr->StdCmapInfo.head = sc;
	}
	Scr->StdCmapInfo.tail = sc;
    }
    sc->nmaps = nmaps;
    sc->maps = maps;

    return;
}

void
RemoveRGBColormap (Atom a)
{
    StdCmap *sc, *prev;

    prev = NULL;
    for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {  
	if (sc->atom == a) break;
	prev = sc;
    }
    if (sc) {				/* found one */
	if (sc->maps) XFree ((char *) sc->maps);
	if (prev) prev->next = sc->next;
	if (Scr->StdCmapInfo.head == sc) Scr->StdCmapInfo.head = sc->next;
	if (Scr->StdCmapInfo.tail == sc) Scr->StdCmapInfo.tail = prev;
	if (Scr->StdCmapInfo.mru == sc) Scr->StdCmapInfo.mru = NULL;
    }
    return;
}

void
LocateStandardColormaps()
{
    Atom *atoms;
    int natoms;
    int i;

    atoms = XListProperties (dpy, Scr->Root, &natoms);
    for (i = 0; i < natoms; i++) {
	XStandardColormap *maps = NULL;
	int nmaps;

	if (XGetRGBColormaps (dpy, Scr->Root, &maps, &nmaps, atoms[i])) {
	    /* if got one, then append to current list */
	    InsertRGBColormap (atoms[i], maps, nmaps, False);
	}
    }
    if (atoms) XFree ((char *) atoms);
    return;
}

void
GetColor(int kind, Pixel *what, char *name)
{
    XColor color, junkcolor;
    Status stat = 0;
    Colormap cmap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;

#ifndef TOM
    if (!Scr->FirstTime)
	return;
#endif

    if (Scr->Monochrome != kind)
	return;

    if (!XAllocNamedColor (dpy, cmap, name, &color, &junkcolor))
    {
	/* if we could not allocate the color, let's see if this is a
	 * standard colormap
	 */
	XStandardColormap *stdcmap = NULL;

	/* parse the named color */
	if (name[0] != '#')
	    stat = XParseColor (dpy, cmap, name, &color);
	if (!stat)
	{
	    fprintf (stderr, "%s:  invalid color name \"%s\"\n", 
		     ProgramName, name);
	    return;
	}

	/*
	 * look through the list of standard colormaps (check cache first)
	 */
	if (Scr->StdCmapInfo.mru && Scr->StdCmapInfo.mru->maps &&
	    (Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex].colormap ==
	     cmap)) {
	    stdcmap = &(Scr->StdCmapInfo.mru->maps[Scr->StdCmapInfo.mruindex]);
	} else {
	    StdCmap *sc;

	    for (sc = Scr->StdCmapInfo.head; sc; sc = sc->next) {
		int i;

		for (i = 0; i < sc->nmaps; i++) {
		    if (sc->maps[i].colormap == cmap) {
			Scr->StdCmapInfo.mru = sc;
			Scr->StdCmapInfo.mruindex = i;
			stdcmap = &(sc->maps[i]);
			goto gotit;
		    }
		}
	    }
	}

      gotit:
	if (stdcmap) {
            color.pixel = (stdcmap->base_pixel +
			   ((Pixel)(((float)color.red / 65535.0) *
				    stdcmap->red_max + 0.5) *
			    stdcmap->red_mult) +
			   ((Pixel)(((float)color.green /65535.0) *
				    stdcmap->green_max + 0.5) *
			    stdcmap->green_mult) +
			   ((Pixel)(((float)color.blue  / 65535.0) *
				    stdcmap->blue_max + 0.5) *
			    stdcmap->blue_mult));
        } else {
	    fprintf (stderr, "%s:  unable to allocate color \"%s\"\n", 
		     ProgramName, name);
	    return;
	}
    }

    *what = color.pixel;
}

void
GetColorValue(int kind, XColor *what, char *name)
{
    XColor junkcolor;
    Colormap cmap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;

#ifndef TOM
    if (!Scr->FirstTime)
	return;
#endif

    if (Scr->Monochrome != kind)
	return;

    if (!XLookupColor (dpy, cmap, name, what, &junkcolor))
    {
	fprintf (stderr, "%s:  invalid color name \"%s\"\n", 
		 ProgramName, name);
    }
    else
    {
	what->pixel = AllPlanes;
    }
}

/* 
 * The following functions are sensible to 'use_fontset'.
 * When 'use_fontset' is True,
 *  - XFontSet-related internationalized functions are used
 *     so as multibyte languages can be displayed.
 * When 'use_fontset' is False,
 *  - XFontStruct-related conventional functions are used
 *     so as 8-bit characters can be displayed even when
 *     locale is not set properly.
 */
void
GetFont(MyFont *font)
{
    char *deffontname = "fixed";
    char **missing_charset_list_return;
    int missing_charset_count_return;
    char *def_string_return;
    XFontSetExtents *font_extents;
    XFontStruct **xfonts;
    char **font_names;
    register int i;
    int ascent;
    int descent;
    int fnum;
    char *basename2;

#ifdef TWM_USE_XFT

    XFontStruct *xlfd;
    Atom   atom;
    char  *atom_name;

    if (Scr->use_xft > 0) {

	if (font->xft != NULL) {
	    XftFontClose (dpy, font->xft);
	    font->xft = NULL;
	}

	/* test if core font: */
	fnum = 0;
	xlfd = NULL;
	font_names = XListFontsWithInfo (dpy, font->name, 5, &fnum, &xlfd);
	if (font_names != NULL && xlfd != NULL) {
	    for (i = 0; i < fnum; ++i)
		if (XGetFontProperty (xlfd+i, XA_FONT, &atom) == True) {
		    atom_name = XGetAtomName (dpy, atom);
		    if (atom_name) {
			if (PrintErrorMessages)
			    fprintf (stderr, "%s: Xft loading X%d core font '%s'.\n",
					ProgramName, XProtocolVersion(dpy), atom_name);
			font->xft = XftFontOpenXlfd (dpy, Scr->screen, atom_name);
			XFree (atom_name);
			break;
		    }
		}
	    XFreeFontInfo (font_names, xlfd, fnum);
	}

	/* next, try Xft font: */
	if (font->xft == NULL)
	    font->xft = XftFontOpenName (dpy, Scr->screen, font->name);

	/* fallback: */
	if (font->xft == NULL) {
	    if (Scr->DefaultFont.name != NULL)
		deffontname = Scr->DefaultFont.name;

	    fnum = 0;
	    xlfd = NULL;
	    font_names = XListFontsWithInfo (dpy, deffontname, 5, &fnum, &xlfd);
	    if (font_names != NULL && xlfd != NULL) {
		for (i = 0; i < fnum; ++i)
		    if (XGetFontProperty (xlfd+i, XA_FONT, &atom) == True) {
			atom_name = XGetAtomName (dpy, atom);
			if (atom_name) {
			    font->xft = XftFontOpenXlfd (dpy, Scr->screen, atom_name);
			    XFree (atom_name);
			    break;
			}
		    }
		XFreeFontInfo (font_names, xlfd, fnum);
	    }

	    if (font->xft == NULL)
		font->xft = XftFontOpenName (dpy, Scr->screen, deffontname);

	    if (font->xft == NULL) {
		fprintf (stderr, "%s:  unable to open fonts \"%s\" or \"%s\"\n",
			ProgramName, font->name, deffontname);
		exit(1);
	    }
	}

	font->height = font->xft->ascent + font->xft->descent;
	font->y = font->xft->ascent;
	font->ascent = font->xft->ascent;
	font->descent = font->xft->descent;
	font->ellen = MyFont_TextWidth (font, "...", 3);
	return;
    }

#endif /*TWM_USE_XFT*/

    if (use_fontset) {
	if (font->fontset != NULL){
	    XFreeFontSet(dpy, font->fontset);
	}

	basename2 = (char *)malloc(strlen(font->name) + 3);
	if (basename2) sprintf(basename2, "%s,*", font->name);
	else basename2 = font->name;
	if( (font->fontset = XCreateFontSet(dpy, basename2,
					    &missing_charset_list_return,
					    &missing_charset_count_return,
					    &def_string_return)) == NULL) {
	    fprintf (stderr, "%s:  unable to open fontset \"%s\"\n",
			 ProgramName, font->name);
	    exit(1);
	}
	if (PrintErrorMessages)
	    fprintf (stderr, "%s: Loading X%d core fontset '%s'\n",
			ProgramName, XProtocolVersion(dpy), basename2);
	if (basename2 != font->name) free(basename2);
	for(i=0; i<missing_charset_count_return; i++){
	    printf("%s: warning: font for charset %s is lacking.\n",
		   ProgramName, missing_charset_list_return[i]);
	}

	font_extents = XExtentsOfFontSet(font->fontset);
	fnum = XFontsOfFontSet(font->fontset, &xfonts, &font_names);
	for( i = 0, ascent = 0, descent = 0; i<fnum; i++){
#if 1
	    if (PrintErrorMessages)
		fprintf (stderr, "%6d. '%s' ascent = %d descent = %d\n", i+1,
			font_names[i], (*xfonts)->ascent, (*xfonts)->descent);
#endif
	    if (ascent < (*xfonts)->ascent) ascent = (*xfonts)->ascent;
	    if (descent < (*xfonts)->descent) descent = (*xfonts)->descent;
	    xfonts++;
	}
	font->height = font_extents->max_logical_extent.height;
	font->y = ascent;
	font->ascent = ascent;
	font->descent = descent;
	font->ellen = MyFont_TextWidth (font, "...", 3);
	if (PrintErrorMessages)
	    fprintf (stderr, "%s: Extents for '%s' height = %d, ascent = %d, descent = %d\n",
			ProgramName, font->name, font->height, font->ascent, font->descent);
	return;
    }

    if (font->font != NULL)
	XFreeFont(dpy, font->font);

    if ((font->font = XLoadQueryFont(dpy, font->name)) == NULL)
    {
	if (Scr->DefaultFont.name) {
	    deffontname = Scr->DefaultFont.name;
	}
	if ((font->font = XLoadQueryFont(dpy, deffontname)) == NULL)
	{
	    fprintf (stderr, "%s:  unable to open fonts \"%s\" or \"%s\"\n",
		     ProgramName, font->name, deffontname);
	    exit(1);
	}

    }
    font->height = font->font->ascent + font->font->descent;
    font->y = font->font->ascent;
    font->ascent = font->font->ascent;
    font->descent = font->font->descent;
    font->ellen = MyFont_TextWidth (font, "...", 3);
}

int
MyFont_TextWidth(MyFont *font, char *string, int len)
{
    XRectangle ink_rect;
    XRectangle logical_rect;

#ifdef TWM_USE_XFT
    XGlyphInfo size;

    if (font->scr->use_xft > 0) {
#ifdef X_HAVE_UTF8_STRING
	if (use_fontset)
	    XftTextExtentsUtf8 (dpy, font->xft, (XftChar8*)(string), len, &size);
	else
#endif
	    XftTextExtents8 (dpy, font->xft, (XftChar8*)(string), len, &size);
	return size.width;
    }
#endif

    if (use_fontset) {
	XmbTextExtents(font->fontset, string, len, &ink_rect, &logical_rect);
	return logical_rect.width;
    }
    return XTextWidth(font->font, string, len);
}

#ifdef TWM_USE_XFT
static Bool
MyFont_DrawXftString (XftDraw *draw, XftColor *col, XftFont *font,
                     int x, int y, char *string, int len)
{
    if (draw) {
#ifdef X_HAVE_UTF8_STRING
	if (use_fontset)
	    XftDrawStringUtf8 (draw, col, font, x, y, (XftChar8*)(string), len);
	else
#endif
	    XftDrawString8 (draw, col, font, x, y, (XftChar8*)(string), len);
	return True;
    }
    return False;
}
#endif

void
MyFont_DrawImageString (MyWindow *win, MyFont *font, ColorPair *col,
                       int x, int y, char *string, int len)
{
#ifdef TWM_USE_XFT
    if (font->scr->use_xft > 0) {
	XClearArea (dpy, win->win, 0, 0, 0, 0, False);
	MyFont_DrawXftString (win->xft, &col->xft, font->xft, x, y, string, len);
	return;
    }
#endif

    Gcv.foreground = col->fore;
    Gcv.background = col->back;
    if (use_fontset) {
	XChangeGC (dpy, font->scr->NormalGC, GCForeground|GCBackground, &Gcv);
	XmbDrawImageString (dpy, win->win, font->fontset, font->scr->NormalGC, x, y, string, len);
    } else {
	Gcv.font = font->font->fid;
	XChangeGC (dpy, font->scr->NormalGC, GCFont|GCForeground|GCBackground, &Gcv);
	XDrawImageString (dpy, win->win, font->scr->NormalGC, x, y, string, len);
    }
}

void
MyFont_DrawString (MyWindow *win, MyFont *font, ColorPair *col,
                  int x, int y, char *string, int len)
{
#ifdef TWM_USE_XFT
    if (font->scr->use_xft > 0) {
	MyFont_DrawXftString (win->xft, &col->xft, font->xft, x, y, string, len);
	return;
    }
#endif

    Gcv.foreground = col->fore;
    Gcv.background = col->back;
    if (use_fontset) {
	XChangeGC (dpy, font->scr->NormalGC, GCForeground|GCBackground, &Gcv);
	XmbDrawString (dpy, win->win, font->fontset, font->scr->NormalGC, x, y, string, len);
    } else {
	Gcv.font = font->font->fid;
	XChangeGC (dpy, font->scr->NormalGC, GCFont|GCForeground|GCBackground, &Gcv);
	XDrawString (dpy, win->win, font->scr->NormalGC, x, y, string, len);
    }
}

void
MyFont_DrawShapeString (Drawable mask, MyFont *font,
                       int x, int y, char *string, int len,
                       int offx, int offy)
{
    if (XGetGeometry(dpy, mask, &JunkRoot, &JunkX, &JunkY, &JunkWidth, &JunkHeight,
		&JunkBW, &JunkDepth))
    {
	GC gc;
	switch (JunkDepth) {
#ifdef TWM_USE_RENDER
	case 8:
	case 4:
            gc = font->scr->AlphaGC;
	    break;
#endif
	case 1:
	    gc = font->scr->BitGC;
	    break;
	default:
	    return;
	}
#ifdef TWM_USE_XFT
	if (font->scr->use_xft > 0)
	{
	    XftDraw *txt;
	    switch (JunkDepth) {
#ifdef TWM_USE_RENDER
	    case 8:
	    case 4:
		txt = XftDrawCreateAlpha (dpy, mask, font->scr->DepthA);
		break;
#endif
	    case 1:
		txt = XftDrawCreateBitmap (dpy, mask);
		break;
	    }
	    if (txt) {
		MyFont_DrawXftString (txt, &font->scr->XftWhite, font->xft, x, y, string, len);
		XftDrawDestroy (txt);
	    }
	}
	else
#endif
	{
	    Gcv.foreground = AllPlanes;
	    Gcv.background = 0;
	    if (use_fontset) {
		XChangeGC (dpy, gc, (GCForeground|GCBackground), &Gcv);
		XmbDrawString (dpy, mask, font->fontset, gc, x, y, string, len);
	    } else {
		Gcv.font = font->scr->IconFont.font->fid;
		XChangeGC (dpy, gc, GCFont|GCForeground|GCBackground, &Gcv);
		XDrawString (dpy, mask, gc, x, y, string, len);
	    }
	}
	XSetFunction (dpy, gc, GXor);
#if 0	/* sanity check (if necessary, then do on startup!): */
	if (offx > font->height || offx < -font->height)
	    offx = 0;
	if (offy > font->height || offy < -font->height)
	    offy = 0;
#endif
#if 1
	if (offy != 0)
	    XCopyArea (dpy, mask, mask, gc, 0, 0, JunkWidth, JunkHeight, 0, offy);
	if (offx != 0)
	    XCopyArea (dpy, mask, mask, gc, 0, 0, JunkWidth, JunkHeight, offx, 0);
#else
	if (offy != 0 || offx != 0)
	    XCopyArea (dpy, mask, mask, gc, 0, 0, JunkWidth, JunkHeight, offx, offy);
#endif
	XSetFunction (dpy, gc, GXcopy); /*restore*/
    }
}

static int
MyFont_StringLength (MyFont *font, char *string, int length, int width)
{
    /* calculate string length in characters not exceeding 'width' in pixels */
    int  i, w;

    w = MyFont_TextWidth (font, string, length);
    width = w - width;
    if (width > 0 && length > 1) {
	width += font->ellen; /* account for "..." */
	i = 0;
	/* cut off 'width' pixels from tail */
	do {
	    length -= 2; /* speedup with 2-increment */
	    i += 2;
	    w = MyFont_TextWidth (font, string+length, i);
	} while (width > w && length > 1);
	w = MyFont_TextWidth (font, string+length+1, i-1);
	if (width <= w)
	    ++length;
#if 1
	while (length > 1 && (string[length-1] == '.' || string[length-1] == ' '))
	    --length;
#endif
    }
    return length;
}

void
MyFont_DrawImageStringEllipsis (MyWindow *win, MyFont *font, ColorPair *col,
                       int x, int y, char *string, int len, int width)
{
    char buf[200+3+1];
    int l = MyFont_StringLength (font, string, len, width);

    if (l < len) {
	if (l > sizeof(buf)-(3+1))
	    l = sizeof(buf)-(3+1);
	strncpy (buf, string, l);
	strncpy (buf+l, "...", 3+1);
	MyFont_DrawImageString (win, font, col, x, y, buf, l+3);
    } else
	MyFont_DrawImageString (win, font, col, x, y, string, len);
}

void
MyFont_DrawStringEllipsis (MyWindow *win, MyFont *font, ColorPair *col,
                       int x, int y, char *string, int len, int width)
{
    char buf[200+3+1];
    int l = MyFont_StringLength (font, string, len, width);

    if (l < len) {
	if (l > sizeof(buf)-(3+1))
	    l = sizeof(buf)-(3+1);
	strncpy (buf, string, l);
	strncpy (buf+l, "...", 3+1);
	MyFont_DrawString (win, font, col, x, y, buf, l+3);
    } else
	MyFont_DrawString (win, font, col, x, y, string, len);
}

void
MyFont_DrawShapeStringEllipsis (Drawable mask, MyFont *font,
		       int x, int y, char *string, int len, int width,
		       int offx, int offy)
{
    char buf[200+3+1];
    int l = MyFont_StringLength (font, string, len, width);

    if (l < len) {
	if (l > sizeof(buf)-(3+1))
	    l = sizeof(buf)-(3+1);
	strncpy (buf, string, l);
	strncpy (buf+l, "...", 3+1);
	MyFont_DrawShapeString (mask, font, x, y, buf, l+3, offx, offy);
    } else
	MyFont_DrawShapeString (mask, font, x, y, string, len, offx, offy);
}

/*
 * The following functions are internationalized substitutions
 * for XFetchName and XGetIconName using XGetWMName and
 * XGetWMIconName.  
 *
 * Please note that the second arguments have to be freed using free(),
 * not XFree().
 */
Status
I18N_FetchName (Window w, char **winname)
{
    int    status;
    XTextProperty text_prop;
    char **list;
    int    num;

    text_prop.value = NULL;
    status = XGetWMName(dpy, w, &text_prop);

    if (status == 0 || text_prop.value == NULL || text_prop.nitems == 0) {
	if (text_prop.value)
	    XFree(text_prop.value);
	*winname = NULL;
	return 0;
    }

    list = NULL;
#if defined TWM_USE_XFT && defined X_HAVE_UTF8_STRING
    if (use_fontset)
	status = Xutf8TextPropertyToTextList(dpy, &text_prop, &list, &num);
    else
#endif
	status = XmbTextPropertyToTextList(dpy, &text_prop, &list, &num);

    if (status < Success || list == NULL || *list == NULL)
	*winname = strdup((char *)text_prop.value);
    else
	*winname = strdup(*list);

    if (list)
	XFreeStringList(list);
    XFree(text_prop.value);
    return 1;
}

Status
I18N_GetIconName (Window w, char **iconname)
{
    int    status;
    XTextProperty text_prop;
    char **list;
    int    num;

    text_prop.value = NULL;
    status = XGetWMIconName(dpy, w, &text_prop);

    if (status == 0 || text_prop.value == NULL || text_prop.nitems == 0) {
	if (text_prop.value)
	    XFree(text_prop.value);
	*iconname = NULL;
	return 0;
    }

    list = NULL;
#if defined TWM_USE_XFT && defined X_HAVE_UTF8_STRING
    if (use_fontset)
	status = Xutf8TextPropertyToTextList(dpy, &text_prop, &list, &num);
    else
#endif
	status = XmbTextPropertyToTextList(dpy, &text_prop, &list, &num);

    if (status < Success || list == NULL || *list == NULL)
	*iconname = strdup((char *)text_prop.value);
    else
	*iconname = strdup(*list);

    if (list)
	XFreeStringList(list);
    XFree(text_prop.value);
    return 1;
}

/**
 * separate routine to set focus to make things more understandable
 * and easier to debug
 */
void
SetFocus (TwmWindow *tmp_win, Time time)
{
    Window w = (tmp_win ? tmp_win->w : PointerRoot);

#ifdef DEBUG_EVENTS
    if (tmp_win) {
	fprintf (stderr, "util.c -- SetFocus(): Focusing on window \"%s\"\n", tmp_win->full_name);
    } else {
	fprintf (stderr, "util.c -- SetFocus(): Unfocusing; Focus was \"%s\"\n",
		Focus != NULL ? Focus->full_name : "(nil)");
    }
#endif

    XSetInputFocus (dpy, w, RevertToPointerRoot, time);
    XSync (dpy, False);
}


#ifdef NOPUTENV
/**
 * define our own putenv() if the system doesn't have one.
 * putenv(s): place s (a string of the form "NAME=value") in
 * the environment; replacing any existing NAME.  s is placed in
 * environment, so if you change s, the environment changes (like
 * putenv on a sun).  Binding removed if you putenv something else
 * called NAME.
 */
int
putenv(char *s)
{
    char *v;
    int varlen, idx;
    extern char **environ;
    char **newenv;
    static int virgin = 1; /* true while "environ" is a virgin */

    v = index(s, '=');
    if(v == 0)
	return 0; /* punt if it's not of the right form */
    varlen = (v + 1) - s;

    for (idx = 0; environ[idx] != 0; idx++) {
	if (strncmp(environ[idx], s, varlen) == 0) {
	    if(v[1] != 0) { /* true if there's a value */
		environ[idx] = s;
		return 0;
	    } else {
		do {
		    environ[idx] = environ[idx+1];
		} while(environ[++idx] != 0);
		return 0;
	    }
	}
    }
    
    /* add to environment (unless no value; then just return) */
    if(v[1] == 0)
	return 0;
    if(virgin) {
	register i;

	newenv = (char **) malloc((unsigned) ((idx + 2) * sizeof(char*)));
	if(newenv == 0)
	    return -1;
	for(i = idx-1; i >= 0; --i)
	    newenv[i] = environ[i];
	virgin = 0;     /* you're not a virgin anymore, sweety */
    } else {
	newenv = (char **) realloc((char *) environ,
				   (unsigned) ((idx + 2) * sizeof(char*)));
	if (newenv == 0)
	    return -1;
    }

    environ = newenv;
    environ[idx] = s;
    environ[idx+1] = 0;
    
    return 0;
}
#endif /* NOPUTENV */


static Pixmap 
CreateXLogoPixmap (unsigned *widthp, unsigned *heightp)
{
    int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (h < 0) h = 0;

    *widthp = *heightp = (unsigned int) h;
    if (Scr->tbpm.xlogo == None) {
	GC gc, gcBack;

	Scr->tbpm.xlogo = XCreatePixmap (dpy, Scr->Root, h, h, 1);
	gc = XCreateGC (dpy, Scr->tbpm.xlogo, 0L, NULL);
	XSetForeground (dpy, gc, 0);
	XFillRectangle (dpy, Scr->tbpm.xlogo, gc, 0, 0, h, h);
	XSetForeground (dpy, gc, 1);
	gcBack = XCreateGC (dpy, Scr->tbpm.xlogo, 0L, NULL);
	XSetForeground (dpy, gcBack, 0);

	/*
	 * draw the logo large so that it gets as dense as possible; then white
	 * out the edges so that they look crisp
	 */
	XmuDrawLogo (dpy, Scr->tbpm.xlogo, gc, gcBack, -1, -1, h + 2, h + 2);
	XDrawRectangle (dpy, Scr->tbpm.xlogo, gcBack, 0, 0, h - 1, h - 1);

	/*
	 * done drawing
	 */
	XFreeGC (dpy, gc);
	XFreeGC (dpy, gcBack);
    }
    return Scr->tbpm.xlogo;
}


static Pixmap 
CreateResizePixmap (unsigned *widthp, unsigned *heightp)
{
    int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;
    if (h < 1) h = 1;

    *widthp = *heightp = (unsigned int) h;
    if (Scr->tbpm.resize == None) {
	XPoint	points[3];
	GC gc;
	int w;
	int lw;

	/*
	 * create the pixmap
	 */
	Scr->tbpm.resize = XCreatePixmap (dpy, Scr->Root, h, h, 1);
	gc = XCreateGC (dpy, Scr->tbpm.resize, 0L, NULL);
	XSetForeground (dpy, gc, 0);
	XFillRectangle (dpy, Scr->tbpm.resize, gc, 0, 0, h, h);
	XSetForeground (dpy, gc, 1);
	lw = h / 16;
	if (lw == 1)
	    lw = 0;
	XSetLineAttributes (dpy, gc, lw, LineSolid, CapButt, JoinMiter);

	/*
	 * draw the resize button, 
	 */
	w = (h * 2) / 3;
	points[0].x = w;
	points[0].y = 0;
	points[1].x = w;
	points[1].y = w;
	points[2].x = 0;
	points[2].y = w;
	XDrawLines (dpy, Scr->tbpm.resize, gc, points, 3, CoordModeOrigin);
	w = w / 2;
	points[0].x = w;
	points[0].y = 0;
	points[1].x = w;
	points[1].y = w;
	points[2].x = 0;
	points[2].y = w;
	XDrawLines (dpy, Scr->tbpm.resize, gc, points, 3, CoordModeOrigin);

	/*
	 * done drawing
	 */
	XFreeGC(dpy, gc);
    }
    return Scr->tbpm.resize;
}


static Pixmap 
CreateDotPixmap (unsigned *widthp, unsigned *heightp)
{
    int h = Scr->TBInfo.width - Scr->TBInfo.border * 2;

    h = h * 3 / 4;
    if (h < 1) h = 1;
    if (!(h & 1))
	h--;
    *widthp = *heightp = (unsigned int) h;
    if (Scr->tbpm.delete == None) {
	GC  gc;
	Pixmap pix;

	pix = Scr->tbpm.delete = XCreatePixmap (dpy, Scr->Root, h, h, 1);
	gc = XCreateGC (dpy, pix, 0L, NULL);
	XSetLineAttributes (dpy, gc, h, LineSolid, CapRound, JoinRound);
	XSetForeground (dpy, gc, 0L);
	XFillRectangle (dpy, pix, gc, 0, 0, h, h);
	XSetForeground (dpy, gc, 1L);
	XDrawLine (dpy, pix, gc, h/2, h/2, h/2, h/2);
	XFreeGC (dpy, gc);
    }
    return Scr->tbpm.delete;
}

#define questionmark_width 8
#define questionmark_height 8
static char questionmark_bits[] = {
   0x38, 0x7c, 0x64, 0x30, 0x18, 0x00, 0x18, 0x18};

static Pixmap 
CreateQuestionPixmap (unsigned *widthp, unsigned *heightp)
{
    *widthp = questionmark_width;
    *heightp = questionmark_height;
    if (Scr->tbpm.question == None) {
	Scr->tbpm.question = XCreateBitmapFromData (dpy, Scr->Root,
						    questionmark_bits,
						    questionmark_width,
						    questionmark_height);
    }
    /*
     * this must succeed or else we are in deep trouble elsewhere
     */
    return Scr->tbpm.question;
}


static Pixmap 
CreateMenuPixmap (unsigned *widthp, unsigned *heightp)
{
    if (Scr->tbpm.menu == None)
	Scr->tbpm.menu = CreateMenuIcon (Scr->TBInfo.width - Scr->TBInfo.border * 2,
					widthp, heightp);
    return Scr->tbpm.menu;
}

Pixmap 
CreateMenuIcon (int height, unsigned *widthp, unsigned *heightp)
{
    /*
     *  Client window title bar icons and pull-right submenu icons
     *  (may have differing sizes).
     */
    int h, w;
    int ih, iw;
    int	ix, iy;
    int	mh, mw;
    int	tw, th;
    int	lw, lh;
    int	lx, ly;
    int	lines, dly;
    int off;
    int	bw;
    Pixmap pix;
    GC  gc;

    h = height;
    w = h * 7 / 8;
    if (h < 1)
	h = 1;
    if (w < 1)
	w = 1;
    *widthp = w;
    *heightp = h;

    pix = XCreatePixmap (dpy, Scr->Root, w, h, 1);
    gc = XCreateGC (dpy, pix, 0L, NULL);
    XSetForeground (dpy, gc, 0L);
    XFillRectangle (dpy, pix, gc, 0, 0, w, h);
    XSetForeground (dpy, gc, 1L);
    ix = 1;
    iy = 1;
    ih = h - iy * 2;
    iw = w - ix * 2;
    off = ih / 8;
    mh = ih - off;
    mw = iw - off;
    bw = mh / 16;
    if (bw == 0 && mw > 2)
	bw = 1;
    tw = mw - bw * 2;
    th = mh - bw * 2;
    XFillRectangle (dpy, pix, gc, ix, iy, mw, mh);
    XFillRectangle (dpy, pix, gc, ix + iw - mw, iy + ih - mh, mw, mh);
    XSetForeground (dpy, gc, 0L);
    XFillRectangle (dpy, pix, gc, ix+bw, iy+bw, tw, th);
    XSetForeground (dpy, gc, 1L);
    lw = tw / 2;
    if ((tw & 1) ^ (lw & 1))
	lw++;
    lx = ix + bw + (tw - lw) / 2;

    lh = th / 2 - bw;
    if ((lh & 1) ^ ((th - bw) & 1))
	lh++;
    ly = iy + bw + (th - bw - lh) / 2;

    lines = 3;
    if ((lh & 1) && lh < 6)
    {
	lines--;
    }
    dly = lh / (lines - 1);
    while (lines--)
    {
	XFillRectangle (dpy, pix, gc, lx, ly, lw, bw);
	ly += dly;
    }
    XFreeGC (dpy, gc);

    return pix;
}

Pixmap
CreateIconMgrIcon (int depth, int height, unsigned *widthp, unsigned *heightp)
{
    char *bits;

    if (height <= siconify_11x11_height) { /* original icon "siconify.bm" */
	*widthp = siconify_11x11_width;
	*heightp = siconify_11x11_height;
	bits = siconify_11x11_bits;
    } else if (height <= siconify_13x13_height) {
	*widthp = siconify_13x13_width;
	*heightp = siconify_13x13_height;
	bits = siconify_13x13_bits;
    } else if (height <= siconify_15x15_height) {
	*widthp = siconify_15x15_width;
	*heightp = siconify_15x15_height;
	bits = siconify_15x15_bits;
    } else if (height <= siconify_17x17_height) {
	*widthp = siconify_17x17_width;
	*heightp = siconify_17x17_height;
	bits = siconify_17x17_bits;
    } else if (height <= siconify_19x19_height) {
	*widthp = siconify_19x19_width;
	*heightp = siconify_19x19_height;
	bits = siconify_19x19_bits;
    } else {
	*widthp = siconify_21x21_width;
	*heightp = siconify_21x21_height;
	bits = siconify_21x21_bits;
    }
    return XCreatePixmapFromBitmapData (dpy, Scr->Root, bits, *widthp, *heightp,
		WhitePixel(dpy, Scr->screen), BlackPixel(dpy, Scr->screen), depth);
}

void
Bell(int type, int percent, Window win)
{
#ifdef XKB
    XkbStdBell(dpy, win, percent, type);
#else
    XBell(dpy, percent);
#endif
    return;
}

#ifdef TWM_USE_XFT
XftDraw *
MyXftDrawCreate (Window win)
{
    Colormap cmap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
    XftDraw *draw = XftDrawCreate (dpy, win, Scr->d_visual, cmap);
    if (!draw)
	fprintf (stderr, "%s: XftDrawCreate() failed.\n", ProgramName);
#ifdef XFT_DEBUG
    if (PrintErrorMessages)
	XftMemReport ();
#endif
    return draw;
}

void
MyXftDrawDestroy (XftDraw *draw)
{
    if (draw)
	XftDrawDestroy (draw);
#ifdef XFT_DEBUG
    if (PrintErrorMessages)
	XftMemReport ();
#endif
}

void
CopyPixelToXftColor (unsigned long pixel, XftColor *col)
{
    col->pixel = pixel;
    CopyPixelToXRenderColor (pixel, &col->color);
}
#endif

#if defined TWM_USE_XFT || defined TWM_USE_RENDER
void
CopyPixelToXRenderColor (unsigned long pixel, XRenderColor *col)
{
    /* color already allocated, extract RGB values: */
    XColor tmp;
    tmp.pixel = pixel;
    XQueryColor (dpy, Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c, &tmp);
    col->red = tmp.red;
    col->green = tmp.green;
    col->blue = tmp.blue;
    col->alpha = 0xffff;
}
#endif

#ifdef TWM_USE_RENDER
Picture
Create_Color_Pen (XRenderColor *col)
{
    XRenderPictureAttributes patt;
    XRenderPictFormat * format = XRenderFindVisualFormat (dpy, Scr->d_visual);
    Pixmap pix = XCreatePixmap (dpy, Scr->Root, 1, 1, Scr->d_depth);
    patt.repeat = RepeatNormal;
    Picture pen = XRenderCreatePicture (dpy, pix, format, CPRepeat, &patt);
    XRenderFillRectangle (dpy, PictOpSrc, pen, col, 0, 0, 1, 1);
    XFreePixmap (dpy, pix);
    return pen;
}
#endif

int
ParsePanelIndex (char *name)
{
    int k;
    char *s;

    if (name == NULL || name[0] == '\0'
	    || strcmp(name, ".") == 0 || XmuCompareISOLatin1(name, "current") == 0)
	/*
	 * return 'current panel'
	 * (though, if zooming, 'f.function-code <= F_MAXIMIZE'
	 * overrides this and enforces 'full X11-screen')
	 */
	return 0;
    else if (strcmp(name, "0") == 0 || XmuCompareISOLatin1(name, "all") == 0)
	/* return 'full X11-screen' */
	return -1;

    /* check if panel index is encoded as numeric string "1", "2", ... etc */
    s = name;
    while (*s && isascii(*s) && isdigit(*s))
	s++;
    if (*s == '\0')
	return atoi (name);

#ifdef TILED_SCREEN
    if (Scr->use_panels == TRUE)
    {
	/*
	 * Check if panel index is alphanumeric name "Xinerama0", "Xinerama1", etc
	 * or Xrandr connector name "LVDS", "VGA", "TMDS-1", "TV", etc
	 */
	if (Scr->panel_names != NULL) {
	    for (k = 0; k < Scr->npanels; ++k)
		if (Scr->panel_names[k] != NULL
			&& strcmp(Scr->panel_names[k], name) == 0)
		    break;
	    if (k < Scr->npanels)
		return k+1; /* count from 1 */
	}

	k = FindNearestPanelToMouse();

	if (XmuCompareISOLatin1(name, "pointer") == 0)
	    return k+1; /* count from 1 */

	if (XmuCompareISOLatin1(name, "next") == 0) {
	    if (Scr->npanels > 1)
		return ((k+1)%Scr->npanels) + 1;
	    else
		return k+1;
	}

	if (XmuCompareISOLatin1(name, "prev") == 0) {
	    if (Scr->npanels > 1)
		return ((k-1+Scr->npanels)%Scr->npanels) + 1;
	    else
		return k+1;
	}
    }
#endif

    return 0; /* fallback: 'current panel' */
}

int
ParsePanelZoomType (char *name)
{
    if (name != NULL && name[0] != '\0')
    {
	int mask;

	if (XmuCompareISOLatin1(name, "maximize") == 0)
	    return F_PANELMAXIMIZE;
	if (XmuCompareISOLatin1(name, "full") == 0)
	    return F_PANELFULLZOOM;
	if (XmuCompareISOLatin1(name, "horizontal") == 0)
	    return F_PANELHORIZOOM;
	if (XmuCompareISOLatin1(name, "vertical") == 0)
	    return F_PANELZOOM;
	if (XmuCompareISOLatin1(name, "left") == 0)
	    return F_PANELLEFTZOOM;
	if (XmuCompareISOLatin1(name, "right") == 0)
	    return F_PANELRIGHTZOOM;
	if (XmuCompareISOLatin1(name, "top") == 0)
	    return F_PANELTOPZOOM;
	if (XmuCompareISOLatin1(name, "bottom") == 0)
	    return F_PANELBOTTOMZOOM;

	mask = XParseGeometry (name, &JunkX, &JunkY, &JunkWidth, &JunkHeight);
	if (((mask & (XValue|WidthValue)) == (XValue|WidthValue))
		|| ((mask & (YValue|HeightValue)) == (YValue|HeightValue)))
	    return F_PANELGEOMETRYZOOM;
	/* special case geometry "+0+0" or "-0-0" (or "+0-0" or "-0+0"), no "WxH" part */
	if (((mask & XValue) != 0) && (JunkX == 0) && ((mask & WidthValue) == 0)
		&& ((mask & YValue) != 0) && (JunkY == 0) && ((mask & HeightValue) == 0))
	    return F_PANELGEOMETRYZOOM;
    }

    return F_PANELZOOM; /* fallback */
}

int
ParsePanelMoveType (char *name)
{
    if (name != NULL && name[0] != '\0')
    {
	int mask;

	if (XmuCompareISOLatin1(name, "left") == 0)
	    return F_PANELLEFTMOVE;
	if (XmuCompareISOLatin1(name, "right") == 0)
	    return F_PANELRIGHTMOVE;
	if (XmuCompareISOLatin1(name, "top") == 0)
	    return F_PANELTOPMOVE;
	if (XmuCompareISOLatin1(name, "bottom") == 0)
	    return F_PANELBOTTOMMOVE;

	mask = XParseGeometry (name, &JunkX, &JunkY, &JunkWidth, &JunkHeight);
	if (((mask & (XValue|WidthValue)) == (XValue|WidthValue))
		|| ((mask & (YValue|HeightValue)) == (YValue|HeightValue)))
	    return F_PANELGEOMETRYMOVE;
	/* special case geometry "+0+0" or "-0-0" (or "+0-0" or "-0+0"), no "WxH" part */
	if (((mask & XValue) != 0) && (JunkX == 0) && ((mask & WidthValue) == 0)
		&& ((mask & YValue) != 0) && (JunkY == 0) && ((mask & HeightValue) == 0))
	    return F_PANELGEOMETRYMOVE;
    }

    return F_PANELTOPMOVE; /* fallback */
}

#ifdef TILED_SCREEN
int
ComputeTiledAreaBoundingBox (struct ScreenInfo *scr)
{
    int i, r = FALSE;

    if (scr->panels != NULL)
	/* bypass panel management if the only panel exactly covers the X11-screen: */
	if (scr->npanels > 1
		|| Lft(scr->panels[0]) != 0 || Bot(scr->panels[0]) != 0
		|| AreaWidth(scr->panels[0]) != scr->MyDisplayWidth
		|| AreaHeight(scr->panels[0]) != scr->MyDisplayHeight)
	{
	    Lft(scr->panels_bb) = Lft(scr->panels[0]);
	    Bot(scr->panels_bb) = Bot(scr->panels[0]);
	    Rht(scr->panels_bb) = Rht(scr->panels[0]);
	    Top(scr->panels_bb) = Top(scr->panels[0]);

	    for (i = 1; i < scr->npanels; ++i) {
		/* (x0,y0), (x1,y1) of tiled area bounding-box: */
		if (Lft(scr->panels_bb) > Lft(scr->panels[i]))
		    Lft(scr->panels_bb) = Lft(scr->panels[i]);
		if (Bot(scr->panels_bb) > Bot(scr->panels[i]))
		    Bot(scr->panels_bb) = Bot(scr->panels[i]);
		if (Rht(scr->panels_bb) < Rht(scr->panels[i]))
		    Rht(scr->panels_bb) = Rht(scr->panels[i]);
		if (Top(scr->panels_bb) < Top(scr->panels[i]))
		    Top(scr->panels_bb) = Top(scr->panels[i]);
	    }
	    r = TRUE;
	}
    return r;
}
#endif

#ifdef TWM_USE_XINERAMA
int
GetXineramaTilesGeometries (struct ScreenInfo *scr)
{
    XineramaScreenInfo *si;
    int r, i;

    if (scr->panel_names != NULL) {
	for (i = 0; i < scr->npanels; ++i)
	    if (scr->panel_names[i] != NULL)
		free (scr->panel_names[i]);
	free (scr->panel_names);
	scr->panel_names = NULL;
    }
    if (scr->panels != NULL) {
	free (scr->panels);
	scr->panels = NULL;
    }
    scr->npanels = 0;

    r = FALSE;
    si = XineramaQueryScreens (dpy, &scr->npanels);
    if (si != (XineramaScreenInfo*)(0)) {
	if (scr->npanels > 0) {
	    scr->panel_names = (char**) calloc (scr->npanels, sizeof(scr->panel_names[0]));
	    scr->panels = (int(*)[4]) calloc (scr->npanels, sizeof(scr->panels[0]));
	    if (scr->panels != (int(*)[4])(0)) {
		for (i = 0; i < scr->npanels; ++i) {
		    Lft(scr->panels[i]) = si[i].x_org; /* x0 */
		    Bot(scr->panels[i]) = si[i].y_org; /* y0 */
		    Rht(scr->panels[i]) = si[i].x_org + si[i].width  - 1; /* x1 */
		    Top(scr->panels[i]) = si[i].y_org + si[i].height - 1; /* y1 */
#if 1
		    if (PrintErrorMessages)
			fprintf (stderr,
			    "%s: Xinerama panel %d (screen_number = %d) at x = %d, y = %d, width = %d, height = %d detected.\n",
			    ProgramName, i+1, si[i].screen_number,
			    si[i].x_org, si[i].y_org, si[i].width, si[i].height);
#endif
		    if (scr->panel_names != NULL) {
			char name[8+6+1];
			sprintf (name, "Xinerama%d", (si[i].screen_number%1000000));
			name[sizeof(name)-1] = '\0';
			scr->panel_names[i] = strdup (name);
		    }
		}
		r = TRUE;
	    }
	}
	XFree (si);
    }
    return r;
}
#endif

#ifdef TWM_USE_XRANDR
int
GetXrandrTilesGeometries (struct ScreenInfo *scr)
{
    int r = FALSE;

    if (HasXrandr == True)
    {
#if RANDR_MAJOR > 1 || (RANDR_MAJOR == 1 && RANDR_MINOR >= 2)
	int i, j, k;
	XRRScreenResources * res;

	if (scr->panel_names != NULL) {
	    for (i = 0; i < scr->npanels; ++i)
		if (scr->panel_names[i] != NULL)
		    free (scr->panel_names[i]);
	    free (scr->panel_names);
	    scr->panel_names = NULL;
	}
	if (scr->panels != NULL) {
	    free (scr->panels);
	    scr->panels = NULL;
	}
	scr->npanels = 0;

	res = XRRGetScreenResources (dpy, scr->Root);
	if (res != (XRRScreenResources*)(0)) {
	    if (res->ncrtc > 0) {
		scr->panel_names = (char**) calloc (res->ncrtc, sizeof(scr->panel_names[0]));
		scr->panels = (int(*)[4]) calloc (res->ncrtc, sizeof(scr->panels[0]));
		if (scr->panels != (int(*)[4])(0)) {
		    for (i = 0; i < res->ncrtc; ++i) {
			XRRCrtcInfo * crt = XRRGetCrtcInfo (dpy, res, res->crtcs[i]);
			if (crt != (XRRCrtcInfo*)(0)) {
			    if (crt->mode != None) {
				k = RR_Connected + RR_Disconnected + RR_UnknownConnection; /*some invalid value*/
				for (j = 0; j < crt->noutput; ++j) {
				    XRROutputInfo * out = XRRGetOutputInfo (dpy, res, crt->outputs[j]);
				    if (out != (XRROutputInfo*)(0)) {
					if (out->crtc == res->crtcs[i]) {
					    if (scr->panel_names != NULL)
						scr->panel_names[scr->npanels] = strdup (out->name);
					    k = out->connection;
					    j = crt->noutput; /*exit 'for'*/
					}
					XRRFreeOutputInfo (out);
				    }
				}
				Lft(scr->panels[scr->npanels]) = crt->x; /* x0 */
				Bot(scr->panels[scr->npanels]) = crt->y; /* y0 */
				Rht(scr->panels[scr->npanels]) = crt->x + crt->width  - 1; /* x1 */
				Top(scr->panels[scr->npanels]) = crt->y + crt->height - 1; /* y1 */
#if 1
				if (PrintErrorMessages)
				    fprintf (stderr,
					"%s: %s panel %d (connector '%s' state '%s') on X%d-screen %d at x = %d, y = %d, width = %d, height = %d detected.\n",
					ProgramName, RANDR_NAME, scr->npanels+1, (scr->panel_names&&scr->panel_names[scr->npanels]?scr->panel_names[scr->npanels]:"unknown"),
					(k==RR_Connected?"RR_Connected":(k==RR_Disconnected?"RR_Disconnected":(k==RR_UnknownConnection?"RR_UnknownConnection":("RR_???")))),
					XProtocolVersion(dpy), scr->screen, crt->x, crt->y, crt->width, crt->height);
#endif
				scr->npanels++;
			    }
			    XRRFreeCrtcInfo (crt);
			}
		    }
		    r = TRUE;
		}
	    }
	    XRRFreeScreenResources (res);
	}
#endif
    }
    return r;
}
#endif


#ifdef TWM_USE_OPACITY
void
SetWindowOpacity (Window win, unsigned int opacity)
{
    /* rescale opacity from  0...255  to  0x00000000...0xffffffff */
    opacity *= 0x01010101;
    if (opacity == 0xffffffff)
	XDeleteProperty (dpy, win, _XA_NET_WM_WINDOW_OPACITY);
    else
	XChangeProperty (dpy, win, _XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char*)(&opacity), 1);
}

void
PropagateWindowOpacity (TwmWindow *tmp)
{
    Atom type;
    int  fmt;
    unsigned char *data;
    unsigned long  n, left;

    /* propagate 'opacity' property from 'client' to 'frame' window: */
    if (XGetWindowProperty (dpy, tmp->w, _XA_NET_WM_WINDOW_OPACITY, 0, 1, False,
				XA_CARDINAL, &type, &fmt, &n, &left, &data) == Success
			&& data != NULL) {
	XChangeProperty (dpy, tmp->frame, _XA_NET_WM_WINDOW_OPACITY, XA_CARDINAL, 32,
			PropModeReplace, data, 1);
	XFree ((void*)data);
    }
}
#endif
