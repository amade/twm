/*
 * 
Copyright 1989,1998  The Open Group

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
/* $XFree86: xc/programs/twm/iconmgr.c,v 1.5 2001/01/17 23:45:06 dawes Exp $ */

/***********************************************************************
 *
 * $Xorg: iconmgr.c,v 1.4 2001/02/09 02:05:36 xorgcvs Exp $
 *
 * Icon Manager routines
 *
 * 09-Mar-89 Tom LaStrange		File Created
 *
 ***********************************************************************/
/* $XFree86: xc/programs/twm/iconmgr.c,v 1.5 2001/01/17 23:45:06 dawes Exp $ */

#include <stdio.h>
#include <string.h>
#include "twm.h"
#include "util.h"
#include "parse.h"
#include "screen.h"
#include "events.h"
#include "resize.h"
#include "add_window.h"
#include <X11/Xos.h>
#include <X11/Xmu/CharSet.h>
#ifdef macII
int strcmp(); /* missing from string.h in AUX 2.0 */
#endif

WList *DownIconManager = NULL;


static void WarpToIconMgrEntry (WList *tmp)
{
    if ((tmp->iconmgr->twm_win->mapped == TRUE) || (WarpThere(tmp->twm) == 0))
	WarpToIconManager (tmp);
}


/**
 * create all the icon manager windows for this screen.
 */
void CreateIconManagers()
{
    IconMgr *p;
    int mask;
    char str[100];
    char str1[100];
    Pixel background;
    char *icon_name;
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    XClassHint class;
    XSizeHints size;
    XWMHints input;

    if (Scr->siconifyPm == None)
    {
	Scr->siconifyPm = CreateIconMgrIcon (1, Scr->IconManagerFont.height/2,
			&Scr->iconifybox_width, &Scr->iconifybox_height);
	Scr->iconmgr_textx = 2*Scr->iconifybox_width;
    }
    if (Scr->IconMgrIcon == None)
	Scr->IconMgrIcon = CreateIconMgrIcon (1, Scr->IconManagerFont.height, &JunkX, &JunkY);


    for (p = &Scr->iconmgr; p != NULL; p = p->next)
    {
	/*
	 * panel name is encoded into geometry string, e.g. "1200x20+10-10@1"
	 * where "@1" is panel "1"
	 */
	int panel;
	char *panel_name = strchr (p->geometry, '@');
	if (panel_name != NULL)
	    *panel_name++ = '\0';
	panel = ParsePanelIndex (panel_name);

	JunkX = p->x;
	JunkY = p->y;

	mask = XParseGeometry(p->geometry, &JunkX, &JunkY,
			      (unsigned int *) &p->width, (unsigned int *)&p->height);

	if (p->width <= 0)
	    p->width = 150;
	if (p->height <= 0)
	    p->height = 10;

	/*
	 * Having icon manager height correct here is crucial for exact placement.
	 * (The 'height' given in "IconManagerGeometry" is never used in fact.)
	 */
#ifdef TWM_USE_SPACING
	if (p->columns > 1)
	    p->height = 144*Scr->IconManagerFont.height/100; /*multicolumn baselineskip*/
	else
	    p->height = 131*Scr->IconManagerFont.height/100; /*unicolumn baselineskip*/
	if (p->height < (Scr->iconifybox_height + 8))
	    p->height = Scr->iconifybox_height + 8;
#else
	p->height = Scr->IconManagerFont.height + 10;
	if (p->height < (Scr->iconifybox_height + 4))
	    p->height = Scr->iconifybox_height + 4;
#endif
#ifdef TILED_SCREEN
	if (Scr->use_panels == TRUE)
	    EnsureGeometryVisibility (panel, mask, &JunkX, &JunkY, p->width, p->height);
	else
#endif
	{
	    if (mask & XNegative)
		JunkX += Scr->MyDisplayWidth  - p->width;
	    if (mask & YNegative)
		JunkY += Scr->MyDisplayHeight - p->height;
	}

	background = Scr->IconManagerC.back;
	GetColorFromList(Scr->IconManagerBL, p->name, (XClassHint *)NULL,
			 &background);

	attributes.border_pixel = Scr->BorderColor;
	attributes.background_pixel = background;
	attributes.colormap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
	valuemask = (CWColormap | CWBorderPixel | CWBackPixel);

	p->w = XCreateWindow (dpy, Scr->Root,
				JunkX, JunkY, p->width, p->height,
				0 /*(unsigned int)Scr->BorderWidth*/,
				Scr->d_depth,
				(unsigned int) CopyFromParent,
				Scr->d_visual,
				valuemask, &attributes);

	strncpy (str, p->name, sizeof(str)-14);
	str[sizeof(str)-14] = '\0';
	strcat(str, " Icon Manager");
	strncpy (str1, p->name, sizeof(str1)-7);
	str1[sizeof(str1)-7] = '\0';
	strcat(str1, " Icons");
	if (p->icon_name)
	    icon_name = p->icon_name;
	else
	    icon_name = str1;

	class.res_name = p->name;
	class.res_class = "TWM Icon Managers";
	input.input = True;
	input.window_group = Scr->iconmgr.w;
	input.icon_pixmap = input.icon_mask = Scr->IconMgrIcon;
	input.flags = (InputHint | WindowGroupHint | IconPixmapHint | IconMaskHint);
	JunkMask = 0;
	if (mask & XNegative)
	    JunkMask |= 1;
	if (mask & YNegative)
	    JunkMask |= 2;
	switch (JunkMask) {
	case 0x0:
	    size.win_gravity = NorthWestGravity;
	    break;
	case 0x1:
	    size.win_gravity = NorthEastGravity;
	    break;
	case 0x2:
	    size.win_gravity = SouthWestGravity;
	    break;
	case 0x3:
	    size.win_gravity = SouthEastGravity;
	    break;
	}
	size.x = size.y = 0;
	size.width = p->width;
	size.height = p->height;
	size.flags = (PPosition | PSize | PWinGravity);

	XmbSetWMProperties (dpy, p->w, str, icon_name, NULL, 0, &size, &input, &class);

	p->twm_win = AddWindow(p->w, TRUE, p);
	SetMapStateProp (p->twm_win, WithdrawnState);

#ifdef TWM_USE_OPACITY
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == FALSE)
#endif
	    SetWindowOpacity (p->twm_win->frame, Scr->IconMgrOpacity);
#endif
    }
    for (p = &Scr->iconmgr; p != NULL; p = p->next)
    {
	GrabButtons(p->twm_win);
	GrabKeys(p->twm_win);
    }
}

