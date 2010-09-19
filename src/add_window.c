/* $XFree86: xc/programs/twm/add_window.c,v 1.11 2002/04/04 14:05:58 eich Exp $ */
/*****************************************************************************/
/*

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


/**********************************************************************
 *
 * $Xorg: add_window.c,v 1.5 2001/02/09 02:05:36 xorgcvs Exp $
 *
 * Add a new window, put the titlbar and other stuff around
 * the window
 *
 * 31-Mar-88 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#include <stdio.h>
#include "twm.h"
#include <X11/Xatom.h>
#include "util.h"
#include "resize.h"
#include "parse.h"
#include "list.h"
#include "events.h"
#include "menus.h"
#include "screen.h"
#include "iconmgr.h"
#include "session.h"
#include "add_window.h"

#define gray_width 2
#define gray_height 2
static char gray_bits[] = {
   0x02, 0x01};

int AddingX;
int AddingY;
int AddingW;
int AddingH;

static void do_add_binding ( int button, int context, int modifier, int func );
static Window CreateHighlightWindow ( TwmWindow *tmp_win );
static void CreateWindowTitlebarButtons ( TwmWindow *tmp_win );
static void SetWindowGroupHintForFrame (TwmWindow *tmp_win, XID window_group);


char NoName[] = "Untitled"; /* name if no name is specified */


/**
 * Look for a nonoccupied rectangular area to place a new client.
 * If found return this area in 'pos', otherwise don't change 'pos'.
 * Begin with decomposing screen empty space into (overlapping) 'tiles'.
 *
 *  \param lst		TWM window list
 *  \param twm		new client
 *  \param tiles	tile list to search
 *  \param pos		return found tile
 */
#define OVERLAP(a,f)	((a)->x < (f)->frame_x+(f)->frame_width && (f)->frame_x < (a)->x+(a)->width \
			&& (a)->y < (f)->frame_y+(f)->frame_height && (f)->frame_y < (a)->y+(a)->height)
static int
find_empty_area (TwmWindow *lst, TwmWindow *twm, PlaceXY *tiles, PlaceXY *pos)
{
    PlaceXY *t, **pt;

    /* skip unmapped windows and iconmanagers: */
    while (lst != NULL && (lst->mapped == FALSE || lst->iconmgr == TRUE))
	lst = lst->next;

    if (lst == NULL)
    {
	/* decomposition done, pick and return one 'tile': */
	PlaceXY *m;
	int w, h, d;

	w = twm->attr.width + 2*twm->frame_bw;
	h = twm->attr.height + 2*twm->frame_bw + twm->title_height;
	d = 2*Scr->TitleHeight;
	m = t = NULL;

	while (tiles != NULL) {
	    if (tiles->width >= w && tiles->height >= h) {
		int x, y;
		x = tiles->x + twm->attr.width/4;
		y = tiles->y + twm->attr.height/4;
		if (tiles->width >= w+d && tiles->height >= h+d) {
		    if ((m == NULL) || (m->y >= y) || (m->x > x))
			m = tiles; /* best choice */
		}
		if ((t == NULL) || (t->y >= y) || (t->x > x))
		    t = tiles; /* second choice */
	    }
	    tiles = tiles->next;
	}

	if (m != NULL)
	    t = m;

	if (t != NULL) {
	    pos->x = t->x;
	    pos->y = t->y;
	    pos->width = t->width;
	    pos->height = t->height;
	    return TRUE;
	}
    }
    else
    {
	/* proceed with screen 'tile' decomposition: */
	for (pt = &tiles; (*pt) != NULL; pt = &((*pt)->next))
	    if (OVERLAP((*pt), lst)) {
		auto PlaceXY _buf[4], *buf = _buf;

		t = (*pt)->next; /*remove current tile*/

		if (lst->frame_x + lst->frame_width < (*pt)->x + (*pt)->width) {
		    /* add right subtile: */
		    buf[0].x = lst->frame_x + lst->frame_width;
		    buf[0].y = (*pt)->y;
		    buf[0].width = (*pt)->x + (*pt)->width - (lst->frame_x + lst->frame_width);
		    buf[0].height = (*pt)->height;
		    if (buf[0].width >= twm->attr.width) { /*preselection*/
			buf[0].next = t;
			t = &buf[0];
		    }
		}
		if (lst->frame_y + lst->frame_height < (*pt)->y + (*pt)->height) {
		    /* add bottom subtile: */
		    buf[1].x = (*pt)->x;
		    buf[1].y = lst->frame_y + lst->frame_height;
		    buf[1].width = (*pt)->width;
		    buf[1].height = (*pt)->y + (*pt)->height - (lst->frame_y + lst->frame_height);
		    if (buf[1].height >= twm->attr.height) {
			buf[1].next = t;
			t = &buf[1];
		    }
		}
		if (lst->frame_y > (*pt)->y) { /*swap 2, 3 to rearrange*/
		    /* add top subtile: */
		    buf[2].x = (*pt)->x;
		    buf[2].y = (*pt)->y;
		    buf[2].width = (*pt)->width;
		    buf[2].height = lst->frame_y - (*pt)->y;
		    if (buf[2].height >= twm->attr.height) {
			buf[2].next = t;
			t = &buf[2];
		    }
		}
		if (lst->frame_x > (*pt)->x) {
		    /* add left subtile: */
		    buf[3].x = (*pt)->x;
		    buf[3].y = (*pt)->y;
		    buf[3].width = lst->frame_x - (*pt)->x;
		    buf[3].height = (*pt)->height;
		    if (buf[3].width >= twm->attr.width) {
			buf[3].next = t;
			t = &buf[3];
		    }
		}

		(*pt) = t;

		return find_empty_area (lst, twm, tiles, pos); /*continue subtiling*/
	    }
	return find_empty_area (lst->next, twm, tiles, pos); /*start over with next client*/
    }
    return FALSE;
}


int
FindEmptyArea (TwmWindow *lst, TwmWindow *twm, PlaceXY *tiles, PlaceXY *pos)
{
    TwmWindow *tmp_win;
    int cnt = 0;

    /* count how many clients there are to be considered */
    for (tmp_win = lst; tmp_win; tmp_win = tmp_win->next)
	if (tmp_win->mapped == TRUE && tmp_win->iconmgr == FALSE)
	    if (OVERLAP(tiles, tmp_win))
		++cnt;

    if (cnt > 20)
	return FALSE; /* the algorithm complexity grows intractably in the number of clients, don't use it */
    else
	return find_empty_area (lst, twm, tiles, pos);
}


/**  
 * map gravity to (x,y) offset signs for adding to x and y when window is 
 * mapped to get proper placement.
 *
 *  \param tmp    window from which to get gravity
 *  \param xp,yp  return values
 * 
 */
void
GetGravityOffsets (TwmWindow *tmp, int *xp, int *yp)
{
    static struct _gravity_offset {
	int x, y;
    } gravity_offsets[11] = {
	{  0,  0 },			/* ForgetGravity */
	{ -1, -1 },			/* NorthWestGravity */
	{  0, -1 },			/* NorthGravity */
	{  1, -1 },			/* NorthEastGravity */
	{ -1,  0 },			/* WestGravity */
	{  0,  0 },			/* CenterGravity */
	{  1,  0 },			/* EastGravity */
	{ -1,  1 },			/* SouthWestGravity */
	{  0,  1 },			/* SouthGravity */
	{  1,  1 },			/* SouthEastGravity */
	{  0,  0 },			/* StaticGravity */
    };
    register int g = ((tmp->hints.flags & PWinGravity) 
		      ? tmp->hints.win_gravity : NorthWestGravity);

    if (g < ForgetGravity || g > StaticGravity) {
	*xp = *yp = 0;
    } else {
	*xp = gravity_offsets[g].x;
	*yp = gravity_offsets[g].y;
    }
}


static TwmWindow *
GroupMember (TwmWindow *tmp, TwmWindow *win)
{
    while (tmp != NULL) {
	if (tmp != win && IS_RELATED_WIN(tmp,win))
	    break;
	tmp = tmp->next;
    }
    return tmp;
}


/** 
 * add a new window to the twm list.
 *
 *  \return  pointer to the TwmWindow structure
 *
 *	\param w      the window id of the window to add
 *	\param iconm  flag to tell if this is an icon manager window
 *	\param iconp  pointer to icon manager struct
 */
