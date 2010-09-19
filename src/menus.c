/* $XFree86: xc/programs/twm/menus.c,v 1.18tsi Exp $ */
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


/***********************************************************************
 *
 * $Xorg: menus.c,v 1.6 2001/02/09 02:05:36 xorgcvs Exp $
 *
 * twm menu code
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#include <stdio.h>
#include <string.h>
#include <X11/Xos.h>
#include "twm.h"
#include "gc.h"
#include "menus.h"
#include "resize.h"
#include "events.h"
#include "util.h"
#include "parse.h"
#include "gram.h"
#include "screen.h"
#include "menus.h"
#include "iconmgr.h"
#include "add_window.h"
#include "icons.h"
#include "session.h"
#include <X11/Xmu/CharSet.h>
#include "version.h"
#include <X11/extensions/sync.h>
#include <X11/SM/SMlib.h>

int RootFunction = 0;
MenuRoot *ActiveMenu = NULL;		/**< the active menu */
MenuItem *ActiveItem = NULL;		/**< the active menu item */
int MoveFunction;			/**< either F_MOVE or F_FORCEMOVE */
int WindowMoved = FALSE;
int menuFromFrameOrWindowOrTitlebar = FALSE;

int ConstMove = FALSE;		/**< constrained move variables */
int ConstMoveDir;
int ConstMoveX;
int ConstMoveY;
int ConstMoveXL;
int ConstMoveXR;
int ConstMoveYT;
int ConstMoveYB;
 
/* Globals used to keep track of whether the mouse has moved during
   a resize function. */
int ResizeOrigX;
int ResizeOrigY;

int MenuDepth = 0;		/**< number of menus up */
static struct {
    int x;
    int y;
} MenuOrigins[MAXMENUDEPTH];
static Cursor LastCursor;

static Bool belongs_to_twm_window ( TwmWindow *t, Window w );
static void send_clientmessage ( Window w, Atom a, Time timestamp );

static void HideIconManager ( IconMgr *ip );
static void ShowIconManager ( IconMgr *ip );

#define SHADOWWIDTH 5			/* in pixels */




/**
 * initialize menu roots
 */
void
InitMenus()
{
    int i, j, k;
    FuncKey *key, *tmp;

    for (i = 0; i < MAX_BUTTONS+1; i++)
	for (j = 0; j < NUM_CONTEXTS; j++)
	    for (k = 0; k < MOD_SIZE; k++)
	    {
		Scr->Mouse[i][j][k].func = 0;
		Scr->Mouse[i][j][k].item = NULL;
	    }

    Scr->DefaultFunction.func = 0;
    Scr->WindowFunction.func = 0;

    if (FirstScreen)
    {
	for (key = Scr->FuncKeyRoot.next; key != NULL;)
	{
	    free(key->name);
	    tmp = key;
	    key = key->next;
	    free((char *) tmp);
	}
	Scr->FuncKeyRoot.next = NULL;
    }

}



/**
 * add a function key to the list
 *
 *  \param name     the name of the key
 *  \param cont     the context to look for the key press in
 *  \param mods     modifier keys that need to be pressed
 *  \param func     the function to perform
 *  \param win_name the window name (if any)
 *  \param action   the action string associated with the function (if any)
 */
Bool AddFuncKey (char *name, int cont, int mods, int func, char *win_name, 
                 char *action)
{
    FuncKey *tmp;
    KeySym keysym;
    KeyCode keycode;

    /*
     * Don't let a 0 keycode go through, since that means AnyKey to the
     * XGrabKey call in GrabKeys().
     */
    if ((keysym = XStringToKeysym(name)) == NoSymbol ||
	(keycode = XKeysymToKeycode(dpy, keysym)) == 0)
    {
	return False;
    }

    /* see if there already is a key defined for this context */
    for (tmp = Scr->FuncKeyRoot.next; tmp != NULL; tmp = tmp->next)
    {
	if (tmp->keysym == keysym &&
	    tmp->cont == cont &&
	    tmp->mods == mods)
	    break;
    }

    if (tmp == NULL)
    {
	tmp = (FuncKey *) malloc(sizeof(FuncKey));
	tmp->next = Scr->FuncKeyRoot.next;
	Scr->FuncKeyRoot.next = tmp;
    }

    tmp->name = name;
    tmp->keysym = keysym;
    tmp->keycode = keycode;
    tmp->cont = cont;
    tmp->mods = mods;
    tmp->func = func;
    tmp->win_name = win_name;
    tmp->action = action;

    return True;
}



int CreateTitleButton (char *name, int func, char *action, MenuRoot *menuroot, 
                       Bool rightside, Bool append)
{
    TitleButton *tb = (TitleButton *) malloc (sizeof(TitleButton));

    if (!tb) {
	fprintf (stderr,
		 "%s:  unable to allocate %ld bytes for title button\n",
		 ProgramName, (unsigned long)sizeof(TitleButton));
	return 0;
    }

    tb->next = NULL;
    tb->name = name;			/* note that we are not copying */
    tb->bitmap = None;			/* WARNING, values not set yet */
    tb->width = 0;			/* see InitTitlebarButtons */
    tb->height = 0;			/* ditto */
    tb->func = func;
    tb->action = action;
    tb->menuroot = menuroot;
    tb->rightside = rightside;
    if (rightside) {
	Scr->TBInfo.nright++;
    } else {
	Scr->TBInfo.nleft++;
    }

    /*
     * Cases for list:
     * 
     *     1.  empty list, prepend left       put at head of list
     *     2.  append left, prepend right     put in between left and right
     *     3.  append right                   put at tail of list
     *
     * Do not refer to widths and heights yet since buttons not created
     * (since fonts not loaded and heights not known).
     */
    if ((!Scr->TBInfo.head) || ((!append) && (!rightside))) {	/* 1 */
	tb->next = Scr->TBInfo.head;
	Scr->TBInfo.head = tb;
    } else if (append && rightside) {	/* 3 */
	register TitleButton *t;
	for /* SUPPRESS 530 */
	  (t = Scr->TBInfo.head; t->next; t = t->next);
	t->next = tb;
	tb->next = NULL;
    } else {				/* 2 */
	register TitleButton *t, *prev = NULL;
	for (t = Scr->TBInfo.head; t && !t->rightside; t = t->next) {
	    prev = t;
	}
	if (prev) {
	    tb->next = prev->next;
	    prev->next = tb;
	} else {
	    tb->next = Scr->TBInfo.head;
	    Scr->TBInfo.head = tb;
	}
    }

    return 1;
}



/**
 * Do all the necessary stuff to load in a titlebar button.  If we can't find 
 * the button, then put in a question; if we can't find the question mark, 
 * something is wrong and we are probably going to be in trouble later on.
 */
void InitTitlebarButtons ()
{
    TitleButton *tb;
    int h;

    /*
     * initialize dimensions
     */
    Scr->TBInfo.width = (Scr->TitleHeight -
			 2 * (Scr->FramePadding + Scr->ButtonIndent));
    Scr->TBInfo.pad = ((Scr->TitlePadding > 1)
		       ? ((Scr->TitlePadding + 1) / 2) : 1);
    h = Scr->TBInfo.width - 2 * Scr->TBInfo.border;

    /*
     * add in some useful buttons and bindings so that novices can still
     * use the system.
     */
    if (!Scr->NoDefaults) {
	/* insert extra buttons */
	if (!CreateTitleButton (TBPM_ICONIFY, F_ICONIFY, "", (MenuRoot *) NULL,
				False, False)) {
	    fprintf (stderr, "%s:  unable to add iconify button\n",
		     ProgramName);
	}
	if (!CreateTitleButton (TBPM_RESIZE, F_RESIZE, "", (MenuRoot *) NULL,
				True, True)) {
	    fprintf (stderr, "%s:  unable to add resize button\n",
		     ProgramName);
	}
	AddDefaultBindings ();
    }
    ComputeCommonTitleOffsets ();

    /*
     * load in images and do appropriate centering
     */

    for (tb = Scr->TBInfo.head; tb; tb = tb->next) {
	tb->bitmap = FindBitmap (tb->name, &tb->width, &tb->height);
	if (!tb->bitmap) {
	    tb->bitmap = FindBitmap (TBPM_QUESTION, &tb->width, &tb->height);
	    if (!tb->bitmap) {		/* cannot happen (see util.c) */
		fprintf (stderr,
			 "%s:  unable to add titlebar button \"%s\"\n",
			 ProgramName, tb->name);
	    }
	}

	tb->dstx = (h - tb->width + 1) / 2;
	if (tb->dstx < 0) {		/* clip to minimize copying */
	    tb->srcx = -(tb->dstx);
	    tb->width = h;
	    tb->dstx = 0;
	} else {
	    tb->srcx = 0;
	}
	tb->dsty = (h - tb->height + 1) / 2;
	if (tb->dsty < 0) {
	    tb->srcy = -(tb->dsty);
	    tb->height = h;
	    tb->dsty = 0;
	} else {
	    tb->srcy = 0;
	}
    }
}



/**
 * Do all the necessary stuff to create titlebar top-left/top-right
 * corner shape masks. Called once, during screen initialisation on startup.
 */
void InitTitlebarCorners (void)
{
#define RoundedCorner3x3_width  3
#define RoundedCorner3x3_height 3
    static unsigned char RoundedCornerLeft3x3_bits[]  = {0x07, 0x01, 0x01};
    static unsigned char RoundedCornerRight3x3_bits[] = {0x07, 0x04, 0x04};

    if (Scr->RoundedTitle == TRUE)
    {
	Scr->Tcorner.left = XCreatePixmapFromBitmapData (dpy, Scr->Root,
		(char*)RoundedCornerLeft3x3_bits,
		RoundedCorner3x3_width, RoundedCorner3x3_height,
		WhitePixel(dpy, Scr->screen), BlackPixel(dpy, Scr->screen), 1);
	Scr->Tcorner.right = XCreatePixmapFromBitmapData (dpy, Scr->Root,
		(char*)RoundedCornerRight3x3_bits,
		RoundedCorner3x3_width, RoundedCorner3x3_height,
		WhitePixel(dpy, Scr->screen), BlackPixel(dpy, Scr->screen), 1);
	Scr->Tcorner.width = RoundedCorner3x3_width;
	Scr->Tcorner.height = RoundedCorner3x3_height;
	if (Scr->Tcorner.left == None || Scr->Tcorner.right == None)
	    Scr->RoundedTitle = FALSE;
    }
}



void
PaintEntry(MenuRoot *mr, MenuItem *mi, int exposure)
{
    int y_offset;
    int text_y;

#ifdef TWM_USE_RENDER
    Picture pic_win;
    if (Scr->use_xrender == TRUE)
	pic_win = XRenderCreatePicture (dpy, mr->w.win, Scr->FormatRGB, CPGraphicsExposure, &Scr->PicAttr);
#endif

#ifdef DEBUG_MENUS
    fprintf(stderr, "Paint entry\n");
#endif
    y_offset = mi->item_num * Scr->EntryHeight;
#ifdef TWM_USE_SPACING
    if (mi->func == F_TITLE)
	text_y = y_offset + Scr->MenuTitleFont.y
			+ (Scr->EntryHeight - Scr->MenuTitleFont.height) / 2;
    else
	text_y = y_offset + Scr->MenuFont.y		/*text vertically centered*/
			+ (Scr->EntryHeight - Scr->MenuFont.height) / 2;
#else
    text_y = y_offset + Scr->MenuFont.y;
#endif

#ifdef TWM_USE_RENDER
    if (Scr->use_xrender == TRUE)
    {
	if (mr->backingstore != None)
	    XCopyArea (dpy, mr->backingstore, mr->w.win, Scr->NormalGC, 0, y_offset, mr->width, Scr->EntryHeight, 0, y_offset);
	else if (Scr->XCompMgrRunning == TRUE)
	    XRenderFillRectangle (dpy, PictOpSrc, pic_win, &Scr->XRcol32Clear, 0, y_offset, mr->width, Scr->EntryHeight);
    }
#endif

    if (mi->func != F_TITLE)
    {
	int x, y;

	if (mi->state)
	{
#ifdef TWM_USE_RENDER
	    if (Scr->use_xrender == TRUE)
		XRenderFillRectangle (dpy, PictOpOver, pic_win, &mi->XRcolMenuHiB, 0, y_offset, mr->width, Scr->EntryHeight);
	    else
#endif
	    {
		XSetForeground(dpy, Scr->NormalGC, mi->MenuHiC.back);
		XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
				mr->width, Scr->EntryHeight);
	    }

	    MyFont_DrawString(&mr->w, &Scr->MenuFont, &mi->MenuHiC, mi->x,
		text_y, mi->item, mi->strlen);

#ifdef TWM_USE_XFT
	    /*
	     * initialise NormalGC with color for XCopyPlane() later
	     * ("non-TWM_USE_XFT" does it in MyFont_DrawString() already):
	     */
	    if (Scr->use_xft > 0)
		FB(Scr, mi->MenuHiC.fore, mi->MenuHiC.back);
#endif
	}
	else
	{
	    if (mi->user_colors || !exposure)
	    {
#ifdef TWM_USE_RENDER
		if (Scr->use_xrender == TRUE)
		    XRenderFillRectangle (dpy, PictOpOver, pic_win, &mi->XRcolMenuB, 0, y_offset, mr->width, Scr->EntryHeight);
		else
#endif
		{
		    XSetForeground(dpy, Scr->NormalGC, mi->MenuC.back);
		    XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
				    mr->width, Scr->EntryHeight);
		}
	    }

	    /* MyFont_DrawString() sets NormalGC: */
	    MyFont_DrawString(&mr->w, &Scr->MenuFont, &mi->MenuC,
		mi->x, text_y, mi->item, mi->strlen);

#ifdef TWM_USE_XFT
	    if (Scr->use_xft > 0)
		FB(Scr, mi->MenuC.fore, mi->MenuC.back);
#endif
	}

	if (mi->func == F_MENU)
	{
#ifdef TWM_USE_SPACING
	    x = mr->width - Scr->pullW - Scr->MenuFont.height/3;
	    y = y_offset	/*submenu iconic knob vertically centered*/
			+ (Scr->EntryHeight - Scr->pullH) / 2;
#else
	    x = mr->width - Scr->pullW - 5;
	    y = y_offset + ((Scr->MenuFont.height - Scr->pullH) / 2);
#endif
	    x = mr->width - Scr->pullW - 5;
	    y = y_offset + ((Scr->MenuFont.height - Scr->pullH) / 2);

#ifdef TWM_USE_RENDER
	    if (Scr->use_xrender == TRUE)
		XRenderComposite (dpy, PictOpOver, (mi->state?mi->PenMenuHiF:mi->PenMenuF), Scr->PenpullPm, pic_win, 0, 0, 0, 0, x, y, Scr->pullW, Scr->pullH);
	    else
#endif
		XCopyPlane(dpy, Scr->pullPm, mr->w.win, Scr->NormalGC, 0, 0,
				Scr->pullW, Scr->pullH, x, y, 1);
	}
    }
    else
    {
	int y;

#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE)
	    XRenderFillRectangle (dpy, PictOpOver, pic_win, &mi->XRcolMenuB, 0, y_offset, mr->width, Scr->EntryHeight);
	else
#endif
	{
	    /* fill the rectangle with the title background color */
	    XSetForeground(dpy, Scr->NormalGC, mi->MenuC.back);
	    XFillRectangle(dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
				mr->width, Scr->EntryHeight);
	}

	if (Scr->MenuBorderWidth > 0) /*draw separator only if menu has border*/
	{
	    XSetForeground(dpy, Scr->NormalGC, Scr->MenuBorderColor);
	    /* now draw the dividing lines */
	    if (y_offset)
	      XDrawLine (dpy, mr->w.win, Scr->NormalGC, 0, y_offset,
			 mr->width, y_offset);
	    y = ((mi->item_num+1) * Scr->EntryHeight)-1;
	    XDrawLine(dpy, mr->w.win, Scr->NormalGC, 0, y, mr->width, y);
	}

	/* finally render the title */
	MyFont_DrawString(&mr->w, &Scr->MenuTitleFont, &mi->MenuC, mi->x,
	    text_y, mi->item, mi->strlen);
    }