/**
 * allocate a new icon manager
 *
 *  \param name     the name of this icon manager
 *  \param con_name the name of the associated icon
 *  \param geom	    a geometry string to eventually parse
 *	\param columns  the number of columns this icon manager has
 */
IconMgr *AllocateIconManager(char *name, char *icon_name, char *geom, int columns, int ext1, int ext2)
{
    IconMgr *p;

#ifdef DEBUG_ICONMGR
    fprintf(stderr, "AllocateIconManager\n");
    fprintf(stderr, "  name=\"%s\" icon_name=\"%s\", geom=\"%s\", col=%d\n",
	name, icon_name, geom, columns);
#endif

    if (Scr->NoIconManagers)
	return NULL;

    p = (IconMgr *) calloc (1, sizeof(IconMgr));
    p->name = name;
    p->icon_name = icon_name;
    p->geometry = geom;
    p->first = NULL;
    p->last = NULL;
    p->active = NULL;
    p->hilited = NULL;
    p->scr = Scr;
    if (columns == 0)
	columns = 1;
    if (columns < 0) {
	p->reversed = TRUE;
	p->columns = -columns;
    } else {
	p->reversed = FALSE;
	p->columns = +columns;
    }
    if ((ext1 == D_WEST) || (ext2 == D_WEST))
	p->Xnegative = TRUE;
    else
	p->Xnegative = FALSE;
    if ((ext1 == D_NORTH) || (ext2 == D_NORTH))
	p->Ynegative = TRUE;
    else
	p->Ynegative = FALSE;
    p->count = 0;
    p->cur_columns = 0;
    p->cur_rows = 0;
    p->x = 0;
    p->y = 0;
    p->width = 150;
    p->height = 10;

    Scr->iconmgr.lasti->next = p;
    p->prev = Scr->iconmgr.lasti;
    Scr->iconmgr.lasti = p;
    p->next = NULL;

    return(p);
}

/**
 * move the pointer around in an icon manager
 *
 *  \param dir one of the following:
 *    - F_FORWICONMGR:  forward in the window list
 *    - F_BACKICONMGR:  backward in the window list
 *    - F_UPICONMGR:    up one row
 *    - F_DOWNICONMG:   down one row
 *    - F_LEFTICONMGR:  left one column
 *    - F_RIGHTICONMGR: right one column
 */
void MoveIconManager(int dir)
{
    IconMgr *ip;
    WList *tmp = NULL;
    int cur_row, cur_col, new_row, new_col;
    int row_inc, col_inc;
    int got_it;

    /* try to find an iconmanager if there is none 'active': */
    if (Scr->ActiveIconMgr == NULL) {
	for (ip = &Scr->iconmgr; ip != NULL; ip = ip->next)
	    if (ip->first != NULL) {
		SetIconMgrEntryActive (ip->first);
		WarpToIconMgrEntry (ip->first);
		break;
	    }
	return;
    }
    /* try to find a client if there is none 'active': */
    if (Scr->ActiveIconMgr->active == NULL) {
	if (Scr->ActiveIconMgr->first == NULL) {
	    for (ip = &Scr->iconmgr; ip != NULL; ip = ip->next)
		if (ip->first != NULL) {
		    SetIconMgrEntryActive (ip->first);
		    break;
		}
	} else
	    Scr->ActiveIconMgr->active = Scr->ActiveIconMgr->first;
	if (Scr->ActiveIconMgr->active != NULL)
	    WarpToIconMgrEntry (Scr->ActiveIconMgr->active);
	return;
    }

    ip = Scr->ActiveIconMgr;
    cur_row = ip->active->row;
    cur_col = ip->active->col;

    row_inc = 0;
    col_inc = 0;
    got_it = FALSE;

    switch (dir)
    {
	case F_FORWICONMGR:
	    if ((tmp = ip->active->next) == NULL)
		tmp = ip->first;
	    got_it = TRUE;
	    break;

	case F_BACKICONMGR:
	    if ((tmp = ip->active->prev) == NULL)
		tmp = ip->last;
	    got_it = TRUE;
	    break;

	case F_UPICONMGR:
	    row_inc = -1;
	    break;

	case F_DOWNICONMGR:
	    row_inc = 1;
	    break;

	case F_LEFTICONMGR:
	    col_inc = -1;
	    break;

	case F_RIGHTICONMGR:
	    col_inc = 1;
	    break;
    }

    /* If got_it is FALSE ast this point then we got a left, right,
     * up, or down, command.  We will enter this loop until we find
     * a window to warp to.
     */
    new_row = cur_row;
    new_col = cur_col;

    while (!got_it)
    {
	new_row += row_inc;
	new_col += col_inc;
	if (new_row < 0)
	    new_row = ip->cur_rows - 1;
	if (new_col < 0)
	    new_col = ip->cur_columns - 1;
	if (new_row >= ip->cur_rows)
	    new_row = 0;
	if (new_col >= ip->cur_columns)
	    new_col = 0;
	    
	/* Now let's go through the list to see if there is an entry with this
	 * new position
	 */
	for (tmp = ip->first; tmp != NULL; tmp = tmp->next)
	{
	    if (tmp->row == new_row && tmp->col == new_col)
	    {
		got_it = TRUE;
		break;
	    }
	}
    }

    if (!got_it)
    {
	fprintf (stderr, 
		 "%s:  unable to find window (%d, %d) in icon manager\n", 
		 ProgramName, new_row, new_col);
	return;
    }

    if (ip->twm_win->mapped != TRUE && Scr->WarpUnmapped == TRUE)
    {
	static TwmWindow *savedwarp = NULL;

	if (ip->active->twm == savedwarp) {
	    Iconify (ip->active->twm, 0, 0);
	    savedwarp = NULL;
	}
	if (tmp != NULL && tmp->twm->mapped != TRUE)
	    savedwarp = tmp->twm;
	else
	    savedwarp = NULL;
    }

    if (tmp != NULL)
	WarpToIconMgrEntry (tmp);
}