TwmWindow *
AddWindow(Window w, int iconm, IconMgr *iconp)
{
    TwmWindow *tmp_win;			/* new twm window structure */
    TwmWindow **ptmp_win;
    int stat;
    XEvent event;
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    int width, height;			/* tmp variable */
    int ask_user;		/* don't know where to put the window */
    int gravx, gravy;			/* gravity signs for positioning */
    int namelen;
    int bw2;
    short saved_x, saved_y, restore_icon_x, restore_icon_y;
    unsigned short saved_width, saved_height;
    Bool restore_iconified = 0;
    Bool restore_icon_info_present = 0;
    int restoredFromPrevSession;
    Bool width_ever_changed_by_user;
    Bool height_ever_changed_by_user;
    XTextProperty text_prop;		/* propagate window name */
    char *name;

#ifdef DEBUG
    fprintf(stderr, "AddWindow: w = 0x%x\n", w);
#endif

    /* allocate space for the twm window */
    tmp_win = (TwmWindow *)calloc(1, sizeof(TwmWindow));
    if (tmp_win == 0)
    {
	fprintf (stderr, "%s: Unable to allocate memory to manage window ID %lx.\n",
		 ProgramName, w);
	return NULL;
    }
    tmp_win->w = w;
    tmp_win->zoomed = ZOOM_NONE;
    tmp_win->iconmgr = iconm;
    tmp_win->iconmgrp = iconp;
    tmp_win->cmaps.number_cwins = 0;

    XSelectInput(dpy, tmp_win->w, PropertyChangeMask);

    XGetWindowAttributes(dpy, tmp_win->w, &tmp_win->attr);

    if (!I18N_FetchName(tmp_win->w, &name))
      name = NULL;
    if (name == NULL)
	tmp_win->name = strdup(NoName);
    else {
      tmp_win->name = strdup(name);
      free(name);
    }
    tmp_win->full_name = strdup(tmp_win->name);
    namelen = strlen (tmp_win->name);
    tmp_win->nameChanged = 0;

    if (!I18N_GetIconName(tmp_win->w, &name)) {
	tmp_win->icon_name = strdup(tmp_win->name);
    } else {
	if (name == NULL) {
	    tmp_win->icon_name = strdup(tmp_win->name);
	} else {
	    tmp_win->icon_name = strdup(name);
	    free(name);
	}
    }

    tmp_win->class = NoClass;
    XGetClassHint(dpy, tmp_win->w, &tmp_win->class);
    if (tmp_win->class.res_name == NULL)
	tmp_win->class.res_name = NoName;
    if (tmp_win->class.res_class == NULL)
	tmp_win->class.res_class = NoName;

    FetchWmProtocols (tmp_win);
    FetchWmColormapWindows (tmp_win);
    tmp_win->transient = Transient (tmp_win->w, &tmp_win->transientfor);

    if (GetWindowConfig (tmp_win,
	&saved_x, &saved_y, &saved_width, &saved_height,
	&restore_iconified, &restore_icon_info_present,
	&restore_icon_x, &restore_icon_y,
	&width_ever_changed_by_user, &height_ever_changed_by_user))
    {
	tmp_win->attr.x = saved_x;
	tmp_win->attr.y = saved_y;

	tmp_win->widthEverChangedByUser = width_ever_changed_by_user;
	tmp_win->heightEverChangedByUser = height_ever_changed_by_user;
	
	if (width_ever_changed_by_user)
	    tmp_win->attr.width = saved_width;

	if (height_ever_changed_by_user)
	    tmp_win->attr.height = saved_height;

	restoredFromPrevSession = 1;
    }
    else
    {
	tmp_win->widthEverChangedByUser = False;
	tmp_win->heightEverChangedByUser = False;

	restoredFromPrevSession = 0;
    }

    tmp_win->wmhints = XGetWMHints(dpy, tmp_win->w);

    if (!tmp_win->wmhints)
    {
	tmp_win->wmhints = (XWMHints *) malloc (sizeof(XWMHints));
	tmp_win->wmhints->flags = 0;
    }

    if (tmp_win->wmhints && (tmp_win->wmhints->flags & WindowGroupHint)) 
	tmp_win->group = tmp_win->wmhints->window_group;
    else
	tmp_win->group = tmp_win->w; /* None; */

    if (tmp_win->wmhints)
    {
	if (restore_iconified
		|| (Scr->StartIconified != NULL
			&& LookInList(Scr->StartIconified, tmp_win->full_name, &tmp_win->class) != NULL
			/*&& tmp_win->transient == FALSE*/))
	{
	    TwmWindow *tmp;
	    if (restore_iconified)
		tmp = NULL;
	    else
	    {
		/* skip initial iconifying if some relative window is mapped */
		tmp = Scr->TwmRoot.next;
		while (tmp != NULL) {
		    tmp = GroupMember (tmp, tmp_win);
		    if (tmp == NULL || tmp->icon == FALSE)
			break;
		    tmp = tmp->next;
		}
	    }
	    if (tmp == NULL) {
		tmp_win->wmhints->initial_state = IconicState;
		tmp_win->wmhints->flags |= StateHint;
	    }
	}

	if (restore_icon_info_present)
	{
	    tmp_win->wmhints->icon_x = restore_icon_x;
	    tmp_win->wmhints->icon_y = restore_icon_y;
	    tmp_win->wmhints->flags |= IconPositionHint;
	}
    }


    /*
     * do initial clip; should look at window gravity
     */
    if (tmp_win->attr.width > Scr->MaxWindowWidth)
      tmp_win->attr.width = Scr->MaxWindowWidth;
    if (tmp_win->attr.height > Scr->MaxWindowHeight)
      tmp_win->attr.height = Scr->MaxWindowHeight;


    /*
     * The July 27, 1988 draft of the ICCCM ignores the size and position
     * fields in the WM_NORMAL_HINTS property.
     */

    tmp_win->highlight = Scr->Highlight && 
	(!(short)(long) LookInList(Scr->NoHighlight, tmp_win->full_name, 
	    &tmp_win->class));

    tmp_win->stackmode = Scr->StackMode &&
	(!(short)(long) LookInList(Scr->NoStackModeL, tmp_win->full_name, 
	    &tmp_win->class));

    tmp_win->titlehighlight = Scr->TitleHighlight && 
	(!(short)(long) LookInList(Scr->NoTitleHighlight, tmp_win->full_name, 
	    &tmp_win->class));

    tmp_win->auto_raise = (short)(long) LookInList(Scr->AutoRaise, 
						  tmp_win->full_name,
					    &tmp_win->class);
    if (tmp_win->auto_raise) Scr->NumAutoRaises++;
    tmp_win->iconify_by_unmapping = Scr->IconifyByUnmapping;
    if (Scr->IconifyByUnmapping)
    {
	tmp_win->iconify_by_unmapping = iconm ? FALSE :
	    !(short)(long) LookInList(Scr->DontIconify, tmp_win->full_name,
		&tmp_win->class);
    }
    tmp_win->iconify_by_unmapping |= 
	(short)(long) LookInList(Scr->IconifyByUn, tmp_win->full_name,
	    &tmp_win->class);

    tmp_win->squeeze_info = NULL;
    /*
     * get the squeeze information; note that this does not have to be freed
     * since it is coming from the screen list
     */
    if (HasShape) {
	if (!LookInList (Scr->DontSqueezeTitleL, tmp_win->full_name, 
			 &tmp_win->class)) {
	    tmp_win->squeeze_info = (SqueezeInfo *)
	      LookInList (Scr->SqueezeTitleL, tmp_win->full_name,
			  &tmp_win->class);
	    if (!tmp_win->squeeze_info) {
		static SqueezeInfo default_squeeze = { J_LEFT, 0, 0 };
		if (Scr->SqueezeTitle)
		  tmp_win->squeeze_info = &default_squeeze;
	    }
	}
      }

    tmp_win->old_bw = tmp_win->attr.border_width;

    if ((Scr->ClientBorderWidth == TRUE)
	    || (Scr->ClientBorderWidthL != NULL
		&& LookInList(Scr->ClientBorderWidthL, tmp_win->full_name, &tmp_win->class) != NULL)
	    || (Scr->NoClientBorderWidthL != NULL
		&& LookInList(Scr->NoClientBorderWidthL, tmp_win->full_name, &tmp_win->class) == NULL))
    {
	tmp_win->ClientBorderWidth = TRUE;
	tmp_win->frame_bw = tmp_win->old_bw;
    } else {
	tmp_win->ClientBorderWidth = FALSE;
	tmp_win->frame_bw = Scr->BorderWidth;
    }
    bw2 = tmp_win->frame_bw * 2;

    tmp_win->title_height = Scr->TitleHeight + tmp_win->frame_bw;
    if (Scr->NoTitlebar)
        tmp_win->title_height = 0;
    if (LookInList(Scr->MakeTitle, tmp_win->full_name, &tmp_win->class))
        tmp_win->title_height = Scr->TitleHeight + tmp_win->frame_bw;
    if (LookInList(Scr->NoTitle, tmp_win->full_name, &tmp_win->class))
        tmp_win->title_height = 0;

    /* if it is a transient window, don't put a title on it */
    if (tmp_win->transient &&
	    !((Scr->DecorateTransients == TRUE)
		|| (Scr->DecorateTransientsL != NULL
		    && LookInList(Scr->DecorateTransientsL, tmp_win->full_name, &tmp_win->class) != NULL)
		|| (Scr->NoDecorateTransientsL != NULL
		    && LookInList(Scr->NoDecorateTransientsL, tmp_win->full_name, &tmp_win->class) == NULL)))
    {
	tmp_win->title_height = 0;
    }

    GetWindowSizeHints (tmp_win);

    if (restoredFromPrevSession)
    {
	/*
	 * When restoring window positions from the previous session,
	 * we always use NorthWest gravity.
	 */

	gravx = gravy = -1;
    }
    else
    {
	GetGravityOffsets (tmp_win, &gravx, &gravy);
    }

    /*
     * Don't bother user if:
     * 
     *     o  the window is a transient, or
     * 
     *     o  a USPosition was requested, or
     * 
     *     o  a PPosition was requested and UsePPosition is ON or
     *        NON_ZERO if the window is at other than (0,0)
     */
    ask_user = TRUE;
    if (tmp_win->transient || 
	(tmp_win->hints.flags & USPosition) ||
        ((tmp_win->hints.flags & PPosition) && Scr->UsePPosition &&
	 (Scr->UsePPosition == PPOS_ON || 
	  tmp_win->attr.x != 0 || tmp_win->attr.y != 0)))
      ask_user = FALSE;

    /*
     * do any prompting for position
     */
    if (HandlingEvents && ask_user && !restoredFromPrevSession) {
      if ((Scr->RandomPlacement == TRUE)
	    || (Scr->RandomPlacementL != NULL
		&& LookInList(Scr->RandomPlacementL, tmp_win->full_name, &tmp_win->class) != NULL)
	    || (Scr->NoRandomPlacementL != NULL
		&& LookInList(Scr->NoRandomPlacementL, tmp_win->full_name, &tmp_win->class) == NULL))
      {
	/* just stick it somewhere */
	static int PlaceX = 50;
	static int PlaceY = 50;
	PlaceXY area;

#ifdef TILED_SCREEN
	if (Scr->use_panels == TRUE)
	{
	    TwmWindow *tmp;
	    int k;

	    /* look for "group leader" panel: */
	    tmp = GroupMember (Scr->TwmRoot.next, tmp_win);
	    if (tmp != NULL)
		k = FindNearestPanelToClient (tmp);
	    else
		k = FindNearestPanelToMouse();

	    area.x = Lft(Scr->panels[k]);
	    area.y = Bot(Scr->panels[k]);
	    area.width = AreaWidth(Scr->panels[k]);
	    area.height = AreaHeight(Scr->panels[k]);
	}
	else
#endif
	{
	    area.x = area.y = 0;
	    area.width = Scr->MyDisplayWidth;
	    area.height = Scr->MyDisplayHeight;
	}
	area.next = NULL;

	if (FindEmptyArea(Scr->TwmRoot.next, tmp_win, &area, &area) == TRUE)
	{
	    int x, y, b, d;
	    b = Scr->TitleHeight + tmp_win->frame_bw;
	    d = (gravy < 0 ? tmp_win->title_height : 0);
	    /* slightly off-centered: */
	    x = (area.width - tmp_win->attr.width) / 3;
	    y = (area.height - tmp_win->attr.height + d) / 2;
	    /* tight placing: */
	    if (y < b+d && x > b)
	        x = b;
	    if (x < b && y > b+d)
	        y = b+d;
	    /* loosen placing: */
	    if (area.width > 4*b + tmp_win->attr.width && x > 2*b)
		x = 2*b;
	    if (area.height > 4*b + tmp_win->attr.height && y > 2*b)
		y = 2*b;
	    tmp_win->attr.x = area.x + x;
	    tmp_win->attr.y = area.y + y;
	    /* reset random-placement "empty space" locator: */
	    PlaceX = PlaceY = 50;
	} else {
	    if ((PlaceX + tmp_win->attr.width) > area.width)
		PlaceX = 50;
	    if ((PlaceY + tmp_win->attr.height) > area.height)
		PlaceY = 50;

	    tmp_win->attr.x = area.x + PlaceX;
	    tmp_win->attr.y = area.y + PlaceY;
	    PlaceX += 30;
	    PlaceY += 30;
	}
      }
      else /* else prompt */
      {
	if (!(tmp_win->wmhints && tmp_win->wmhints->flags & StateHint &&
	      tmp_win->wmhints->initial_state == IconicState))
	{
	    Bool firsttime = True;

	    /* better wait until all the mouse buttons have been 
	     * released.
	     */
	    while (TRUE)
	    {
		XUngrabServer(dpy);
		XSync(dpy, 0);
		XGrabServer(dpy);

		JunkMask = 0;
		if (!XQueryPointer (dpy, Scr->Root, &JunkRoot, 
				    &JunkChild, &JunkX, &JunkY,
				    &AddingX, &AddingY, &JunkMask))
		  JunkMask = 0;

		JunkMask &= (Button1Mask | Button2Mask | Button3Mask |
			     Button4Mask | Button5Mask);

		/*
		 * watch out for changing screens
		 */
		if (firsttime) {
		    if (JunkRoot != Scr->Root) {
			register int scrnum;

			for (scrnum = 0; scrnum < NumScreens; scrnum++) {
			    if (JunkRoot == RootWindow (dpy, scrnum)) break;
			}

			if (scrnum != NumScreens) PreviousScreen = scrnum;
		    }
		    firsttime = False;
		}

		/*
		 * wait for buttons to come up; yuck
		 */
		if (JunkMask != 0) continue;

		/* 
		 * this will cause a warp to the indicated root
		 */
		stat = XGrabPointer(dpy, Scr->Root, False,
		    ButtonPressMask | ButtonReleaseMask |
		    PointerMotionMask | PointerMotionHintMask,
		    GrabModeAsync, GrabModeAsync,
		    Scr->Root, UpperLeftCursor, CurrentTime);

		if (stat == GrabSuccess)
		    break;
	    }

	    width = (SIZE_HINDENT + MyFont_TextWidth (&Scr->SizeFont,
						tmp_win->name, namelen));
#ifdef TWM_USE_SPACING
	    /* append half-point width: */
	    width += MyFont_TextWidth (&Scr->SizeFont, ".", 1) / 2;
#endif
	    height = Scr->SizeFont.height + SIZE_VINDENT * 2;

#ifdef TILED_SCREEN
	    if (Scr->use_panels == TRUE) {
		int k = FindNearestPanelToMouse();
		XMoveWindow (dpy, Scr->SizeWindow.win, Lft(Scr->panels[k]), Bot(Scr->panels[k]));
	    }
#endif
	    XResizeWindow (dpy, Scr->SizeWindow.win, width + SIZE_HINDENT, height);
	    XMapRaised(dpy, Scr->SizeWindow.win);
	    InstallRootColormap();

	    MyFont_DrawImageString (&Scr->SizeWindow, &Scr->SizeFont,
				    &Scr->DefaultC,
				    SIZE_HINDENT,
				    SIZE_VINDENT + Scr->SizeFont.ascent,
				    tmp_win->name, namelen);

	    AddingW = tmp_win->attr.width + bw2;
	    AddingH = tmp_win->attr.height + tmp_win->title_height + bw2;
  
  	    if (Scr->DontMoveOff) {
  		/*
  		 * Make sure the initial outline comes up on the screen.  
  		 */
		EnsureRectangleOnScreen (&AddingX, &AddingY, AddingW, AddingH);
  	    }

	    MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
	      	        tmp_win->frame_bw, tmp_win->title_height);

	    while (TRUE)
		{
		XMaskEvent(dpy, ButtonPressMask | PointerMotionMask, &event);

		if (Event.type == MotionNotify) {
		    /* discard any extra motion events before a release */
		    while(XCheckMaskEvent(dpy,
			ButtonMotionMask | ButtonPressMask, &Event))
			if (Event.type == ButtonPress)
			    break;
		}
		
		if (event.type == ButtonPress) {
		  AddingX = event.xbutton.x_root;
		  AddingY = event.xbutton.y_root;
		  
		  /* DontMoveOff prohibits user form off-screen placement */
		  if (Scr->DontMoveOff)
			EnsureRectangleOnScreen (&AddingX, &AddingY, AddingW, AddingH);
		  break;
		}

		if (event.type != MotionNotify) {
		    continue;
	    }

		XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &AddingX, &AddingY, &JunkMask);

		if (Scr->DontMoveOff)
		    EnsureRectangleOnScreen (&AddingX, &AddingY, AddingW, AddingH);

		MoveOutline(Scr->Root, AddingX, AddingY, AddingW, AddingH,
			    tmp_win->frame_bw, tmp_win->title_height);

	    }

	    if (event.xbutton.button == Button2) {
		int lastx, lasty;

		Scr->SizeStringOffset = width +
		  MyFont_TextWidth(&Scr->SizeFont, ": ", 2);
		XResizeWindow (dpy, Scr->SizeWindow.win, Scr->SizeStringOffset +
			       Scr->SizeStringWidth, height);
		MyFont_DrawString (&Scr->SizeWindow, &Scr->SizeFont,
				  &Scr->DefaultC, width,
				  SIZE_VINDENT + Scr->SizeFont.ascent,
				  ": ", 2);
		if (0/*Scr->AutoRelativeResize*/) {
		    int dx = (tmp_win->attr.width / 4);
		    int dy = (tmp_win->attr.height / 4);
		    
#define HALF_AVE_CURSOR_SIZE 8		/* so that it is visible */
		    if (dx < HALF_AVE_CURSOR_SIZE) dx = HALF_AVE_CURSOR_SIZE;
		    if (dy < HALF_AVE_CURSOR_SIZE) dy = HALF_AVE_CURSOR_SIZE;
#undef HALF_AVE_CURSOR_SIZE
		    dx += (tmp_win->frame_bw + 1);
		    dy += (bw2 + tmp_win->title_height + 1);
		    if (AddingX + dx >= Scr->MyDisplayWidth)
		      dx = Scr->MyDisplayWidth - AddingX - 1;
		    if (AddingY + dy >= Scr->MyDisplayHeight)
		      dy = Scr->MyDisplayHeight - AddingY - 1;
		    if (dx > 0 && dy > 0)
		      XWarpPointer (dpy, None, None, 0, 0, 0, 0, dx, dy);
		} else {
		    XWarpPointer (dpy, None, Scr->Root, 0, 0, 0, 0,
				  AddingX + AddingW/2, AddingY + AddingH/2);
		}
		AddStartResize(tmp_win, AddingX, AddingY, AddingW, AddingH);

		lastx = -10000;
		lasty = -10000;
		while (TRUE)
		{
		    XMaskEvent(dpy,
			       ButtonReleaseMask | ButtonMotionMask, &event);

		    if (Event.type == MotionNotify) {
			/* discard any extra motion events before a release */
			while(XCheckMaskEvent(dpy,
			    ButtonMotionMask | ButtonReleaseMask, &Event))
			    if (Event.type == ButtonRelease)
				break;
		    }

		    if (event.type == ButtonRelease)
		    {
			AddEndResize(tmp_win);
			break;
		    }

		    if (event.type != MotionNotify) {
			continue;
		    }

		    /*
		     * XXX - if we are going to do a loop, we ought to consider
		     * using multiple GXxor lines so that we don't need to 
		     * grab the server.
		     */
		    XQueryPointer(dpy, Scr->Root, &JunkRoot, &JunkChild,
			&JunkX, &JunkY, &AddingX, &AddingY, &JunkMask);

		    if (lastx != AddingX || lasty != AddingY)
		    {
			DoResize(AddingX, AddingY, tmp_win);

			lastx = AddingX;
			lasty = AddingY;
		    }

		}
	    } 
	    else if (event.xbutton.button == Button3)
	    {
		int maxw, maxh;

#ifdef TILED_SCREEN
		if (Scr->use_panels == TRUE) {
		    int Area[4];
		    Lft(Area) = AddingX;
		    Rht(Area) = AddingX + AddingW - 1;
		    Bot(Area) = AddingY;
		    Top(Area) = AddingY + AddingH - 1;
		    PanelsFullZoom (Area);
		    maxw = Rht(Area)+1 - AddingX;
		    maxh = Top(Area)+1 - AddingY;
		}
		else
#endif
		{
		    maxw = Scr->MyDisplayWidth  - AddingX;
		    maxh = Scr->MyDisplayHeight - AddingY;
		}

		/*
		 * Make window go to bottom of screen, and clip to right edge.
		 * This is useful when popping up large windows and fixed
		 * column text windows.
		 */
		if (AddingW > maxw) AddingW = maxw;
		AddingH = maxh;

		AddingW -= bw2;
		AddingH -= bw2;
		ConstrainSize (tmp_win, &AddingW, &AddingH);  /* w/o borders */
		AddingW += bw2;
		AddingH += bw2;
	    }
	    else
	    {
		XMaskEvent(dpy, ButtonReleaseMask, &event);
	    }

	    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
	    XUnmapWindow(dpy, Scr->SizeWindow.win);
	    UninstallRootColormap();
	    XUngrabPointer(dpy, CurrentTime);

	    tmp_win->attr.x = AddingX;
	    tmp_win->attr.y = AddingY + tmp_win->title_height;
	    tmp_win->attr.width = AddingW - bw2;
	    tmp_win->attr.height = AddingH - tmp_win->title_height - bw2;

	    XUngrabServer(dpy);
	}
      }
    } else {				/* put it where asked, mod title bar */
	if (tmp_win->transient)
	{
	    /* some toolkits put transients beyond screen: */
#ifdef TILED_SCREEN
	    if (Scr->use_panels == TRUE)
	    {
		TwmWindow *tmp;
		int k = FindNearestPanelToMouse();

		/* look for "parent" panel: */
		for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
		    if (tmp != tmp_win && tmp->w == tmp_win->transientfor)
			break;

		if (tmp != NULL) {
		    if ((Distance1D(tmp_win->attr.x, tmp_win->attr.x+tmp_win->attr.width-1,
				Lft(Scr->panels[k]), Rht(Scr->panels[k])) < 0)
			|| (Distance1D(tmp_win->attr.y, tmp_win->attr.y+tmp_win->attr.height-1,
				Bot(Scr->panels[k]), Top(Scr->panels[k])) < 0))
			k = FindNearestPanelToClient (tmp);
		}

		EnsureRectangleOnPanel (k, &tmp_win->attr.x, &tmp_win->attr.y,
			tmp_win->attr.width + 2*tmp_win->frame_bw,
			tmp_win->attr.height + 2*tmp_win->frame_bw);
	    }
	    else
#endif
		EnsureRectangleOnScreen (&tmp_win->attr.x, &tmp_win->attr.y,
			tmp_win->attr.width + 2*tmp_win->frame_bw,
			tmp_win->attr.height + 2*tmp_win->frame_bw);
	}
	else
	{
#ifdef TILED_SCREEN
	    if (Scr->use_panels == TRUE && tmp_win->iconmgr == FALSE) {
		/*
		 * Iconmanagers exact placing is ensured in CreateIconManagers().
		 *
		 * Currently we can't put a client narrower than 2*Scr->TitleHeight
		 * exactly centered onto two panels boundary (horizontally or vertically).
		 * Because of possible dead areas one cannot use neither the screen size
		 * nor the panelled area bounding box in the test below.
		 */
		int k, a[4];
		Lft(a) = tmp_win->attr.x;
		Rht(a) = tmp_win->attr.x + tmp_win->attr.width - 1;
		Bot(a) = tmp_win->attr.y;
		Top(a) = tmp_win->attr.y + tmp_win->attr.height - 1;
		k = FindNearestPanelToArea (a);
		if (tmp_win->attr.x > Rht(Scr->panels[k])+1 - Scr->TitleHeight)
		    tmp_win->attr.x = Rht(Scr->panels[k])+1 - Scr->TitleHeight;
		if (tmp_win->attr.x + tmp_win->attr.width < Lft(Scr->panels[k]) + Scr->TitleHeight)
		    tmp_win->attr.x = Lft(Scr->panels[k]) + Scr->TitleHeight - tmp_win->attr.width;
		if (tmp_win->attr.y > Top(Scr->panels[k])+1 - Scr->TitleHeight)
		    tmp_win->attr.y = Top(Scr->panels[k])+1 - Scr->TitleHeight;
		if (tmp_win->attr.y + tmp_win->attr.height < Bot(Scr->panels[k]) + Scr->TitleHeight)
		    tmp_win->attr.y = Bot(Scr->panels[k]) + Scr->TitleHeight - tmp_win->attr.height;
	    }
	    else
#endif
	    {
		/* one "average" cursor width/height: */
		if (tmp_win->attr.x > Scr->MyDisplayWidth - Scr->TitleHeight)
		    tmp_win->attr.x = Scr->MyDisplayWidth - Scr->TitleHeight;
		if (tmp_win->attr.x + tmp_win->attr.width < Scr->TitleHeight)
		    tmp_win->attr.x = Scr->TitleHeight - tmp_win->attr.width;
		if (tmp_win->attr.y > Scr->MyDisplayHeight - Scr->TitleHeight)
		    tmp_win->attr.y = Scr->MyDisplayHeight - Scr->TitleHeight;
		if (tmp_win->attr.y + tmp_win->attr.height < Scr->TitleHeight)
		    tmp_win->attr.y = Scr->TitleHeight - tmp_win->attr.height;
	    }
	}

	/* if the gravity is towards the top, move it by the title height */
	if (gravy < 0) tmp_win->attr.y -= gravy * tmp_win->title_height;
    }