#ifdef TWM_USE_RENDER
    if (Scr->use_xrender == TRUE)
	XRenderFreePicture (dpy, pic_win);
#endif
}
    

void
PaintMenu(MenuRoot *mr, XEvent *e)
{
    MenuItem *mi;

    for (mi = mr->first; mi != NULL; mi = mi->next)
    {
	int y_offset = mi->item_num * Scr->EntryHeight;

	/* be smart about handling the expose, redraw only the entries
	 * that we need to
	 */
	if (e->xexpose.y < (y_offset + Scr->EntryHeight) &&
	    (e->xexpose.y + e->xexpose.height) > y_offset)
	{
#ifdef TWM_USE_RENDER
	    if (Scr->use_xrender == TRUE)
		PaintEntry(mr, mi, False);
	    else /* fallthrough */
#endif
#ifdef TWM_USE_XFT
	    if (Scr->use_xft > 0)
		PaintEntry(mr, mi, False); /* enforce clear area first */
	    else
#endif
		PaintEntry(mr, mi, True);
	}
    }
    XSync(dpy, 0);
}



static Bool fromMenu;

void
UpdateMenu()
{
    MenuItem *mi;
    int i, x, y, x_root, y_root, entry;
    int done;
    MenuItem *badItem = NULL;

    fromMenu = TRUE;

    while (TRUE)
    {
	/* block until there is an event */
        if (!menuFromFrameOrWindowOrTitlebar) {
	  XMaskEvent(dpy,
		     ButtonPressMask | ButtonReleaseMask |
		     EnterWindowMask | ExposureMask |
		     VisibilityChangeMask | LeaveWindowMask |
		     ButtonMotionMask, &Event);
	}
	if (Event.type == MotionNotify) {
	    /* discard any extra motion events before a release */
	    while(XCheckMaskEvent(dpy,
		ButtonMotionMask | ButtonReleaseMask, &Event))
		if (Event.type == ButtonRelease)
		    break;
	}

	if (!DispatchEvent ())
	    continue;

	if (Event.type == ButtonRelease || Cancel) {
	  menuFromFrameOrWindowOrTitlebar = FALSE;
	  fromMenu = FALSE;
	  return;
	}

	if (Event.type != MotionNotify)
	    continue;

	done = FALSE;
	XQueryPointer( dpy, ActiveMenu->w.win, &JunkRoot, &JunkChild,
	    &x_root, &y_root, &x, &y, &JunkMask);

	/* if we haven't recieved the enter notify yet, wait */
	if (ActiveMenu && !ActiveMenu->entered)
	    continue;

	XFindContext(dpy, ActiveMenu->w.win, ScreenContext, (caddr_t *)&Scr);

	if (x < 0 || y < 0 ||
	    x >= ActiveMenu->width || y >= ActiveMenu->height)
	{
	    if (ActiveItem && ActiveItem->func != F_TITLE)
	    {
		ActiveItem->state = 0;
		PaintEntry(ActiveMenu, ActiveItem, False);
	    }
	    ActiveItem = NULL;
	    continue;
	}

	/* look for the entry that the mouse is in */
	entry = y / Scr->EntryHeight;
	for (i = 0, mi = ActiveMenu->first; mi != NULL; i++, mi=mi->next)
	{
	    if (i == entry)
		break;
	}

	/* if there is an active item, we might have to turn it off */
	if (ActiveItem)
	{
	    /* is the active item the one we are on ? */
	    if (ActiveItem->item_num == entry && ActiveItem->state)
		done = TRUE;

	    /* if we weren't on the active entry, let's turn the old
	     * active one off 
	     */
	    if (!done && ActiveItem->func != F_TITLE)
	    {
		ActiveItem->state = 0;
		PaintEntry(ActiveMenu, ActiveItem, False);
	    }
	}

	/* if we weren't on the active item, change the active item and turn
	 * it on 
	 */
	if (!done)
	{
	    ActiveItem = mi;
	    if (ActiveItem->func != F_TITLE && !ActiveItem->state)
	    {
		ActiveItem->state = 1;
		PaintEntry(ActiveMenu, ActiveItem, False);
	    }
	}

	/* now check to see if we were over the arrow of a pull right entry */
#ifdef TWM_USE_RENDER
	i = ActiveMenu->width;
#if 0
	if (Scr->XCompMgrRunning == FALSE)
	    i -= Scr->pullW;
#endif
#ifdef TWM_USE_SPACING
	i -= Scr->MenuFont.height/3;
#else
	i -= 5;
#endif
#else
	i = (ActiveMenu->width >> 1);
#endif
	if (ActiveItem->func == F_MENU && x >= i)
	{
	    MenuRoot *save = ActiveMenu;
	    int savex = MenuOrigins[MenuDepth - 1].x; 
	    int savey = MenuOrigins[MenuDepth - 1].y;

	    if (MenuDepth < MAXMENUDEPTH) {
#ifdef TWM_USE_RENDER
		if (ActiveItem->state) {
		    ActiveItem->state = 0;
		    PaintEntry (ActiveMenu, ActiveItem, False);
		    ActiveItem->state = 1;
		}
#endif
		PopUpMenu (ActiveItem->sub, 
			   (savex + i + 1),
			   (savey + ActiveItem->item_num * Scr->EntryHeight)
			   /*(savey + ActiveItem->item_num * Scr->EntryHeight +
			    (Scr->EntryHeight >> 1))*/, False);
	    } else if (!badItem) {
		Bell(XkbBI_MinorError,0,None);
		badItem = ActiveItem;
	    }

	    /* if the menu did get popped up, unhighlight the active item */
	    if (save != ActiveMenu && ActiveItem->state)
	    {
		ActiveItem->state = 0;
#ifndef TWM_USE_RENDER
		PaintEntry(save, ActiveItem, False);
#endif
		ActiveItem = NULL;
	    }
	}
	if (badItem != ActiveItem) badItem = NULL;
	XFlush(dpy);
    }

}



/**
 * create a new menu root
 *
 *  \param name  the name of the menu root
 */
MenuRoot *
NewMenuRoot(char *name)
{
    MenuRoot *tmp;

#define UNUSED_PIXEL ((unsigned long) (~0))	/* more than 24 bits */

    tmp = (MenuRoot *) calloc (1, sizeof(MenuRoot));
    tmp->MenuHiC.fore = UNUSED_PIXEL;
    tmp->MenuHiC.back = UNUSED_PIXEL;
    tmp->name = name;
    tmp->prev = NULL;
    tmp->first = NULL;
    tmp->last = NULL;
    tmp->items = 0;
    tmp->width = 0;
    tmp->mapped = NEVER_MAPPED;
    tmp->pull = FALSE;
    tmp->w.win = None;
    tmp->shadow = None;
#ifdef TWM_USE_RENDER
    tmp->backingstore = None;
#endif
    tmp->real_menu = FALSE;

    if (Scr->MenuList == NULL)
    {
	Scr->MenuList = tmp;
	Scr->MenuList->next = NULL;
    }

    if (Scr->LastMenu == NULL)
    {
	Scr->LastMenu = tmp;
	Scr->LastMenu->next = NULL;
    }
    else
    {
	Scr->LastMenu->next = tmp;
	Scr->LastMenu = tmp;
	Scr->LastMenu->next = NULL;
    }

    if (strcmp(name, TWM_WINDOWS) == 0)
	Scr->Windows = tmp;

    return (tmp);
}



/**
 * add an item to a root menu
 *
 *  \param menu   pointer to the root menu to add the item
 *  \param item   the text to appear in the menu
 *  \param action the string to possibly execute
 *  \param sub    the menu root if it is a pull-right entry
 *  \param func   the numeric function
 *  \param fore   foreground color string
 *  \param back   background color string
 */
MenuItem *
AddToMenu(MenuRoot *menu, char *item, char *action, MenuRoot *sub, int func, 
          char *fore, char *back)
{
    MenuItem *tmp;
    int width;

#ifdef DEBUG_MENUS
    fprintf(stderr, "adding menu item=\"%s\", action=%s, sub=%d, f=%d\n",
	item, action, sub, func);
#endif

    tmp = (MenuItem *) malloc(sizeof(MenuItem));
    tmp->root = menu;

    if (menu->first == NULL)
    {
	menu->first = tmp;
	tmp->prev = NULL;
    }
    else
    {
	menu->last->next = tmp;
	tmp->prev = menu->last;
    }
    menu->last = tmp;

    tmp->item = item;
    tmp->strlen = strlen(item);
    tmp->action = action;
    tmp->next = NULL;
    tmp->sub = NULL;
    tmp->state = 0;
    tmp->func = func;

#ifdef TWM_USE_RENDER
/*  tmp->XRcolMenuB = {0};
    tmp->XRcolMenuHiB = {0};*/
    tmp->PenMenuF = None;
    tmp->PenMenuHiF = None;
#endif

    if (!Scr->HaveFonts) CreateFonts();
    width = MyFont_TextWidth(&Scr->MenuFont, item, tmp->strlen);
    if (width <= 0)
	width = 1;
    if (width > menu->width)
	menu->width = width;

    tmp->user_colors = FALSE;
    if (Scr->Monochrome == COLOR && fore != NULL)
    {
	int save;

	save = Scr->FirstTime;
	Scr->FirstTime = TRUE;
	GetColor(COLOR, &tmp->MenuC.fore, fore);
	GetColor(COLOR, &tmp->MenuC.back, back);
	Scr->FirstTime = save;
	tmp->user_colors = TRUE;
    }
    if (sub != NULL)
    {
	tmp->sub = sub;
	menu->pull = TRUE;
    }
    tmp->item_num = menu->items++;

    return (tmp);
}


void
MakeMenus()
{
    MenuRoot *mr;

    if (Scr->Shadow == TRUE && 2*Scr->MenuBorderWidth >= SHADOWWIDTH)
	Scr->Shadow = FALSE; /* Shadow window will be 100% obscured */

    /* create the pull right pixmap if needed */
    if (Scr->pullPm == None)
	Scr->pullPm = CreateMenuIcon (Scr->MenuFont.height, &Scr->pullW, &Scr->pullH);

#ifdef TWM_USE_RENDER
    if (Scr->use_xrender == TRUE)
    {
	Pixmap pullPm = XCreatePixmap (dpy, Scr->Root, Scr->pullW, Scr->pullH, Scr->DepthA);
	FBgc(Scr->AlphaGC, AllPlanes, 0);
	XCopyPlane (dpy, Scr->pullPm, pullPm, Scr->AlphaGC, 0, 0, Scr->pullW, Scr->pullH, 0, 0, 1);
	Scr->PenpullPm = XRenderCreatePicture (dpy, pullPm, XRenderFindStandardFormat (dpy, Scr->FormatA), CPGraphicsExposure, &Scr->PicAttr);
	XFreePixmap (dpy, pullPm);
    }
    if (Scr->Shadow)
    {
	CopyPixelToXRenderColor (Scr->MenuShadowColor, &Scr->XRcolShadow);
#ifdef TWM_USE_OPACITY
	Scr->XRcolShadow.alpha = (Scr->MenuOpacity>>1) * 257; /* advance menu shadow transparency by 2 */
	if (Scr->XRcolShadow.alpha < 0x2000)
	    Scr->XRcolShadow.alpha = 0x2000; /* keep a very soft shadow */
#else
	Scr->XRcolShadow.alpha = 0x8000; /* opacity not configured anyways make shadows half-transparent */
#endif
	Scr->XRcolShadow.red = Scr->XRcolShadow.red * Scr->XRcolShadow.alpha / 0xffff; /* premultiply */
	Scr->XRcolShadow.green = Scr->XRcolShadow.green * Scr->XRcolShadow.alpha / 0xffff;
	Scr->XRcolShadow.blue = Scr->XRcolShadow.blue * Scr->XRcolShadow.alpha / 0xffff;
    }
    else
	Scr->XRcolShadow.red = Scr->XRcolShadow.green = Scr->XRcolShadow.blue = Scr->XRcolShadow.alpha = 0;
#endif

    for (mr = Scr->MenuList; mr != NULL; mr = mr->next)
    {
	if (mr->real_menu == FALSE)
	    continue;

	MakeMenu(mr);
    }
}


void
MakeMenu(MenuRoot *mr)
{
#ifdef TWM_USE_RENDER
    XRenderColor xrcol;
#endif
    MenuItem *start, *end, *cur, *tmp;
    XColor f1, f2, f3;
    XColor b1, b2, b3;
    XColor save_fore, save_back;
    int num, i;
    int fred, fgreen, fblue;
    int bred, bgreen, bblue;
    int width;
    unsigned long valuemask;
    XSetWindowAttributes attributes;
    Colormap cmap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;

#ifdef TWM_USE_SPACING
    Scr->EntryHeight = 120*Scr->MenuFont.height/100; /*baselineskip 1.2*/
#else
    Scr->EntryHeight = Scr->MenuFont.height + 4;
#endif

    /* lets first size the window accordingly */
    if (mr->mapped == NEVER_MAPPED)
    {
	if (mr->pull == TRUE)
	{
#ifdef TWM_USE_SPACING
	    mr->width += Scr->pullW + Scr->MenuFont.height/3;
#else
	    mr->width += 16 + 10;
#endif
	}

#ifdef TWM_USE_SPACING
	width = mr->width + 2*Scr->MenuFont.height/3;
#else
	width = mr->width + 10;
#endif
	for (cur = mr->first; cur != NULL; cur = cur->next)
	{
	    if (cur->func != F_TITLE)
#ifdef TWM_USE_SPACING
		cur->x = Scr->MenuFont.height/3;
#else
		cur->x = 5;
#endif
	    else
	    {
		cur->x = width - MyFont_TextWidth(&Scr->MenuTitleFont, cur->item, cur->strlen);
		cur->x /= 2;
	    }
	}
	mr->height = mr->items * Scr->EntryHeight;

#ifdef TWM_USE_SPACING
	mr->width += 2*Scr->MenuFont.height/3;
#else
	mr->width += 10;
#endif
	if (Scr->Shadow)
	{
	    /*
	     * Make sure that you don't draw into the shadow window or else
	     * the background bits there will get saved
	     */
	    valuemask = (CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect);
	    attributes.colormap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
	    attributes.background_pixel = Scr->MenuShadowColor;
	    attributes.border_pixel = Scr->MenuShadowColor;
	    attributes.override_redirect = True;
	    if (Scr->SaveUnder) {
		valuemask |= CWSaveUnder;
		attributes.save_under = True;
	    }

#ifdef TWM_USE_RENDER
	    if (Scr->use_xrender == TRUE)
		valuemask &= ~CWBackPixel;
#endif

	    mr->shadow = XCreateWindow (dpy, Scr->Root, 0, 0,
					(unsigned int) mr->width, 
					(unsigned int) mr->height,
					(unsigned int)0,
					Scr->d_depth,
					(unsigned int) CopyFromParent,
					Scr->d_visual,
					valuemask, &attributes);
	}

	valuemask = (CWColormap | CWBackPixel | CWBorderPixel | CWOverrideRedirect | CWEventMask);
	attributes.colormap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
	attributes.background_pixel = Scr->MenuC.back;
	attributes.border_pixel = Scr->MenuBorderColor;
	attributes.override_redirect = True;
	attributes.event_mask = (ExposureMask | EnterWindowMask);
	if (Scr->SaveUnder) {
	    valuemask |= CWSaveUnder;
	    attributes.save_under = True;
	}
	if (Scr->BackingStore) {
	    valuemask |= CWBackingStore;
	    attributes.backing_store = Always;
	}

/*#ifdef TWM_USE_OPACITY*/
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE) {
	    valuemask &= ~CWBackPixel;
	    if (Scr->XCompMgrRunning == TRUE)
		mr->backingstore = None;
	    else
		mr->backingstore = XCreatePixmap (dpy, Scr->Root, mr->width, mr->height, Scr->d_depth);
	}
#endif
/*#endif*/

	mr->w.win = XCreateWindow (dpy, Scr->Root, 0, 0,
				(unsigned int) mr->width,
				(unsigned int) mr->height,
				(unsigned int) Scr->MenuBorderWidth,
				Scr->d_depth,
				(unsigned int) CopyFromParent,
				Scr->d_visual,
				valuemask, &attributes);

#ifdef TWM_USE_OPACITY
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == FALSE)
#endif
	{
	    SetWindowOpacity (mr->w.win, Scr->MenuOpacity);
	    if (Scr->Shadow)
		SetWindowOpacity (mr->shadow, Scr->MenuOpacity>>3);
	}