/**
 * jump from one icon manager to another, possibly even on another screen
 *  \param dir one of the following:
 *    - F_NEXTICONMGR - go to the next icon manager 
 *    - F_PREVICONMGR - go to the previous one
 */

void JumpIconManager(int dir)
{
    IconMgr *ip, *tmp_ip = NULL;
    ScreenInfo *sp;
    int screen;
    WList *tmp = NULL;

    /* try to find an iconmanager if there is noone 'active': */
    if (Scr->ActiveIconMgr == NULL) {
	for (ip = &Scr->iconmgr; ip != NULL; ip = ip->next)
	    if (ip->first != NULL) {
		SetIconMgrEntryActive (ip->first);
		WarpToIconMgrEntry (ip->first);
		break;
	    }
	return;
    }

#define ITER(i) (dir == F_NEXTICONMGR ? (i)->next : (i)->prev)
#define IPOFSP(sp) (dir == F_NEXTICONMGR ? &(sp->iconmgr) : sp->iconmgr.lasti)
#define TEST(i) if ((i)->count > 0) \
		{ tmp = (i)->active != NULL ? (i)->active : (i)->first; break; }

    ip = Scr->ActiveIconMgr;

    for (tmp_ip = ITER(ip); tmp_ip; tmp_ip = ITER(tmp_ip)) {
	TEST (tmp_ip);
    }

    if (tmp == NULL) {
	int origscreen = ip->scr->screen;
	int inc = (dir == F_NEXTICONMGR ? 1 : -1);

	for (screen = origscreen + inc; ; screen += inc) {
	    if (screen >= NumScreens)
	      screen = 0;
	    else if (screen < 0)
	      screen = NumScreens - 1;

	    sp = ScreenList[screen];
	    if (sp) {
		/* check 'ActiveIconMgr' on that screen first: */
		if (sp->ActiveIconMgr != NULL) {
		    tmp_ip = sp->ActiveIconMgr;
		    TEST (sp->ActiveIconMgr);
		}
		for (tmp_ip = IPOFSP (sp); tmp_ip; tmp_ip = ITER(tmp_ip)) {
		    TEST (tmp_ip);
		}
	    }
	    if (tmp != NULL || screen == origscreen) break;
	}
    }

#undef ITER
#undef IPOFSP
#undef TEST

    if (tmp != NULL)
	WarpToIconMgrEntry (tmp);
    else
	Bell (XkbBI_MinorError,0,None);
}

/**
 * add a window to an icon manager
 *
 *  \param tmp_win the TwmWindow structure
 */