#ifdef DEBUG
	fprintf(stderr, "  position window  %d, %d  %dx%d\n", 
	    tmp_win->attr.x,
	    tmp_win->attr.y,
	    tmp_win->attr.width,
	    tmp_win->attr.height);
#endif

    if (!tmp_win->ClientBorderWidth) {	/* need to adjust for twm borders */
	int delta = tmp_win->attr.border_width - tmp_win->frame_bw;
	tmp_win->attr.x += gravx * delta;
	tmp_win->attr.y += gravy * delta;
    }

    tmp_win->title_width = tmp_win->attr.width;

    if (tmp_win->old_bw) XSetWindowBorderWidth (dpy, tmp_win->w, 0);

    tmp_win->name_width = MyFont_TextWidth(&Scr->TitleBarFont, tmp_win->name,
					    namelen);

    tmp_win->iconified = FALSE;
    tmp_win->icon = FALSE;
    tmp_win->icon_on = FALSE;

    if (HandlingEvents == TRUE) {
	XGrabServer (dpy); /* on twm startup the server is grabbed in twm.c */
	XSync (dpy, False);
    }

    /*
     * Make sure the client window still exists.  We don't want to leave an
     * orphan frame window if it doesn't.  Since we now have the server 
     * grabbed, the window can't disappear later without having been 
     * reparented, so we'll get a DestroyNotify for it.  We won't have 
     * gotten one for anything up to here, however.
     */
    if (XGetGeometry(dpy, tmp_win->w, &JunkRoot, &JunkX, &JunkY,
		     &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth) == 0)
    {
	/*
	 * Allocated are full_name, name, icon_name, wmhints, res_name/-class, cwins.
	 * (See also HandleDestroyNotify().)
	 */
	free_window_names (tmp_win, True, True, True);		/* 1, 2, 3 */
	if (tmp_win->wmhints)					/* 4 */
	    XFree ((char *)tmp_win->wmhints);
	if (tmp_win->class.res_name && tmp_win->class.res_name != NoName)  /* 5 */
	    XFree ((char *)tmp_win->class.res_name);
	if (tmp_win->class.res_class && tmp_win->class.res_class != NoName) /* 6 */
	    XFree ((char *)tmp_win->class.res_class);
	free_cwins (tmp_win);				/* 9 */
	free((char *)tmp_win);
	if (HandlingEvents == TRUE)
	    XUngrabServer(dpy);
	return(NULL);
    }

    if (Scr->iconmgr.reversed == TRUE) {
	/* prepend as first element into the twm list: */
	tmp_win->next = Scr->TwmRoot.next;
	if (Scr->TwmRoot.next != NULL)
	    Scr->TwmRoot.next->prev = tmp_win;
	tmp_win->prev = &Scr->TwmRoot;
	Scr->TwmRoot.next = tmp_win;
    } else {
	/* append as last element into the twm list: */
	tmp_win->next = NULL;
	tmp_win->prev = &Scr->TwmRoot;
	ptmp_win = &Scr->TwmRoot.next;
	while (*ptmp_win) {
	    tmp_win->prev = (*ptmp_win);
	    ptmp_win = &((*ptmp_win)->next);
	}
	(*ptmp_win) = tmp_win;
    }

    /* WindowRing has precedence over NoWindowRing: */
    if (tmp_win->iconmgr == FALSE
	&& (LookInList(Scr->WindowRingL, tmp_win->full_name, &tmp_win->class) != NULL
	    || (Scr->WindowRingL == NULL && Scr->NoWindowRingL != NULL
		&& LookInList(Scr->NoWindowRingL, tmp_win->full_name, &tmp_win->class) == NULL)))
    {
	if (Scr->Ring) {
	    if (Scr->iconmgr.reversed == TRUE) {
		/* prepend as first element into the ring list: */
		tmp_win->ring.next = Scr->Ring->ring.next;
		if (Scr->Ring->ring.next->ring.prev) /*why 'if'? list is cyclic!*/
		    Scr->Ring->ring.next->ring.prev = tmp_win;
		Scr->Ring->ring.next = tmp_win;
		tmp_win->ring.prev = Scr->Ring;
	    } else {
		/* append as last element into the ring list: */
		tmp_win->ring.next = Scr->Ring;
		tmp_win->ring.prev = Scr->Ring->ring.prev;
		Scr->Ring->ring.prev->ring.next = tmp_win;
		Scr->Ring->ring.prev = tmp_win;
	    }
	} else {
	    tmp_win->ring.next = tmp_win->ring.prev = Scr->Ring = tmp_win;
	}
    } else
      tmp_win->ring.next = tmp_win->ring.prev = NULL;

    tmp_win->ring.curs_x = tmp_win->ring.curs_y = -1; /* set in SetupFrame() */

    /* get all the colors for the window */

    tmp_win->BorderColor = Scr->BorderColor;
    tmp_win->IconBorderColor = Scr->IconBorderColor;
    tmp_win->BorderTileC = Scr->BorderTileC;
    tmp_win->TitleC = Scr->TitleC;
    tmp_win->IconC = Scr->IconC;
    if (Scr->IconBitmapColor == ((Pixel)(~0)))
	tmp_win->IconBitmapColor = Scr->IconC.fore;
    else
	tmp_win->IconBitmapColor = Scr->IconBitmapColor;

    GetColorFromList(Scr->BorderColorL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->BorderColor);
    GetColorFromList(Scr->IconBorderColorL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->IconBorderColor);
    GetColorFromList(Scr->IconBitmapColorL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->IconBitmapColor);
    GetColorFromList(Scr->BorderTileForegroundL, tmp_win->full_name,
	&tmp_win->class, &tmp_win->BorderTileC.fore);
    GetColorFromList(Scr->BorderTileBackgroundL, tmp_win->full_name,
	&tmp_win->class, &tmp_win->BorderTileC.back);
    GetColorFromList(Scr->TitleForegroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->TitleC.fore);
    GetColorFromList(Scr->TitleBackgroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->TitleC.back);
    GetColorFromList(Scr->IconForegroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->IconC.fore);
    GetColorFromList(Scr->IconBackgroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->IconC.back);

    if (Scr->TitleButtonC.fore == ((Pixel)(~0))) /* UNUSED_PIXEL */
	tmp_win->TitleButtonC.fore = tmp_win->TitleC.fore;
    else
	tmp_win->TitleButtonC.fore = Scr->TitleButtonC.fore;
    GetColorFromList(Scr->TitleButtonForegroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->TitleButtonC.fore);
    if (Scr->TitleButtonC.back == ((Pixel)(~0)))
	tmp_win->TitleButtonC.back = tmp_win->TitleC.back;
    else
	tmp_win->TitleButtonC.back = Scr->TitleButtonC.back;
    GetColorFromList(Scr->TitleButtonBackgroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->TitleButtonC.back);
    if (Scr->TitleHighlightC.fore == ((Pixel)(~0)))
	tmp_win->TitleHighlightC.fore = tmp_win->TitleC.fore;
    else
	tmp_win->TitleHighlightC.fore = Scr->TitleHighlightC.fore;
    GetColorFromList(Scr->TitleHighlightForegroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->TitleHighlightC.fore);
    if (Scr->TitleHighlightC.back == ((Pixel)(~0)))
	tmp_win->TitleHighlightC.back = tmp_win->TitleC.back;
    else
	tmp_win->TitleHighlightC.back = Scr->TitleHighlightC.back;
    GetColorFromList(Scr->TitleHighlightBackgroundL, tmp_win->full_name, &tmp_win->class,
	&tmp_win->TitleHighlightC.back);

