/*
 * 
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
 * */
/* $XFree86: xc/programs/twm/icons.c,v 1.6 2001/12/14 20:01:08 dawes Exp $ */

/**********************************************************************
 *
 * $Xorg: icons.c,v 1.4 2001/02/09 02:05:36 xorgcvs Exp $
 *
 * Icon releated routines
 *
 * 10-Apr-89 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#include <stdio.h>
#include <string.h>
#include "twm.h"
#include "screen.h"
#include "icons.h"
#include "gram.h"
#include "parse.h"
#include "util.h"

#define iconWidth(w)	(Scr->IconBorderWidth * 2 + w->icon_w_width)
#define iconHeight(w)	(Scr->IconBorderWidth * 2 + w->icon_w_height)

static void splitEntry ( IconEntry *ie, int grav1, int grav2, int w, int h );
static IconEntry * FindIconEntry ( TwmWindow *tmp_win, IconRegion **irp );
static IconEntry * prevIconEntry ( IconEntry *ie, IconRegion *ir );
static void mergeEntries ( IconEntry *old, IconEntry *ie );

static void
splitEntry (IconEntry *ie, int grav1, int grav2, int w, int h)
{
    IconEntry	*new;

    switch (grav1) {
    case D_NORTH:
    case D_SOUTH:
	if (w != ie->w)
	    splitEntry (ie, grav2, grav1, w, ie->h);
	if (h != ie->h) {
	    new = (IconEntry *)malloc (sizeof (IconEntry));
	    new->twm_win = 0;
	    new->used = 0;
	    new->next = ie->next;
	    ie->next = new;
	    new->x = ie->x;
	    new->h = (ie->h - h);
	    new->w = ie->w;
	    ie->h = h;
	    if (grav1 == D_SOUTH) {
		new->y = ie->y;
		ie->y = new->y + new->h;
	    } else
		new->y = ie->y + ie->h;
	}
	break;
    case D_EAST:
    case D_WEST:
	if (h != ie->h)
	    splitEntry (ie, grav2, grav1, ie->w, h);
	if (w != ie->w) {
	    new = (IconEntry *)malloc (sizeof (IconEntry));
	    new->twm_win = 0;
	    new->used = 0;
	    new->next = ie->next;
	    ie->next = new;
	    new->y = ie->y;
	    new->w = (ie->w - w);
	    new->h = ie->h;
	    ie->w = w;
	    if (grav1 == D_EAST) {
		new->x = ie->x;
		ie->x = new->x + new->w;
	    } else
		new->x = ie->x + ie->w;
	}
	break;
    }
}

int
roundUp (int v, int multiple)
{
    return ((v + multiple - 1) / multiple) * multiple;
}

void
PlaceIcon(TwmWindow *tmp_win, int def_x, int def_y, int *final_x, int *final_y)
{
    IconRegion	*ir;
    IconEntry	*ie;
    int		w = 0, h = 0;

    ie = 0;
    for (ir = Scr->FirstRegion; ir; ir = ir->next) {
	w = roundUp (iconWidth (tmp_win), ir->stepx);
	h = roundUp (iconHeight (tmp_win), ir->stepy);
	for (ie = ir->entries; ie; ie=ie->next) {
	    if (ie->used)
		continue;
	    if (ie->w >= w && ie->h >= h)
		break;
	}
	if (ie)
	    break;
    }
    if (ie) {
	splitEntry (ie, ir->grav1, ir->grav2, w, h);
	ie->used = 1;
	ie->twm_win = tmp_win;
	*final_x = ie->x + (ie->w - iconWidth (tmp_win)) / 2;
	*final_y = ie->y + (ie->h - iconHeight (tmp_win)) / 2;
    } else {
	*final_x = def_x;
	*final_y = def_y;
    }
    return;
}

static IconEntry *
FindIconEntry (TwmWindow *tmp_win, IconRegion **irp)
{
    IconRegion	*ir;
    IconEntry	*ie;

    for (ir = Scr->FirstRegion; ir; ir = ir->next) {
	for (ie = ir->entries; ie; ie=ie->next)
	    if (ie->twm_win == tmp_win) {
		if (irp)
		    *irp = ir;
		return ie;
	    }
    }
    return 0;
}

void
IconUp (TwmWindow *tmp_win)
{
    int		x, y;
    int		defx, defy;
    struct IconRegion *ir;

    /*
     * If the client specified a particular location, let's use it (this might
     * want to be an option at some point).  Otherwise, try to fit within the
     * icon region.
     */
    if (tmp_win->wmhints && (tmp_win->wmhints->flags & IconPositionHint))
      return;

    if (tmp_win->icon_moved) {
	if (!XGetGeometry (dpy, tmp_win->icon_w.win, &JunkRoot, &defx, &defy,
			   &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
	  return;

	x = defx + ((int) JunkWidth) / 2;
	y = defy + ((int) JunkHeight) / 2;

	for (ir = Scr->FirstRegion; ir; ir = ir->next) {
	    if (x >= ir->x && x < (ir->x + ir->w) &&
		y >= ir->y && y < (ir->y + ir->h))
	      break;
	}
	if (!ir) return;		/* outside icon regions, leave alone */
    }

    defx = -100;
    defy = -100;
    PlaceIcon(tmp_win, defx, defy, &x, &y);
    if (x != defx || y != defy) {
	XMoveWindow (dpy, tmp_win->icon_w.win, x, y);
	tmp_win->icon_moved = FALSE;	/* since we've restored it */
    }
}

static IconEntry *
prevIconEntry (IconEntry *ie, IconRegion *ir)
{
    IconEntry	*ip;

    if (ie == ir->entries)
	return 0;
    for (ip = ir->entries; ip->next != ie; ip=ip->next)
	;
    return ip;
}

/**
 * old is being freed; and is adjacent to ie.  Merge
 * regions together
 */
static void
mergeEntries (IconEntry *old, IconEntry *ie)
{
    if (old->y == ie->y) {
	ie->w = old->w + ie->w;
	if (old->x < ie->x)
	    ie->x = old->x;
    } else {
	ie->h = old->h + ie->h;
	if (old->y < ie->y)
	    ie->y = old->y;
    }
}

void
IconDown (TwmWindow *tmp_win)
{
    IconEntry	*ie, *ip, *in;
    IconRegion	*ir;

    ie = FindIconEntry (tmp_win, &ir);
    if (ie) {
	ie->twm_win = 0;
	ie->used = 0;
	ip = prevIconEntry (ie, ir);
	in = ie->next;
	for (;;) {
	    if (ip && ip->used == 0 &&
	       ((ip->x == ie->x && ip->w == ie->w) ||
	        (ip->y == ie->y && ip->h == ie->h)))
	    {
	    	ip->next = ie->next;
	    	mergeEntries (ie, ip);
	    	free ((char *) ie);
		ie = ip;
	    	ip = prevIconEntry (ip, ir);
	    } else if (in && in->used == 0 &&
	       ((in->x == ie->x && in->w == ie->w) ||
	        (in->y == ie->y && in->h == ie->h)))
	    {
	    	ie->next = in->next;
	    	mergeEntries (in, ie);
	    	free ((char *) in);
	    	in = ie->next;
	    } else
		break;
	}
    }
}

void
AddIconRegion(char *geom, int grav1, int grav2, int stepx, int stepy)
{
    IconRegion *ir;
    int mask;

    /*
     * panel name is encoded into geometry string as "1200x20+10-10@1"
     * where "@1" is panel "1"
     */
    int panel;
    char *panel_name = strchr (geom, '@');
    if (panel_name != NULL)
        *panel_name++ = '\0';
    panel = ParsePanelIndex (panel_name);

    ir = (IconRegion *)malloc(sizeof(IconRegion));
    ir->next = NULL;
    if (Scr->LastRegion)
	Scr->LastRegion->next = ir;
    Scr->LastRegion = ir;
    if (!Scr->FirstRegion)
	Scr->FirstRegion = ir;

    ir->entries = NULL;
    ir->grav1 = grav1;
    ir->grav2 = grav2;
    if (stepx <= 0)
	stepx = 1;
    if (stepy <= 0)
	stepy = 1;
    ir->stepx = stepx;
    ir->stepy = stepy;
    ir->x = ir->y = 0;
    ir->w = ir->h = 100;

    mask = XParseGeometry(geom, &ir->x, &ir->y, (unsigned int *)&ir->w, (unsigned int *)&ir->h);

    if (ir->w <= 0)
	ir->w = 100;
    if (ir->h <= 0)
	ir->h = 100;

#ifdef TILED_SCREEN
    if (Scr->use_panels == TRUE)
	EnsureGeometryVisibility (panel, mask, &ir->x, &ir->y, ir->w, ir->h);
    else
#endif
    {
	if (mask & XNegative)
	    ir->x += Scr->MyDisplayWidth  - ir->w;
	if (mask & YNegative)
	    ir->y += Scr->MyDisplayHeight - ir->h;
    }

    ir->entries = (IconEntry *)malloc(sizeof(IconEntry));
    ir->entries->next = 0;
    ir->entries->x = ir->x;
    ir->entries->y = ir->y;
    ir->entries->w = ir->w;
    ir->entries->h = ir->h;
    ir->entries->twm_win = 0;
    ir->entries->used = 0;
}

#ifdef comment
void
FreeIconEntries (IconRegion *ir)
{
    IconEntry	*ie, *tmp;

    for (ie = ir->entries; ie; ie=tmp)
    {
	tmp = ie->next;
	free ((char *) ie);
    }
}

void
FreeIconRegions()
{
    IconRegion *ir, *tmp;

    for (ir = Scr->FirstRegion; ir != NULL;)
    {
	tmp = ir;
	FreeIconEntries (ir);
	ir = ir->next;
	free((char *) tmp);
    }
    Scr->FirstRegion = NULL;
    Scr->LastRegion = NULL;
}
#endif

void SetIconWindowHintForFrame (TwmWindow *tmp_win, Window icon_window)
{
    XWMHints hints, *hintsp;
    hintsp = XGetWMHints (dpy, tmp_win->frame);
    if (hintsp == NULL) {
	hintsp = &hints;
	hintsp->flags = 0;
    }
    hintsp->icon_window = icon_window;
    hintsp->flags |= IconWindowHint;
    XSetWMHints (dpy, tmp_win->frame, hintsp);
    if (hintsp != &hints)
	XFree (hintsp);
}

Visual *
FindVisual (int screen, int *class, int *depth)
{
    XVisualInfo *xvi, tmp;
    Visual *vis = NULL;
    int n = 0;

    tmp.screen = screen;
    tmp.class = (*class);
    tmp.depth = (*depth);
    xvi = XGetVisualInfo (dpy, (VisualScreenMask
				| ((*depth) >  0 ? VisualDepthMask : 0)
				| ((*class) > -1 ? VisualClassMask : 0)),
			    &tmp, &n);
    if (xvi)
    {
#if defined TWM_USE_RENDER
	if ((*depth) == 32)
	{
	    int i;
	    for (i = 0; i < n; ++i)
	    {
		XRenderPictFormat * format = XRenderFindVisualFormat (dpy, xvi[i].visual);
		if (format->type == PictTypeDirect && format->direct.alphaMask)
		{
		    vis = xvi[i].visual;
		    (*class) = xvi[i].class;
		    (*depth) = xvi[i].depth;
		    break;
		}
	    }
	}
	if (vis == NULL) /* fallthrough */
#endif
	if (0 < n) {
	    vis = xvi[0].visual;
	    (*class) = xvi[0].class;
	    (*depth) = xvi[0].depth;
	}
	XFree (xvi);
    }
    return vis;
}

Bool
ScreenDepthSupported (int depth)
{
    Bool ret = False;
    int ndepths = 0, *depths = NULL;

    depths = XListDepths (dpy, Scr->screen, &ndepths);
    if (depths) {
	int i;
	for (i = 0; i < ndepths; ++i)
	    if (depths[i] == depth) {
		ret = True;
		break;
	    }
	XFree (depths);
    }
    return ret;
}

Pixmap
CreateIconWMhintsPixmap (TwmWindow *tmp_win, int *depth)
{
    int pm_depth;
    Pixmap pm = None;

    if (tmp_win->wmhints && (tmp_win->wmhints->flags & IconPixmapHint))
    {
	Bool dptOk;
	XGetGeometry (dpy, tmp_win->wmhints->icon_pixmap, &JunkRoot, &JunkX, &JunkY,
			(unsigned int *)&tmp_win->icon_width, (unsigned int *)&tmp_win->icon_height,
			&JunkBW, (unsigned int *)&pm_depth);
#if 0
	/* create 'icon_pixmap' copy as deep as 'twm'-depth: */
	pm = XCreatePixmap (dpy, Scr->Root, tmp_win->icon_width, tmp_win->icon_height, Scr->d_depth);

	if (pm_depth == 1) {
	    FB(Scr, tmp_win->IconBitmapColor, tmp_win->IconC.back);
	    XCopyPlane (dpy, tmp_win->wmhints->icon_pixmap, pm, Scr->NormalGC,
			0, 0, tmp_win->icon_width, tmp_win->icon_height, 0, 0, 1);
	} else if (pm_depth == Scr->d_depth)
	    XCopyArea (dpy, tmp_win->wmhints->icon_pixmap, pm, Scr->NormalGC,
			0,0, tmp_win->icon_width, tmp_win->icon_height, 0, 0);
	else
	{
	    /*
	     * This is a trick: copy 'icon_pixmap' plane-by-plane into backup copy,
	     * regardless of how it appears later in respect to the 'twm'-colormap.
	     */
	    int i;
	    XSetForeground (dpy, Scr->NormalGC, 0);
	    XFillRectangle (dpy, pm, Scr->NormalGC, 0, 0, tmp_win->icon_width, tmp_win->icon_height);
	    XSetBackground (dpy, Scr->NormalGC, 0);
	    XSetForeground (dpy, Scr->NormalGC, AllPlanes);
	    for (i = 0; i != pm_depth && i < Scr->d_depth; ++i) {
		XSetPlaneMask (dpy, Scr->NormalGC, (1<<i));
		XCopyPlane (dpy, tmp_win->wmhints->icon_pixmap, pm, Scr->NormalGC,
			    0, 0, tmp_win->icon_width, tmp_win->icon_height, 0, 0, (1<<i));
	    }
	    XSetPlaneMask (dpy, Scr->NormalGC, AllPlanes);
	}
	/* attention: set 'icon_pixmap' depth to 'twm'-depth: */
	(*depth) = Scr->d_depth;
#else
	/* create 'icon_pixmap' copy as deep as it is ("tmp_win->icon_bm_w" has to be as deep): */
	dptOk = ScreenDepthSupported (pm_depth);
	if (pm_depth == 1 || dptOk == False)
	    pm = XCreatePixmap (dpy, Scr->Root, tmp_win->icon_width, tmp_win->icon_height, Scr->d_depth);
	else
	    pm = XCreatePixmap (dpy, Scr->Root, tmp_win->icon_width, tmp_win->icon_height, pm_depth);
	if (pm != None)
	{
	    if (pm_depth == 1) {
		FB(Scr, tmp_win->IconBitmapColor, tmp_win->IconC.back);
		XCopyPlane (dpy, tmp_win->wmhints->icon_pixmap, pm, Scr->NormalGC,
			    0, 0, tmp_win->icon_width, tmp_win->icon_height, 0, 0, 1);
		pm_depth = Scr->d_depth;
	    } else if (dptOk == True) {
		GC gc;
		Gcv.graphics_exposures = False;
		gc = XCreateGC (dpy, pm, GCGraphicsExposures, &Gcv);
		XCopyArea (dpy, tmp_win->wmhints->icon_pixmap, pm, gc,
			    0, 0, tmp_win->icon_width, tmp_win->icon_height, 0, 0);
		XFreeGC (dpy, gc);
	    } else {
		/*
		 * This is a trick: copy 'icon_pixmap' plane-by-plane into backup copy,
		 * regardless of how it appears later in respect to the 'twm'-colormap.
		 */
		int i;
		XSetForeground (dpy, Scr->NormalGC, 0);
		XFillRectangle (dpy, pm, Scr->NormalGC, 0, 0, tmp_win->icon_width, tmp_win->icon_height);
		XSetBackground (dpy, Scr->NormalGC, 0);
		XSetForeground (dpy, Scr->NormalGC, AllPlanes);
		for (i = 0; i != pm_depth && i < Scr->d_depth; ++i) {
		    XSetPlaneMask (dpy, Scr->NormalGC, (1<<i));
		    XCopyPlane (dpy, tmp_win->wmhints->icon_pixmap, pm, Scr->NormalGC,
				0, 0, tmp_win->icon_width, tmp_win->icon_height, 0, 0, (1<<i));
		}
		XSetPlaneMask (dpy, Scr->NormalGC, AllPlanes);
		pm_depth = Scr->d_depth;
	    }
	    (*depth) = pm_depth;
	}
#endif
    }
    return pm;
}

void
ComputeIconSize (TwmWindow *tmp_win, int *bm_x, int *bm_y)
{
    tmp_win->icon_w_width = MyFont_TextWidth(&Scr->IconFont,
	tmp_win->icon_name, strlen(tmp_win->icon_name));

#ifdef TWM_USE_SPACING
    tmp_win->icon_w_width += Scr->IconFont.height; /*approx. '1ex' on both sides*/
#else
    tmp_win->icon_w_width += 6;
#endif

    if (tmp_win->icon_w_width > Scr->IconRegionEntryMaxWidth)
	tmp_win->icon_w_width = Scr->IconRegionEntryMaxWidth;

    if (tmp_win->icon_w_width < tmp_win->icon_width)
    {
	tmp_win->icon_x = (tmp_win->icon_width - tmp_win->icon_w_width)/2;
	tmp_win->icon_x += 3;
	tmp_win->icon_w_width = tmp_win->icon_width;
    }
    else
    {
#ifdef TWM_USE_SPACING
	tmp_win->icon_x = Scr->IconFont.height/2;
#else
	tmp_win->icon_x = 3;
#endif
    }

#ifdef TWM_USE_SPACING
    /* icon label height := 1.44 times font height: */
    tmp_win->icon_w_height = tmp_win->icon_height + 144*Scr->IconFont.height/100;
    tmp_win->icon_y = tmp_win->icon_height + Scr->IconFont.y + 44*Scr->IconFont.height/200;
#else
    tmp_win->icon_y = tmp_win->icon_height + Scr->IconFont.height;
    tmp_win->icon_w_height = tmp_win->icon_height + Scr->IconFont.height + 4;
#endif

    if (tmp_win->icon_w_width == tmp_win->icon_width)
	(*bm_x) = 0;
    else
	(*bm_x) = (tmp_win->icon_w_width - tmp_win->icon_width)/2;
    (*bm_y) = 0;
}

Window
CreateIconBMWindow (TwmWindow *tmp_win, int x, int y, Pixmap pm, int pm_depth)
{
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    Visual *vis;

    attributes.background_pixmap = pm;
    attributes.border_pixel = BlackPixel (dpy, Scr->screen);
    valuemask = (CWBackPixmap | CWBorderPixel);

    if (pm_depth == tmp_win->attr.depth) { /* give icon pixmap the client visual */
	vis = tmp_win->attr.visual;
	attributes.colormap = tmp_win->attr.colormap;
	valuemask |= CWColormap;
    } else if (pm_depth == Scr->d_depth) { /* give icon pixmap the 'twm' visual */
	vis = Scr->d_visual;
	attributes.colormap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
	valuemask |= CWColormap;
    } else {
	int class = -1;
	vis = FindVisual (Scr->screen, &class, &pm_depth);
	if (vis) {
#if 0
	    attributes.colormap = XCreateColormap (dpy, Scr->Root, vis, AllocNone);
	    valuemask |= CWColormap; /* do we leak this colorpam, without any colors? */
#endif
	} else {
	    /* here we have problems: no visual, no colormap; expect X-error below. */
	    attributes.colormap = None;
	    XSync (dpy, False);
	    fprintf (stderr, "%s: Problem creating an icon visual occurred (client '%s').\n",
			ProgramName, tmp_win->full_name);
	    fflush (stderr);
	}
    }

    return XCreateWindow (dpy, tmp_win->icon_w.win, x, y,
				(unsigned int)tmp_win->icon_width,
				(unsigned int)tmp_win->icon_height,
				(unsigned int) 0,
				pm_depth,
				(unsigned int) CopyFromParent,
				vis,
				valuemask, &attributes);
}

void
CreateIconWindow(TwmWindow *tmp_win, int def_x, int def_y)
{
#ifdef TWM_USE_RENDER
    XRenderColor xrcol;
#endif
    unsigned long event_mask;
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    Pixmap pm = None;			/* tmp pixmap variable */
    int pm_depth;			/* depth of tmp_win->wmhints->icon_pixmap */
    int x, y, final_x, final_y;


    tmp_win->forced = FALSE;
    tmp_win->icon_not_ours = FALSE;
    tmp_win->icon_width = 0;
    tmp_win->icon_height = 0;
    FB(Scr, tmp_win->IconBitmapColor, tmp_win->IconC.back);
    pm_depth = Scr->d_depth;

    /* now go through the steps to get an icon window,  if ForceIcon is 
     * set, then no matter what else is defined, the bitmap from the
     * .twmrc file is used
     */
    if (Scr->ForceIcon)
    {
	char *icon_name;
	Pixmap bm;

	icon_name = LookInNameList(Scr->IconNames, tmp_win->full_name);
        if (icon_name == NULL)
	    icon_name = LookInList(Scr->IconNames, tmp_win->full_name,
				   &tmp_win->class);

	bm = None;
	if (icon_name != NULL)
	{
	    if ((bm = (Pixmap)LookInNameList(Scr->Icons, icon_name)) == None)
	    {
		if ((bm = GetBitmap (icon_name)) != None)
		    AddToList(&Scr->Icons, icon_name, (char *)bm);
	    }
	}

	if (bm != None)
	{
	    XGetGeometry(dpy, bm, &JunkRoot, &JunkX, &JunkY,
		(unsigned int *) &tmp_win->icon_width, (unsigned int *)&tmp_win->icon_height,
		&JunkBW, &JunkDepth);

	    pm = XCreatePixmap(dpy, Scr->Root, tmp_win->icon_width,
		tmp_win->icon_height, Scr->d_depth);

	    /* the copy plane works on color ! */
	    XCopyPlane(dpy, bm, pm, Scr->NormalGC,
		0,0, tmp_win->icon_width, tmp_win->icon_height, 0, 0, 1 );

	    tmp_win->forced = TRUE;
	}
    }

    /* if the pixmap is still NULL, we didn't get one from the above code,
     * that could mean that ForceIcon was not set, or that the window
     * was not in the Icons list, now check the WM hints for an icon
     */
    if (pm == None)
	pm = CreateIconWMhintsPixmap (tmp_win, &pm_depth);

    /* if we still haven't got an icon, let's look in the Icon list 
     * if ForceIcon is not set
     */
    if (pm == None && !Scr->ForceIcon)
    {
	char *icon_name;
	Pixmap bm;

	icon_name = LookInNameList(Scr->IconNames, tmp_win->full_name);
        if (icon_name == NULL)
	    icon_name = LookInList(Scr->IconNames, tmp_win->full_name,
				   &tmp_win->class);

	bm = None;
	if (icon_name != NULL)
	{
	    if ((bm = (Pixmap)LookInNameList(Scr->Icons, icon_name)) == None)
	    {
		if ((bm = GetBitmap (icon_name)) != None)
		    AddToList(&Scr->Icons, icon_name, (char *)bm);
	    }
	}

	if (bm != None)
	{
	    XGetGeometry(dpy, bm, &JunkRoot, &JunkX, &JunkY,
		(unsigned int *)&tmp_win->icon_width, (unsigned int *)&tmp_win->icon_height,
		&JunkBW, &JunkDepth);

	    pm = XCreatePixmap(dpy, Scr->Root, tmp_win->icon_width,
		tmp_win->icon_height, Scr->d_depth);

	    /* the copy plane works on color ! */
	    XCopyPlane(dpy, bm, pm, Scr->NormalGC,
		0,0, tmp_win->icon_width, tmp_win->icon_height, 0, 0, 1 );
	}
    }

    /* if we still don't have an icon, assign the UnknownIcon */

    if (pm == None && Scr->UnknownPm != None)
    {
	tmp_win->icon_width = Scr->UnknownWidth;
	tmp_win->icon_height = Scr->UnknownHeight;

	pm = XCreatePixmap(dpy, Scr->Root, tmp_win->icon_width,
	    tmp_win->icon_height, Scr->d_depth);

	/* the copy plane works on color ! */
	XCopyPlane(dpy, Scr->UnknownPm, pm, Scr->NormalGC,
	    0,0, tmp_win->icon_width, tmp_win->icon_height, 0, 0, 1 );
    }

    ComputeIconSize (tmp_win, &x, &y);

    event_mask = 0;
    if (tmp_win->wmhints && tmp_win->wmhints->flags & IconWindowHint)
    {
	tmp_win->icon_w.win = tmp_win->wmhints->icon_window;
	if (tmp_win->forced ||
	    XGetGeometry(dpy, tmp_win->icon_w.win, &JunkRoot, &JunkX, &JunkY,
		     (unsigned int *)&tmp_win->icon_w_width, (unsigned int *)&tmp_win->icon_w_height,
		     &JunkBW, &JunkDepth) == 0)
	{
	    tmp_win->icon_w.win = None;
	    tmp_win->wmhints->flags &= ~IconWindowHint;
	}
	else
	{
	    tmp_win->icon_not_ours = TRUE;
	    event_mask = EnterWindowMask | LeaveWindowMask;
	    SetIconWindowHintForFrame (tmp_win, tmp_win->icon_w.win);
	}
    }
    else
    {
	tmp_win->icon_w.win = None;
    }

    if (tmp_win->icon_w.win == None)
    {
	attributes.colormap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
	attributes.border_pixel = tmp_win->IconBorderColor;
	attributes.background_pixel = tmp_win->IconC.back;
	valuemask = (CWColormap | CWBorderPixel | CWBackPixel);

/*#ifdef TWM_USE_OPACITY*/
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE)
	    valuemask &= ~CWBackPixel;
#endif
/*#endif*/

	tmp_win->icon_w.win = XCreateWindow (dpy, Scr->Root, 0, 0,
					    tmp_win->icon_w_width,
					    tmp_win->icon_w_height,
					    Scr->IconBorderWidth,
					    Scr->d_depth,
					    (unsigned int) CopyFromParent,
					    Scr->d_visual,
					    valuemask, &attributes);
	event_mask = ExposureMask;

#ifdef TWM_USE_RENDER
	CopyPixelToXRenderColor (tmp_win->IconC.back, &xrcol);
#ifdef TWM_USE_OPACITY
	if (Scr->XCompMgrRunning == TRUE) {
	    xrcol.alpha = Scr->IconOpacity * 257;
	    xrcol.red = xrcol.red * xrcol.alpha / 0xffff; /* premultiply */
	    xrcol.green = xrcol.green * xrcol.alpha / 0xffff;
	    xrcol.blue = xrcol.blue * xrcol.alpha / 0xffff;
	}
#endif
	if (tmp_win->PenIconB != None)
	    XRenderFreePicture (dpy, tmp_win->PenIconB);
	tmp_win->PenIconB = Create_Color_Pen (&xrcol);
	if (tmp_win->PicIconB != None) {
	    XRenderFreePicture (dpy, tmp_win->PicIconB);
	    tmp_win->PicIconB = None;
	}
#ifdef TWM_USE_OPACITY
	if (Scr->use_xrender == FALSE)
#endif
#endif
#ifdef TWM_USE_OPACITY
	    SetWindowOpacity (tmp_win->icon_w.win, Scr->IconOpacity);
#endif

#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    tmp_win->icon_w.xft = MyXftDrawCreate (tmp_win->icon_w.win);
#endif
	SetIconWindowHintForFrame (tmp_win, tmp_win->icon_w.win);
    }

    XSelectInput (dpy, tmp_win->icon_w.win,
		  KeyPressMask | ButtonPressMask | ButtonReleaseMask |
		  event_mask);

    tmp_win->icon_bm_w = None;
    if (pm != None &&
	(! (tmp_win->wmhints && tmp_win->wmhints->flags & IconWindowHint)))
    {
	tmp_win->icon_bm_w = CreateIconBMWindow (tmp_win, x, y, pm, pm_depth);
    }

    /* I need to figure out where to put the icon window now, because 
     * getting here means that I am going to make the icon visible
     */
    if (tmp_win->wmhints &&
	tmp_win->wmhints->flags & IconPositionHint)
    {
	final_x = tmp_win->wmhints->icon_x;
	final_y = tmp_win->wmhints->icon_y;
    }
    else
    {
	PlaceIcon(tmp_win, def_x, def_y, &final_x, &final_y);
    }

    EnsureRectangleOnScreen (&final_x, &final_y,
	    tmp_win->icon_w_width  + 2*Scr->IconBorderWidth,
	    tmp_win->icon_w_height + 2*Scr->IconBorderWidth);

    XMoveWindow(dpy, tmp_win->icon_w.win, final_x, final_y);
    tmp_win->iconified = TRUE;

    XMapSubwindows(dpy, tmp_win->icon_w.win);
    XSaveContext(dpy, tmp_win->icon_w.win, TwmContext, (caddr_t)tmp_win);
    XSaveContext(dpy, tmp_win->icon_w.win, ScreenContext, (caddr_t)Scr);
    XDefineCursor(dpy, tmp_win->icon_w.win, Scr->IconCursor);

    SetIconShapeMask (tmp_win);

    if (pm)
	XFreePixmap (dpy, pm);
}