WList *AddIconManager(TwmWindow *tmp_win)
{
#ifdef TWM_USE_RENDER
    XRenderColor xrcol;
#endif
    WList *tmp;
    int h;
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    IconMgr *ip;
    Pixmap pm = None;			/* tmp pixmap variable */

    tmp_win->list = NULL;

    if (tmp_win->iconmgr || tmp_win->transient || Scr->NoIconManagers)
	return NULL;

    if (LookInList(Scr->IconMgrNoShow, tmp_win->full_name, &tmp_win->class))
	return NULL;
    if (Scr->IconManagerDontShow &&
	!LookInList(Scr->IconMgrShow, tmp_win->full_name, &tmp_win->class))
	return NULL;
    if ((ip = (IconMgr *)LookInList(Scr->IconMgrs, tmp_win->full_name,
	    &tmp_win->class)) == NULL)
	ip = &Scr->iconmgr;

    tmp = (WList *) calloc (1, sizeof(WList));
    tmp->iconmgr = ip;
    tmp->next = tmp->prev = NULL;
    tmp->down = FALSE;

    InsertInIconManager(ip, tmp, tmp_win);

    tmp->twm = tmp_win;

    tmp->IconManagerC = Scr->IconManagerC;
    tmp->highlight = Scr->IconManagerHighlight;

    GetColorFromList(Scr->IconManagerFL, tmp_win->full_name, &tmp_win->class,
	&tmp->IconManagerC.fore);
    GetColorFromList(Scr->IconManagerBL, tmp_win->full_name, &tmp_win->class,
	&tmp->IconManagerC.back);
    GetColorFromList(Scr->IconManagerHighlightL, tmp_win->full_name,
	&tmp_win->class, &tmp->highlight);

#ifdef TWM_USE_SPACING
    if (ip->columns > 1)
	h = 144*Scr->IconManagerFont.height/100; /*multicolumn baselineskip*/
    else
	h = 131*Scr->IconManagerFont.height/100; /*unicolumn baselineskip*/
    if (h < (Scr->iconifybox_height + 8))
	h = Scr->iconifybox_height + 8;
#else
    h = Scr->IconManagerFont.height + 10;
    if (h < (Scr->iconifybox_height + 4))
	h = Scr->iconifybox_height + 4;
#endif

    ip->height = h * ip->count;
    tmp->me = ip->count;
    tmp->x = -1;
    tmp->y = -1;

    valuemask = (CWBackPixel | CWBorderPixel | CWEventMask | CWCursor);
    attributes.background_pixel = tmp->IconManagerC.back;
    attributes.border_pixel = tmp->IconManagerC.back;
    attributes.event_mask = (KeyPressMask | ButtonPressMask |
			     ButtonReleaseMask | ExposureMask |
			     EnterWindowMask | LeaveWindowMask);
    attributes.cursor = Scr->IconMgrCursor;

/*#ifdef TWM_USE_OPACITY*/
#ifdef TWM_USE_RENDER
    if (Scr->use_xrender == TRUE) {
//	if (Scr->XCompMgrRunning == FALSE)
	    valuemask &= ~CWBackPixel;
    }
#endif
/*#endif*/

    tmp->w.win = XCreateWindow (dpy, ip->w, 0, 0, (unsigned int) 1,
			    (unsigned int) h, (unsigned int) 0,
			    CopyFromParent, (unsigned int) CopyFromParent,
			    (Visual *) CopyFromParent, valuemask, &attributes);

#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0) {
	tmp->w.xft = MyXftDrawCreate (tmp->w.win);
	CopyPixelToXftColor (tmp->IconManagerC.fore, &tmp->IconManagerC.xft);
    }
#endif

#ifdef TWM_USE_RENDER
    CopyPixelToXRenderColor (tmp->IconManagerC.back, &xrcol);
#ifdef TWM_USE_OPACITY
    if (Scr->XCompMgrRunning == TRUE) {
	xrcol.alpha = Scr->IconMgrOpacity * 257;
	xrcol.red = xrcol.red * xrcol.alpha / 0xffff; /* premultiply */
	xrcol.green = xrcol.green * xrcol.alpha / 0xffff;
	xrcol.blue = xrcol.blue * xrcol.alpha / 0xffff;
    }
#endif
    tmp->PenIconMgrB = Create_Color_Pen (&xrcol);
    CopyPixelToXRenderColor (tmp->highlight, &xrcol);
#ifdef TWM_USE_OPACITY
    if (Scr->XCompMgrRunning == TRUE)
    {
#if 1
	xrcol.alpha = ((3*255 + Scr->IconMgrOpacity)>>2) * 257;
#else
	xrcol.alpha = Scr->IconMgrOpacity * 257;
#endif
	xrcol.red = xrcol.red * xrcol.alpha / 0xffff; /* premultiply */
	xrcol.green = xrcol.green * xrcol.alpha / 0xffff;
	xrcol.blue = xrcol.blue * xrcol.alpha / 0xffff;
    }
#endif
    tmp->PenIconMgrH = Create_Color_Pen (&xrcol);
    tmp->PicIconMgrB = None;
#endif

    attributes.border_pixel = Scr->Black;
    attributes.cursor = Scr->ButtonCursor;
    attributes.event_mask = (ButtonReleaseMask| ButtonPressMask);
    valuemask = (CWBorderPixel | CWBackPixmap | CWCursor | CWEventMask);
    pm = XCreatePixmap (dpy, Scr->Root, Scr->iconifybox_width, Scr->iconifybox_height, Scr->d_depth);
    if (pm != None) {
	/* X-server draws the knob (as background), no Expose: */
	FB(Scr, tmp->IconManagerC.fore, tmp->IconManagerC.back);
	XCopyPlane (dpy, Scr->siconifyPm, pm, Scr->NormalGC,
		0, 0, Scr->iconifybox_width, Scr->iconifybox_height, 0, 0, 1);
    }
    attributes.background_pixmap = pm;

#ifdef TWM_USE_SPACING
    /* "iconified" knob text baseline-aligned: */
    JunkY = (int) (Scr->IconManagerFont.y
			+ (h - Scr->IconManagerFont.height)/2
			- Scr->iconifybox_height);
    if (JunkY < 4)