#endif

#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    mr->w.xft = MyXftDrawCreate (mr->w.win);
#endif

	XSaveContext(dpy, mr->w.win, MenuContext, (caddr_t)mr);
	XSaveContext(dpy, mr->w.win, ScreenContext, (caddr_t)Scr);

	mr->mapped = UNMAPPED;
    }

#if 1
    /* only draw rounded corners if the first item is "menu title": */
    if (Scr->RoundedTitle == TRUE && mr->first != NULL && mr->first->func == F_TITLE) {
	XShapeCombineMask (dpy, mr->w.win, ShapeBounding,
			-Scr->MenuBorderWidth, -Scr->MenuBorderWidth,
			Scr->Tcorner.left, ShapeSubtract);
	XShapeCombineMask (dpy, mr->w.win, ShapeBounding,
			mr->width-Scr->Tcorner.width+Scr->MenuBorderWidth,
			-Scr->MenuBorderWidth,
			Scr->Tcorner.right, ShapeSubtract);
	if (Scr->MenuBorderWidth != 0) {
	    XShapeCombineMask (dpy, mr->w.win, ShapeClip, 0, 0,
			Scr->Tcorner.left, ShapeSubtract);
	    XShapeCombineMask (dpy, mr->w.win, ShapeClip,
			mr->width-Scr->Tcorner.width, 0,
			Scr->Tcorner.right, ShapeSubtract);
	}
	if (Scr->Shadow) {
	    XShapeCombineMask (dpy, mr->shadow, ShapeBounding,
			0, 0,
			Scr->Tcorner.left, ShapeSubtract);
	    XShapeCombineMask (dpy, mr->shadow, ShapeBounding,
			mr->width-Scr->Tcorner.width, 0,
			Scr->Tcorner.right, ShapeSubtract);
	}
    }
#endif

    /* get the default colors into the menus */
    for (tmp = mr->first; tmp != NULL; tmp = tmp->next)
    {
	if (!tmp->user_colors) {
	    if (tmp->func != F_TITLE) {
		tmp->MenuC = Scr->MenuC; /* menuitem default color */
	    } else {
		tmp->MenuC = Scr->MenuTitleC;
	    }
	}

	if (mr->MenuHiC.fore != UNUSED_PIXEL)
	{
	    tmp->MenuHiC = mr->MenuHiC;
	}
	else
	{
	    tmp->MenuHiC.fore = tmp->MenuC.back;
	    tmp->MenuHiC.back = tmp->MenuC.fore;
	}
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0) {
	    CopyPixelToXftColor (tmp->MenuC.fore, &tmp->MenuC.xft);
	    CopyPixelToXftColor (tmp->MenuHiC.fore, &tmp->MenuHiC.xft);
	}
#endif

#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE)
	{
	    CopyPixelToXRenderColor (tmp->MenuC.fore, &xrcol);
	    tmp->PenMenuF = Create_Color_Pen (&xrcol);
	    CopyPixelToXRenderColor (tmp->MenuHiC.fore, &xrcol);
	    tmp->PenMenuHiF = Create_Color_Pen (&xrcol);
	    CopyPixelToXRenderColor (tmp->MenuC.back, &tmp->XRcolMenuB);
	    CopyPixelToXRenderColor (tmp->MenuHiC.back, &tmp->XRcolMenuHiB);
#ifdef TWM_USE_OPACITY
	    if (mr->backingstore != None || Scr->XCompMgrRunning == TRUE)
	    {
		if (tmp->func != F_TITLE) {
		    tmp->XRcolMenuB.alpha = Scr->MenuOpacity * 257;
		    tmp->XRcolMenuB.red = tmp->XRcolMenuB.red * tmp->XRcolMenuB.alpha / 0xffff; /* premultiply */
		    tmp->XRcolMenuB.green = tmp->XRcolMenuB.green * tmp->XRcolMenuB.alpha / 0xffff;
		    tmp->XRcolMenuB.blue = tmp->XRcolMenuB.blue * tmp->XRcolMenuB.alpha / 0xffff;
		}
#if 1
		tmp->XRcolMenuHiB.alpha = ((3*255 + Scr->MenuOpacity)>>2) * 257;
#else
		tmp->XRcolMenuHiB.alpha = Scr->MenuOpacity * 257;
#endif
		tmp->XRcolMenuHiB.red = tmp->XRcolMenuHiB.red * tmp->XRcolMenuHiB.alpha / 0xffff; /* premultiply */
		tmp->XRcolMenuHiB.green = tmp->XRcolMenuHiB.green * tmp->XRcolMenuHiB.alpha / 0xffff;
		tmp->XRcolMenuHiB.blue = tmp->XRcolMenuHiB.blue * tmp->XRcolMenuHiB.alpha / 0xffff;
	    }
#endif
	}
#endif
    }

    if (Scr->Monochrome == MONOCHROME || !Scr->InterpolateMenuColors)
	return;

    start = mr->first;
    while (TRUE)
    {
	for (; start != NULL; start = start->next)
	{
	    if (start->user_colors)
		break;
	}
	if (start == NULL)
	    break;

	for (end = start->next; end != NULL; end = end->next)
	{
	    if (end->user_colors)
		break;
	}
	if (end == NULL)
	    break;

	/* we have a start and end to interpolate between */
	num = end->item_num - start->item_num;

	f1.pixel = start->MenuC.fore;
	XQueryColor(dpy, cmap, &f1);
	f2.pixel = end->MenuC.fore;
	XQueryColor(dpy, cmap, &f2);

	b1.pixel = start->MenuC.back;
	XQueryColor(dpy, cmap, &b1);
	b2.pixel = end->MenuC.back;
	XQueryColor(dpy, cmap, &b2);

	fred = ((int)f2.red - (int)f1.red) / num;
	fgreen = ((int)f2.green - (int)f1.green) / num;
	fblue = ((int)f2.blue - (int)f1.blue) / num;

	bred = ((int)b2.red - (int)b1.red) / num;
	bgreen = ((int)b2.green - (int)b1.green) / num;
	bblue = ((int)b2.blue - (int)b1.blue) / num;

	f3 = f1;
	f3.flags = DoRed | DoGreen | DoBlue;

	b3 = b1;
	b3.flags = DoRed | DoGreen | DoBlue;

	num -= 1;
	for (i = 0, cur = start->next; i < num; i++, cur = cur->next)
	{
	    f3.red += fred;
	    f3.green += fgreen;
	    f3.blue += fblue;
	    save_fore = f3;

	    b3.red += bred;
	    b3.green += bgreen;
	    b3.blue += bblue;
	    save_back = b3;

	    XAllocColor(dpy, cmap, &f3);
	    XAllocColor(dpy, cmap, &b3);
	    cur->MenuHiC.back = cur->MenuC.fore = f3.pixel;
	    cur->MenuHiC.fore = cur->MenuC.back = b3.pixel;
	    cur->user_colors = True;

	    f3 = save_fore;
	    b3 = save_back;
#ifdef TWM_USE_XFT
	    if (Scr->use_xft > 0) {
		CopyPixelToXftColor (cur->MenuC.fore, &cur->MenuC.xft);
		CopyPixelToXftColor (cur->MenuHiC.fore, &cur->MenuHiC.xft);
	    }
#endif

#ifdef TWM_USE_RENDER
	    if (Scr->use_xrender == TRUE)
	    {
		CopyPixelToXRenderColor (cur->MenuC.back, &cur->XRcolMenuB);
		CopyPixelToXRenderColor (tmp->MenuHiC.back, &tmp->XRcolMenuHiB);
#ifdef TWM_USE_OPACITY
		if (mr->backingstore != None || Scr->XCompMgrRunning == TRUE)
		{
		    if (cur->func != F_TITLE) {
			cur->XRcolMenuB.alpha = Scr->MenuOpacity * 257;
			cur->XRcolMenuB.red = cur->XRcolMenuB.red * cur->XRcolMenuB.alpha / 0xffff; /* premultiply */
			cur->XRcolMenuB.green = cur->XRcolMenuB.green * cur->XRcolMenuB.alpha / 0xffff;
			cur->XRcolMenuB.blue = cur->XRcolMenuB.blue * cur->XRcolMenuB.alpha / 0xffff;
		    }
#if 1
		    cur->XRcolMenuHiB.alpha = ((3*255 + Scr->MenuOpacity)>>2) * 257;
#else
		    cur->XRcolMenuHiB.alpha = Scr->MenuOpacity * 257;
#endif
		    cur->XRcolMenuHiB.red = cur->XRcolMenuHiB.red * cur->XRcolMenuHiB.alpha / 0xffff; /* premultiply */
		    cur->XRcolMenuHiB.green = cur->XRcolMenuHiB.green * cur->XRcolMenuHiB.alpha / 0xffff;
		    cur->XRcolMenuHiB.blue = cur->XRcolMenuHiB.blue * cur->XRcolMenuHiB.alpha / 0xffff;
		}
#endif
		if (cur->PenMenuF != None)
		    XRenderFreePicture (dpy, cur->PenMenuF);
		if (cur->PenMenuHiF != None)
		    XRenderFreePicture (dpy, cur->PenMenuHiF);
		CopyPixelToXRenderColor (cur->MenuC.fore, &xrcol);
		cur->PenMenuF = Create_Color_Pen (&xrcol);
		CopyPixelToXRenderColor (cur->MenuHiC.fore, &xrcol);
		cur->PenMenuHiF = Create_Color_Pen (&xrcol);
	    }
#endif
	}
	start = end;
    }
}



/**
 * pop up a pull down menu.
 *
 *  \param menu   the root pointer of the menu to pop up
 *  \param x,y    location of upper left of menu
 *  \param center whether or not to center horizontally over position
 */
#ifdef TWM_USE_RENDER
void RepaintTranslucentShadow (MenuRoot *menu)
{
    if (Scr->use_xrender == TRUE) {
	Picture pic_win = XRenderCreatePicture (dpy, menu->shadow, Scr->FormatRGB, CPGraphicsExposure, &Scr->PicAttr);
	int i = SHADOWWIDTH - (Scr->MenuBorderWidth<<1);
	if (Scr->XCompMgrRunning == TRUE)
	    XRenderFillRectangle (dpy, PictOpSrc, pic_win, &Scr->XRcol32Clear, 0, 0, menu->width, menu->height);
	XRenderFillRectangle (dpy, PictOpOver, pic_win, &Scr->XRcolShadow, menu->width-i, 0, i, menu->height);
	XRenderFillRectangle (dpy, PictOpOver, pic_win, &Scr->XRcolShadow, 0, menu->height-i, menu->width-i, i);
	XRenderFreePicture (dpy, pic_win);
    }
}
#endif

Bool 
PopUpMenu (MenuRoot *menu, int x, int y, Bool center)
{
    int WindowNameCount;
    TwmWindow **WindowNames;
    TwmWindow *tmp_win2,*tmp_win3;
    int i;
    int (*compar)(const char *, const char *) = 
      (Scr->CaseSensitive ? strcmp : XmuCompareISOLatin1);

    if (!menu) return False;

    if (menu == Scr->Windows)
    {
	TwmWindow *tmp_win;

	/* this is the twm windows menu,  let's go ahead and build it */

	DestroyMenu (menu);

	menu->first = NULL;
	menu->last = NULL;
	menu->items = 0;
	menu->width = 0;
	menu->mapped = NEVER_MAPPED;
  	AddToMenu(menu, "TWM Windows", NULLSTR, NULL, F_TITLE,NULLSTR,NULLSTR);
  
        for(tmp_win = Scr->TwmRoot.next , WindowNameCount=0;
            tmp_win != NULL;
            tmp_win = tmp_win->next)
          WindowNameCount++;
        if (WindowNameCount != 0)
        {
            WindowNames =
              (TwmWindow **)malloc(sizeof(TwmWindow *)*WindowNameCount);
            WindowNames[0] = Scr->TwmRoot.next;
            for(tmp_win = Scr->TwmRoot.next->next , WindowNameCount=1;
                tmp_win != NULL;
                tmp_win = tmp_win->next,WindowNameCount++)
            {
                tmp_win2 = tmp_win;
                for (i=0;i<WindowNameCount;i++)
                {
                    if ((*compar)(tmp_win2->name,WindowNames[i]->name) < 0)
                    {
                        tmp_win3 = tmp_win2;
                        tmp_win2 = WindowNames[i];
                        WindowNames[i] = tmp_win3;
                    }
                }
                WindowNames[WindowNameCount] = tmp_win2;
            }
            for (i=0; i<WindowNameCount; i++)
            {
                AddToMenu(menu, WindowNames[i]->name, (char *)WindowNames[i],
                          NULL, F_POPUP,NULL,NULL);
            }
            free(WindowNames);
        }

	MakeMenu(menu);
    }

    if (menu->w.win == None || menu->items == 0) return False;

    /* Prevent recursively bringing up menus. */
    if (menu->mapped == MAPPED) return False;

    /*
     * Dynamically set the parent;  this allows pull-ups to also be main
     * menus, or to be brought up from more than one place.
     */
    menu->prev = ActiveMenu;

    XGrabPointer(dpy, Scr->Root, True,
	ButtonPressMask | ButtonReleaseMask |
	ButtonMotionMask | PointerMotionHintMask,
	GrabModeAsync, GrabModeAsync,
	Scr->Root, Scr->MenuCursor, CurrentTime);

    ActiveMenu = menu;
    menu->mapped = MAPPED;
    menu->entered = FALSE;

    if (center) {
	x -= (menu->width / 2) + Scr->MenuBorderWidth;
	y -= (Scr->EntryHeight / 2) + Scr->MenuBorderWidth; /* sticky menus would be nice here */
    }

    /*
     * clip to screen
     */
    i = 2*Scr->MenuBorderWidth;
    if (Scr->Shadow && (SHADOWWIDTH > i))
	i = SHADOWWIDTH;
#ifdef TILED_SCREEN
    if (Scr->use_panels == TRUE)
    {
	int k = FindNearestPanelToMouse();
	EnsureRectangleOnPanel (k, &x, &y, menu->width + i, menu->height + i);
    }
    else
#endif
	EnsureRectangleOnScreen (&x, &y, menu->width + i, menu->height + i);

    InstallRootColormap();

    MenuOrigins[MenuDepth].x = x;
    MenuOrigins[MenuDepth].y = y;
    MenuDepth++;

    XMoveWindow(dpy, menu->w.win, x, y);
    if (Scr->Shadow) {
	XMoveWindow (dpy, menu->shadow, x + SHADOWWIDTH, y + SHADOWWIDTH);
    }
    if (Scr->Shadow) {
	XRaiseWindow (dpy, menu->shadow);
    }
    XMapRaised(dpy, menu->w.win);
#ifdef TWM_USE_RENDER
    if (menu->backingstore != None)
	XCopyArea (dpy, menu->w.win, menu->backingstore, Scr->NormalGC, 0, 0, menu->width, menu->height, 0, 0);
#endif
    if (Scr->Shadow) {
	XMapWindow (dpy, menu->shadow);
#ifdef TWM_USE_RENDER
	RepaintTranslucentShadow (menu);
#endif
    }
    XSync(dpy, 0);
    return True;
}



/**
 * unhighlight the current menu selection and take down the menus
 */