int
FindIconRegionEntryMaxWidth (struct ScreenInfo *scr)
{
    int   m;

    if (scr->FirstRegion) {
	IconRegion * ir = scr->FirstRegion;
	m = ir->stepx;
	for (ir = ir->next; ir; ir = ir->next)
	    if (m < ir->stepx)
		m = ir->stepx;
	m -= scr->IconBorderWidth * 2;
    } else
	m = scr->MyDisplayWidth;

    return m;
}

void
SetIconShapeMask (TwmWindow *tmp_win)
{
#ifdef TWM_USE_RENDER
    if ((HasShape || (Scr->use_xrender == TRUE)) && (tmp_win->icon_not_ours != TRUE))
#else
    if (HasShape && (tmp_win->icon_not_ours != TRUE))
#endif
    {
	Pixmap mask;
	GC gc;
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE) {
	    mask = XCreatePixmap (dpy, Scr->Root, tmp_win->icon_w_width, tmp_win->icon_w_height, Scr->DepthA);
	    gc = Scr->AlphaGC;
	}
	else
#endif
	{
	    mask = XCreatePixmap (dpy, Scr->Root, tmp_win->icon_w_width, tmp_win->icon_w_height, 1);
	    gc = Scr->BitGC;
	}

	if (mask != None)
	{
	    int x, y;
	    Bool m = False;

	    /* cleanup mask: */
	    XSetForeground (dpy, gc, 0);
	    XFillRectangle (dpy, mask, gc, 0, 0, tmp_win->icon_w_width, tmp_win->icon_w_height);

	    /* set shape for icon caption text: */
	    if (Scr->ShapedIconLabels == TRUE)
	    {
#ifdef TWM_USE_SPACING
		x = tmp_win->icon_w_width - Scr->IconFont.height;
#else
		x = tmp_win->icon_w_width - 6;
#endif
		MyFont_DrawShapeStringEllipsis (mask, &Scr->IconFont,
				    tmp_win->icon_x, tmp_win->icon_y,
				    tmp_win->icon_name, strlen(tmp_win->icon_name),
				    x, Scr->IconLabelOffsetX, Scr->IconLabelOffsetY);
		m = True;
	    } else if (Scr->ShapedIconPixmaps == TRUE && tmp_win->icon_bm_w != None) {
		y = tmp_win->icon_w_height - tmp_win->icon_height;
		XSetForeground (dpy, gc, AllPlanes);
		XFillRectangle (dpy, mask, gc, 0, tmp_win->icon_height, tmp_win->icon_w_width, y);
		m = True;
	    }

	    /* set shape for icon-pixmap window: */
	    if (tmp_win->icon_bm_w != None)
	    {
		Pixmap src = None;

		if ((tmp_win->wmhints->flags & IconPixmapHint)
			    && XGetGeometry(dpy, tmp_win->wmhints->icon_pixmap,
				&JunkRoot, &JunkX, &JunkY, &JunkWidth, &JunkHeight,
				&JunkBW, &JunkDepth) && JunkDepth == 1)
		{
		    src = tmp_win->wmhints->icon_pixmap;
		}
		else if ((tmp_win->wmhints->flags & IconMaskHint)
			    && XGetGeometry(dpy, tmp_win->wmhints->icon_mask,
				&JunkRoot, &JunkX, &JunkY, &JunkWidth, &JunkHeight,
				&JunkBW, &JunkDepth) && JunkDepth == 1)
		{
		    src = tmp_win->wmhints->icon_mask;
		}

#ifdef TWM_USE_RENDER
		if (HasShape && (src != None))
#else
		if (src != None)
#endif
		    XShapeCombineMask (dpy, tmp_win->icon_bm_w, ShapeBounding, 0, 0, src, ShapeSet);

		x = (tmp_win->icon_w_width - tmp_win->icon_width) / 2;
		y = 0;

		XSetForeground (dpy, gc, AllPlanes);
		if (Scr->ShapedIconPixmaps == TRUE) {
		    if (src != None) {
			XSetBackground (dpy, gc, 0);
			XCopyPlane (dpy, src, mask, gc, 0, 0, tmp_win->icon_width, tmp_win->icon_height, x, y, 1);
		    } else {
			XFillRectangle (dpy, mask, gc, x, y, tmp_win->icon_width, tmp_win->icon_height);
		    }
		    m = True;
		} else if (Scr->ShapedIconLabels == TRUE) {
		    XFillRectangle (dpy, mask, gc, x, y, tmp_win->icon_width, tmp_win->icon_height);
		    m = True;
		}
	    }

	    if (m == True)
	    {
#ifdef TWM_USE_RENDER
		if (Scr->use_xrender == TRUE)
		{
		    if (tmp_win->PicIconB != None)
			XRenderFreePicture (dpy, tmp_win->PicIconB);
		    tmp_win->PicIconB = XRenderCreatePicture (dpy, mask, XRenderFindStandardFormat (dpy, Scr->FormatA), CPGraphicsExposure, &Scr->PicAttr);
		}
		else
#endif
		{
		    XRectangle txt;
		    txt.x = 0;
		    txt.y = 0;
		    txt.width = tmp_win->icon_w_width;
		    txt.height = tmp_win->icon_w_height;
		    XShapeCombineMask (dpy, tmp_win->icon_w.win, ShapeBounding, 0, 0, None, ShapeSet);
		    XShapeCombineRectangles (dpy, tmp_win->icon_w.win, ShapeBounding, 0, 0, &txt, 1, ShapeSubtract, Unsorted);
		    XShapeCombineMask (dpy, tmp_win->icon_w.win, ShapeBounding, 0, 0, mask, ShapeUnion);
		}
	    }

	    XFreePixmap (dpy, mask);
	}
    }
}