#ifdef TWM_USE_XFT
    if (Scr->use_xft > 0) {
	CopyPixelToXftColor (tmp_win->TitleC.fore, &tmp_win->TitleC.xft);
	CopyPixelToXftColor (tmp_win->IconC.fore, &tmp_win->IconC.xft);
    }
#endif
#ifdef TWM_USE_RENDER
    tmp_win->PicIconB == None;
    tmp_win->PenIconB == None;
#endif

    /* create windows */

    tmp_win->frame_x = tmp_win->attr.x + tmp_win->old_bw - tmp_win->frame_bw;
    tmp_win->frame_y = tmp_win->attr.y - tmp_win->title_height +
	tmp_win->old_bw - tmp_win->frame_bw;
    tmp_win->frame_width = tmp_win->attr.width;
    tmp_win->frame_height = tmp_win->attr.height + tmp_win->title_height;

    valuemask = CWColormap | CWBackPixmap | CWBorderPixel | CWCursor | CWEventMask;
    attributes.colormap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
    attributes.background_pixmap = None; /* ensure transparency if client has no background */
    attributes.border_pixel = tmp_win->BorderColor;
    attributes.cursor = Scr->FrameCursor;
#if 1
    if (tmp_win->iconmgr == TRUE)
	/* iconmgr special treatment: speedup: skip Enter/Leave */
	attributes.event_mask = SubstructureRedirectMask;
    else