#endif
	/* "iconified" knob vertically centered: */
	JunkY = (int) (h - Scr->iconifybox_height)/2;

    tmp->icon = XCreateWindow (dpy, tmp->w.win, Scr->iconifybox_width/2, JunkY,
			       Scr->iconifybox_width, Scr->iconifybox_height,
			       (unsigned int) 0, CopyFromParent,
			       (unsigned int) CopyFromParent,
			       (Visual *) CopyFromParent,
			       valuemask, &attributes);

    if (pm != None)
	XFreePixmap (dpy, pm);

    if (HasShape)
	XShapeCombineMask (dpy, tmp->icon, ShapeBounding,
				0, 0, Scr->siconifyPm, ShapeSet);

    XSaveContext(dpy, tmp->w.win, IconManagerContext, (caddr_t) tmp);
    XSaveContext(dpy, tmp->w.win, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp->w.win, ScreenContext, (caddr_t) Scr);
    XSaveContext(dpy, tmp->icon, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp->icon, ScreenContext, (caddr_t) Scr);

    tmp_win->list = tmp;
    ip->count += 1;

    PackIconManager(ip);
    XMapWindow(dpy, tmp->w.win);

    if (/*HandlingEvents == FALSE && */
	    Scr->ShowIconManager == TRUE
		|| (Scr->ShowIconManagerL != NULL
		    && LookInList(Scr->ShowIconManagerL, ip->twm_win->full_name, &ip->twm_win->class) != NULL))
    {
	XMapWindow(dpy, ip->w);
	XMapWindow(dpy, ip->twm_win->frame);
    }

    if (ip->active == NULL)
	ip->active = tmp;

    return (tmp);
}

/**
 * put an allocated entry into an icon manager
 *
 *  \param ip  the icon manager pointer
 *  \param tmp the entry to insert
 */
void InsertInIconManager(IconMgr *ip, WList *tmp, TwmWindow *tmp_win)
{
    WList *tmp1;
    int i, added;
    int (*compar)(const char *, const char *) 
	= (Scr->CaseSensitive ? strcmp : XmuCompareISOLatin1);

    added = FALSE;
    if (ip->first == NULL)
    {
	tmp->prev = tmp->next = NULL;
	ip->first = ip->last = tmp;
	added = TRUE;
    }
    else if (Scr->SortIconMgr)
    {
	for (tmp1 = ip->first; tmp1 != NULL; tmp1 = tmp1->next)
	{
	    if (ip->reversed == TRUE)
		i = ((*compar)(tmp_win->icon_name, tmp1->twm->icon_name) > 0);
	    else
		i = ((*compar)(tmp1->twm->icon_name, tmp_win->icon_name) > 0);
	    if (i)
	    {
		tmp->next = tmp1;
		tmp->prev = tmp1->prev;
		tmp1->prev = tmp;
		if (tmp->prev == NULL)
		    ip->first = tmp;
		else
		    tmp->prev->next = tmp;
		added = TRUE;
		break;
	    }
	}
    }

    if (!added)
    {
	if (Scr->SortIconMgr == FALSE && ip->reversed == TRUE) {
	    /* prepend as first element: */
	    tmp->next = ip->first;
	    tmp->prev = NULL;
	    ip->first->prev = tmp;
	    ip->first = tmp;
	} else {
	    /* append as last element: */
	    tmp->prev = ip->last;
	    tmp->next = NULL;
	    ip->last->next = tmp;
	    ip->last = tmp;
	}
    }
}

void RemoveFromIconManager(IconMgr *ip, WList *tmp)
{
    if (tmp->prev == NULL)
	ip->first = tmp->next;
    else
	tmp->prev->next = tmp->next;

    if (tmp->next == NULL)
	ip->last = tmp->prev;
    else
	tmp->next->prev = tmp->prev;
}

/**
 * remove a window from the icon manager
 *  \param tmp_win the TwmWindow structure
 */
void RemoveIconManager(TwmWindow *tmp_win)
{
    IconMgr *ip;
    WList *tmp;

    if (tmp_win->list == NULL)
	return;

    tmp = tmp_win->list;
    tmp_win->list = NULL;
    ip = tmp->iconmgr;

    if (ip->hilited == tmp)
	ip->hilited = NULL;

    if (ip->active == tmp) {
	if (tmp->next != NULL)
	    ip->active = tmp->next;
	else if (tmp->prev != NULL)
	    ip->active = tmp->prev;
	else
	    ip->active = NULL;
    }

    RemoveFromIconManager(ip, tmp);

#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0)
	MyXftDrawDestroy(tmp->w.xft);
#endif

#ifdef TWM_USE_RENDER
    if (tmp->PenIconMgrH != None)
	XRenderFreePicture (dpy, tmp->PenIconMgrH);
    if (tmp->PenIconMgrB != None)
	XRenderFreePicture (dpy, tmp->PenIconMgrB);
    if (tmp->PicIconMgrB != None)
	XRenderFreePicture (dpy, tmp->PicIconMgrB);
#endif

    XDeleteContext(dpy, tmp->icon, TwmContext);
    XDeleteContext(dpy, tmp->icon, ScreenContext);
    XDestroyWindow(dpy, tmp->icon);
    XDeleteContext(dpy, tmp->w.win, IconManagerContext);
    XDeleteContext(dpy, tmp->w.win, TwmContext);
    XDeleteContext(dpy, tmp->w.win, ScreenContext);
    XDestroyWindow(dpy, tmp->w.win);
    XSync (dpy, False);
    DiscardWindowEvents (tmp->w.win, NoEventMask);
    ip->count -= 1;
    free((char *) tmp);

    PackIconManager(ip);

    if (ip->count == 0)
    {
	XUnmapWindow(dpy, ip->twm_win->frame);
    }

}