void
PopDownMenu()
{
    MenuRoot *tmp;

    if (ActiveMenu == NULL)
	return;

    if (ActiveItem)
    {
	ActiveItem->state = 0;
	PaintEntry(ActiveMenu, ActiveItem, False);
    }

    MenuDepth = 0;
    for (tmp = ActiveMenu; tmp != NULL; tmp = tmp->prev)
    {
	if (Scr->Shadow) {
	    XUnmapWindow (dpy, tmp->shadow);
	}
	XUnmapWindow(dpy, tmp->w.win);
	tmp->mapped = UNMAPPED;
	UninstallRootColormap();
    }

    XFlush(dpy);
    ActiveMenu = NULL;
    ActiveItem = NULL;
    if (Context == C_WINDOW || Context == C_FRAME || Context == C_TITLE)
      menuFromFrameOrWindowOrTitlebar = TRUE;
}



/**
 * look for a menu root
 *
 *  \return a pointer to the menu root structure
 *
 *  \param name the name of the menu root
 */
MenuRoot *
FindMenuRoot(char *name)
{
    MenuRoot *tmp;

    for (tmp = Scr->MenuList; tmp != NULL; tmp = tmp->next)
    {
	if (strcmp(name, tmp->name) == 0)
	    return (tmp);
    }
    return NULL;
}



static Bool 
belongs_to_twm_window (TwmWindow *t, Window w)
{
    if (!t) return False;

    if (w == t->frame || w == t->title_w.win || w == t->hilite_w ||
	w == t->icon_w.win || w == t->icon_bm_w) return True;
    
    if (t && t->titlebuttons) {
	register TBWindow *tbw;
	register int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
	for (tbw = t->titlebuttons; nb > 0; tbw++, nb--) {
	    if (tbw->window == w) return True;
	}
    }
    return False;
}



static void
SaveRingMouseLocation (TwmWindow *t)
{
    if (t != NULL)
	if (True == XQueryPointer (dpy, t->frame, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY, &HotX, &HotY, &JunkMask))
	    if (HotX >= -t->frame_bw && HotX < t->frame_width + t->frame_bw
		&& HotY >= -t->frame_bw && HotY < t->frame_height + t->frame_bw)
	    {
		t->ring.curs_x = HotX;
		t->ring.curs_y = HotY;
	    }
}



void 
resizeFromCenter(Window w, TwmWindow *tmp_win)
{
  int lastx, lasty, bw2;
  XEvent event;
#if 0
  int namelen;
  int width, height;

  namelen = strlen (tmp_win->name);
#endif
  bw2 = tmp_win->frame_bw * 2;
  AddingW = tmp_win->attr.width + bw2;
  AddingH = tmp_win->attr.height + tmp_win->title_height + bw2;
#if 0
  width = (SIZE_HINDENT + MyFont_TextWidth (&Scr->SizeFont,
					     tmp_win->name, namelen));
  height = Scr->SizeFont.height + SIZE_VINDENT * 2;
#endif
  XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
	       (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, 
	       &JunkBW, &JunkDepth);
  XWarpPointer(dpy, None, w,
	       0, 0, 0, 0, DragWidth/2, DragHeight/2);   
  XQueryPointer (dpy, Scr->Root, &JunkRoot, 
		 &JunkChild, &JunkX, &JunkY,
		 &AddingX, &AddingY, &JunkMask);
#if 0
  Scr->SizeStringOffset = width +
    MyFont_TextWidth(&Scr->SizeFont, ": ", 2);
  XResizeWindow (dpy, Scr->SizeWindow.win, Scr->SizeStringOffset +
		 Scr->SizeStringWidth, height);
  MyFont_DrawImageString (&Scr->SizeWindow, &Scr->SizeFont, Scr->NormalGC,
		    width, SIZE_VINDENT + Scr->SizeFont.ascent,
		    ": ", 2);
#endif
  lastx = -10000;
  lasty = -10000;
#if 0
  MoveOutline(Scr->Root,
	      origDragX - JunkBW, origDragY - JunkBW,
	      DragWidth * JunkBW, DragHeight * JunkBW,
	      tmp_win->frame_bw,
	      tmp_win->title_height);
#endif
  MenuStartResize(tmp_win, origDragX, origDragY, DragWidth, DragHeight);
  while (TRUE)
    {
      XMaskEvent(dpy,
		 ButtonPressMask | PointerMotionMask, &event);
      
      if (event.type == MotionNotify) {
	/* discard any extra motion events before a release */
	while(XCheckMaskEvent(dpy,
			      ButtonMotionMask | ButtonPressMask, &event))
	  if (event.type == ButtonPress)
	    break;
      }
      
      if (event.type == ButtonPress)
	{
	  MenuEndResize(tmp_win);
	  XMoveResizeWindow(dpy, w, AddingX, AddingY, AddingW, AddingH);
	  break;
	}
      
/*    if (!DispatchEvent ()) continue; */

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
	  MenuDoResize(AddingX, AddingY, tmp_win);
	  
	  lastx = AddingX;
	  lasty = AddingY;
	}
      
    }
} 



/** \fn ExecuteFunction
 * execute a twm root function.
 *
 *  \param func     the function to execute
 *  \param action   the menu action to execute 
 *  \param w        the window to execute this function on
 *  \param tmp_win  the twm window structure
 *  \param event    the event that caused the function
 *  \param context  the context in which the button was pressed
 *  \param pulldown flag indicating execution from pull down menu
 *
 *  \return TRUE if should continue with remaining actions, 
 *           else FALSE to abort
 */

int
WarpThere(TwmWindow *t) 
{
    if (Scr->WarpUnmapped || t->mapped) {
        if (!t->mapped) DeIconify (t);
        WarpToWindow (t); 
        return 1; 
    }    
    return 0;
}