#endif
	attributes.event_mask = (SubstructureRedirectMask |
			     ButtonPressMask | ButtonReleaseMask |
			     EnterWindowMask | LeaveWindowMask | FocusChangeMask);
    if (tmp_win->attr.save_under) {
	attributes.save_under = True;
	valuemask |= CWSaveUnder;
    }

    tmp_win->frame = XCreateWindow (dpy, Scr->Root, tmp_win->frame_x,
				    tmp_win->frame_y, 
				    (unsigned int) tmp_win->frame_width,
				    (unsigned int) tmp_win->frame_height,
				    (unsigned int) tmp_win->frame_bw,
				    Scr->d_depth,
				    (unsigned int) CopyFromParent,
				    Scr->d_visual, valuemask, &attributes);

    if (attributes.background_pixmap != None && attributes.background_pixmap != CopyFromParent)
	XFreePixmap (dpy, attributes.background_pixmap);

    if (tmp_win->title_height)
    {
	valuemask = (CWEventMask | CWBorderPixel | CWBackPixel);
	attributes.event_mask = (KeyPressMask | ButtonPressMask |
				 ButtonReleaseMask | ExposureMask);
	attributes.border_pixel = tmp_win->BorderColor;
	attributes.background_pixel = tmp_win->TitleC.back;
	tmp_win->title_w.win = XCreateWindow (dpy, tmp_win->frame,
					  -tmp_win->frame_bw,
					  -tmp_win->frame_bw,
					  (unsigned int) tmp_win->attr.width, 
					  (unsigned int) Scr->TitleHeight,
					  (unsigned int) tmp_win->frame_bw,
					  Scr->d_depth,
					  (unsigned int) CopyFromParent,
					  Scr->d_visual, valuemask,
					  &attributes);
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    tmp_win->title_w.xft = MyXftDrawCreate (tmp_win->title_w.win);
#endif
    }
    else {
	tmp_win->title_w.win = 0;
	tmp_win->squeeze_info = NULL;
    }

    if (tmp_win->highlight)
    {
	if (Scr->borderPm == None)
	    tmp_win->gray = XCreatePixmapFromBitmapData (dpy, Scr->Root,
		gray_bits, gray_width, gray_height, 
		tmp_win->BorderTileC.fore, tmp_win->BorderTileC.back,
		Scr->d_depth);
	else {
	    tmp_win->gray = XCreatePixmap (dpy, Scr->Root,
			    Scr->border_pm_width, Scr->border_pm_height,
			    Scr->d_depth);
	    if (tmp_win->gray != None) {
		FB(Scr, tmp_win->BorderTileC.fore, tmp_win->BorderTileC.back);
		XCopyPlane (dpy, Scr->borderPm, tmp_win->gray, Scr->NormalGC,
			    0, 0, Scr->border_pm_width, Scr->border_pm_height,
			    0, 0, 1);
	    }
	}
	SetBorder (tmp_win, False);
    }
    else
	tmp_win->gray = None;

	
    if (tmp_win->title_w.win) {
	CreateWindowTitlebarButtons (tmp_win);
	ComputeTitleLocation (tmp_win);
	XMoveWindow (dpy, tmp_win->title_w.win,
		     tmp_win->title_x, tmp_win->title_y);
	XDefineCursor(dpy, tmp_win->title_w.win, Scr->TitleCursor);
    }

    valuemask = (CWEventMask | CWDontPropagate);