void ActiveIconManager(WList *active)
{
    if (active->iconmgr->hilited != active) {
	active->iconmgr->hilited = active;
	SetIconMgrEntryActive (active);
	if (active->iconmgr->scr->FilledIconMgrHighlight == TRUE) {
	    XSetWindowBackground (dpy, active->w.win, active->highlight);
	    SetIconManagerLabelShapeMask (active);
	    XClearArea (dpy, active->w.win, 0, 0, 0, 0, True); /*enforce Expose*/
	} else
	    DrawIconManagerBorder (active);
    }
}

void NotActiveIconManager(WList *active)
{
    if (active->iconmgr->hilited != NULL) {
	active->iconmgr->hilited = NULL;
	if (active->iconmgr->scr->FilledIconMgrHighlight == TRUE) {
	    XSetWindowBackground (dpy, active->w.win, active->IconManagerC.back);
	    SetIconManagerLabelShapeMask (active);
	    XClearArea (dpy, active->w.win, 0, 0, 0, 0, True); /*enforce Expose*/
	} else
	    DrawIconManagerBorder (active);
    }
}

void DrawIconManagerBorder(WList *tmp)
{
    if (tmp->iconmgr->twm_win->mapped == TRUE)
    {
	if (tmp->iconmgr->scr->ShapedIconMgrLabels == TRUE)
	{
	    if (tmp->iconmgr->hilited == tmp && tmp->iconmgr->scr->Highlight)
	    {
		if (tmp->iconmgr->scr->FilledIconMgrHighlight == TRUE)
		    XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->iconmgr->twm_win->BorderColor);
		else
		    XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->highlight);
	    }
	    else
		XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->iconmgr->twm_win->BorderColor);
	}
	else if (tmp->iconmgr->scr->FilledIconMgrHighlight == TRUE)
	{
	    if (tmp->iconmgr->hilited == tmp && tmp->iconmgr->scr->Highlight)
		XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->iconmgr->twm_win->BorderColor);
	    else
		XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->IconManagerC.fore);
	}
	else
	{
	    /*
	     * Historical iconmanager highlight painting:
	     */
	    XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->IconManagerC.fore /*tmp->iconmgr->twm_win->BorderColor*/);
	    XDrawRectangle (dpy, tmp->w.win, tmp->iconmgr->scr->NormalGC, 2, 2,
			    tmp->width-5, tmp->height-5);
	    if (tmp->iconmgr->hilited == tmp && tmp->iconmgr->scr->Highlight)
		XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->highlight);
	    else
		XSetForeground (dpy, tmp->iconmgr->scr->NormalGC, tmp->IconManagerC.back);
	    XDrawRectangle (dpy, tmp->w.win, tmp->iconmgr->scr->NormalGC, 0, 0,
			    tmp->width-1, tmp->height-1);
	}
	XDrawRectangle (dpy, tmp->w.win, tmp->iconmgr->scr->NormalGC, 1, 1,
			    tmp->width-3, tmp->height-3);
    }
}

/**
 * sort The Dude
 *
 *  \param ip a pointer to the icon manager struture
 */
void SortIconManager(IconMgr *ip)
{
    WList *tmp1, *tmp2;
    int i, done;
    int (*compar)(const char *, const char *) 
	= (Scr->CaseSensitive ? strcmp : XmuCompareISOLatin1);

    if (ip == NULL)
	ip = Scr->ActiveIconMgr;

    if (ip == NULL)
	return;

    done = FALSE;
    do
    {
	for (tmp1 = ip->first; tmp1 != NULL; tmp1 = tmp1->next)
	{
	    if ((tmp2 = tmp1->next) == NULL)
	    {
		done = TRUE;
		break;
	    }
	    if (ip->reversed == TRUE)
		i = ((*compar)(tmp2->twm->icon_name, tmp1->twm->icon_name) > 0);
	    else
		i = ((*compar)(tmp1->twm->icon_name, tmp2->twm->icon_name) > 0);
	    if (i)
	    {
		/* take it out and put it back in */
		RemoveFromIconManager(ip, tmp2);
		InsertInIconManager(ip, tmp2, tmp2->twm);
		break;
	    }
	}
    }
    while (!done);
    PackIconManager(ip);
}

/**
 * swap two consecutive elements
 *
 *  \param lst a pointer to the current icon manager list entry
 *  \param dir one of the following:
 *    - F_NEXTICONMGR - swap current and next entry
 *    - F_PREVICONMGR - swap current and prev entry
 */