int
ExecuteFunction(int func, char *action, Window w, TwmWindow *tmp_win, 
                XEvent *eventp, int context, int pulldown)
{
    static Time last_time = 0;
    char tmp[200];
    char *ptr;
    char buff[MAX_FILE_SIZE];
    int count, fd;
    Window rootw;
    int origX, origY;
    int do_next_action = TRUE;
    int moving_icon = FALSE;
    Bool fromtitlebar = False;

    RootFunction = 0;
    if (Cancel)
	return TRUE;			/* XXX should this be FALSE? */

    switch (func)
    {
    case F_UPICONMGR:
    case F_LEFTICONMGR:
    case F_RIGHTICONMGR:
    case F_DOWNICONMGR:
    case F_FORWICONMGR:
    case F_BACKICONMGR:
    case F_NEXTICONMGR:
    case F_PREVICONMGR:
    case F_SWAPICONMGRENTRY:
    case F_NOP:
    case F_TITLE:
    case F_DELTASTOP:
    case F_RAISELOWER:
    case F_WARPTOSCREEN:
    case F_WARPTO:
    case F_WARPRING:
    case F_WARPTOICONMGR:
    case F_WARPNEXT:
    case F_WARPPREV:
    case F_COLORMAP:
#ifdef TWM_USE_SLOPPYFOCUS
    case F_SLOPPYFOCUS:
#endif
	break;
    default:
	switch (func)
	{
	/* restrict mouse to the screen of function execution: */
	case F_FORCEMOVE:
	case F_MOVE:
	case F_RESIZE:
	    JunkRoot = Scr->Root;
	    break;
	default:
	    /*
	     * evtl. don't warp mouse to another screen
	     * (as otherwise imposed by 'confine_to' Scr->Root):
	     */
	    JunkRoot = None;
	    break;
	}
	XGrabPointer (dpy, Scr->Root, True,
			ButtonPressMask | ButtonReleaseMask,
			GrabModeAsync, GrabModeAsync,
			JunkRoot, Scr->WaitCursor, CurrentTime);
	break;
    }

    switch (func)
    {
    case F_NOP:
    case F_TITLE:
	break;

    case F_DELTASTOP:
	if (WindowMoved) do_next_action = FALSE;
	break;

    case F_RESTART:
    {
	XSync (dpy, 0);
	Reborder (eventp->xbutton.time);
	XSync (dpy, 0);
	if (smcConn)
	    SmcCloseConnection (smcConn, 0, NULL);
	execvp(*Argv, Argv);
	fprintf (stderr, "%s:  unable to restart:  %s\n", ProgramName, *Argv);
	break;
    }

    case F_UPICONMGR:
    case F_DOWNICONMGR:
    case F_LEFTICONMGR:
    case F_RIGHTICONMGR:
    case F_FORWICONMGR:
    case F_BACKICONMGR:
	if (Scr->NoIconManagers == FALSE) {
#ifdef TWM_USE_SLOPPYFOCUS
	    if ((FocusRoot == TRUE) || (SloppyFocus == TRUE))
#else
	    if (FocusRoot == TRUE) /* nonfunctional: skip if f.focus active */
#endif
	    {
		SaveRingMouseLocation (tmp_win);
		MoveIconManager (func);
	    }
	}
        break;

    case F_NEXTICONMGR:
    case F_PREVICONMGR:
	if (Scr->NoIconManagers == FALSE) {
#ifdef TWM_USE_SLOPPYFOCUS
	    if ((FocusRoot == TRUE) || (SloppyFocus == TRUE))
#else
	    if (FocusRoot == TRUE) /* makes no sense too: skip if f.focus active */
#endif
	    {
		SaveRingMouseLocation (tmp_win);
		JumpIconManager (func);
	    }
	}
        break;

    case F_SHOWLIST:
	if (Scr->NoIconManagers == FALSE) {
	    if (context == C_ROOT) {
		IconMgr *p;
		for (p = &Scr->iconmgr; p != NULL; p = p->next)
		    ShowIconManager (p);
	    }
	    else if (tmp_win && tmp_win->list)
		ShowIconManager (tmp_win->list->iconmgr);
	    else if (Scr->ActiveIconMgr)
		ShowIconManager (Scr->ActiveIconMgr);
	    else
		ShowIconManager (&Scr->iconmgr);
	}
	break;

    case F_HIDELIST:
	if (Scr->NoIconManagers == FALSE) {
	    if (context == C_ROOT) {
		IconMgr *p;
		for (p = &Scr->iconmgr; p != NULL; p = p->next)
		    HideIconManager (p);
	    }
	    else if (tmp_win && tmp_win->list)
		HideIconManager (tmp_win->list->iconmgr);
	    else if (Scr->ActiveIconMgr)
		HideIconManager (Scr->ActiveIconMgr);
	    else
		HideIconManager (&Scr->iconmgr);
	}
	break;

    case F_SORTICONMGR:
	if (Scr->NoIconManagers == FALSE)
	{
	    int save_sort;

	    if (DeferExecution(context, func, Scr->SelectCursor))
		return TRUE;

	    save_sort = Scr->SortIconMgr;
	    Scr->SortIconMgr = TRUE;

	    if (context == C_ICONMGR)
		SortIconManager((IconMgr *) NULL);
	    else if (tmp_win->iconmgr)
		SortIconManager(tmp_win->iconmgrp);
	    else 
		Bell(XkbBI_Info,0,tmp_win->w);

	    Scr->SortIconMgr = save_sort;
	}
	break;

    case F_SWAPICONMGRENTRY:
	if (Scr->NoIconManagers == FALSE && Scr->SortIconMgr == FALSE) {
	    if (tmp_win && tmp_win->list) {
#ifdef TWM_USE_SLOPPYFOCUS
		if ((FocusRoot == TRUE) || (SloppyFocus == TRUE))
#else
		if (FocusRoot == TRUE) /* skip if f.focus active */
#endif
		{
#ifdef TWM_USE_SLOPPYFOCUS
		    if (SloppyFocus == TRUE)
			FocusOnRoot();
#endif
		    if (strcmp (action, WARPSCREEN_NEXT) == 0)
			SwapIconManagerEntry (tmp_win->list, F_NEXTICONMGR);
		    else if (strcmp (action, WARPSCREEN_PREV) == 0)
			SwapIconManagerEntry (tmp_win->list, F_PREVICONMGR);
		}
	    } else
		Bell(XkbBI_MinorError,0,None);
	}
	break;

    case F_IDENTIFY:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	Identify(tmp_win);
	break;

    case F_VERSION:
	Identify ((TwmWindow *) NULL);
	break;

    case F_AUTORAISE:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	tmp_win->auto_raise = !tmp_win->auto_raise;
	if (tmp_win->auto_raise) ++(Scr->NumAutoRaises);
	else --(Scr->NumAutoRaises);
	break;

    case F_BEEP:
	Bell(XkbBI_Info,0,None);
	break;

    case F_POPUP:
	tmp_win = (TwmWindow *)action;
	if (Scr->WindowFunction.func != 0)
	{
	   ExecuteFunction(Scr->WindowFunction.func,
			   Scr->WindowFunction.item->action,
			   w, tmp_win, eventp, C_FRAME, FALSE);
	}
	else
	{
#ifdef TILED_SCREEN
	    if (Scr->use_panels == TRUE)
	    {
		int k = FindNearestPanelToClient (tmp_win);

		JunkX = tmp_win->frame_width  + 2*tmp_win->frame_bw;
		JunkY = tmp_win->frame_height + 2*tmp_win->frame_bw;
		if (Distance1D(tmp_win->frame_x, tmp_win->frame_x+JunkX,
			    Lft(Scr->panels[k]), Rht(Scr->panels[k])) < Scr->TitleHeight-1
		    || Distance1D(tmp_win->frame_y, tmp_win->frame_y+JunkY,
			    Bot(Scr->panels[k]), Top(Scr->panels[k])) < Scr->TitleHeight-1)
		{
		    EnsureRectangleOnPanel (k, &tmp_win->frame_x, &tmp_win->frame_y,
						JunkX, JunkY);
		    XMoveWindow (dpy, tmp_win->frame, tmp_win->frame_x, tmp_win->frame_y);
		}
	    }
	    else
#endif
	    {
		while (tmp_win->frame_x >= Scr->MyDisplayWidth)
		    tmp_win->frame_x -= Scr->MyDisplayWidth;
		while (tmp_win->frame_x < 0)
		    tmp_win->frame_x += Scr->MyDisplayWidth;
		while (tmp_win->frame_y >= Scr->MyDisplayHeight)
		    tmp_win->frame_y -= Scr->MyDisplayHeight;
		while (tmp_win->frame_y < 0)
		    tmp_win->frame_y += Scr->MyDisplayHeight;
		XMoveWindow (dpy, tmp_win->frame, tmp_win->frame_x, tmp_win->frame_y);
	    }
	    DeIconify(tmp_win);
	    XRaiseWindow (dpy, tmp_win->frame);
	}
	break;

    case F_RESIZE:
	EventHandler[EnterNotify] = HandleUnknown;
	EventHandler[LeaveNotify] = HandleUnknown;
	if (DeferExecution(context, func, Scr->MoveCursor))
	    return TRUE;

	PopDownMenu();

	if (pulldown)
	    XWarpPointer(dpy, None, Scr->Root, 
		0, 0, 0, 0, eventp->xbutton.x_root, eventp->xbutton.y_root);

	if (w != tmp_win->icon_w.win) {	/* can't resize icons */

	  if ((Context == C_FRAME || Context == C_WINDOW || Context == C_TITLE)
	      && fromMenu) 
	    resizeFromCenter(w, tmp_win);
	  else {
	    /*
	     * see if this is being done from the titlebar
	     */
	    fromtitlebar = 
	      belongs_to_twm_window (tmp_win, eventp->xbutton.window);
	    
	    /* Save pointer position so we can tell if it was moved or
	       not during the resize. */
	    ResizeOrigX = eventp->xbutton.x_root;
	    ResizeOrigY = eventp->xbutton.y_root;
	    
	    StartResize (eventp, tmp_win, fromtitlebar);
	    
	    do {
	      XMaskEvent(dpy,
			   ButtonPressMask | ButtonReleaseMask |
			   EnterWindowMask | LeaveWindowMask |
			   ButtonMotionMask, &Event);
		
		if (fromtitlebar && Event.type == ButtonPress) {
		  fromtitlebar = False;
		    continue;
		  }
		
	    	if (Event.type == MotionNotify) {
		  /* discard any extra motion events before a release */
		  while
		    (XCheckMaskEvent
		     (dpy, ButtonMotionMask | ButtonReleaseMask, &Event))
		      if (Event.type == ButtonRelease)
			break;
		}
	      
	      if (!DispatchEvent ()) continue;
	      
	    } while (!(Event.type == ButtonRelease || Cancel));
	    return TRUE;
	  }
	} 
	break;


    case F_ZOOM:
    case F_HORIZOOM:
    case F_FULLZOOM:
    case F_MAXIMIZE:
    case F_LEFTZOOM:
    case F_RIGHTZOOM:
    case F_TOPZOOM:
    case F_BOTTOMZOOM:
    case F_PANELZOOM:
    case F_PANELHORIZOOM:
    case F_PANELFULLZOOM:
    case F_PANELMAXIMIZE:
    case F_PANELLEFTZOOM:
    case F_PANELRIGHTZOOM:
    case F_PANELTOPZOOM:
    case F_PANELBOTTOMZOOM:
    case F_PANELGEOMETRYZOOM:
    case F_PANELLEFTMOVE:
    case F_PANELRIGHTMOVE:
    case F_PANELTOPMOVE:
    case F_PANELBOTTOMMOVE:
    case F_PANELGEOMETRYMOVE:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;
	if (func == F_PANELGEOMETRYZOOM || func == F_PANELGEOMETRYMOVE)
	    fullgeomzoom (action, tmp_win, func);
	else
	    fullzoom (ParsePanelIndex(action), tmp_win, func);

	XUngrabPointer (dpy, CurrentTime); /* ? */
	XUngrabServer (dpy); /* <-- is the server grabbed here? */

	if (tmp_win->iconmgr == TRUE)
	    /* draw shapes anew as SetFrameShape() removes them called by SetupFrame() */
	    SetIconManagerAllLabelShapeMasks (tmp_win->iconmgrp);
	break;

    case F_MOVE:
    case F_FORCEMOVE:
	if (DeferExecution(context, func, Scr->MoveCursor))
	    return TRUE;

	PopDownMenu();
	rootw = eventp->xbutton.root;
	MoveFunction = func;

	if (pulldown)
	    XWarpPointer(dpy, None, Scr->Root, 
		0, 0, 0, 0, eventp->xbutton.x_root, eventp->xbutton.y_root);

	EventHandler[EnterNotify] = HandleUnknown;
	EventHandler[LeaveNotify] = HandleUnknown;

	if (!Scr->NoGrabServer || !Scr->OpaqueMove) {
	    XGrabServer(dpy);
	}
	XGrabPointer(dpy, eventp->xbutton.root, True,
	    ButtonPressMask | ButtonReleaseMask |
	    ButtonMotionMask | PointerMotionMask, /* PointerMotionHintMask */
	    GrabModeAsync, GrabModeAsync,
	    Scr->Root, Scr->MoveCursor, CurrentTime);

	if (context == C_ICON && tmp_win->icon_w.win)
	{
	    w = tmp_win->icon_w.win;
	    DragX = eventp->xbutton.x;
	    DragY = eventp->xbutton.y;
	    moving_icon = TRUE;
	}

	else if (w != tmp_win->icon_w.win)
	{
	    XTranslateCoordinates(dpy, w, tmp_win->frame,
		eventp->xbutton.x, 
		eventp->xbutton.y, 
		&DragX, &DragY, &JunkChild);

	    w = tmp_win->frame;
	}

	DragWindow = None;

	XGetGeometry(dpy, w, &JunkRoot, &origDragX, &origDragY,
	    (unsigned int *)&DragWidth, (unsigned int *)&DragHeight, &JunkBW,
	    &JunkDepth);

	origX = eventp->xbutton.x_root;
	origY = eventp->xbutton.y_root;
	CurrentDragX = origDragX;
	CurrentDragY = origDragY;

	/*
	 * only do the constrained move if timer is set; need to check it
	 * in case of stupid or wicked fast servers
	 */
	if (ConstrainedMoveTime && 
	    (eventp->xbutton.time - last_time) < ConstrainedMoveTime)
	{
	    int width, height;

	    ConstMove = TRUE;
	    ConstMoveDir = MOVE_NONE;
	    ConstMoveX = eventp->xbutton.x_root - DragX - JunkBW;
	    ConstMoveY = eventp->xbutton.y_root - DragY - JunkBW;
	    width = DragWidth + 2 * JunkBW;
	    height = DragHeight + 2 * JunkBW;
	    ConstMoveXL = ConstMoveX + width/3;
	    ConstMoveXR = ConstMoveX + 2*(width/3);
	    ConstMoveYT = ConstMoveY + height/3;
	    ConstMoveYB = ConstMoveY + 2*(height/3);

	    XWarpPointer(dpy, None, w,
		0, 0, 0, 0, DragWidth/2, DragHeight/2);

	    XQueryPointer(dpy, w, &JunkRoot, &JunkChild,
		&JunkX, &JunkY, &DragX, &DragY, &JunkMask);
	}
	last_time = eventp->xbutton.time;

	if (!Scr->OpaqueMove)
	{
	    InstallRootColormap();
	    if (!Scr->MoveDelta)
	    {
		/*
		 * Draw initial outline.  This was previously done the
		 * first time though the outer loop by dropping out of
		 * the XCheckMaskEvent inner loop down to one of the
		 * MoveOutline's below.
		 */
		MoveOutline(rootw, origDragX, origDragY,
		    DragWidth + 2 * JunkBW, DragHeight + 2 * JunkBW,
		    tmp_win->frame_bw,
		    moving_icon ? 0 : tmp_win->title_height);
		/*
		 * This next line causes HandleReleaseNotify to call
		 * XRaiseWindow().  This is solely to preserve the
		 * previous behaviour that raises a window being moved
		 * on button release even if you never actually moved
		 * any distance (unless you move less than MoveDelta or
		 * NoRaiseMove is set or OpaqueMove is set).
		 */
		DragWindow = w;
	    }
	}

	/*
	 * see if this is being done from the titlebar
	 */
	fromtitlebar = belongs_to_twm_window (tmp_win, eventp->xbutton.window);

	if (menuFromFrameOrWindowOrTitlebar) {
	  /* warp the pointer to the middle of the window */
	  XWarpPointer(dpy, None, Scr->Root, 0, 0, 0, 0, 
		       origDragX + DragWidth / 2, 
		       origDragY + DragHeight / 2);
	  XFlush(dpy);
	}
	
	while (TRUE)
	{
	    long releaseEvent = menuFromFrameOrWindowOrTitlebar ? 
	                          ButtonPress : ButtonRelease;
	    long movementMask = menuFromFrameOrWindowOrTitlebar ?
	                          PointerMotionMask : ButtonMotionMask;

	    /* block until there is an interesting event */
	    XMaskEvent(dpy, ButtonPressMask | ButtonReleaseMask |
				    EnterWindowMask | LeaveWindowMask |
				    ExposureMask | movementMask |
				    VisibilityChangeMask, &Event);

	    /* throw away enter and leave events until release */
	    if (Event.xany.type == EnterNotify ||
		Event.xany.type == LeaveNotify) continue; 

	    if (Event.type == MotionNotify) {
		/* discard any extra motion events before a logical release */
		while(XCheckMaskEvent(dpy,
		    movementMask | releaseEvent, &Event))
		    if (Event.type == releaseEvent)
			break;
	    }

	    /* test to see if we have a second button press to abort move */
	    if (!menuFromFrameOrWindowOrTitlebar &&  !MovedFromKeyPress) {
	        if (Event.type == ButtonPress && DragWindow != None) {
		    if (Scr->OpaqueMove)
		      XMoveWindow (dpy, DragWindow, origDragX, origDragY);
		    else
		        MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
		    DragWindow = None;
                }
	    }
	    if (fromtitlebar && Event.type == ButtonPress) {
		fromtitlebar = False;
		CurrentDragX = origX = Event.xbutton.x_root;
		CurrentDragY = origY = Event.xbutton.y_root;
		XTranslateCoordinates (dpy, rootw, tmp_win->frame,
				       origX, origY,
				       &DragX, &DragY, &JunkChild);
		continue;
	    }

	    if (!DispatchEvent2 ()) continue;

	    if (Cancel)
	    {
		WindowMoved = FALSE;
		if (!Scr->OpaqueMove)
		    UninstallRootColormap();
		    return TRUE;	/* XXX should this be FALSE? */
	    }
	    if (Event.type == releaseEvent)
	    {
		MoveOutline(rootw, 0, 0, 0, 0, 0, 0);
		if (moving_icon &&
		    ((CurrentDragX != origDragX ||
		      CurrentDragY != origDragY)))
		  tmp_win->icon_moved = TRUE;
		if (!Scr->OpaqueMove && menuFromFrameOrWindowOrTitlebar)
		  XMoveWindow(dpy, DragWindow, 
			      Event.xbutton.x_root - DragWidth / 2,
			      Event.xbutton.y_root - DragHeight / 2);
		break;
	    }

	    /* something left to do only if the pointer moved */
	    if (Event.type != MotionNotify)
		continue;

	    XQueryPointer(dpy, rootw, &(eventp->xmotion.root), &JunkChild,
		&(eventp->xmotion.x_root), &(eventp->xmotion.y_root),
		&JunkX, &JunkY, &JunkMask);

	    if (DragWindow == None &&
		abs(eventp->xmotion.x_root - origX) < Scr->MoveDelta &&
	        abs(eventp->xmotion.y_root - origY) < Scr->MoveDelta)
		continue;

	    WindowMoved = TRUE;
	    DragWindow = w;

	    if (!Scr->NoRaiseMove && Scr->OpaqueMove)	/* can't restore... */
	      XRaiseWindow(dpy, DragWindow);

	    if (ConstMove)
	    {
		switch (ConstMoveDir)
		{
		    case MOVE_NONE:
			if (eventp->xmotion.x_root < ConstMoveXL ||
			    eventp->xmotion.x_root > ConstMoveXR)
			    ConstMoveDir = MOVE_HORIZ;

			if (eventp->xmotion.y_root < ConstMoveYT ||
			    eventp->xmotion.y_root > ConstMoveYB)
			    ConstMoveDir = MOVE_VERT;

			XQueryPointer(dpy, DragWindow, &JunkRoot, &JunkChild,
			    &JunkX, &JunkY, &DragX, &DragY, &JunkMask);
			break;

		    case MOVE_VERT:
			ConstMoveY = eventp->xmotion.y_root - DragY - JunkBW;
			break;

		    case MOVE_HORIZ:
			ConstMoveX= eventp->xmotion.x_root - DragX - JunkBW;
			break;
		}

		if (ConstMoveDir != MOVE_NONE)
		{
		    int xl, yt, w, h;

		    xl = ConstMoveX;
		    yt = ConstMoveY;
		    w = DragWidth + 2 * JunkBW;
		    h = DragHeight + 2 * JunkBW;

		    if (Scr->DontMoveOff && MoveFunction != F_FORCEMOVE)
			EnsureRectangleOnScreen (&xl, &yt, w, h);

		    CurrentDragX = xl;
		    CurrentDragY = yt;
		    if (Scr->OpaqueMove)
			XMoveWindow(dpy, DragWindow, xl, yt);
		    else
			MoveOutline(eventp->xmotion.root, xl, yt, w, h,
			    tmp_win->frame_bw, 
			    moving_icon ? 0 : tmp_win->title_height);
		}
	    }
	    else if (DragWindow != None)
	    {
		int xl, yt, w, h;
		if (!menuFromFrameOrWindowOrTitlebar) {
		  xl = eventp->xmotion.x_root - DragX - JunkBW;
		  yt = eventp->xmotion.y_root - DragY - JunkBW;
		}
		else {
		  xl = eventp->xmotion.x_root - (DragWidth / 2);
		  yt = eventp->xmotion.y_root - (DragHeight / 2);
		}		  
		w = DragWidth + 2 * JunkBW;
		h = DragHeight + 2 * JunkBW;

		if (Scr->DontMoveOff && MoveFunction != F_FORCEMOVE)
		    EnsureRectangleOnScreen (&xl, &yt, w, h);

		CurrentDragX = xl;
		CurrentDragY = yt;
		if (Scr->OpaqueMove)
		    XMoveWindow(dpy, DragWindow, xl, yt);
		else
		    MoveOutline(eventp->xmotion.root, xl, yt, w, h,
			tmp_win->frame_bw,
			moving_icon ? 0 : tmp_win->title_height);
	    }

	}
        MovedFromKeyPress = False;

	if (tmp_win->iconmgr == TRUE)
	    /* draw shapes anew as SetFrameShape() removes them called by SetupFrame() */
	    SetIconManagerAllLabelShapeMasks (tmp_win->iconmgrp);

	if (!Scr->OpaqueMove && DragWindow == None)
	    UninstallRootColormap();

        break;

    case F_FUNCTION:
	{
	    MenuRoot *mroot;
	    MenuItem *mitem;

	    if ((mroot = FindMenuRoot(action)) == NULL)
	    {
		fprintf (stderr, "%s: couldn't find function \"%s\"\n", 
			 ProgramName, action);
		return TRUE;
	    }

	    if (NeedToDefer(mroot) && DeferExecution(context, func, Scr->SelectCursor))
		return TRUE;
	    else
	    {
		for (mitem = mroot->first; mitem != NULL; mitem = mitem->next)
		{
		    if (!ExecuteFunction (mitem->func, mitem->action, w,
					  tmp_win, eventp, context, pulldown))
		      break;
		}
	    }
	}
	break;

    case F_DEICONIFY:
    case F_ICONIFY:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (tmp_win->icon)
	{
	    DeIconify(tmp_win);

#ifdef TWM_USE_SLOPPYFOCUS
	    if ((SloppyFocus == TRUE) || (FocusRoot == TRUE))
#else
	    if (FocusRoot == TRUE) /* only warp if f.focus is not active */
#endif
	    {
		if (Scr->WarpCursorL != NULL) {
		    if (LookInList(Scr->WarpCursorL, tmp_win->full_name, &tmp_win->class) != NULL)
			WarpToWindow (tmp_win);
		} else if (Scr->NoWarpCursorL != NULL) {
		    if (LookInList(Scr->NoWarpCursorL, tmp_win->full_name, &tmp_win->class) == NULL)
			WarpToWindow (tmp_win);
		} else if (Scr->WarpCursor == TRUE)
		    WarpToWindow (tmp_win);
	    }
	}
        else if (func == F_ICONIFY)
	{
	    Iconify (tmp_win, eventp->xbutton.x_root - 5,
		     eventp->xbutton.y_root - 5);
	}
	break;

    case F_RAISELOWER:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (!WindowMoved) {
	    XWindowChanges xwc;

	    xwc.stack_mode = Opposite;
	    if (w != tmp_win->icon_w.win)
	      w = tmp_win->frame;
	    XConfigureWindow (dpy, w, CWStackMode, &xwc);
	}
	break;
	
    case F_RAISE:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	/* check to make sure raise is not from the WindowFunction */
	if (w == tmp_win->icon_w.win && Context != C_ROOT)
	    XRaiseWindow(dpy, tmp_win->icon_w.win);
	else
	    XRaiseWindow(dpy, tmp_win->frame);

	break;

    case F_LOWER:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (w == tmp_win->icon_w.win)
	    XLowerWindow(dpy, tmp_win->icon_w.win);
	else
	    XLowerWindow(dpy, tmp_win->frame);

	break;

    case F_FOCUS:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (tmp_win->icon == FALSE && AcceptsInput(tmp_win))
	{
#ifdef TWM_USE_SLOPPYFOCUS
	    if (SloppyFocus == FALSE && FocusRoot == FALSE && Focus == tmp_win)
#else
	    if (!FocusRoot && Focus == tmp_win)
#endif
	    {
		FocusOnRoot();
	    }
	    else
	    {
#ifdef TWM_USE_SLOPPYFOCUS
		if (SloppyFocus == TRUE)
		    SloppyFocus = FALSE;
#endif
		FocusOnClient(tmp_win);
		HighLightIconManager (tmp_win);
	    }
	}
	break;

    case F_DESTROY:
	if (DeferExecution(context, func, Scr->DestroyCursor))
	    return TRUE;

	if (tmp_win->iconmgr)
	    Bell(XkbBI_MinorError,0,tmp_win->w);
	else
	    XKillClient(dpy, tmp_win->w);
	break;

    case F_DELETE:
	if (DeferExecution(context, func, Scr->DestroyCursor))
	    return TRUE;

	if (tmp_win->iconmgr)		/* don't send ourself a message */
	  HideIconManager (tmp_win->iconmgrp);
	else if (tmp_win->protocols & DoesWmDeleteWindow)
	  SendDeleteWindowMessage (tmp_win, LastTimestamp());
	else
	  Bell(XkbBI_MinorError,0,tmp_win->w);
	break;

    case F_SAVEYOURSELF:
	if (DeferExecution (context, func, Scr->SelectCursor))
	  return TRUE;

	if (tmp_win->protocols & DoesWmSaveYourself)
	  SendSaveYourselfMessage (tmp_win, LastTimestamp());
	else
	  Bell(XkbBI_MinorError,0,tmp_win->w);
	break;

    case F_CIRCLEUP:
	XCirculateSubwindowsUp(dpy, Scr->Root);
	break;

    case F_CIRCLEDOWN:
	XCirculateSubwindowsDown(dpy, Scr->Root);
	break;

    case F_EXEC:
	{
	    ScreenInfo *scr;
	    if (FocusRoot != TRUE)
	    /*
	     * f.focus / f.sloppyfocus:  Execute external program
	     * on the screen where the mouse is and not where
	     * the current X11-event occurred.  (The XGrabPointer()
	     * above should not 'confine_to' the mouse onto that
	     * screen as well.)
	     */
		scr = FindPointerScreenInfo();
	    else
		scr = Scr;

	    PopDownMenu();
	    if (!scr->NoGrabServer)
		XUngrabServer (dpy);
	    XSync (dpy, False);
	    Execute (scr, action);
	}
	break;

    case F_UNFOCUS:
	UnHighLightIconManager();
#ifdef TWM_USE_SLOPPYFOCUS
	SloppyFocus = FALSE;
#endif
	FocusOnRoot();
	break;

#ifdef TWM_USE_SLOPPYFOCUS
    case F_SLOPPYFOCUS:
	UnHighLightIconManager();
	FocusOnRoot();      /* Important: first withdraw focus from some */
	SloppyFocus = TRUE; /* client before entering SloppyFocus mode.  */
	break;
#endif

    case F_CUT:
	strncpy (tmp, action, sizeof(tmp)-2);
	tmp[sizeof(tmp)-2] = '\0';
	strcat(tmp, "\n");
	XStoreBytes(dpy, tmp, strlen(tmp));
	break;

    case F_CUTFILE:
	ptr = XFetchBytes(dpy, &count);
	if (ptr) {
	    if (ptr[0] != '\0' && ptr[0] != '\n' && ptr[0] != '\r') {
		strncpy (tmp, ptr, sizeof(tmp)-1);
		tmp[sizeof(tmp)-1] = '\0';
		count = strlen (tmp);
		while (--count >= 0)
		    if (tmp[count] == '\n' || tmp[count] == '\r')
			tmp[count] = '\0';
		XFree (ptr);
		ptr = ExpandFilename(tmp);
		if (ptr) {
		    fd = open (ptr, O_RDONLY);
		    if (fd >= 0) {
			count = read (fd, buff, MAX_FILE_SIZE - 1);
			if (count > 0) XStoreBytes (dpy, buff, count);
			close(fd);
		    } else {
			fprintf (stderr, 
				 "%s:  unable to open cut file \"%s\"\n", 
				 ProgramName, tmp);
		    }
		    if (ptr != tmp) free (ptr);
		} 
	    } else {
		XFree(ptr);
	    }
	} else {
	    fprintf(stderr, "%s:  cut buffer is empty\n", ProgramName);
	}
	break;

    case F_WARPTOSCREEN:
	{
	    ScreenInfo *scr;
	    if (FocusRoot != TRUE) {
		/*
		 * f.focus / f.sloppyfocus is active:  KeyPress X11-events
		 * are delivered into the focused window probably on
		 * another screen, then "Scr" is not where the mouse is:
		 */
		scr = FindPointerScreenInfo();
#ifdef TWM_USE_SLOPPYFOCUS
		if (SloppyFocus == TRUE) {
		    UnHighLightIconManager();
		    FocusOnRoot(); /* drop client focus before screen switch */
		}
#endif
	    } else
		scr = Scr;

            SaveRingMouseLocation (tmp_win);

	    if (strcmp (action, WARPSCREEN_NEXT) == 0) {
		WarpToScreen (scr, scr->screen + 1, 1);
	    } else if (strcmp (action, WARPSCREEN_PREV) == 0) {
		WarpToScreen (scr, scr->screen - 1, -1);
	    } else if (strcmp (action, WARPSCREEN_BACK) == 0) {
		WarpToScreen (scr, PreviousScreen, 0);
	    } else {
		WarpToScreen (scr, atoi (action), 0);
	    }
	}
	break;

    case F_COLORMAP:
	{
	    if (strcmp (action, COLORMAP_NEXT) == 0) {
		BumpWindowColormap (tmp_win, 1);
	    } else if (strcmp (action, COLORMAP_PREV) == 0) {
		BumpWindowColormap (tmp_win, -1);
	    } else {
		BumpWindowColormap (tmp_win, 0);
	    }
	}
	break;

    case F_WARPPREV:
    case F_WARPNEXT:
#ifdef TWM_USE_SLOPPYFOCUS
	    if ((FocusRoot == TRUE) || (SloppyFocus == TRUE))
#else
	    if (FocusRoot == TRUE) /* nonfunctional: skip if f.focus active */
#endif
	    {
		register TwmWindow *t;
		static TwmWindow *savedwarp = NULL;
		TwmWindow *of, *l, *n;
		int c=0;

		SaveRingMouseLocation (tmp_win);

#define wseq(w) (func == F_WARPNEXT ? (w)->next : (w)->prev)
#define nwin(w) ((w) && (n=wseq(w)) != NULL && n != &Scr->TwmRoot ? n : l)
#define bwin(w) (!(w)||(w)->iconmgr||(w)==of||!(Scr->WarpUnmapped||(w)->mapped))

		of=(tmp_win ? tmp_win : &Scr->TwmRoot);

		for(t=Scr->TwmRoot.next; t; t=t->next) if(!bwin(t)) break;
		if(!t) break;	/* no windows we can use */

		if(func == F_WARPPREV) for(l=of; l->next; l=l->next) ;
		else l = Scr->TwmRoot.next;

		for(t=of; bwin(t) && c < 2; t=nwin(t)) if(t == of) c++;

		if(bwin(t) || c >= 2) Bell(XkbBI_MinorError,0,None);
		else {
			if(of && of == savedwarp) {
				Iconify(of, 0, 0);
				savedwarp = NULL;
			}
			if(!t->mapped) savedwarp = t; else savedwarp = NULL;
			WarpThere(t);
		}
		break;
	    }

    case F_WARPTO:
	{
	    register TwmWindow *t;
	    int len;

	    len = strlen(action);

	    if (len == 0) {
		if (tmp_win != NULL)
		    WarpThere(tmp_win);
		else if (Scr->NoIconManagers == FALSE
			&& Scr->ActiveIconMgr != NULL
			&& Scr->ActiveIconMgr->active != NULL)
		    WarpThere(Scr->ActiveIconMgr->active->twm);
		else if (Focus != NULL)
		    WarpThere(Focus);
		else
		    Bell(XkbBI_MinorError,0,None);
		break;
	    }

	    for (t = Scr->TwmRoot.next; t != NULL; t = t->next) {
		if (!strncmp(action, t->name, len)) 
                    if (WarpThere(t)) break;
	    }
	    if (!t) {
		for (t = Scr->TwmRoot.next; t != NULL; t = t->next) {
		    if (!strncmp(action, t->class.res_name, len)) 
                        if (WarpThere(t)) break;
		}
		if (!t) {
		    for (t = Scr->TwmRoot.next; t != NULL; t = t->next) {
			if (!strncmp(action, t->class.res_class, len)) 
                            if (WarpThere(t)) break;
		    }
		}
	    }

	    if (!t) 
		Bell(XkbBI_MinorError,0,None);
	}
	break;

    case F_WARPTOICONMGR:
	{
	    TwmWindow *t;
	    int len;

	    SaveRingMouseLocation (tmp_win);

	    len = strlen(action);
	    if (len == 0) {
		if (tmp_win && tmp_win->list) {
		    WarpToIconManager (tmp_win->list);
		} else if (Scr->ActiveIconMgr) {
		    if (Scr->ActiveIconMgr->active)
			WarpToIconManager (Scr->ActiveIconMgr->active);
		    else if (Scr->ActiveIconMgr->first)
			WarpToIconManager (Scr->ActiveIconMgr->first);
		} else if (Scr->iconmgr.active) {
		    WarpToIconManager (Scr->iconmgr.active);
		} else if (Scr->iconmgr.first) {
		    WarpToIconManager (Scr->iconmgr.first);
		}
	    } else
		for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
		    if (strncmp (action, t->icon_name, len) == 0) {
			if (t->list) {
			    WarpToIconManager (t->list);
			    break;
			}
		    }
	}
	break;
	
    case F_WARPRING:
	switch (action[0]) {
	  case 'n':
	    WarpAlongRing (&eventp->xbutton, True);
	    break;
	  case 'p':
	    WarpAlongRing (&eventp->xbutton, False);
	    break;
	  default:
	    Bell(XkbBI_MinorError,0,None);
	    break;
	}
	break;

    case F_FILE:
	ptr = ExpandFilename(action);
	fd = open(ptr, O_RDONLY);
	if (fd >= 0)
	{
	    count = read(fd, buff, MAX_FILE_SIZE - 1);
	    if (count > 0)
		XStoreBytes(dpy, buff, count);

	    close(fd);
	}
	else
	{
	    fprintf (stderr, "%s:  unable to open file \"%s\"\n", 
		     ProgramName, ptr);
	}
	if (ptr != action) free(ptr);
	break;

    case F_REFRESH:
	{
	    XSetWindowAttributes attributes;
	    unsigned long valuemask;

	    valuemask = (CWBackPixel | CWBackingStore | CWSaveUnder);
	    attributes.background_pixel = Scr->Black;
	    attributes.backing_store = NotUseful;
	    attributes.save_under = False;
	    w = XCreateWindow (dpy, Scr->Root, 0, 0,
			       (unsigned int) Scr->MyDisplayWidth,
			       (unsigned int) Scr->MyDisplayHeight,
			       (unsigned int) 0,
			       CopyFromParent, (unsigned int) CopyFromParent,
			       (Visual *) CopyFromParent, valuemask,
			       &attributes);
	    XMapWindow (dpy, w);
	    XDestroyWindow (dpy, w);
	    XFlush (dpy);
	}
	break;

    case F_WINREFRESH:
	if (DeferExecution(context, func, Scr->SelectCursor))
	    return TRUE;

	if (context == C_ICON && tmp_win->icon_w.win)
	    w = XCreateSimpleWindow(dpy, tmp_win->icon_w.win,
		0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);
	else
	    w = XCreateSimpleWindow(dpy, tmp_win->frame,
		0, 0, 9999, 9999, 0, Scr->Black, Scr->Black);

	XMapWindow(dpy, w);
	XDestroyWindow(dpy, w);
	XFlush(dpy);
	break;

    case F_QUIT:
	Done(NULL, NULL);
	break;

    case F_PRIORITY:
	if (HasSync)
	{
	    if (DeferExecution (context, func, Scr->SelectCursor))
		return TRUE;
	    (void)XSyncSetPriority(dpy, tmp_win->w, atoi(action));
        }
	break;
   case F_STARTWM:
	execlp("/bin/sh", "sh", "-c", action, (void *)NULL);
	fprintf (stderr, "%s:  unable to start:  %s\n", ProgramName, *Argv);
	break;

    }

    if (ButtonPressed == -1) XUngrabPointer(dpy, CurrentTime);
    return do_next_action;
}