#if 1
    if (tmp_win->iconmgr == TRUE)
	/* iconmgr special treatment: speedup: skip Enter/Leave */
	attributes.event_mask = StructureNotifyMask;
    else
#endif
	attributes.event_mask = (StructureNotifyMask | PropertyChangeMask |
			     ColormapChangeMask | VisibilityChangeMask |
			     EnterWindowMask | LeaveWindowMask | FocusChangeMask);
    attributes.do_not_propagate_mask = ButtonPressMask | ButtonReleaseMask;
    XChangeWindowAttributes (dpy, tmp_win->w, valuemask, &attributes);

    if (tmp_win->title_w.win) {
	XMapWindow (dpy, tmp_win->title_w.win);
    }

    if (HasShape) {
	int xws, yws, xbs, ybs;
	unsigned wws, hws, wbs, hbs;
	int boundingShaped, clipShaped;

	XShapeSelectInput (dpy, tmp_win->w, ShapeNotifyMask);
	XShapeQueryExtents (dpy, tmp_win->w,
			    &boundingShaped, &xws, &yws, &wws, &hws,
			    &clipShaped, &xbs, &ybs, &wbs, &hbs);
	tmp_win->wShaped = boundingShaped;
    }

    if (!tmp_win->iconmgr)
	XAddToSaveSet(dpy, tmp_win->w);
	
    XReparentWindow(dpy, tmp_win->w, tmp_win->frame, 0, tmp_win->title_height);
    /*
     * Reparenting generates an UnmapNotify event, followed by a MapNotify.
     * Set the map state to FALSE to prevent a transition back to
     * WithdrawnState in HandleUnmapNotify.  Map state gets set correctly
     * again in HandleMapNotify.
     */
    tmp_win->mapped = FALSE;

    SetupFrame (tmp_win, tmp_win->frame_x, tmp_win->frame_y,
		tmp_win->frame_width, tmp_win->frame_height, -1, True);
#if 1
    /* ensure F_PANELGEOMETRYZOOM/F_PANELGEOMETRYMOVE have sane values */
    tmp_win->save_frame_height = tmp_win->frame_width;
    tmp_win->save_frame_width = tmp_win->frame_height;
    tmp_win->save_frame_x = tmp_win->frame_x;
    tmp_win->save_frame_y = tmp_win->frame_y;
    tmp_win->zoomed = ZOOM_NONE;
#endif
    /* wait until the window is iconified and the icon window is mapped
     * before creating the icon window 
     */
    tmp_win->icon_w.win = (Window) 0;

    if (tmp_win->iconmgr) /* Tricky: we are our own icon window! (We override this in icon.c) */
	SetIconWindowHintForFrame (tmp_win, tmp_win->frame);

    if (tmp_win->wmhints && (tmp_win->wmhints->flags & WindowGroupHint)) /*propagate window group*/
	SetWindowGroupHintForFrame (tmp_win, tmp_win->wmhints->window_group);

    if (tmp_win->transient == TRUE) { /*propagate transient-for hint*/
	TwmWindow *tmp;
	for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
	    if (tmp_win->transientfor == tmp->w) {
		XSetTransientForHint (dpy, tmp_win->frame, tmp->frame);
		break;
	    }
    }

    if (XGetWMName (dpy, tmp_win->w, &text_prop)) { /*propagate window name*/
	XSetWMName (dpy, tmp_win->frame, &text_prop);
	if (text_prop.value)
	    XFree (text_prop.value);
    } else
	XStoreName (dpy, tmp_win->frame, tmp_win->full_name);

    if (!tmp_win->iconmgr)
    {
	GrabButtons(tmp_win);
	GrabKeys(tmp_win);
    }

    (void) AddIconManager(tmp_win);

    XSaveContext(dpy, tmp_win->w, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp_win->w, ScreenContext, (caddr_t) Scr);
    XSaveContext(dpy, tmp_win->frame, TwmContext, (caddr_t) tmp_win);
    XSaveContext(dpy, tmp_win->frame, ScreenContext, (caddr_t) Scr);
    if (tmp_win->title_height)
    {
	int i;
	int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;

	XSaveContext(dpy, tmp_win->title_w.win, TwmContext, (caddr_t) tmp_win);
	XSaveContext(dpy, tmp_win->title_w.win, ScreenContext, (caddr_t) Scr);
	for (i = 0; i < nb; i++) {
	    XSaveContext(dpy, tmp_win->titlebuttons[i].window, TwmContext,
			 (caddr_t) tmp_win);
	    XSaveContext(dpy, tmp_win->titlebuttons[i].window, ScreenContext,
			 (caddr_t) Scr);
	}
	if (tmp_win->hilite_w)
	{
	    XSaveContext(dpy, tmp_win->hilite_w, TwmContext, (caddr_t)tmp_win);
	    XSaveContext(dpy, tmp_win->hilite_w, ScreenContext, (caddr_t)Scr);
	}
    }

    if (HandlingEvents == TRUE)
	XUngrabServer (dpy); /* on twm startup the server is ungrabbed in twm.c */

    /* if we were in the middle of a menu activated function, regrab
     * the pointer 
     */
    if (RootFunction)
	ReGrab();

    return (tmp_win);
}


/**
 * checks to see if we should really put a twm frame on the window
 *
 *  \return TRUE  - go ahead and place the window
 *  \return FALSE - don't frame the window
 *	\param w the window to check
 */
int
MappedNotOverride(Window w)
{
    XWindowAttributes wa;

    XGetWindowAttributes(dpy, w, &wa);
    return ((wa.map_state != IsUnmapped) && (wa.override_redirect != True));
}


/**
 * attach default bindings so that naive users don't get messed up if they 
 * provide a minimal twmrc.
 */
static void do_add_binding (int button, int context, int modifier, int func)
{
    MouseButton *mb = &Scr->Mouse[button][context][modifier];

    if (mb->func) return;		/* already defined */

    mb->func = func;
    mb->item = NULL;
}

void
AddDefaultBindings ()
{
    /*
     * The bindings are stored in Scr->Mouse, indexed by
     * Mouse[button_number][C_context][modifier].
     */

#define NoModifierMask 0

    do_add_binding (Button1, C_TITLE, NoModifierMask, F_MOVE);
    do_add_binding (Button1, C_ICON, NoModifierMask, F_ICONIFY);
    do_add_binding (Button1, C_ICONMGR, NoModifierMask, F_ICONIFY);

    do_add_binding (Button2, C_TITLE, NoModifierMask, F_RAISELOWER);
    do_add_binding (Button2, C_ICON, NoModifierMask, F_ICONIFY);
    do_add_binding (Button2, C_ICONMGR, NoModifierMask, F_ICONIFY);

#undef NoModifierMask
}




/**
 * grab needed buttons for the window
 *
 *  \param[in] tmp_win the twm window structure to use
 */
void
GrabButtons(TwmWindow *tmp_win)
{
    int i, j;

    for (i = 0; i < MAX_BUTTONS+1; i++)
    {
	for (j = 0; j < MOD_SIZE; j++)
	{
	    if (Scr->Mouse[i][C_WINDOW][j].func != 0)
	    {
	        /* twm used to do this grab on the application main window,
                 * tmp_win->w . This was not ICCCM complient and was changed.
		 */
		XGrabButton(dpy, i, j, tmp_win->frame, 
			    True, ButtonPressMask | ButtonReleaseMask,
			    GrabModeAsync, GrabModeAsync, None, 
			    Scr->FrameCursor);
	    }
	}
    }
}

/**
 * grab needed keys for the window
 *
 *  \param[in] tmp_win the twm window structure to use
 */
void
GrabKeys(TwmWindow *tmp_win)
{
    FuncKey *tmp;
    IconMgr *p;

    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	switch (tmp->cont)
	{
	case C_WINDOW:
	    XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->w, True,
		GrabModeAsync, GrabModeAsync);
	    break;

	case C_ICON:
	    if (tmp_win->icon_w.win)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->icon_w.win, True,
		    GrabModeAsync, GrabModeAsync);

	case C_TITLE:
	    if (tmp_win->title_w.win)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->title_w.win, True,
		    GrabModeAsync, GrabModeAsync);
	    break;

	case C_NAME:
	    XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->w, True,
		GrabModeAsync, GrabModeAsync);
	    if (tmp_win->icon_w.win)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->icon_w.win, True,
		    GrabModeAsync, GrabModeAsync);
	    if (tmp_win->title_w.win)
		XGrabKey(dpy, tmp->keycode, tmp->mods, tmp_win->title_w.win, True,
		    GrabModeAsync, GrabModeAsync);
	    break;
	/*
	case C_ROOT:
	    XGrabKey(dpy, tmp->keycode, tmp->mods, Scr->Root, True,
		GrabModeAsync, GrabModeAsync);
	    break;
	*/
	}
    }
    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	if (tmp->cont == C_ICONMGR && !Scr->NoIconManagers)
	{
	    for (p = &Scr->iconmgr; p != NULL; p = p->next)
	    {
		XUngrabKey(dpy, tmp->keycode, tmp->mods, p->twm_win->w);
	    }
	}
    }
}