void SwapIconManagerEntry (WList *lst, int dir)
{
    if (lst->iconmgr && lst->iconmgr->count > 1)
    {
	WList *tmp;

	RemoveFromIconManager (lst->iconmgr, lst);

	switch (dir) {
	case F_NEXTICONMGR:
	    if (lst->next == NULL) {
		tmp = lst->prev;
		lst->next = lst->iconmgr->first;
		lst->prev = NULL;
		lst->iconmgr->first->prev = lst;
		lst->iconmgr->first = lst;
	    } else {
		tmp = lst->next;
		lst->next = tmp->next;
		if (tmp->next == NULL)
		    lst->iconmgr->last = lst;
		else
		    tmp->next->prev = lst;
		tmp->next = lst;
		lst->prev = tmp;
	    }
	    break;
	case F_PREVICONMGR:
	    if (lst->prev == NULL) {
		tmp = lst->next;
		lst->prev = lst->iconmgr->last;
		lst->next = NULL;
		lst->iconmgr->last->next = lst;
		lst->iconmgr->last = lst;
	    } else {
		tmp = lst->prev;
		lst->prev = tmp->prev;
		if (tmp->prev == NULL)
		    lst->iconmgr->first = lst;
		else
		    tmp->prev->next = lst;
		tmp->prev = lst;
		lst->next = tmp;
	    }
	    break;
	}

	/*
	 * To avoid focus from falling into "neighbour" client right
	 * after completing PackIconManager() and so trigger StolenFocus
	 * recovery right after WarpToIconMgrEntry() simply unmap-and-map that
	 * neighbour client's icon manager entry window for the period
	 * of these activities.
	 */
	XUnmapWindow (dpy, tmp->w.win);
	PackIconManager (lst->iconmgr);
	WarpToIconMgrEntry (lst);
	XSync (dpy, False);
	XMapWindow (dpy, tmp->w.win);
    }
}

/**
 * pack the icon manager windows following 
 *		an addition or deletion
 *
 *  \param ip a pointer to the icon manager struture
 */
void PackIconManager(IconMgr *ip)
{
    int newwidth, i, row, col, maxcol,  colinc, rowinc, wheight, wwidth;
    int new_x, new_y;
    WList *tmp;

#ifdef TWM_USE_SPACING
    if (ip->columns > 1) /* 100*pow(sqrt(1.2), i) for i = 2, 3, 4 */
	wheight = 144*ip->scr->IconManagerFont.height/100; /* i = 4 multicolumn*/
    else
	wheight = 131*ip->scr->IconManagerFont.height/100; /* i = 3 unicolumn*/
    if (wheight < (ip->scr->iconifybox_height + 8))
	wheight = ip->scr->iconifybox_height + 8;
#else
    wheight = ip->scr->IconManagerFont.height + 10;
    if (wheight < (ip->scr->iconifybox_height + 4))
	wheight = ip->scr->iconifybox_height + 4;
#endif

    wwidth = ip->width / (ip->cur_columns > 0 ? ip->cur_columns : ip->columns);

    rowinc = wheight;
    colinc = wwidth;

    row = 0;
    col = ip->columns;
    maxcol = 0;
    for (i = 0, tmp = ip->first; tmp != NULL; i++, tmp = tmp->next)
    {
	tmp->me = i;
	if (++col >= ip->columns)
	{
	    col = 0;
	    row += 1;
	}
	if (col > maxcol)
	    maxcol = col;

	new_x = col * colinc;
	new_y = (row-1) * rowinc;

	/* if the position or size has not changed, don't touch it */
	if (tmp->x != new_x || tmp->y != new_y ||
	    tmp->width != wwidth || tmp->height != wheight)
	{
	    XMoveResizeWindow(dpy, tmp->w.win, new_x, new_y, wwidth, wheight);

	    tmp->row = row-1;
	    tmp->col = col;
	    tmp->x = new_x;
	    tmp->y = new_y;
	    tmp->width = wwidth;
	    tmp->height = wheight;
	}
    }
    maxcol += 1;

    ip->cur_rows = row;
    ip->cur_columns = maxcol;
    ip->height = row * rowinc;
    if (ip->height == 0)
    	ip->height = rowinc;
    newwidth = maxcol * colinc;
    if (newwidth == 0)
	newwidth = colinc;

    XResizeWindow(dpy, ip->w, newwidth, ip->height);

    if ((ip->Xnegative == TRUE) || (ip->Ynegative == TRUE))
	if (XGetGeometry(dpy, ip->twm_win->frame, &JunkRoot, &JunkX, &JunkY,
		&JunkWidth, &JunkHeight, &JunkBW, &JunkDepth))
	{
	    if (ip->Xnegative == TRUE)
		ip->twm_win->frame_x = JunkX + JunkWidth  - newwidth;
	    if (ip->Ynegative == TRUE)
		ip->twm_win->frame_y = JunkY + JunkHeight - (ip->height + ip->twm_win->title_height);
	}

    SetupWindow (ip->twm_win,
		   ip->twm_win->frame_x, ip->twm_win->frame_y,
		   newwidth, ip->height + ip->twm_win->title_height, -1);

    /* draw shapes anew: */
    SetIconManagerAllLabelShapeMasks (ip);
}


void SetIconManagerAllLabelShapeMasks (IconMgr *ip)
{
    WList *tmp;
    XRectangle txt;

    if (HasShape && (ip->scr->ShapedIconMgrLabels == TRUE && (ip->twm_win->mapped == TRUE || HandlingEvents == FALSE))) {
#ifdef TWM_USE_RENDER
	if (ip->scr->use_xrender == FALSE || /*XCM*/ ip->scr->XCompMgrRunning == FALSE)
	    /*
	     * XOrg XComposite extension has problems with XShape extension, so we
	     * use 100% transparency in pixels outside of the "shape".  If there is
	     * no composite manager (e.g. "xcompmgr") running in the background, then
	     * XComposite extension is probably not utilised, and it should be safe
	     * to use the XShape extension.
	     */
#endif
	{
	    txt.x = 0;
	    txt.y = 0;
	    txt.width = ip->width;
	    txt.height = ip->height;
	    XShapeCombineRectangles (dpy, ip->twm_win->frame, ShapeBounding,
					0, ip->twm_win->title_height,
					&txt, 1, ShapeSubtract, Unsorted);
	}
	for (tmp = ip->first; tmp != NULL; tmp = tmp->next)
	    SetIconManagerLabelShapeMask (tmp);
    }
}