/**
 * defer the execution of a function to the next button press if the context 
 *  is C_ROOT
 *
 *  \param context the context in which the mouse button was pressed
 *	\param func    the function to defer
 *  \param cursor  cursor the cursor to display while waiting
 */
int
DeferExecution(int context, int func, Cursor cursor)
{
  if (context == C_ROOT)
    {
	LastCursor = cursor;
	XGrabPointer(dpy, Scr->Root, True,
	    ButtonPressMask | ButtonReleaseMask,
	    GrabModeAsync, GrabModeAsync,
	    Scr->Root, cursor, CurrentTime);

	RootFunction = func;

	return (TRUE);
    }
    
    return (FALSE);
}



/**
 *regrab the pointer with the LastCursor;
 */
void
ReGrab()
{
    XGrabPointer(dpy, Scr->Root, True,
	ButtonPressMask | ButtonReleaseMask,
	GrabModeAsync, GrabModeAsync,
	Scr->Root, LastCursor, CurrentTime);
}



/**
 * checks each function in the list to see if it is one that needs 
 * to be deferred.
 *
 *  \param root the menu root to check
 */
Bool
NeedToDefer(MenuRoot *root)
{
    MenuItem *mitem;

    for (mitem = root->first; mitem != NULL; mitem = mitem->next)
    {
	switch (mitem->func)
	{
	case F_IDENTIFY:
	case F_RESIZE:
	case F_MOVE:
	case F_FORCEMOVE:
	case F_DEICONIFY:
	case F_ICONIFY:
	case F_RAISELOWER:
	case F_RAISE:
	case F_LOWER:
	case F_FOCUS:
	case F_DESTROY:
	case F_WINREFRESH:
	case F_ZOOM:
	case F_FULLZOOM:
	case F_MAXIMIZE:
	case F_HORIZOOM:
        case F_RIGHTZOOM:
        case F_LEFTZOOM:
        case F_TOPZOOM:
        case F_BOTTOMZOOM:
	case F_PANELZOOM:
	case F_PANELHORIZOOM:
	case F_PANELFULLZOOM:
	case F_PANELMAXIMIZE:
	case F_PANELLEFTZOOM:
	case F_PANELRIGHTZOOM:
	case F_PANELTOPZOOM:
	case F_PANELBOTTOMZOOM:
	case F_PANELGEOMETRYZOOM:
	case F_PANELLEFTMOVE:
	case F_PANELRIGHTMOVE:
	case F_PANELTOPMOVE:
	case F_PANELBOTTOMMOVE:
	case F_PANELGEOMETRYMOVE:
	case F_AUTORAISE:
	    return TRUE;
	}
    }
    return FALSE;
}



#if defined(sun) && defined(SVR4)
#include <sys/wait.h>

/**
 * execute the string by /bin/sh
 *  \param s  the string containing the command
 */
static int 
System (char *s)
{
    int pid, status;
    if ((pid = fork ()) == 0) {
	(void) setpgrp();
	execl ("/bin/sh", "sh", "-c", s, 0);
    } else
	waitpid (pid, &status, 0);
    return status;
}
#define system(s) System(s)

#endif

void
Execute (ScreenInfo *scr, char *s)
{
	/* FIXME: is all this stuff needed?  There could be security problems here. */
    static char buf[256];
    char *ds = DisplayString (dpy);
    char *colon, *dot1;
    char oldDisplay[256];
    char *doisplay;
    int restorevar = 0;

    oldDisplay[0] = '\0';
    doisplay=getenv("DISPLAY");
    if (doisplay)
	strcpy (oldDisplay, doisplay);

    /*
     * Build a display string using the current screen number, so that
     * X programs which get fired up from a menu come up on the screen
     * that they were invoked from, unless specifically overridden on
     * their command line.
     */
    colon = strrchr (ds, ':');
    if (colon) {			/* if host[:]:dpy */
	strcpy (buf, "DISPLAY=");
	strcat (buf, ds);
	colon = buf + 8 + (colon - ds);	/* use version in buf */
	dot1 = strchr (colon, '.');	/* first period after colon */
	if (!dot1) dot1 = colon + strlen (colon);  /* if not there, append */
	(void) sprintf (dot1, ".%d", scr->screen);
	putenv (buf);
	restorevar = 1;
    }

    (void) system (s);

    if (restorevar) {		/* why bother? */
	(void) sprintf (buf, "DISPLAY=%s", oldDisplay);
	putenv (buf);
    }
}