static Window CreateHighlightWindow (TwmWindow *tmp_win)
{
    XSetWindowAttributes attributes;	/* attributes for create windows */
    Pixmap pm = None;
    unsigned long valuemask;
    int h = (Scr->TitleHeight - 2 * Scr->FramePadding);
    Window w;


    /*
     * If a special highlight pixmap was given, use that.  Otherwise,
     * use a nice, even gray pattern.  The old horizontal lines look really
     * awful on interlaced monitors (as well as resembling other looks a
     * little bit too closely), but can be used by putting
     *
     *                 Pixmaps { TitleHighlight "hline2" }
     *
     * (or whatever the horizontal line bitmap is named) in the startup
     * file.  If all else fails, use the foreground color to look like a 
     * solid line.
     */
    if (!Scr->hilitePm) {
	Scr->hilitePm = XCreateBitmapFromData (dpy, tmp_win->title_w.win,
					       gray_bits, gray_width,
					       gray_height);
	Scr->hilite_pm_width = gray_width;
	Scr->hilite_pm_height = gray_height;
    }
    if (Scr->hilitePm) {
	pm = XCreatePixmap (dpy, tmp_win->title_w.win,
			    Scr->hilite_pm_width, Scr->hilite_pm_height,
			    Scr->d_depth);
	FB(Scr, tmp_win->TitleHighlightC.fore, tmp_win->TitleHighlightC.back);
	XCopyPlane (dpy, Scr->hilitePm, pm, Scr->NormalGC,
		    0, 0, Scr->hilite_pm_width, Scr->hilite_pm_height,
		    0, 0, 1);
    }
    if (pm) {
	valuemask = CWBackPixmap;
	attributes.background_pixmap = pm;
    } else {
	valuemask = CWBackPixel;
	attributes.background_pixel = tmp_win->TitleC.fore;
    }

    w = XCreateWindow (dpy, tmp_win->title_w.win, 0, Scr->FramePadding,
		       (unsigned int) Scr->TBInfo.width, (unsigned int) h,
		       (unsigned int) 0,
		       Scr->d_depth, (unsigned int) CopyFromParent,
		       Scr->d_visual, valuemask, &attributes);
    if (pm) XFreePixmap (dpy, pm);
    return w;
}


void ComputeCommonTitleOffsets ()
{
    int buttonwidth = (Scr->TBInfo.width + Scr->TBInfo.pad);

    Scr->TBInfo.leftx = Scr->TBInfo.rightoff = Scr->FramePadding;
    if (Scr->TBInfo.nleft > 0)
      Scr->TBInfo.leftx += Scr->ButtonIndent;
    Scr->TBInfo.titlex = (Scr->TBInfo.leftx +
			  (Scr->TBInfo.nleft * buttonwidth) - Scr->TBInfo.pad +
			  Scr->TitlePadding);
    if (Scr->TBInfo.nright > 0)
      Scr->TBInfo.rightoff += (Scr->ButtonIndent +
			       ((Scr->TBInfo.nright * buttonwidth) -
				Scr->TBInfo.pad));
    return;
}

void ComputeWindowTitleOffsets (TwmWindow *tmp_win, int width, Bool squeeze)
{
    tmp_win->highlightx = (Scr->TBInfo.titlex + tmp_win->name_width);
    if (tmp_win->hilite_w || Scr->TBInfo.nright > 0) 
      tmp_win->highlightx += Scr->TitlePadding;
    tmp_win->rightx = width - Scr->TBInfo.rightoff;
    if (squeeze && tmp_win->squeeze_info) {
	int rx = (tmp_win->highlightx + 
		  (tmp_win->hilite_w
		    ? Scr->TBInfo.width * 2 : 0) +
		  (Scr->TBInfo.nright > 0 ? Scr->TitlePadding : 0) +
		  Scr->FramePadding);
	if (rx < tmp_win->rightx) tmp_win->rightx = rx;
    }
    return;
}


/**
 * calculate the position of the title window.  We need to take the frame_bw 
 * into account since we want (0,0) of the title window to line up with (0,0) 
 * of the frame window.
 */
void ComputeTitleLocation (register TwmWindow *tmp)
{
    tmp->title_x = -tmp->frame_bw;
    tmp->title_y = -tmp->frame_bw;

    if (tmp->squeeze_info) {
	register SqueezeInfo *si = tmp->squeeze_info;
	int basex;
	int maxwidth = tmp->frame_width;
	int tw = tmp->title_width;

	/*
	 * figure label base from squeeze info (justification fraction)
	 */
	if (si->denom == 0) {	/* num is pixel based */
	    if ((basex = si->num) == 0) {  /* look for special cases */
		switch (si->justify) {
		  case J_RIGHT:
		    basex = maxwidth;
		    break;
		  case J_CENTER:
		    basex = maxwidth / 2;
		break;
		}
	    }
	} else {			/* num/denom is fraction */
	    basex = ((si->num * maxwidth) / si->denom);
	    if (si->num < 0) basex += maxwidth;
	}

	/*
	 * adjust for left (nop), center, right justify and clip
	 */
	switch (si->justify) {
	  case J_CENTER:
	    basex -= tw / 2;
	    break;
	  case J_RIGHT:
	    basex -= tw - 1;
	    break;
	}
	if (basex > maxwidth - tw + 1)
	  basex = maxwidth - tw + 1;
	if (basex < 0) basex = 0;

	tmp->title_x = basex - tmp->frame_bw;
    }
}


static void CreateWindowTitlebarButtons (TwmWindow *tmp_win)
{
    unsigned long valuemask;		/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    int leftx, rightx, y;
    TitleButton *tb;
    int nb;

    if (tmp_win->title_height == 0)
    {
	tmp_win->hilite_w = 0;
	return;
    }


    /*
     * create the title bar windows; let the event handler deal with painting
     * so that we don't have to spend two pixmaps (or deal with hashing)
     */
    ComputeWindowTitleOffsets (tmp_win, tmp_win->attr.width, False);

    leftx = y = Scr->TBInfo.leftx;
    rightx = tmp_win->rightx;

    attributes.win_gravity = NorthWestGravity;
    attributes.border_pixel = tmp_win->TitleButtonC.fore;
    attributes.event_mask = (ButtonPressMask | ButtonReleaseMask);
    attributes.cursor = Scr->ButtonCursor;
    valuemask = (CWWinGravity | CWBackPixmap | CWBorderPixel | CWEventMask |
		 CWCursor);

    tmp_win->titlebuttons = NULL;
    nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
    if (nb > 0) {
	tmp_win->titlebuttons = (TBWindow *) malloc (nb * sizeof(TBWindow));
	if (!tmp_win->titlebuttons) {
	    fprintf (stderr, "%s:  unable to allocate %d titlebuttons\n", 
		     ProgramName, nb);
	} else {
	    Pixmap pm;
	    TBWindow *tbw;
	    int boxwidth = (Scr->TBInfo.width + Scr->TBInfo.pad);
	    unsigned int h = (Scr->TBInfo.width - Scr->TBInfo.border * 2);

	    for (tb = Scr->TBInfo.head, tbw = tmp_win->titlebuttons; tb;
		 tb = tb->next, tbw++) {
		int x;
		if (tb->rightside) {
		    x = rightx;
		    rightx += boxwidth;
		    attributes.win_gravity = NorthEastGravity;
		} else {
		    x = leftx;
		    leftx += boxwidth;
		    attributes.win_gravity = NorthWestGravity;
		}
		pm = XCreatePixmap (dpy, Scr->Root, h, h, Scr->d_depth);
		if (pm != None) {
		    /* X-server draws the bitmap (as background), no Expose: */
		    FB(Scr, tmp_win->TitleButtonC.back, tmp_win->TitleButtonC.fore);
		    XFillRectangle (dpy, pm, Scr->NormalGC, 0, 0, h, h);
		    FB(Scr, tmp_win->TitleButtonC.fore, tmp_win->TitleButtonC.back);
		    XCopyPlane (dpy, tb->bitmap, pm, Scr->NormalGC,
				tb->srcx, tb->srcy, tb->width, tb->height,
				tb->dstx, tb->dsty, 1);
		}
		attributes.background_pixmap = pm;
		tbw->window = XCreateWindow (dpy, tmp_win->title_w.win, x, y, h, h,
					     (unsigned int) Scr->TBInfo.border,
					     CopyFromParent, (unsigned int) CopyFromParent,
					     (Visual *) CopyFromParent,
					     valuemask, &attributes);
		if (pm != None)
		    XFreePixmap (dpy, pm);
		tbw->info = tb;
	    }
	}
    }

    tmp_win->hilite_w = (tmp_win->titlehighlight 
			 ? CreateHighlightWindow (tmp_win) : None);

    XMapSubwindows(dpy, tmp_win->title_w.win);
    if (tmp_win->hilite_w)
      XUnmapWindow(dpy, tmp_win->hilite_w);
    return;
}


void
SetHighlightPixmap (char *filename)
{
    Pixmap pm = GetBitmap (filename);

    if (pm) {
	if (Scr->hilitePm) {
	    XFreePixmap (dpy, Scr->hilitePm);
	}
	Scr->hilitePm = pm;
	Scr->hilite_pm_width = JunkWidth;
	Scr->hilite_pm_height = JunkHeight;
    }
}


void
SetBorderPixmap (char *filename)
{
    Pixmap pm = GetBitmap (filename);

    if (pm) {
	if (Scr->borderPm) {
	    XFreePixmap (dpy, Scr->borderPm);
	}
	Scr->borderPm = pm;
	Scr->border_pm_width = JunkWidth;
	Scr->border_pm_height = JunkHeight;
    }
}