void SetIconManagerLabelShapeMask (WList *tmp)
{
    if (HasShape && (tmp->iconmgr->scr->ShapedIconMgrLabels == TRUE && (tmp->iconmgr->twm_win->mapped == TRUE || HandlingEvents == FALSE)))
    {
	Pixmap mask;
	GC gc;

#ifdef TWM_USE_RENDER
	if (tmp->iconmgr->scr->use_xrender == TRUE && /*XCM*/ tmp->iconmgr->scr->XCompMgrRunning == TRUE)
	{
	    mask = XCreatePixmap (dpy, tmp->iconmgr->scr->Root, tmp->width, tmp->height, tmp->iconmgr->scr->DepthA);
	    gc = tmp->iconmgr->scr->AlphaGC;
	}
	else
#endif
	{
	    mask = XCreatePixmap (dpy, tmp->iconmgr->scr->Root, tmp->width, tmp->height, 1);
	    gc = tmp->iconmgr->scr->BitGC;
	}

	if (mask != None)
	{
	    /* cleanup mask: */
	    XSetForeground (dpy, gc, 0);
	    XFillRectangle (dpy, mask, gc, 0, 0, tmp->width, tmp->height);

	    /* "unshape" hilited entry: */
	    if (tmp->iconmgr->hilited == tmp && tmp->iconmgr->scr->FilledIconMgrHighlight == TRUE)
	    {
		XSetForeground (dpy, gc, AllPlanes);
		XFillRectangle (dpy, mask, gc, 1, 1, tmp->width-2, tmp->height-2);
	    }
	    else /* "shape" non-hilited entry: */
	    {
		/* add iconmanager label text shape: */
		MyFont_DrawShapeStringEllipsis (mask,
			&tmp->iconmgr->scr->IconManagerFont,
			tmp->iconmgr->scr->iconmgr_textx,
#ifdef TWM_USE_SPACING
			tmp->iconmgr->scr->IconManagerFont.y
			    + (tmp->height-tmp->iconmgr->scr->IconManagerFont.height)/2,
#else
			tmp->iconmgr->scr->IconManagerFont.y+4,
#endif
			tmp->twm->icon_name, strlen (tmp->twm->icon_name),
			tmp->width - tmp->iconmgr->scr->iconmgr_textx
				    - tmp->iconmgr->scr->IconManagerFont.ellen/2,
			tmp->iconmgr->scr->IconMgrLabelOffsetX,
			tmp->iconmgr->scr->IconMgrLabelOffsetY);

		XSetForeground (dpy, gc, AllPlanes);

		/* draw outline: */
		XDrawRectangle (dpy, mask, gc, 1, 1, tmp->width-3, tmp->height-3);

		/* add "iconified" button area shape: */
		JunkX = tmp->iconmgr->scr->iconifybox_width/2;
#ifdef TWM_USE_SPACING
		JunkY = tmp->iconmgr->scr->IconManagerFont.y
			    + (tmp->height - tmp->iconmgr->scr->IconManagerFont.height)/2
			    - tmp->iconmgr->scr->iconifybox_height;
		if (JunkY < 4)
#endif
		    JunkY = (tmp->height - tmp->iconmgr->scr->iconifybox_height)/2;

		XFillRectangle (dpy, mask, gc, JunkX, JunkY,
			tmp->iconmgr->scr->iconifybox_width, tmp->iconmgr->scr->iconifybox_height);

#ifdef TWM_USE_RENDER
		if (tmp->iconmgr->scr->use_xrender == FALSE || /*XCM*/ tmp->iconmgr->scr->XCompMgrRunning == FALSE)
#endif
		{
		    /* subtract inverted mask: switch off background pixels: */
		    XSetFunction (dpy, gc, GXinvert);
		    XFillRectangle (dpy, mask, gc, 0, 0, tmp->width, tmp->height);
		    XShapeCombineMask (dpy, tmp->iconmgr->twm_win->frame, ShapeBounding,
					tmp->x, tmp->y+tmp->iconmgr->twm_win->title_height,
					mask, ShapeSubtract);
		    /* double invert mask to switch on foreground pixes: */
		    XFillRectangle (dpy, mask, gc, 0, 0, tmp->width, tmp->height);
		    XSetFunction (dpy, gc, GXcopy); /*restore*/
		}
	    }

#ifdef TWM_USE_RENDER
	    if (tmp->iconmgr->scr->use_xrender == TRUE && /*XCM*/ tmp->iconmgr->scr->XCompMgrRunning == TRUE)
	    {
		if (tmp->PicIconMgrB != None)
		    XRenderFreePicture (dpy, tmp->PicIconMgrB);
		tmp->PicIconMgrB = XRenderCreatePicture (dpy, mask, XRenderFindStandardFormat (dpy, tmp->iconmgr->scr->FormatA),
							CPGraphicsExposure, &tmp->iconmgr->scr->PicAttr);
	    }
	    else
#endif
	    {
		XShapeCombineMask (dpy, tmp->iconmgr->twm_win->frame, ShapeBounding,
				tmp->x, tmp->y+tmp->iconmgr->twm_win->title_height,
				mask, ShapeUnion);
	    }

	    XFreePixmap (dpy, mask);
	}
    }
}