/**
 * put input focus on the root window.
 */
void
FocusedOnRoot()
{
    InstallWindowColormaps (0, &Scr->TwmRoot);
    Focus = NULL;
    FocusRoot = TRUE;
}

void
FocusOnRoot()
{
    SetFocus ((TwmWindow *) NULL, LastTimestamp());
    FocusedOnRoot();
}

void
FocusedOnClient (TwmWindow *tmp_win)
{
    InstallWindowColormaps (0, tmp_win);
    Focus = tmp_win;
    FocusRoot = FALSE;
}

void
FocusOnClient (TwmWindow *tmp_win)
{
    /* check if client is going to accept focus: */
    if (AcceptsInput(tmp_win) || ExpectsTakeFocus(tmp_win))
    {
	/*
	 * set focus if
	 * 'Passive' / 'Locally Active'
	 * ICCCM focus model:
	 */
	if (AcceptsInput(tmp_win))
	    SetFocus (tmp_win, LastTimestamp());
	/*
	 * send WM_TAKE_FOCUS if
	 * 'Locally Active' / 'Globally Active'
	 * ICCCM focus model:
	 */
	if (ExpectsTakeFocus(tmp_win))
	    SendTakeFocusMessage (tmp_win, LastTimestamp());

	FocusedOnClient (tmp_win);
    }
}

void
DeIconify(TwmWindow *tmp_win)
{
    TwmWindow *t;

    if (tmp_win->icon)
    {
	if (tmp_win->icon_on)
	    Zoom(tmp_win->icon_w.win, tmp_win->frame);
	else if (tmp_win->group != (Window) 0)
	{
	    for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
	    {
		if (tmp_win->group == t->w && t->icon_on)
		{
		    Zoom(t->icon_w.win, tmp_win->frame);
		    break;
		}
	    }
	}
    }

    /* take down the icon, iconmgr knob */
    if (tmp_win->icon_w.win) {
	XUnmapWindow(dpy, tmp_win->icon_w.win);
	IconDown (tmp_win);
    }
    if (tmp_win->list)
	XUnmapWindow(dpy, tmp_win->list->icon);

    /* de-iconify the main window */
    XMapWindow(dpy, tmp_win->w);
    tmp_win->mapped = TRUE;
    if (Scr->NoRaiseDeicon)
	XMapWindow(dpy, tmp_win->frame);
    else
	XMapRaised(dpy, tmp_win->frame);
    SetMapStateProp(tmp_win, NormalState);

    tmp_win->icon = FALSE;
    tmp_win->icon_on = FALSE;

    /* now de-iconify transients */
    for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
	{
	  if (t->transient && t->transientfor == tmp_win->w)
	    {
	      if (t->icon_on)
		Zoom(t->icon_w.win, t->frame);
	      else
		Zoom(tmp_win->icon_w.win, t->frame);

	      if (t->icon_w.win) {
		XUnmapWindow(dpy, t->icon_w.win);
		IconDown (t);
	      }
	      if (t->list)
		  XUnmapWindow(dpy, t->list->icon);

	      XMapWindow(dpy, t->w);
	      t->mapped = TRUE;
	      if (Scr->NoRaiseDeicon)
		XMapWindow(dpy, t->frame);
	      else
		XMapRaised(dpy, t->frame);
	      SetMapStateProp(t, NormalState);

	      t->icon = FALSE;
	      t->icon_on = FALSE;
	    }
	}
    
    XSync (dpy, False);
}



void
Iconify(TwmWindow *tmp_win, int def_x, int def_y)
{
    TwmWindow *t;
    int iconify;
    XWindowAttributes winattrs;
    unsigned long eventMask;

    iconify = ((!tmp_win->iconify_by_unmapping) || tmp_win->transient);
    if (iconify)
    {
	if (tmp_win->icon_w.win == (Window) 0)
	    CreateIconWindow(tmp_win, def_x, def_y);
	else
	    IconUp(tmp_win);
    }

    XGetWindowAttributes(dpy, tmp_win->w, &winattrs);
    eventMask = winattrs.your_event_mask;

    /* iconify transients first */
    for (t = Scr->TwmRoot.next; t != NULL; t = t->next)
      {
	if (t->transient && t->transientfor == tmp_win->w)
	  {
	    if (iconify)
	      {
		if (t->icon_on)
		    Zoom(t->icon_w.win, tmp_win->icon_w.win);
		else
		    Zoom(t->frame, tmp_win->icon_w.win);
	      }
	    
	    /*
	     * Prevent the receipt of an UnmapNotify, since that would
	     * cause a transition to the Withdrawn state.
	     */
	    t->mapped = FALSE;
	    XSelectInput(dpy, t->w, eventMask & ~StructureNotifyMask);
	    XUnmapWindow(dpy, t->w);
	    XSelectInput(dpy, t->w, eventMask);
	    XUnmapWindow(dpy, t->frame);
	    SetMapStateProp(t, IconicState);
	    if (t == Focus)
		FocusOnRoot();

	    if (t->icon_w.win)
		XUnmapWindow(dpy, t->icon_w.win);
	    if (t->list)
		XMapWindow(dpy, t->list->icon);

	    t->icon = TRUE;
	    t->icon_on = FALSE;
	  }
      } 
    
    if (iconify)
	Zoom(tmp_win->frame, tmp_win->icon_w.win);

    /*
     * Prevent the receipt of an UnmapNotify, since that would
     * cause a transition to the Withdrawn state.
     */
    tmp_win->mapped = FALSE;
    XSelectInput(dpy, tmp_win->w, eventMask & ~StructureNotifyMask);
    XUnmapWindow(dpy, tmp_win->w);
    XSelectInput(dpy, tmp_win->w, eventMask);
    XUnmapWindow(dpy, tmp_win->frame);
    SetMapStateProp(tmp_win, IconicState);
    if (tmp_win == Focus)
	FocusOnRoot();

    XSync (dpy, False); /*ensure the client window is gone*/

    /* put up the icon, iconmgr knob */
    if (tmp_win->list) {
	XMapWindow(dpy, tmp_win->list->icon);
	NotActiveIconManager(tmp_win->list);
    }
    if (iconify) {
	XMapRaised(dpy, tmp_win->icon_w.win);
	tmp_win->icon_on = TRUE;
    } else
	tmp_win->icon_on = FALSE;
    tmp_win->icon = TRUE;
}



void
RaiseInfoWindow (int px, int py)
{
    if (InfoLines > 0) {
	int i, twidth, width, height;

	XUnmapWindow (dpy, Scr->InfoWindow.win);

	(void) sprintf(Info[InfoLines++], "Click to dismiss....");

	/* figure out the width and height of the info window: */
#ifdef TWM_USE_SPACING
	height  = InfoLines * (120*Scr->DefaultFont.height/100); /*baselineskip 1.2*/
	height += Scr->DefaultFont.height - Scr->DefaultFont.y;
#else
	height = InfoLines * (Scr->DefaultFont.height+2);
#endif
	width = 1;
	for (i = 0; i < InfoLines; i++)
	{
	    twidth = MyFont_TextWidth(&Scr->DefaultFont, Info[i], strlen(Info[i]));
	    if (twidth > width)
		width = twidth;
	}

#ifdef TWM_USE_SPACING
	width += Scr->DefaultFont.height;
#else
	width += 10;		/* some padding */
#endif

	/* put the Infowindow centered and slightly below the location given: */
	px -= (width / 2);
	py -= (height / 3);

#ifdef TILED_SCREEN
	if (Scr->use_panels == TRUE)
	{
	    i = FindNearestPanelToMouse();
	    EnsureRectangleOnPanel (i, &px, &py,
		    width + 2*Scr->BorderWidth, height + 2*Scr->BorderWidth);
	}
	else
#endif
	    EnsureRectangleOnScreen (&px, &py,
		    width + 2*Scr->BorderWidth, height + 2*Scr->BorderWidth);

	XMoveResizeWindow(dpy, Scr->InfoWindow.win, px, py, width, height);
	XMapRaised(dpy, Scr->InfoWindow.win);
    }
}

void
Identify (TwmWindow *t)
{
    int i, n, x, y;
    unsigned int wwidth, wheight, bw, depth;

    n = 0;
    (void) sprintf(Info[n++], "Twm version:  %s", Version);
    Info[n++][0] = '\0';

    if (t)
    {
	TwmWindow *tmp;
	char *f;
	XGetGeometry (dpy, t->w, &JunkRoot, &JunkX, &JunkY,
		      &wwidth, &wheight, &bw, &depth);
	(void) XTranslateCoordinates (dpy, t->w, Scr->Root, 0, 0,
				      &x, &y, &JunkChild);
	(void) sprintf(Info[n++], "Name             = \"%s\"", t->full_name);
	(void) sprintf(Info[n++], "Icon name        = \"%s\"", t->icon_name);
	(void) sprintf(Info[n++], "Class.res_name   = \"%s\"", t->class.res_name);
	(void) sprintf(Info[n++], "Class.res_class  = \"%s\"", t->class.res_class);

	if (PrintErrorMessages)
	{
	    Info[n++][0] = '\0';
	    if (t->wmhints != NULL && (t->wmhints->flags & WindowGroupHint)) {
		for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
		    if (tmp->w == t->wmhints->window_group)
			break;
		if (tmp != NULL)
		    (void) sprintf(Info[n++], "Group member of  = \"%s\"", tmp->full_name);
		else
		    (void) sprintf(Info[n++], "Group member of  = 0x0%lx", (long)(t->wmhints->window_group));
	    }
	    if (t->transient) {
		for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
		    if (tmp->w == t->transientfor)
			break;
		if (tmp != NULL)
		    (void) sprintf(Info[n++], "Transient for    = \"%s\"", tmp->full_name);
		else
		    (void) sprintf(Info[n++], "Transient for    = 0x0%lx", (long)(t->transientfor));
	    }

	    if (t->wmhints != NULL && (t->wmhints->flags & StateHint))
	    {
		switch (t->wmhints->initial_state) {
		case NormalState:
		    f = (char*)(0);
		    break;
#if WithdrawnState == DontCareState
		case WithdrawnState:
		    f = "Withdrawn/DontCareState";
		    break;
#else
		case WithdrawnState:
		    f = "Withdrawn";
		    break;
		case DontCareState:
		    f = "DontCareState";
		    break;
#endif
		case IconicState:
		    f = "Iconic";
		    break;
		case ZoomState:
		    f = "ZoomState";
		    break;
		case InactiveState:
		    f = "InactiveState";
		    break;
		default:
		    f = "Unknown";
		    break;
		}
		if (f != (char*)(0))
		    (void) sprintf(Info[n++], "Initial state    = %s", f);
	    }

	    if (GetWMState(t->w, &i, &JunkChild) == True)
	    {
		switch (i) {
		case NormalState:
		    f = (char*)(0);
		    break;
#if WithdrawnState == DontCareState
		case WithdrawnState:
		    f = "Withdrawn/DontCareState";
		    break;
#else
		case WithdrawnState:
		    f = "Withdrawn";
		    break;
		case DontCareState:
		    f = "DontCareState";
		    break;
#endif
		case IconicState:
		    f = "Iconic";
		    break;
		case ZoomState:
		    f = "ZoomState";
		    break;
		case InactiveState:
		    f = "InactiveState";
		    break;
		default:
		    f = "Unknown";
		    break;
		}
		if (f != (char*)(0))
		    (void) sprintf(Info[n++], "_XA_WM_STATE     = %s", f);
	    }

	    if (t->wmhints != NULL && (t->wmhints->flags & IconWindowHint))
		(void) sprintf(Info[n++], "Icon window      = foreign");

	    if (t->wmhints != NULL && (t->wmhints->flags & InputHint)) {
		i  = (t->wmhints->input == True) ? 2 : 0;
		i |= (t->protocols & DoesWmTakeFocus) ? 1 : 0;
	    } else {
		if (t->protocols & DoesWmTakeFocus)
		    i = 5;
		else
		    i = 6;
	    }
	    switch (i) {
	    case 0: f = "No Input"; break;
	    case 2: f = "Passive"; break;
	    case 3: f = "Locally Active"; break;
	    case 1: f = "Globally Active"; break;
	    case 6: f = "Unknown (Passive?)"; break;
	    case 5: f = "Unknown (Globally Active?)"; break;
	    }
	    (void) sprintf(Info[n++], "ICCCM Focusing   = %s", f);
	    if (t->hints.width || t->hints.height || t->hints.x || t->hints.y)
		(void) sprintf(Info[n++], "XSizeHints       = %dx%d%+d%+d",
			t->hints.width, t->hints.height, t->hints.x, t->hints.y);
	    if (t->hints.flags & USPosition)
		(void) sprintf(Info[n++], "USPosition       = true");
	    if (t->hints.flags & USSize)
		(void) sprintf(Info[n++], "USSize           = true");
	    if (t->hints.flags & PPosition)
		(void) sprintf(Info[n++], "PPosition        = true");
	    if (t->hints.flags & PSize)
		(void) sprintf(Info[n++], "PSize            = true");
	    if (t->hints.flags & PBaseSize)
		(void) sprintf(Info[n++], "PBaseSize        = %dx%d",
			t->hints.base_width, t->hints.base_height);

	    if (t->hints.flags & PWinGravity)
	    {
		switch (t->hints.win_gravity)
		{
		case ForgetGravity:
		    f = "ForgetGravity"; break;
		case NorthWestGravity:
		    f = "NorthWestGravity"; break;
		case NorthGravity:
		    f = "NorthGravity"; break;
		case NorthEastGravity:
		    f = "NorthEastGravity"; break;
		case WestGravity:
		    f = "WestGravity"; break;
		case CenterGravity:
		    f = "CenterGravity"; break;
		case EastGravity:
		    f = "EastGravity"; break;
		case SouthWestGravity:
		    f = "SouthWestGravity"; break;
		case SouthGravity:
		    f = "SouthGravity"; break;
		case SouthEastGravity:
		    f = "SouthEastGravity"; break;
		case StaticGravity:
		    f = "StaticGravity"; break;
		default:
		    f = "Unknown"; break;
		}
		if (t->hints.win_gravity != NorthWestGravity)
		    (void) sprintf(Info[n++], "PWinGravity      = %s", f);
	    }

	    switch (t->attr.win_gravity)
	    {
#if UnmapGravity == ForgetGravity
		case UnmapGravity:
		    f = "ForgetGravity/UnmapGravity"; break;
#else
		case UnmapGravity:
		    f = "UnmapGravity"; break;
		case ForgetGravity:
		    f = "ForgetGravity"; break;
#endif
		case NorthWestGravity:
		    f = "NorthWestGravity"; break;
		case NorthGravity:
		    f = "NorthGravity"; break;
		case NorthEastGravity:
		    f = "NorthEastGravity"; break;
		case WestGravity:
		    f = "WestGravity"; break;
		case CenterGravity:
		    f = "CenterGravity"; break;
		case EastGravity:
		    f = "EastGravity"; break;
		case SouthWestGravity:
		    f = "SouthWestGravity"; break;
		case SouthGravity:
		    f = "SouthGravity"; break;
		case SouthEastGravity:
		    f = "SouthEastGravity"; break;
		case StaticGravity:
		    f = "StaticGravity"; break;
		default:
		    f = "Unknown"; break;
	    }
	    if (t->attr.win_gravity != NorthWestGravity)
		(void) sprintf(Info[n++], "WinGravity       = %s", f);

	    switch (t->attr.bit_gravity)
	    {
		case ForgetGravity:
		    f = "ForgetGravity"; break;
		case NorthWestGravity:
		    f = "NorthWestGravity"; break;
		case NorthGravity:
		    f = "NorthGravity"; break;
		case NorthEastGravity:
		    f = "NorthEastGravity"; break;
		case WestGravity:
		    f = "WestGravity"; break;
		case CenterGravity:
		    f = "CenterGravity"; break;
		case EastGravity:
		    f = "EastGravity"; break;
		case SouthWestGravity:
		    f = "SouthWestGravity"; break;
		case SouthGravity:
		    f = "SouthGravity"; break;
		case SouthEastGravity:
		    f = "SouthEastGravity"; break;
		case StaticGravity:
		    f = "StaticGravity"; break;
		default:
		    f = "Unknown"; break;
	    }
	    if (t->attr.bit_gravity != NorthWestGravity)
		(void) sprintf(Info[n++], "BitGravity       = %s", f);

	    (void) sprintf(Info[n++], "Frame  XID       = 0x0%lx", (long)(t->frame));
	    (void) sprintf(Info[n++], "Client XID       = 0x0%lx", (long)(t->w));
	    if (t->attr.colormap != DefaultColormap(dpy, Scr->screen))
		(void) sprintf(Info[n++], "Client Colormap  = 0x0%lx", (long)(t->attr.colormap));
	}

	Info[n++][0] = '\0';
	(void) sprintf(Info[n++], "Geometry/root    = %dx%d%+d%+d", wwidth, wheight, x, y);
	(void) sprintf(Info[n++], "Border width     = %d", t->old_bw);
	(void) sprintf(Info[n++], "Depth            = %d", depth);
	if (HasSync)
	{
	    int priority;
	    (void)XSyncGetPriority(dpy, t->w, &priority);
	    (void) sprintf(Info[n++], "Priority         = %d", priority);
	}
    }
    else
    {
	if (PrintErrorMessages)
	{
	    char *w = NULL, *r = NULL, *t = NULL;
	    JunkX = RevertToPointerRoot;
	    JunkChild = None;
	    XGetInputFocus (dpy, &JunkChild, &JunkX);
	    switch (JunkChild) {
	    case None:
		w = "None";
		break;
	    case PointerRoot:
		w = "PointerRoot";
		break;
	    default:
		if (I18N_FetchName (JunkChild, &t))
		    w = t;
	    }
	    switch (JunkX) {
	    case RevertToNone:
		r = "RevertToNone";
		break;
	    case RevertToPointerRoot:
		r = "RevertToPointerRoot";
		break;
	    case RevertToParent:
		r = "RevertToParent";
		break;
	    default:
		r = "?";
	    }
	    if (w)
		(void) sprintf(Info[n++], "Focus: window = '%s' revert to = '%s'", w, r);
	    else
		(void) sprintf(Info[n++], "Focus: window = 0x0%lx revert to = '%s'", (long)JunkChild, r);
	    if (t != NULL)
		free (t);
#ifdef TILED_SCREEN
	    if (Scr->use_panels == TRUE)
	    {
		i = FindNearestPanelToMouse();
		if (i >= 0 && i < Scr->npanels)
		    (void) sprintf(Info[n++], "Panel %d (connector '%s'): x = %d  y = %d  w = %d  h = %d", i+1,
			    (Scr->panel_names && Scr->panel_names[i] ? Scr->panel_names[i] : "unknown"),
			    Lft(Scr->panels[i]), Bot(Scr->panels[i]),
			    AreaWidth(Scr->panels[i]), AreaHeight(Scr->panels[i]));
	    }
#endif
	}
    }

    Info[n++][0] = '\0';
    InfoLines = n;

    if (XQueryPointer (dpy, Scr->Root, &JunkRoot, &JunkChild, &x, &y,
			&i, &i, &bw) == False)
	x = y = 0;

    RaiseInfoWindow (x, y);
}