void
FetchWmProtocols (TwmWindow *tmp)
{
    unsigned long flags = 0L;
    Atom *protocols = NULL;
    int n;

    if (XGetWMProtocols (dpy, tmp->w, &protocols, &n)) {
	register int i;
	register Atom *ap;

	for (i = 0, ap = protocols; i < n; i++, ap++) {
	    if (*ap == _XA_WM_TAKE_FOCUS) flags |= DoesWmTakeFocus;
	    if (*ap == _XA_WM_SAVE_YOURSELF) flags |= DoesWmSaveYourself;
	    if (*ap == _XA_WM_DELETE_WINDOW) flags |= DoesWmDeleteWindow;
	}
	if (protocols) XFree ((char *) protocols);
    }
    tmp->protocols = flags;
}

TwmColormap *
CreateTwmColormap(Colormap c)
{
    TwmColormap *cmap;
    cmap = (TwmColormap *) malloc(sizeof(TwmColormap));
    if (!cmap ||
	XSaveContext(dpy, c, ColormapContext, (caddr_t) cmap)) {
	if (cmap) free((char *) cmap);
	return (NULL);
    }
    cmap->c = c;
    cmap->state = 0;
    cmap->install_req = 0;
    cmap->w = None;
    cmap->refcnt = 1;
    return (cmap);
}

ColormapWindow *
CreateColormapWindow(Window w, Bool creating_parent, Bool property_window)
{
    XWindowAttributes attributes;
    if (XGetWindowAttributes(dpy, w, &attributes))
	return CreateColormapWindowAttr (w, &attributes, creating_parent, property_window);
    return NULL;
}

ColormapWindow *
CreateColormapWindowAttr(Window w, XWindowAttributes *attributes, Bool creating_parent, Bool property_window)
{
    ColormapWindow *cwin;
    TwmColormap *cmap;

    cwin = (ColormapWindow *) malloc(sizeof(ColormapWindow));
    if (cwin) {
	if (XSaveContext(dpy, w, ColormapContext, (caddr_t) cwin)) {
	    free((char *) cwin);
	    return (NULL);
	}

	if (XFindContext(dpy, attributes->colormap,  ColormapContext,
		(caddr_t *)&cwin->colormap) == XCNOENT) {
	    cwin->colormap = cmap = CreateTwmColormap(attributes->colormap);
	    if (!cmap) {
		XDeleteContext(dpy, w, ColormapContext);
		free((char *) cwin);
		return (NULL);
	    }
	} else {
	    cwin->colormap->refcnt++;
	}

	cwin->w = w;
	/*
	 * Assume that windows in colormap list are
	 * obscured if we are creating the parent window.
	 * Otherwise, we assume they are unobscured.
	 */
	cwin->visibility = creating_parent ?
	    VisibilityPartiallyObscured : VisibilityUnobscured;
	cwin->refcnt = 1;

	/*
	 * If this is a ColormapWindow property window and we
	 * are not monitoring ColormapNotify or VisibilityNotify
	 * events, we need to.
	 */
	if (property_window &&
	    (attributes->your_event_mask &
		(ColormapChangeMask|VisibilityChangeMask)) !=
		    (ColormapChangeMask|VisibilityChangeMask)) {
	    XSelectInput(dpy, w, attributes->your_event_mask |
		(ColormapChangeMask|VisibilityChangeMask));
	}
    }

    return (cwin);
}

void		
FetchWmColormapWindows (TwmWindow *tmp)
{
    register int i, j;
    Window *cmap_windows = NULL;
    Bool can_free_cmap_windows = False;
    int number_cmap_windows = 0;
    ColormapWindow **cwins = NULL;
    int previously_installed;

    number_cmap_windows = 0;

    if (/* SUPPRESS 560 */(previously_installed = 
       (Scr->cmapInfo.cmaps == &tmp->cmaps && tmp->cmaps.number_cwins))) {
	cwins = tmp->cmaps.cwins;
	for (i = 0; i < tmp->cmaps.number_cwins; i++)
	    cwins[i]->colormap->state = 0;
    }

    if (XGetWMColormapWindows (dpy, tmp->w, &cmap_windows, 
			       &number_cmap_windows) &&
	number_cmap_windows > 0) {

	can_free_cmap_windows = False;
	/*
	 * check if the top level is in the list, add to front if not
	 */
	for (i = 0; i < number_cmap_windows; i++) {
	    if (cmap_windows[i] == tmp->w) break;
	}
	if (i == number_cmap_windows) {	 /* not in list */
	    Window *new_cmap_windows =
	      (Window *) malloc (sizeof(Window) * (number_cmap_windows + 1));

	    if (!new_cmap_windows) {
		fprintf (stderr, 
			 "%s:  unable to allocate %d element colormap window array\n",
			ProgramName, number_cmap_windows+1);
		goto done;
	    }
	    new_cmap_windows[0] = tmp->w;  /* add to front */
	    for (i = 0; i < number_cmap_windows; i++) {	 /* append rest */
		new_cmap_windows[i+1] = cmap_windows[i];
	    }
	    XFree ((char *) cmap_windows);
	    can_free_cmap_windows = True;  /* do not use XFree any more */
	    cmap_windows = new_cmap_windows;
	    number_cmap_windows++;
	}

	cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *) *
		number_cmap_windows);
	if (cwins) {
	    for (i = 0; i < number_cmap_windows; i++) {

		/*
		 * Copy any existing entries into new list.
		 */
		for (j = 0; j < tmp->cmaps.number_cwins; j++) {
		    if (tmp->cmaps.cwins[j]->w == cmap_windows[i]) {
			cwins[i] = tmp->cmaps.cwins[j];
			cwins[i]->refcnt++;
			break;
		    }
		}

		/*
		 * If the colormap window is not being pointed by
		 * some other applications colormap window list,
		 * create a new entry.
		 */
		if (j == tmp->cmaps.number_cwins) {
		    if (XFindContext(dpy, cmap_windows[i], ColormapContext,
				     (caddr_t *)&cwins[i]) == XCNOENT) {
			if ((cwins[i] = CreateColormapWindow(cmap_windows[i],
				    (Bool) tmp->cmaps.number_cwins == 0,
				    True)) == NULL) {
			    int k;
			    for (k = i + 1; k < number_cmap_windows; k++)
				cmap_windows[k-1] = cmap_windows[k];
			    i--;
			    number_cmap_windows--;
			}
		    } else
			cwins[i]->refcnt++;
		}
	    }
	}
    }

    /* No else here, in case we bailed out of clause above.
     */
    if (number_cmap_windows == 0) {
	cwins = (ColormapWindow **) calloc(1, sizeof(ColormapWindow *));
	if (cwins) {
	    if (XFindContext(dpy, tmp->w, ColormapContext, (caddr_t *)&cwins[0]) ==
		    XCNOENT)
		cwins[0] = CreateColormapWindow(tmp->w,
				(Bool) tmp->cmaps.number_cwins == 0, False);
	    else
		cwins[0]->refcnt++;
	    if (cwins[0])
		number_cmap_windows = 1;
	    else {
		free (cwins);
		cwins = NULL;
	    }
	}
    }

    if (tmp->cmaps.number_cwins)
	free_cwins(tmp);

    tmp->cmaps.cwins = cwins;
    tmp->cmaps.number_cwins = number_cmap_windows;
    if (number_cmap_windows > 1)
	tmp->cmaps.scoreboard = 
	  (char *) calloc(1, ColormapsScoreboardLength(&tmp->cmaps));
		
    if (previously_installed)
	InstallWindowColormaps(PropertyNotify, (TwmWindow *) NULL);

  done:
    if (cmap_windows) {
	if (can_free_cmap_windows)
	  free ((char *) cmap_windows);
	else
	  XFree ((char *) cmap_windows);
    }

    return;
}


void GetWindowSizeHints (TwmWindow *tmp)
{
    long supplied = 0;

    if (!XGetWMNormalHints (dpy, tmp->w, &tmp->hints, &supplied))
      tmp->hints.flags = 0;

    if (tmp->hints.flags & PResizeInc) {
	if (tmp->hints.width_inc == 0) tmp->hints.width_inc = 1;
	if (tmp->hints.height_inc == 0) tmp->hints.height_inc = 1;
    }

    if (!(supplied & PWinGravity) && (tmp->hints.flags & USPosition)) {
	static int gravs[] = { SouthEastGravity, SouthWestGravity,
			       NorthEastGravity, NorthWestGravity };
	int right =  tmp->attr.x + tmp->attr.width + 2 * tmp->old_bw;
	int bottom = tmp->attr.y + tmp->attr.height + 2 * tmp->old_bw;
	tmp->hints.win_gravity = 
	  gravs[((Scr->MyDisplayHeight - bottom < tmp->title_height) ? 0 : 2) |
		((Scr->MyDisplayWidth - right   < tmp->title_height) ? 0 : 1)];
	tmp->hints.flags |= PWinGravity;
    }
}


static void SetWindowGroupHintForFrame (TwmWindow *tmp_win, XID window_group)
{
    XWMHints hints, *hintsp;
    hintsp = XGetWMHints (dpy, tmp_win->frame);
    if (hintsp == NULL) {
	hintsp = &hints;
	hintsp->flags = 0;
    }
    hintsp->window_group = window_group;
    hintsp->flags |= WindowGroupHint;
    XSetWMHints (dpy, tmp_win->frame, hintsp);
    if (hintsp != &hints)
	XFree (hintsp);
}