void
SetMapStateProp(TwmWindow *tmp_win, int state)
{
    unsigned long data[2];		/* "suggested" by ICCCM version 1 */
  
    data[0] = (unsigned long) state;
    data[1] = (unsigned long) (tmp_win->iconify_by_unmapping ? None :
			   tmp_win->icon_w.win);

    XChangeProperty (dpy, tmp_win->w, _XA_WM_STATE, _XA_WM_STATE, 32, 
		 PropModeReplace, (unsigned char *) data, 2);
}



Bool 
GetWMState (Window w, int *statep, Window *iwp)
{
    Atom actual_type;
    int actual_format;
    unsigned long nitems, bytesafter;
    unsigned long *datap = NULL;
    Bool retval = False;

    if (XGetWindowProperty (dpy, w, _XA_WM_STATE, 0L, 2L, False, _XA_WM_STATE,
			    &actual_type, &actual_format, &nitems, &bytesafter,
			    (unsigned char **) &datap) != Success || !datap)
      return False;

    if (nitems <= 2) {			/* "suggested" by ICCCM version 1 */
	*statep = (int) datap[0];
	*iwp = (Window) datap[1];
	retval = True;
    }

    XFree ((char *) datap);
    return retval;
}


void
WarpToScreen (ScreenInfo *scr, int n, int inc)
{
    ScreenInfo *newscr = NULL;

#ifdef TILED_SCREEN
    if (scr->use_panels == TRUE && scr->npanels > 0)
    {
	int k = FindNearestPanelToMouse();

	if (inc != 0)
	    n = k + inc;
	while (n < 0)
	    n += scr->npanels;
	n %= scr->npanels;

	XQueryPointer (dpy, scr->Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &JunkWidth, &JunkHeight, &JunkMask);

	if (JunkX < Lft(scr->panels[k]) || JunkX > Rht(scr->panels[k])
	    || JunkY < Bot(scr->panels[k]) || JunkY > Top(scr->panels[k]))
	{
	    /* fetch mouse from dead area */
	    JunkX = (Lft(scr->panels[k]) + Rht(scr->panels[k])) / 2;
	    JunkY = (Bot(scr->panels[k]) + Top(scr->panels[k])) / 2;
	}
	else if (JunkX < Lft(scr->panels[n]) || JunkX > Rht(scr->panels[n])
	    || JunkY < Bot(scr->panels[n]) || JunkY > Top(scr->panels[n]))
	{
	    /* warp, keep relative position resp. top-left pixel */
	    JunkX = Lft(scr->panels[n]) + (JunkX - Lft(scr->panels[k]))
			* AreaWidth(Scr->panels[n])  / AreaWidth(Scr->panels[k]);
	    JunkY = Bot(scr->panels[n]) + (JunkY - Bot(scr->panels[k]))
			* AreaHeight(Scr->panels[n]) / AreaHeight(Scr->panels[k]);
	}

	XWarpPointer (dpy, None, scr->Root, 0, 0, 0, 0, JunkX, JunkY);
	PreviousScreen = k; /* save previous panel (on 'global' screen) */
	return;
    }
#endif

    while (!newscr) {
					/* wrap around */
	if (n < 0) 
	  n = NumScreens - 1;
	else if (n >= NumScreens)
	  n = 0;

	newscr = ScreenList[n];
	if (!newscr) {			/* make sure screen is managed */
	    if (inc) {			/* walk around the list */
		n += inc;
		continue;
	    }
	    fprintf (stderr, "%s:  unable to warp to unmanaged screen %d\n", 
		     ProgramName, n);
	    Bell(XkbBI_MinorError,0,None);
	    return;
	}
    }

    if (scr->screen == n) return;	/* already on that screen */

    PreviousScreen = scr->screen;
    XQueryPointer (dpy, scr->Root, &JunkRoot, &JunkChild, &JunkX, &JunkY,
		   &JunkWidth, &JunkHeight, &JunkMask);

    /* keep relative position: */
    JunkX = (JunkX * newscr->MyDisplayWidth)  / scr->MyDisplayWidth;
    JunkY = (JunkY * newscr->MyDisplayHeight) / scr->MyDisplayHeight;
    XWarpPointer (dpy, None, newscr->Root, 0, 0, 0, 0, JunkX, JunkY);
}




/**
 * rotate our internal copy of WM_COLORMAP_WINDOWS
 */
void
BumpWindowColormap (TwmWindow *tmp, int inc)
{
    int i, j, previously_installed;
    ColormapWindow **cwins;

    if (!tmp) return;

    if (inc && tmp->cmaps.number_cwins > 0) {
	cwins = (ColormapWindow **) malloc(sizeof(ColormapWindow *)*
					   tmp->cmaps.number_cwins);
	if (cwins) {		
	    if ((previously_installed =
		/* SUPPRESS 560 */(Scr->cmapInfo.cmaps == &tmp->cmaps &&
	        tmp->cmaps.number_cwins))) {
		for (i = tmp->cmaps.number_cwins; i-- > 0; )
		    tmp->cmaps.cwins[i]->colormap->state = 0;
	    }

	    for (i = 0; i < tmp->cmaps.number_cwins; i++) {
		j = i - inc;
		if (j >= tmp->cmaps.number_cwins)
		    j -= tmp->cmaps.number_cwins;
		else if (j < 0)
		    j += tmp->cmaps.number_cwins;
		cwins[j] = tmp->cmaps.cwins[i];
	    }

	    free((char *) tmp->cmaps.cwins);

	    tmp->cmaps.cwins = cwins;

	    if (tmp->cmaps.number_cwins > 1)
		bzero (tmp->cmaps.scoreboard, 
		       ColormapsScoreboardLength(&tmp->cmaps));

	    if (previously_installed)
		InstallWindowColormaps(PropertyNotify, (TwmWindow *) NULL);
	}
    } else
	FetchWmColormapWindows (tmp);
}


static void
HideIconManager (IconMgr *ip)
{
    SetMapStateProp (ip->twm_win, WithdrawnState);
    XUnmapWindow(dpy, ip->twm_win->frame);
    if (ip->twm_win->icon_w.win)
      XUnmapWindow (dpy, ip->twm_win->icon_w.win);
    ip->twm_win->mapped = FALSE;
    ip->twm_win->icon = TRUE;
}


static void
ShowIconManager (IconMgr *ip)
{
    if (ip->count > 0) {
	if (ip->twm_win->mapped != TRUE) {
	    DeIconify (ip->twm_win);
	    SetIconManagerAllLabelShapeMasks (ip); /* while iconmgr not mapped we didn't fiddle with shapes */
	}
	XRaiseWindow (dpy, ip->twm_win->frame);
    }
}



void
SetBorder (TwmWindow *tmp, Bool onoroff)
{
    if (tmp->highlight) {
	if (onoroff) {
	    XSetWindowBorder (dpy, tmp->frame, tmp->BorderColor);
	    if (tmp->title_w.win)
	      XSetWindowBorder (dpy, tmp->title_w.win, tmp->BorderColor);
	} else {
	    XSetWindowBorderPixmap (dpy, tmp->frame, tmp->gray);
	    if (tmp->title_w.win)
	      XSetWindowBorderPixmap (dpy, tmp->title_w.win, tmp->gray);
	}
    }
}


void
DestroyMenu (MenuRoot *menu)
{
    MenuItem *item;

    if (menu->w.win) {
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    MyXftDrawDestroy (menu->w.xft);
#endif
	XDeleteContext (dpy, menu->w.win, MenuContext);
	XDeleteContext (dpy, menu->w.win, ScreenContext);
	if (Scr->Shadow) XDestroyWindow (dpy, menu->shadow);
	XDestroyWindow(dpy, menu->w.win);
#ifdef TWM_USE_RENDER
	if (menu->backingstore != None)
	    XFreePixmap (dpy, menu->backingstore);
#endif
    }

    for (item = menu->first; item; ) {
	MenuItem *tmp = item;
#ifdef TWM_USE_RENDER
	if (item->PenMenuF != None)
	    XRenderFreePicture (dpy, item->PenMenuF);
	if (item->PenMenuHiF != None)
	    XRenderFreePicture (dpy, item->PenMenuHiF);
#endif
	item = item->next;
	free ((char *) tmp);
    }
}



/*
 * warping routines
 */

void 
WarpAlongRing (XButtonEvent *ev, Bool forward)
{
    static TwmWindow *savedwarp = NULL;
    TwmWindow *r, *head;

    if (Scr->RingLeader)
      head = Scr->RingLeader;
    else if (!(head = Scr->Ring)) 
      return;

    if (forward) {
	for (r = head->ring.next; r != head; r = r->ring.next) {
	    if (!r || r->mapped || Scr->WarpUnmapped) break;
	}
    } else {
	for (r = head->ring.prev; r != head; r = r->ring.prev) {
	    if (!r || r->mapped || Scr->WarpUnmapped) break;
	}
    }

    if (r != NULL)
    {
	TwmWindow *p = Scr->RingLeader, *t;

	if (p && (p->mapped || Scr->WarpUnmapped) &&
	    XFindContext (dpy, ev->window, TwmContext, (caddr_t *)&t) == XCSUCCESS &&
	    p == t)
	{
	    /* only save mouse location if warping from inside the client area: */
	    HotX = ev->x_root - (p->frame_x + p->frame_bw);
	    HotY = ev->y_root - (p->frame_y + p->frame_bw);
	    if (HotX >= -p->frame_bw && HotX < p->frame_width + p->frame_bw
		    && HotY >= -p->frame_bw && HotY < p->frame_height + p->frame_bw)
	    {
		p->ring.curs_x = HotX;
		p->ring.curs_y = HotY;
	    }
	}

	if (p && p == savedwarp) {
	    Iconify (p, 0, 0);
	    savedwarp = NULL;
	}
	if (!r->mapped)
	    savedwarp = r;
	else
	    savedwarp = NULL;

	Scr->RingLeader = r;
	WarpThere (r);
    }
}



void 
WarpToWindow (TwmWindow *t)
{
    if (t->auto_raise || !Scr->NoRaiseWarp)
	AutoRaiseWindow (t);
#if 1
    /* check if already there: */
    if (False == XQueryPointer (dpy, t->frame, &JunkRoot, &JunkChild,
				&JunkX, &JunkY, &HotX, &HotY, &JunkMask)
	    || HotX < -t->frame_bw || HotX >= t->frame_width  + t->frame_bw
	    || HotY < (Scr->TitleFocus == TRUE ? -t->frame_bw : t->title_height) || HotY >= t->frame_height + t->frame_bw)
#endif
    {
#ifdef TWM_USE_SLOPPYFOCUS
	if (SloppyFocus == TRUE && !AcceptsInput(t))
	    FocusOnRoot();
#endif
	XWarpPointer (dpy, None, t->frame, 0, 0, 0, 0, t->ring.curs_x, t->ring.curs_y);
    }
}

void
WarpToIconManager (WList *t)
{
    if (t->iconmgr->twm_win->mapped != TRUE)
	ShowIconManager (t->iconmgr);
    else
	XRaiseWindow (dpy, t->iconmgr->twm_win->frame);
    XWarpPointer (dpy, None, t->icon, 0, 0, 0, 0,
		    Scr->iconifybox_width/2, Scr->iconifybox_height/2);
}




/*
 * ICCCM Client Messages - Section 4.2.8 of the ICCCM dictates that all
 * client messages will have the following form:
 *
 *     event type	ClientMessage
 *     message type	_XA_WM_PROTOCOLS
 *     window		tmp->w
 *     format		32
 *     data[0]		message atom
 *     data[1]		time stamp
 */
static void 
send_clientmessage (Window w, Atom a, Time timestamp)
{
    XClientMessageEvent ev;

    ev.type = ClientMessage;
    ev.window = w;
    ev.message_type = _XA_WM_PROTOCOLS;
    ev.format = 32;
    ev.data.l[0] = a;
    ev.data.l[1] = timestamp;
    XSendEvent (dpy, w, False, 0L, (XEvent *) &ev);
    XSync (dpy, False); /* speedup/sync message delivery*/
}

void
SendDeleteWindowMessage (TwmWindow *tmp, Time timestamp)
{
    send_clientmessage (tmp->w, _XA_WM_DELETE_WINDOW, timestamp);
}

void
SendSaveYourselfMessage (TwmWindow *tmp, Time timestamp)
{
    send_clientmessage (tmp->w, _XA_WM_SAVE_YOURSELF, timestamp);
}

void
SendTakeFocusMessage (TwmWindow *tmp, Time timestamp)
{
    send_clientmessage (tmp->w, _XA_WM_TAKE_FOCUS, timestamp);
    if (tmp->hilite_w) /* ...no FocusIn event coming */
	XMapWindow (dpy, tmp->hilite_w);
}
