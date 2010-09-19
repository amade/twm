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
 * $Xorg: resize.c,v 1.5 2001/02/09 02:05:37 xorgcvs Exp $
 *
 * window resizing borrowed from the "wm" window manager
 *
 * 11-Dec-87 Thomas E. LaStrange                File created
 *
 ***********************************************************************/

/* $XFree86: xc/programs/twm/resize.c,v 1.7 2001/01/17 23:45:07 dawes Exp $ */

#include <stdio.h>
#include <string.h>
#include "twm.h"
#include "parse.h"
#include "util.h"
#include "resize.h"
#include "iconmgr.h"
#include "add_window.h"
#include "screen.h"
#include "events.h"

#define MINHEIGHT 0     /* had been 32 */
#define MINWIDTH 0      /* had been 60 */

static int dragx;       /* all these variables are used */
static int dragy;       /* in resize operations */
static int dragWidth;
static int dragHeight;

static int origx;
static int origy;
static int origWidth;
static int origHeight;
static int origMask;

static int clampTop;
static int clampBottom;
static int clampLeft;
static int clampRight;
static int clampDX;
static int clampDY;

static int last_width;
static int last_height;


static void 
do_auto_clamp (TwmWindow *tmp_win, XEvent *evp)
{
    int x, y, h, v;

    switch (evp->type) {
      case ButtonPress:
	x = evp->xbutton.x_root;
	y = evp->xbutton.y_root;
	break;
      case KeyPress:
	x = evp->xkey.x_root;
	y = evp->xkey.y_root;
	break;
      default:
	if (!XQueryPointer (dpy, Scr->Root, &JunkRoot, &JunkChild,
			    &x, &y, &JunkX, &JunkY, &JunkMask))
	  return;
    }

    h = ((x - dragx) / (dragWidth < 3 ? 1 : (dragWidth / 3)));
    v = ((y - dragy - tmp_win->title_height) / 
	 (dragHeight < 3 ? 1 : (dragHeight / 3)));
	
    if (h <= 0) {
	clampLeft = 1;
	clampDX = (x - dragx);
    } else if (h >= 2) {
	clampRight = 1;
	clampDX = (x - dragx - dragWidth);
    }

    if (v <= 0) {
	clampTop = 1;
	clampDY = (y - dragy);
    } else if (v >= 2) {
	clampBottom = 1;
	clampDY = (y - dragy - dragHeight);
    }
}


/**
 * begin a window resize operation
 *  \param ev           the event structure (button press)
 *  \param tmp_win      the TwmWindow pointer
 *  \param fromtitlebar action invoked from titlebar button
 */
void
StartResize(XEvent *evp, TwmWindow *tmp_win, Bool fromtitlebar)
{
    ResizeWindow = tmp_win->frame;
    XGrabServer(dpy);
    XGrabPointer(dpy, Scr->Root, True,
        ButtonPressMask | ButtonReleaseMask |
	ButtonMotionMask | PointerMotionHintMask,
        GrabModeAsync, GrabModeAsync,
        Scr->Root, Scr->ResizeCursor, CurrentTime);

    XGetGeometry (dpy, (Drawable) tmp_win->frame, &JunkRoot, &dragx, &dragy,
	    (unsigned int *)&dragWidth, (unsigned int *)&dragHeight, &JunkBW, &JunkDepth);
    dragx += tmp_win->frame_bw;
    dragy += tmp_win->frame_bw;
    origx = dragx;
    origy = dragy;
    origWidth = dragWidth;
    origHeight = dragHeight;
    clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;

    if (Scr->AutoRelativeResize && !fromtitlebar)
      do_auto_clamp (tmp_win, evp);

#ifdef TILED_SCREEN
    if (Scr->use_panels == TRUE) {
	int k = FindNearestPanelToMouse();
	XMoveWindow (dpy, Scr->SizeWindow.win, Lft(Scr->panels[k]), Bot(Scr->panels[k]));
    }
#endif
    Scr->SizeStringOffset = SIZE_HINDENT;
    XResizeWindow (dpy, Scr->SizeWindow.win,
		   Scr->SizeStringWidth + SIZE_HINDENT * 2,
		   Scr->SizeFont.height + SIZE_VINDENT * 2);
    XMapRaised(dpy, Scr->SizeWindow.win);
    InstallRootColormap();
    last_width = 0;
    last_height = 0;
    DisplaySize(tmp_win, origWidth, origHeight);
    MoveOutline (Scr->Root, dragx - tmp_win->frame_bw,
		 dragy - tmp_win->frame_bw, dragWidth + 2 * tmp_win->frame_bw,
		 dragHeight + 2 * tmp_win->frame_bw,
		 tmp_win->frame_bw, tmp_win->title_height);
}



void
MenuStartResize(TwmWindow *tmp_win, int x, int y, int w, int h)
{
    XGrabServer(dpy);
    XGrabPointer(dpy, Scr->Root, True,
        ButtonPressMask | ButtonMotionMask | PointerMotionMask,
        GrabModeAsync, GrabModeAsync,
        Scr->Root, Scr->ResizeCursor, CurrentTime);
    dragx = x + tmp_win->frame_bw;
    dragy = y + tmp_win->frame_bw;
    origx = dragx;
    origy = dragy;
    dragWidth = origWidth = w; /* - 2 * tmp_win->frame_bw; */
    dragHeight = origHeight = h; /* - 2 * tmp_win->frame_bw; */
    clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;
    last_width = 0;
    last_height = 0;
#ifdef TILED_SCREEN
    if (Scr->use_panels == TRUE) {
	int k = FindNearestPanelToMouse();
	XMoveWindow (dpy, Scr->SizeWindow.win, Lft(Scr->panels[k]), Bot(Scr->panels[k]));
    }
#endif
    Scr->SizeStringOffset = SIZE_HINDENT;
    XResizeWindow (dpy, Scr->SizeWindow.win,
		   Scr->SizeStringWidth + SIZE_HINDENT * 2,
		   Scr->SizeFont.height + SIZE_VINDENT * 2);
    XMapRaised(dpy, Scr->SizeWindow.win);
    DisplaySize(tmp_win, origWidth, origHeight);
    MoveOutline (Scr->Root, dragx - tmp_win->frame_bw,
		 dragy - tmp_win->frame_bw, 
		 dragWidth + 2 * tmp_win->frame_bw,
		 dragHeight + 2 * tmp_win->frame_bw,
		 tmp_win->frame_bw, tmp_win->title_height);
}

/**
 * begin a windorew resize operation from AddWindow
 *  \param tmp_win the TwmWindow pointer
 */
void
AddStartResize(TwmWindow *tmp_win, int x, int y, int w, int h)
{
    XGrabServer(dpy);
    XGrabPointer(dpy, Scr->Root, True,
        ButtonReleaseMask | ButtonMotionMask | PointerMotionHintMask,
        GrabModeAsync, GrabModeAsync,
        Scr->Root, Scr->ResizeCursor, CurrentTime);

    dragx = x + tmp_win->frame_bw;
    dragy = y + tmp_win->frame_bw;
    origx = dragx;
    origy = dragy;
    dragWidth = origWidth = w - 2 * tmp_win->frame_bw;
    dragHeight = origHeight = h - 2 * tmp_win->frame_bw;
    clampTop = clampBottom = clampLeft = clampRight = clampDX = clampDY = 0;
/*****
    if (Scr->AutoRelativeResize) {
	clampRight = clampBottom = 1;
    }
*****/
    last_width = 0;
    last_height = 0;
    DisplaySize(tmp_win, origWidth, origHeight);
}



void
MenuDoResize(int x_root, int y_root, TwmWindow *tmp_win)
{
    int action;

    action = 0;

    x_root -= clampDX;
    y_root -= clampDY;

    if (clampTop) {
        int         delta = y_root - dragy;
        if (dragHeight - delta < MINHEIGHT) {
            delta = dragHeight - MINHEIGHT;
            clampTop = 0;
        }
        dragy += delta;
        dragHeight -= delta;
        action = 1;
    }
    else if (y_root <= dragy/* ||
             y_root == findRootInfo(root)->rooty*/) {
        dragy = y_root;
        dragHeight = origy + origHeight -
            y_root;
        clampBottom = 0;
        clampTop = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampLeft) {
        int         delta = x_root - dragx;
        if (dragWidth - delta < MINWIDTH) {
            delta = dragWidth - MINWIDTH;
            clampLeft = 0;
        }
        dragx += delta;
        dragWidth -= delta;
        action = 1;
    }
    else if (x_root <= dragx/* ||
             x_root == findRootInfo(root)->rootx*/) {
        dragx = x_root;
        dragWidth = origx + origWidth -
            x_root;
        clampRight = 0;
        clampLeft = 1;
	clampDX = 0;
        action = 1;
    }
    if (clampBottom) {
        int         delta = y_root - dragy - dragHeight;
        if (dragHeight + delta < MINHEIGHT) {
            delta = MINHEIGHT - dragHeight;
            clampBottom = 0;
        }
        dragHeight += delta;
        action = 1;
    }
    else if (y_root >= dragy + dragHeight) {
        dragy = origy;
        dragHeight = 1 + y_root - dragy;
        clampTop = 0;
        clampBottom = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampRight) {
        int         delta = x_root - dragx - dragWidth;
        if (dragWidth + delta < MINWIDTH) {
            delta = MINWIDTH - dragWidth;
            clampRight = 0;
        }
        dragWidth += delta;
        action = 1;
    }
    else if (x_root >= dragx + dragWidth) {
        dragx = origx;
        dragWidth = 1 + x_root - origx;
        clampLeft = 0;
        clampRight = 1;
	clampDX = 0;
        action = 1;
    }

    if (action) {
        ConstrainSize (tmp_win, &dragWidth, &dragHeight);
        if (clampLeft)
            dragx = origx + origWidth - dragWidth;
        if (clampTop)
            dragy = origy + origHeight - dragHeight;
        MoveOutline(Scr->Root,
            dragx - tmp_win->frame_bw,
            dragy - tmp_win->frame_bw,
            dragWidth + 2 * tmp_win->frame_bw,
            dragHeight + 2 * tmp_win->frame_bw,
	    tmp_win->frame_bw, tmp_win->title_height);
    }

    DisplaySize(tmp_win, dragWidth, dragHeight);
}


/**
 * move the rubberband around.  This is called for each motion event when 
 * we are resizing
 *
 *  \param x_root  the X corrdinate in the root window
 *  \param y_root  the Y corrdinate in the root window
 *  \param tmp_win the current twm window
 */
void
DoResize(int x_root, int y_root, TwmWindow *tmp_win)
{
    int action;

    action = 0;

    x_root -= clampDX;
    y_root -= clampDY;

    if (clampTop) {
        int         delta = y_root - dragy;
        if (dragHeight - delta < MINHEIGHT) {
            delta = dragHeight - MINHEIGHT;
            clampTop = 0;
        }
        dragy += delta;
        dragHeight -= delta;
        action = 1;
    }
    else if (y_root <= dragy/* ||
             y_root == findRootInfo(root)->rooty*/) {
        dragy = y_root;
        dragHeight = origy + origHeight -
            y_root;
        clampBottom = 0;
        clampTop = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampLeft) {
        int         delta = x_root - dragx;
        if (dragWidth - delta < MINWIDTH) {
            delta = dragWidth - MINWIDTH;
            clampLeft = 0;
        }
        dragx += delta;
        dragWidth -= delta;
        action = 1;
    }
    else if (x_root <= dragx/* ||
             x_root == findRootInfo(root)->rootx*/) {
        dragx = x_root;
        dragWidth = origx + origWidth -
            x_root;
        clampRight = 0;
        clampLeft = 1;
	clampDX = 0;
        action = 1;
    }
    if (clampBottom) {
        int         delta = y_root - dragy - dragHeight;
        if (dragHeight + delta < MINHEIGHT) {
            delta = MINHEIGHT - dragHeight;
            clampBottom = 0;
        }
        dragHeight += delta;
        action = 1;
    }
    else if (y_root >= dragy + dragHeight - 1/* ||
           y_root == findRootInfo(root)->rooty
           + findRootInfo(root)->rootheight - 1*/) {
        dragy = origy;
        dragHeight = 1 + y_root - dragy;
        clampTop = 0;
        clampBottom = 1;
	clampDY = 0;
        action = 1;
    }
    if (clampRight) {
        int         delta = x_root - dragx - dragWidth;
        if (dragWidth + delta < MINWIDTH) {
            delta = MINWIDTH - dragWidth;
            clampRight = 0;
        }
        dragWidth += delta;
        action = 1;
    }
    else if (x_root >= dragx + dragWidth - 1/* ||
             x_root == findRootInfo(root)->rootx +
             findRootInfo(root)->rootwidth - 1*/) {
        dragx = origx;
        dragWidth = 1 + x_root - origx;
        clampLeft = 0;
        clampRight = 1;
	clampDX = 0;
        action = 1;
    }

    if (action) {
        ConstrainSize (tmp_win, &dragWidth, &dragHeight);
        if (clampLeft)
            dragx = origx + origWidth - dragWidth;
        if (clampTop)
            dragy = origy + origHeight - dragHeight;
        MoveOutline(Scr->Root,
            dragx - tmp_win->frame_bw,
            dragy - tmp_win->frame_bw,
            dragWidth + 2 * tmp_win->frame_bw,
            dragHeight + 2 * tmp_win->frame_bw,
	    tmp_win->frame_bw, tmp_win->title_height);
    }

    DisplaySize(tmp_win, dragWidth, dragHeight);
}

/**
 * display the size in the dimensions window.
 *
 *  \param tmp_win the current twm window
 *  \param width   the width of the rubber band
 *  \param height  the height of the rubber band
 */
void
DisplaySize(TwmWindow *tmp_win, int width, int height)
{
    char str[100];
    int dwidth;
    int dheight;

    if (last_width == width && last_height == height)
        return;

    last_width = width;
    last_height = height;

    dheight = height - tmp_win->title_height;
    dwidth = width;

    /*
     * ICCCM says that PMinSize is the default is no PBaseSize is given,
     * and vice-versa.
     */
    if (tmp_win->hints.flags&(PMinSize|PBaseSize) && tmp_win->hints.flags & PResizeInc)
    {
	if (tmp_win->hints.flags & PBaseSize) {
	    dwidth -= tmp_win->hints.base_width;
	    dheight -= tmp_win->hints.base_height;
	} else {
	    dwidth -= tmp_win->hints.min_width;
	    dheight -= tmp_win->hints.min_height;
	}
    }

    if (tmp_win->hints.flags & PResizeInc)
    {
        dwidth /= tmp_win->hints.width_inc;
        dheight /= tmp_win->hints.height_inc;
    }

    (void) sprintf (str, " %4d x %-4d ", dwidth, dheight);
    XRaiseWindow(dpy, Scr->SizeWindow.win);
    XClearArea (dpy, Scr->SizeWindow.win, Scr->SizeStringOffset, 0, 0, 0, False);
    MyFont_DrawString (&Scr->SizeWindow, &Scr->SizeFont,
			    &Scr->DefaultC, Scr->SizeStringOffset,
			    Scr->SizeFont.ascent + SIZE_VINDENT,
			    str, 13);
}

/**
 * finish the resize operation
 */
void
EndResize()
{
    TwmWindow *tmp_win;

#ifdef DEBUG
    fprintf(stderr, "EndResize\n");
#endif

    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
    XUnmapWindow(dpy, Scr->SizeWindow.win);

    XFindContext(dpy, ResizeWindow, TwmContext, (caddr_t *)&tmp_win);

    ConstrainSize (tmp_win, &dragWidth, &dragHeight);

    if (dragWidth != tmp_win->frame_width ||
        dragHeight != tmp_win->frame_height)
            tmp_win->zoomed = ZOOM_NONE;

    SetupWindow (tmp_win, dragx - tmp_win->frame_bw, dragy - tmp_win->frame_bw,
		 dragWidth, dragHeight, -1);

    if (tmp_win->iconmgr)
    {
	tmp_win->iconmgrp->width = dragWidth;
        PackIconManager(tmp_win->iconmgrp);
    }

    if (!Scr->NoRaiseResize)
        XRaiseWindow(dpy, tmp_win->frame);

    UninstallRootColormap();

    ResizeWindow = None;
}

void
MenuEndResize(TwmWindow *tmp_win)
{
    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
    XUnmapWindow(dpy, Scr->SizeWindow.win);
    ConstrainSize (tmp_win, &dragWidth, &dragHeight);
    AddingX = dragx - tmp_win->frame_bw;
    AddingY = dragy - tmp_win->frame_bw;
    AddingW = dragWidth;/* + (2 * tmp_win->frame_bw);*/
    AddingH = dragHeight;/* + (2 * tmp_win->frame_bw);*/
    SetupWindow (tmp_win, AddingX, AddingY, AddingW, AddingH, -1);
}



/**
 * finish the resize operation for AddWindo<w
 */
void
AddEndResize(TwmWindow *tmp_win)
{

#ifdef DEBUG
    fprintf(stderr, "AddEndResize\n");
#endif

    ConstrainSize (tmp_win, &dragWidth, &dragHeight);
    AddingX = dragx - tmp_win->frame_bw;
    AddingY = dragy - tmp_win->frame_bw;
    AddingW = dragWidth + (2 * tmp_win->frame_bw);
    AddingH = dragHeight + (2 * tmp_win->frame_bw);
}

/**
 * adjust the given width and height to account for the constraints imposed 
 * by size hints.
 *
 *      The general algorithm, especially the aspect ratio stuff, is
 *      borrowed from uwm's CheckConsistency routine.
 */
void
ConstrainSize (TwmWindow *tmp_win, int *widthp, int *heightp)
{
#define makemult(a,b) ((b==1) ? (a) : (((int)((a)/(b))) * (b)) )
#define _min(a,b) (((a) < (b)) ? (a) : (b))

    int minWidth, minHeight, maxWidth, maxHeight, xinc, yinc, delta;
    int baseWidth, baseHeight;
    int dwidth = *widthp, dheight = *heightp;


    dheight -= tmp_win->title_height;

    if (tmp_win->hints.flags & PMinSize) {
        minWidth = tmp_win->hints.min_width;
        minHeight = tmp_win->hints.min_height;
    } else if (tmp_win->hints.flags & PBaseSize) {
        minWidth = tmp_win->hints.base_width;
        minHeight = tmp_win->hints.base_height;
    } else
        minWidth = minHeight = 1;

    if (tmp_win->hints.flags & PBaseSize) {
	baseWidth = tmp_win->hints.base_width;
	baseHeight = tmp_win->hints.base_height;
    } else if (tmp_win->hints.flags & PMinSize) {
	baseWidth = tmp_win->hints.min_width;
	baseHeight = tmp_win->hints.min_height;
    } else
	baseWidth = baseHeight = 0;


    if (tmp_win->hints.flags & PMaxSize) {
        maxWidth = _min (Scr->MaxWindowWidth, tmp_win->hints.max_width);
        maxHeight = _min (Scr->MaxWindowHeight, tmp_win->hints.max_height);
    } else {
        maxWidth = Scr->MaxWindowWidth;
	maxHeight = Scr->MaxWindowHeight;
    }

    if (tmp_win->hints.flags & PResizeInc) {
        xinc = tmp_win->hints.width_inc;
        yinc = tmp_win->hints.height_inc;
    } else
        xinc = yinc = 1;

    /*
     * First, clamp to min and max values
     */
    if (dwidth < minWidth) dwidth = minWidth;
    if (dheight < minHeight) dheight = minHeight;

    if (dwidth > maxWidth) dwidth = maxWidth;
    if (dheight > maxHeight) dheight = maxHeight;


    /*
     * Second, fit to base + N * inc
     */
    dwidth = ((dwidth - baseWidth) / xinc * xinc) + baseWidth;
    dheight = ((dheight - baseHeight) / yinc * yinc) + baseHeight;


    /*
     * Third, adjust for aspect ratio
     */
#define maxAspectX tmp_win->hints.max_aspect.x
#define maxAspectY tmp_win->hints.max_aspect.y
#define minAspectX tmp_win->hints.min_aspect.x
#define minAspectY tmp_win->hints.min_aspect.y
    /*
     * The math looks like this:
     *
     * minAspectX    dwidth     maxAspectX
     * ---------- <= ------- <= ----------
     * minAspectY    dheight    maxAspectY
     *
     * If that is multiplied out, then the width and height are
     * invalid in the following situations:
     *
     * minAspectX * dheight > minAspectY * dwidth
     * maxAspectX * dheight < maxAspectY * dwidth
     * 
     */
    
    if (tmp_win->hints.flags & PAspect)
    {
        if (minAspectX * dheight > minAspectY * dwidth)
        {
            delta = makemult(minAspectX * dheight / minAspectY - dwidth,
                             xinc);
            if (dwidth + delta <= maxWidth) dwidth += delta;
            else
            {
                delta = makemult(dheight - dwidth*minAspectY/minAspectX,
                                 yinc);
                if (dheight - delta >= minHeight) dheight -= delta;
            }
        }

        if (maxAspectX * dheight < maxAspectY * dwidth)
        {
            delta = makemult(dwidth * maxAspectY / maxAspectX - dheight,
                             yinc);
            if (dheight + delta <= maxHeight) dheight += delta;
            else
            {
                delta = makemult(dwidth - maxAspectX*dheight/maxAspectY,
                                 xinc);
                if (dwidth - delta >= minWidth) dwidth -= delta;
            }
        }
    }


    /*
     * Fourth, account for border width and title height
     */
    *widthp = dwidth;
    *heightp = dheight + tmp_win->title_height;
}


/**
 * set window sizes, this was called from either AddWindow, EndResize, or 
 * HandleConfigureNotify.
 *
 *  Special Considerations:
 * This routine will check to make sure the window is not completely off the 
 * display, if it is, it'll bring some of it back on.
 *
 * The tmp_win->frame_XXX variables should NOT be updated with the values of 
 * x,y,w,h prior to calling this routine, since the new values are compared 
 * against the old to see whether a synthetic ConfigureNotify event should be 
 * sent.  (It should be sent if the window was moved but not resized.)
 *
 *  \param tmp_win the TwmWindow pointer
 *  \param x       the x coordinate of the upper-left outer corner of the frame
 *  \param y       the y coordinate of the upper-left outer corner of the frame
 *  \param w       the width of the frame window w/o border
 *  \param h       the height of the frame window w/o border
 *  \param bw      the border width of the frame window or -1 not to change
 */
void SetupWindow (TwmWindow *tmp_win, int x, int y, int w, int h, int bw)
{
    SetupFrame (tmp_win, x, y, w, h, bw, False);
}

/**
 *  \param sendEvent whether or not to force a send
 */
void SetupFrame (TwmWindow *tmp_win, int x, int y, int w, int h, int bw, Bool sendEvent)
{
    XEvent client_event;
    XWindowChanges frame_wc, xwc;
    unsigned long frame_mask, xwcm;
    int title_width, title_height;
    int reShape;

#ifdef DEBUG
    fprintf (stderr, "SetupWindow: x=%d, y=%d, w=%d, h=%d, bw=%d\n",
	     x, y, w, h, bw);
#endif

    if (bw < 0)
      bw = tmp_win->frame_bw;		/* -1 means current frame width */

    if (tmp_win->iconmgr) {
	tmp_win->iconmgrp->width = w;
        h = tmp_win->iconmgrp->height + tmp_win->title_height;
    }

    /*
     * According to the July 27, 1988 ICCCM draft, we should send a
     * "synthetic" ConfigureNotify event to the client if the window
     * was moved but not resized.
     */
    if (((x != tmp_win->frame_x || y != tmp_win->frame_y) &&
	 (w == tmp_win->frame_width && h == tmp_win->frame_height)) ||
	(bw != tmp_win->frame_bw))
      sendEvent = TRUE;

    xwcm = CWWidth;
    title_width = xwc.width = w;
    title_height = Scr->TitleHeight + bw;

    ComputeWindowTitleOffsets (tmp_win, xwc.width, True);

    reShape = (tmp_win->wShaped ? TRUE : FALSE);
    if ((Scr->RoundedTitle == TRUE) || tmp_win->squeeze_info)		/* check for title shaping */
    {
	title_width = tmp_win->rightx + Scr->TBInfo.rightoff;
	if (title_width < xwc.width)
	{
	    xwc.width = title_width;
	    if (tmp_win->frame_height != h ||
	    	tmp_win->frame_width != w ||
		tmp_win->frame_bw != bw ||
	    	title_width != tmp_win->title_width)
	    	reShape = TRUE;
	}
	else
	{
	    if (!tmp_win->wShaped) reShape = TRUE;
	    title_width = xwc.width;
	}
    }

    tmp_win->title_width = title_width;
    if (tmp_win->title_height) tmp_win->title_height = title_height;

    if (tmp_win->title_w.win) {
	if (bw != tmp_win->frame_bw) {
	    xwc.border_width = bw;
	    tmp_win->title_x = xwc.x = -bw;
	    tmp_win->title_y = xwc.y = -bw;
	    xwcm |= (CWX | CWY | CWBorderWidth);
	}

	XConfigureWindow(dpy, tmp_win->title_w.win, xwcm, &xwc);
    }

    if (tmp_win->attr.width != w)
	tmp_win->widthEverChangedByUser = True;

    if (tmp_win->attr.height != (h - tmp_win->title_height))
	tmp_win->heightEverChangedByUser = True;

    tmp_win->attr.width = w;
    tmp_win->attr.height = h - tmp_win->title_height;

    XMoveResizeWindow (dpy, tmp_win->w, 0, tmp_win->title_height,
		       w, h - tmp_win->title_height);

    /* 
     * fix up frame and assign size/location values in tmp_win
     */
    frame_mask = 0;
    if (bw != tmp_win->frame_bw) {
	frame_wc.border_width = tmp_win->frame_bw = bw;
	frame_mask |= CWBorderWidth;
    }
    frame_wc.x = tmp_win->frame_x = x;
    frame_wc.y = tmp_win->frame_y = y;
    frame_wc.width = tmp_win->frame_width = w;
    frame_wc.height = tmp_win->frame_height = h;
    frame_mask |= (CWX | CWY | CWWidth | CWHeight);
    XConfigureWindow (dpy, tmp_win->frame, frame_mask, &frame_wc);

    /*
     * fix up highlight window
     */
    if (tmp_win->title_height && tmp_win->hilite_w)
    {
	xwc.width = (tmp_win->rightx - tmp_win->highlightx);
	if (Scr->TBInfo.nright > 0) xwc.width -= Scr->TitlePadding;
        if (xwc.width <= 0) {
            xwc.x = Scr->MyDisplayWidth;	/* move offscreen */
            xwc.width = 1;
        } else {
            xwc.x = tmp_win->highlightx;
        }

        xwcm = CWX | CWWidth;
        XConfigureWindow(dpy, tmp_win->hilite_w, xwcm, &xwc);
    }

    if (HasShape && reShape) {
	SetFrameShape (tmp_win);
    }

    if (sendEvent)
    {
        client_event.type = ConfigureNotify;
        client_event.xconfigure.display = dpy;
        client_event.xconfigure.event = tmp_win->w;
        client_event.xconfigure.window = tmp_win->w;
        client_event.xconfigure.x = (x + tmp_win->frame_bw - tmp_win->old_bw);
        client_event.xconfigure.y = (y + tmp_win->frame_bw +
				     tmp_win->title_height - tmp_win->old_bw);
        client_event.xconfigure.width = tmp_win->frame_width;
        client_event.xconfigure.height = tmp_win->frame_height -
                tmp_win->title_height;
        client_event.xconfigure.border_width = tmp_win->old_bw;
        /* Real ConfigureNotify events say we're above title window, so ... */
	/* what if we don't have a title ????? */
        client_event.xconfigure.above = tmp_win->frame;
        client_event.xconfigure.override_redirect = False;
        XSendEvent(dpy, tmp_win->w, False, StructureNotifyMask, &client_event);
    }

    /*
     * fix up warp-ring mouse coordinates
     */
    if (tmp_win->ring.curs_x < 0 || tmp_win->ring.curs_x >= tmp_win->frame_width ||
	tmp_win->ring.curs_y < (Scr->TitleFocus == TRUE ? 0 : tmp_win->title_height)
				|| tmp_win->ring.curs_y >= tmp_win->frame_height)
    {
	tmp_win->ring.curs_x = 2 * tmp_win->attr.width  / 3;
	tmp_win->ring.curs_y =     tmp_win->attr.height / 3 + tmp_win->title_height;
    }
}


void EnsureRectangleOnScreen (int *x0, int *y0, int w, int h)
{
    if ((*x0) + w > Scr->MyDisplayWidth)
	(*x0) = Scr->MyDisplayWidth - w;
    if ((*x0) < 0)
	(*x0) = 0;
    if ((*y0) + h > Scr->MyDisplayHeight)
	(*y0) = Scr->MyDisplayHeight - h;
    if ((*y0) < 0)
	(*y0) = 0;
}



#ifdef TILED_SCREEN

/*
 * Attention: in the following panelled screen Zoom functions 'top-edge'
 * is not towards the upper edge of the monitor but the edge having
 * greater y-coordinate.  I.e. in fact, it is towards the 'lower-edge'.
 * Imagine 'bot' having y0 and 'top' the y1 Cartesian coordinate,
 * and y0 <= y1 imposed.
 *
 * (Thinking of Lft/Bot/Rht/Top-edges as x0/y0/x1/y1-edges instead
 * is correct and probably less confusing.)
 */

void EnsureRectangleOnPanel (int panel, int *x0, int *y0, int w, int h)
{
    if ((*x0) + w > Rht(Scr->panels[panel])+1)
	(*x0) = Rht(Scr->panels[panel])+1 - w;
    if ((*x0) < Lft(Scr->panels[panel]))
	(*x0) = Lft(Scr->panels[panel]);
    if ((*y0) + h > Top(Scr->panels[panel])+1)
	(*y0) = Top(Scr->panels[panel])+1 - h;
    if ((*y0) < Bot(Scr->panels[panel]))
	(*y0) = Bot(Scr->panels[panel]);
}

void EnsureGeometryVisibility (int panel, int mask, int *x0, int *y0, int w, int h)
{
    int Area[4];

    /*
     * panel < 0 is 'full X11 screen'
     * panel = 0 is 'current panel'
     * panel > 0 is 'panel #'
     */
    --panel;
    if (panel >= 0 && panel < Scr->npanels)
    {
	Lft(Area) = Lft(Scr->panels[panel]);
	Bot(Area) = Bot(Scr->panels[panel]);
	Rht(Area) = Rht(Scr->panels[panel]);
	Top(Area) = Top(Scr->panels[panel]);
    }
    else
    {
	int x, y;

	x = (*x0);
	if (mask & XNegative)
	    x += Scr->MyDisplayWidth - w;
	y = (*y0);
	if (mask & YNegative)
	    y += Scr->MyDisplayHeight - h;

	Lft(Area) = x;
	Rht(Area) = x + w - 1;
	Bot(Area) = y;
	Top(Area) = y + h - 1;

	PanelsFullZoom (Area);
    }

    if (mask & XNegative)
	(*x0) += Rht(Area)+1 - w;
    else
	(*x0) += Lft(Area);
    if (mask & YNegative)
	(*y0) += Top(Area)+1 - h;
    else
	(*y0) += Bot(Area);

    /* final sanity check: */
    if ((*x0) + w > Rht(Area)+1)
	(*x0) = Rht(Area)+1 - w;
    if ((*x0) < Lft(Area))
	(*x0) = Lft(Area);
    if ((*y0) + h > Top(Area)+1)
	(*y0) = Top(Area)+1 - h;
    if ((*y0) < Bot(Area))
	(*y0) = Bot(Area);
}

static int
FindAreaIntersection (int a[4], int b[4])
{
    int dx, dy;

    dx = Distance1D(Lft(a), Rht(a), Lft(b), Rht(b));
    dy = Distance1D(Bot(a), Top(a), Bot(b), Top(b));
    return Distance2D(dx,dy);
}

int FindNearestPanelToArea (int w[4])
{
    /*
     * Find the panel having maximum overlap with 'w'
     * (or, if 'w' is outside of all panels, a panel with minimum
     * distance to 'w').
     */
    int k, a, m, i;

    m = -xmax(Scr->MyDisplayWidth, Scr->MyDisplayHeight); /*some large value*/
    i = 0;
    for (k = 0; k < Scr->npanels; ++k) {
	a = FindAreaIntersection (w, Scr->panels[k]);
	if (m < a)
	    m = a, i = k;
    }
    return i;
}

int FindNearestPanelToPoint (int x, int y)
{
    int k, a, m, i, w[4];

    Lft(w) = Rht(w) = x;
    Bot(w) = Top(w) = y;

    i = 0;
    m = -xmax(Scr->MyDisplayWidth, Scr->MyDisplayHeight);
    for (k = 0; k < Scr->npanels; ++k) {
	a = FindAreaIntersection (w, Scr->panels[k]);
	if (m < a)
	    m = a, i = k;
    }
    return i;
}

int FindNearestPanelToClient (TwmWindow *tmp)
{
    int w[4];

    Lft(w) = tmp->frame_x;
    Rht(w) = tmp->frame_x + tmp->frame_width  + 2*tmp->frame_bw - 1;
    Bot(w) = tmp->frame_y;
    Top(w) = tmp->frame_y + tmp->frame_height + 2*tmp->frame_bw - 1;
    return FindNearestPanelToArea (w);
}

int FindNearestPanelToMouse (void)
{
    if (False == XQueryPointer (dpy, Scr->Root, &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &HotX, &HotY, &JunkMask))
	/* emergency: mouse not on current X11-screen */
	return 0;
    else
	return FindNearestPanelToPoint (JunkX, JunkY);
}

#define IntersectsV(a,b,t)  (((Bot(a)) <= (t)) && ((b) <= (Top(a))))
#define IntersectsH(a,l,r)  (((Lft(a)) <= (r)) && ((l) <= (Rht(a))))
#define OverlapsV(a,w)	    IntersectsV(a,Bot(w),Top(w))
#define OverlapsH(a,w)	    IntersectsH(a,Lft(w),Rht(w))
#define ContainsV(a,p)	    IntersectsV(a,p,p)
#define ContainsH(a,p)	    IntersectsH(a,p,p)

static int BoundingBoxLeft (int w[4])
{
    int i, k;

    i = -1; /* value '-1' is 'not found' */
    /* zoom out: */
    for (k = 0; k < Scr->npanels; ++k)
	/* consider panels vertically overlapping the window "height" (along left edge): */
	if (OverlapsV(Scr->panels[k], w))
	    /* consider panels horizontally intersecting the window left edge: */
	    if (ContainsH(Scr->panels[k], Lft(w))) {
		if ((i == -1) || (Lft(Scr->panels[k]) < Lft(Scr->panels[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting panels found): */
	for (k = 0; k < Scr->npanels; ++k)
	    if (OverlapsV(Scr->panels[k], w))
		/* consider panels having left edge right to the left edge of the window: */
		if (Lft(Scr->panels[k]) > Lft(w)) {
		    if ((i == -1) || (Lft(Scr->panels[k]) < Lft(Scr->panels[i])))
			i = k;
		}
    return i;
}

static int BoundingBoxBottom (int w[4])
{
    int i, k;

    i = -1;
    /* zoom out: */
    for (k = 0; k < Scr->npanels; ++k)
	/* consider panels horizontally overlapping the window "width" (along bottom edge): */
	if (OverlapsH(Scr->panels[k], w))
	    /* consider panels vertically interscting the window bottom edge: */
	    if (ContainsV(Scr->panels[k], Bot(w))) {
		if ((i == -1) || (Bot(Scr->panels[k]) < Bot(Scr->panels[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting panels found): */
	for (k = 0; k < Scr->npanels; ++k)
	    if (OverlapsH(Scr->panels[k], w))
		/* consider panels having bottom edge ontop of the bottom edge of the window: */
		if (Bot(Scr->panels[k]) > Bot(w)) {
		    if ((i == -1) || (Bot(Scr->panels[k]) < Bot(Scr->panels[i])))
			i = k;
		}
    return i;
}

static int BoundingBoxRight (int w[4])
{
    int i, k;

    i = -1;
    /* zoom out: */
    for (k = 0; k < Scr->npanels; ++k)
	/* consider panels vertically overlapping the window "height" (along right edge): */
	if (OverlapsV(Scr->panels[k], w))
	    /* consider panels horizontally intersecting the window right edge: */
	    if (ContainsH(Scr->panels[k], Rht(w))) {
		if ((i == -1) || (Rht(Scr->panels[k]) > Rht(Scr->panels[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting panels found): */
	for (k = 0; k < Scr->npanels; ++k)
	    if (OverlapsV(Scr->panels[k], w))
		/* consider panels having right edge left to the right edge of the window: */
		if (Rht(Scr->panels[k]) < Rht(w)) {
		    if ((i == -1) || (Rht(Scr->panels[k]) > Rht(Scr->panels[i])))
			i = k;
		}
    return i;
}

static int BoundingBoxTop (int w[4])
{
    int i, k;

    i = -1;
    /* zoom out: */
    for (k = 0; k < Scr->npanels; ++k)
	/* consider panels horizontally overlapping the window "width" (along top edge): */
	if (OverlapsH(Scr->panels[k], w))
	    /* consider panels vertically interscting the window top edge: */
	    if (ContainsV(Scr->panels[k], Top(w))) {
		if ((i == -1) || (Top(Scr->panels[k]) > Top(Scr->panels[i])))
		    i = k;
	    }
    if (i == -1)
	/* zoom in (no intersecting panels found): */
	for (k = 0; k < Scr->npanels; ++k)
	    if (OverlapsH(Scr->panels[k], w))
		/* consider panels having top edge below of the top edge of the window: */
		if (Top(Scr->panels[k]) < Top(w)) {
		    if ((i == -1) || (Top(Scr->panels[k]) > Top(Scr->panels[i])))
			i = k;
		}
    return i;
}

static int UncoveredLeft (int skip, int area[4], int thresh)
{
    auto int u, e, k, w[4];

    u = Top(area) - Bot(area);
    e = 0;
    for (k = 0; k < Scr->npanels && e == 0; ++k)
	if (k != skip)
	    /* consider panels vertically intersecting area: */
	    if (IntersectsV(Scr->panels[k], Bot(area), Top(area)))
		/* consider panels horizontally intersecting boundingbox and outside window
		 * (but touches the window on its left):
		 */
		if (ContainsH(Scr->panels[k], Lft(area)-thresh)) {
		    if (Top(Scr->panels[k]) < Top(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Top(Scr->panels[k]) + 1;
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredLeft (skip, w, thresh);
			e = 1;
		    }
		    if (Bot(Scr->panels[k]) > Bot(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Bot(Scr->panels[k]) - 1;
			if (e == 0)
			    u  = UncoveredLeft (skip, w, thresh);
			else
			    u += UncoveredLeft (skip, w, thresh);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomLeft (int bbl, int b, int t, int thresv, int thresh)
{
    int i, k, u;

    i = bbl;
    for (k = 0; k < Scr->npanels; ++k)
	if (k != bbl)
	    /* consider panels vertically intersecting bbl: */
	    if (IntersectsV(Scr->panels[k], b, t))
		/* consider panels having left edge in the bounding region
		 * and to the right of the current best panel 'i':
		 */
		if (ContainsH(Scr->panels[bbl], Lft(Scr->panels[k]))
			&& (Lft(Scr->panels[i]) < Lft(Scr->panels[k])))
		{
		    u = UncoveredLeft (k, Scr->panels[k], thresh);
		    if (u > thresv)
			i = k;
		}
    return i;
}

static int UncoveredBottom (int skip, int area[4], int thresv)
{
    auto int u, e, k, w[4];

    u = Rht(area) - Lft(area);
    e = 0;
    for (k = 0; k < Scr->npanels && e == 0; ++k)
	if (k != skip)
	    /* consider panels horizontally intersecting area: */
	    if (IntersectsH(Scr->panels[k], Lft(area), Rht(area)))
		/* consider panels vertically intersecting boundingbox and outside window
		 * (but touches the window to its bottom):
		 */
		if (ContainsV(Scr->panels[k], Bot(area)-thresv)) {
		    if (Rht(Scr->panels[k]) < Rht(area)) {
			Lft(w) = Rht(Scr->panels[k]) + 1;
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredBottom (skip, w, thresv);
			e = 1;
		    }
		    if (Lft(Scr->panels[k]) > Lft(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Lft(Scr->panels[k]) - 1;
			Top(w) = Top(area);
			if (e == 0)
			    u  = UncoveredBottom (skip, w, thresv);
			else
			    u += UncoveredBottom (skip, w, thresv);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomBottom (int bbb, int l, int r, int thresh, int thresv)
{
    int i, k, u;

    i = bbb;
    for (k = 0; k < Scr->npanels; ++k)
	if (k != bbb)
	    /* consider panels horizontally intersecting bbb: */
	    if (IntersectsH(Scr->panels[k], l, r))
		/* consider panels having bottom edge in the bounding region
		 * and above the current best panel 'i':
		 */
		if (ContainsV(Scr->panels[bbb], Bot(Scr->panels[k]))
			&& (Bot(Scr->panels[i]) < Bot(Scr->panels[k])))
		{
		    u = UncoveredBottom (k, Scr->panels[k], thresv);
		    if (u > thresh)
			i = k;
		}
    return i;
}

static int UncoveredRight(int skip, int area[4], int thresh)
{
    auto int u, e, k, w[4];

    u = Top(area) - Bot(area);
    e = 0;
    for (k = 0; k < Scr->npanels && e == 0; ++k)
	if (k != skip)
	    /* consider panels vertically intersecting area: */
	    if (IntersectsV(Scr->panels[k], Bot(area), Top(area)))
		/* consider panels horizontally intersecting boundingbox and outside window
		 * (but touches the window on its right):
		 */
		if (ContainsH(Scr->panels[k], Rht(area)+thresh)) {
		    if (Top(Scr->panels[k]) < Top(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Top(Scr->panels[k]) + 1;
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredRight (skip, w, thresh);
			e = 1;
		    }
		    if (Bot(Scr->panels[k]) > Bot(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Bot(Scr->panels[k]) - 1;
			if (e == 0)
			    u  = UncoveredRight (skip, w, thresh);
			else
			    u += UncoveredRight (skip, w, thresh);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomRight (int bbr, int b, int t, int thresv, int thresh)
{
    int i, k, u;

    i = bbr;
    for (k = 0; k < Scr->npanels; ++k)
	if (k != bbr)
	    /* consider panels vertically intersecting bbr: */
	    if (IntersectsV(Scr->panels[k], b, t))
		/* consider panels having right edge in the bounding region
		 * and to the left of the current best panel 'i':
		 */
		if (ContainsH(Scr->panels[bbr], Rht(Scr->panels[k]))
			&& (Rht(Scr->panels[k]) < Rht(Scr->panels[i])))
		{
		    u = UncoveredRight (k, Scr->panels[k], thresh);
		    if (u > thresv)
			i = k;
		}
    return i;
}

static int UncoveredTop (int skip, int area[4], int thresv)
{
    auto int u, e, k, w[4];

    u = Rht(area) - Lft(area);
    e = 0;
    for (k = 0; k < Scr->npanels && e == 0; ++k)
	if (k != skip)
	    /* consider panels horizontally intersecting area: */
	    if (IntersectsH(Scr->panels[k], Lft(area), Rht(area)))
		/* consider panels vertically intersecting boundingbox and outside window
		 * (but touches the window on its top):
		 */
		if (ContainsV(Scr->panels[k], Top(area)+thresv)) {
		    if (Rht(Scr->panels[k]) < Rht(area)) {
			Lft(w) = Rht(Scr->panels[k]) + 1;
			Bot(w) = Bot(area);
			Rht(w) = Rht(area);
			Top(w) = Top(area);
			u = UncoveredTop (skip, w, thresv);
			e = 1;
		    }
		    if (Lft(Scr->panels[k]) > Lft(area)) {
			Lft(w) = Lft(area);
			Bot(w) = Bot(area);
			Rht(w) = Lft(Scr->panels[k]) - 1;
			Top(w) = Top(area);
			if (e == 0)
			    u  = UncoveredTop (skip, w, thresv);
			else
			    u += UncoveredTop (skip, w, thresv);
			e = 1;
		    }
		    if (e == 0) {
			u = 0;
			e = 1;
		    }
		}
    return u;
}

static int ZoomTop (int bbt, int l, int r, int thresh, int thresv)
{
    int i, k, u;

    i = bbt;
    for (k = 0; k < Scr->npanels; ++k)
	if (k != bbt)
	    /* consider panels horizontally intersecting bbt: */
	    if (IntersectsH(Scr->panels[k], l, r))
		/* consider panels having top edge in the bounding region
		 * and below the current best panel 'i':
		 */
		if (ContainsV(Scr->panels[bbt], Top(Scr->panels[k]))
			&& (Top(Scr->panels[k]) < Top(Scr->panels[i])))
		{
		    u = UncoveredTop (k, Scr->panels[k], thresv);
		    if (u > thresh)
			i = k;
		}
    return i;
}

void PanelsFullZoom (int Area[4])
{
    /*
     * First step: find screen panels (which overlap the 'Area')
     * which define a "bounding box of maximum area".
     * The following 4 functions return indices to these panels.
     */

    int l, b, r, t, f;

    f = 0;

nxt:;

    l = BoundingBoxLeft (Area);
    b = BoundingBoxBottom (Area);
    r = BoundingBoxRight (Area);
    t = BoundingBoxTop (Area);

    if (l >= 0 && b >=0 && r >= 0 && t >= 0) {
	/*
	 * Second step: take the above bounding box and move its borders
	 * inwards minimising the total area of "dead areas" (i.e. X11-screen
	 * regions not covered by any panels) along borders.
	 *    The following zoom functions return indices to panels
	 * defining the reduced area boundaries. (If panels cover a
	 * "compact area" (i.e. which does not have "slits") then the region
	 * found here has no dead areas as well.)
	 *    In general there may be slits between panels and in
	 * following thresh/thresv define tolerances (slit widths, cumulative
	 * uncovered border) which define "coverage defects" not considered
	 * as "dead areas".
	 *    This treatment is considered only along reduced bounding box
	 * boundaries and not in the "inner area". (So a panel
	 * arrangement could cover e.g. a circular area and a reduced bounding
	 * box is found without dead areas on borders but the inside of the
	 * arrangement can include any number of topological holes, twists
	 * and knots.)
	 */
	/*
	 * thresh - width of a vertical slit between two horizontally
	 *          adjacent panels not considered as a "dead area".
	 * thresv - height of a horizontal slit between two vertically
	 *          adjacent panels not considered as a "dead area".
	 *
	 * Or respectively how much uncovered border is not considered
	 * "uncovered"/"dead" area.
	 */

	int i, j;

	i = Bot(Scr->panels[b]);
	if (i < Bot(Area))
	    i += Bot(Area), i /= 2;
	j = Top(Scr->panels[t]);
	if (j > Top(Area))
	    j += Top(Area), j /= 2;
	l = ZoomLeft (l, i, j, /*uncovered*/10, /*slit*/10);

	i = Lft(Scr->panels[l]);
	if (i < Lft(Area))
	    i += Lft(Area), i /= 2;
	j = Rht(Scr->panels[r]);
	if (j > Rht(Area))
	    j += Rht(Area), j /= 2;
	b = ZoomBottom (b, i, j, 10, 10);

	i = Bot(Scr->panels[b]);
	if (i < Bot(Area))
	    i += Bot(Area), i /= 2;
	j = Top(Scr->panels[t]);
	if (j > Top(Area))
	    j += Top(Area), j /= 2;
	r = ZoomRight (r, i, j, 10, 10);

	i = Lft(Scr->panels[l]);
	if (i < Lft(Area))
	    i += Lft(Area), i /= 2;
	j = Rht(Scr->panels[r]);
	if (j > Rht(Area))
	    j += Rht(Area), j /= 2;
	t = ZoomTop (t, i, j, 10, 10);

	if (l >= 0 && b >=0 && r >= 0 && t >= 0) {
	    Lft(Area) = Lft(Scr->panels[l]);
	    Bot(Area) = Bot(Scr->panels[b]);
	    Rht(Area) = Rht(Scr->panels[r]);
	    Top(Area) = Top(Scr->panels[t]);
	    return;
	}
    }

    /* fallback: */
    Lft(Area) = Lft(Scr->panels_bb);
    Bot(Area) = Bot(Scr->panels_bb);
    Rht(Area) = Rht(Scr->panels_bb);
    Top(Area) = Top(Scr->panels_bb);

    if (f == 0) {
	f = 1;
	goto nxt;
    }
}

#endif /*TILED_SCREEN*/



/**
 * zooms window to full height of screen or to full height and width of screen. 
 * (Toggles so that it can undo the zoom - even when switching between fullzoom
 * and vertical zoom.)
 *
 *  \param tmp_win  the TwmWindow pointer
 */
void
fullzoom (int panel, TwmWindow *tmp_win, int flag)
{
    Bool mm = False;

    if (tmp_win->zoomed == flag
	    && flag != F_PANELGEOMETRYZOOM && flag != F_PANELGEOMETRYMOVE)
    {
	unzoom:;

	dragHeight = tmp_win->save_frame_height;
	dragWidth = tmp_win->save_frame_width;
	dragx = tmp_win->save_frame_x;
	dragy = tmp_win->save_frame_y;
	tmp_win->zoomed = ZOOM_NONE;
    }
    else
    {
	int Area[4], basex, basey, basew, baseh, dx, dy, frame_bw_times_2;

	XGetGeometry (dpy, (Drawable)tmp_win->frame, &JunkRoot,
		    &dragx, &dragy, (unsigned int *)&dragWidth, (unsigned int *)&dragHeight,
		    &JunkBW, &JunkDepth);

	frame_bw_times_2 = (int)(JunkBW) * 2;

#ifdef TILED_SCREEN

	/*
	 * panel-zoom variants (F_PANEL...ZOOM) have their numeric
	 * codes greater than F_MAXIMIZE.  See also parse.h.
	 */
	if (Scr->use_panels == TRUE && flag > F_MAXIMIZE && panel >= 0)
	{
	    /*
	     * On fullzoom() entry:
	     * panel < 0 is 'zoom on X11 logical screen' (denoted by "0" in .twmrc)
	     * panel = 0 is 'zoom on current panel(s)'   (denoted by "." in .twmrc)
	     * panel > 0 is 'zoom on panel #'  (counting in .twmrc starts from "1")
	     */
	    panel--;
	    if (panel >= 0 && panel < Scr->npanels) {
		Lft(Area) = Lft(Scr->panels[panel]);
		Bot(Area) = Bot(Scr->panels[panel]);
		Rht(Area) = Rht(Scr->panels[panel]);
		Top(Area) = Top(Scr->panels[panel]);
	    } else {
		/*
		 * fallback: compute maximum area across
		 * tiles the window initially intersects
		 */
		Lft(Area) = dragx;
		Rht(Area) = dragx + frame_bw_times_2 + dragWidth  - 1;
		Bot(Area) = dragy;
		Top(Area) = dragy + frame_bw_times_2 + dragHeight - 1;
		PanelsFullZoom (Area);
	    }
	    basex = Lft(Area); /* Cartesian origin:   */
	    basey = Bot(Area); /* (x0,y0) = (Lft,Bot) */
	    basew = AreaWidth(Area);
	    baseh = AreaHeight(Area);
	}
	else
#endif
	{
	    basex = 0;
	    basey = 0;
	    basew = Scr->MyDisplayWidth;
	    baseh = Scr->MyDisplayHeight;
	}

	if (tmp_win->zoomed == ZOOM_NONE)
	{
	    tmp_win->save_frame_x = dragx;
	    tmp_win->save_frame_y = dragy;
	    tmp_win->save_frame_width = dragWidth;
	    tmp_win->save_frame_height = dragHeight;
	}

	switch (flag)
        {
        case ZOOM_NONE:
            break;

	case F_PANELZOOM:
        case F_ZOOM:
	    dragHeight = baseh - frame_bw_times_2;
            dragy=basey;
            break;

	case F_PANELHORIZOOM:
        case F_HORIZOOM:
            dragx = basex;
	    dragWidth = basew - frame_bw_times_2;
            break;

	case F_PANELFULLZOOM:
	case F_PANELMAXIMIZE:
        case F_FULLZOOM:
        case F_MAXIMIZE:
            dragx = basex;
            dragy = basey;
	    dragHeight = baseh;
	    dragWidth = basew;
	    if (flag == F_FULLZOOM || flag == F_PANELFULLZOOM) {
		dragHeight -= frame_bw_times_2;
		dragWidth  -= frame_bw_times_2;
	    } else {
		dragx -= (int)(JunkBW);
		dragy -= (int)(JunkBW) + tmp_win->title_height;
		dragHeight += tmp_win->title_height;
	    }
            break;

	case F_PANELLEFTZOOM:
        case F_LEFTZOOM:
            dragx = basex;
            dragy = basey;
	    dragHeight = baseh - frame_bw_times_2;
	    dragWidth = basew/2 - frame_bw_times_2;
            break;

	case F_PANELRIGHTZOOM:
        case F_RIGHTZOOM:
            dragy = basey;
	    dragx = basex + basew/2;
	    dragHeight = baseh - frame_bw_times_2;
	    dragWidth = basew/2 - frame_bw_times_2;
            break;

	case F_PANELTOPZOOM:
        case F_TOPZOOM:
            dragx = basex;
            dragy = basey;
	    dragHeight = baseh/2 - frame_bw_times_2;
	    dragWidth = basew - frame_bw_times_2;
            break;

	case F_PANELBOTTOMZOOM:
        case F_BOTTOMZOOM:
            dragx = basex;
	    dragy = basey + baseh/2;
	    dragHeight = baseh/2 - frame_bw_times_2;
	    dragWidth = basew - frame_bw_times_2;
            break;

	case F_PANELLEFTMOVE:
	    dragx = basex;
	    break;

	case F_PANELRIGHTMOVE:
	    dragx = basex + basew - dragWidth - frame_bw_times_2;
	    break;

	case F_PANELTOPMOVE:
	    dragy = basey;
	    break;

	case F_PANELBOTTOMMOVE:
	    dragy = basey + baseh - dragHeight - frame_bw_times_2;
	    break;


	case F_PANELGEOMETRYZOOM:
	case F_PANELGEOMETRYMOVE:

	    /*
	     * mask and requested geometry are precomputed in fullgeomzoom()
	     * as origMask, origx, origy, origWidth, origHeight
	     */

	    if (((origMask & XValue) != 0) && (origx == 0) && ((origMask & WidthValue) == 0)
		&& ((origMask & YValue) != 0) && (origy == 0) && ((origMask & HeightValue) == 0))
	    {
		/*
		 * special cases: geometry "+0+0" or "-0-0" (i.e. missing WxH part)
		 * recover size/pos, set 'unzoomed'
		 */
		flag = 1;
		if (origMask & XNegative) { /* recover horizontal geometry */
		    dragWidth = tmp_win->save_frame_width;
		    dragx = tmp_win->save_frame_x;
		    flag = 0;
		} else {
		    tmp_win->save_frame_width = dragWidth;
		    tmp_win->save_frame_x = dragx;
		}
		if (origMask & YNegative) { /* recover vertical geometry */
		    dragHeight = tmp_win->save_frame_height;
		    dragy = tmp_win->save_frame_y;
		    flag = 0;
		} else {
		    tmp_win->save_frame_height = dragHeight;
		    tmp_win->save_frame_y = dragy;
		}

		tmp_win->zoomed = ZOOM_NONE; /* set state to 'unzoomed' */
		if (flag == 1)
		    return;

		flag = ZOOM_NONE;
		break;
	    }

	    mm = XQueryPointer (dpy, tmp_win->frame, &JunkRoot, &JunkChild,
				&JunkX, &JunkY, &HotX, &HotY, &JunkMask);

	    /* check if mouse on target area */
	    if (mm == True
		    && basex <= JunkX && JunkX < basex + basew
		    && basey <= JunkY && JunkY < basey + baseh)
	    {
		if ((origx == 0 && origy == 0)
			|| HotX < -(int)(JunkBW) || HotX >= dragWidth  + (int)(JunkBW)
			|| HotY < -(int)(JunkBW) || HotY >= dragHeight + (int)(JunkBW))
		{
		    mm = False;
		}
	    }
	    else
		mm = False;

	    dx = dragx - basex;
	    dy = dragy - basey;

#ifdef TILED_SCREEN

	    if (dx < 0 || basew < dx + dragWidth + frame_bw_times_2
		|| dy < 0 || baseh < dy + dragHeight + frame_bw_times_2)
	    {
		/*
		 * target area/panel is (partially) outside the source area/panel:
		 * recompute dragx, dragy
		 */
		if (dragWidth + frame_bw_times_2 < basew) {
		    if (dragHeight + frame_bw_times_2 < baseh) {
			/* window completely fits onto target panel */
			int k, a;
			Lft(Area) = dragx;
			Rht(Area) = dragx + frame_bw_times_2 + dragWidth  - 1;
			Bot(Area) = dragy;
			Top(Area) = dragy + frame_bw_times_2 + dragHeight - 1;
			k = FindNearestPanelToArea (Area);
			a = FindAreaIntersection (Area, Scr->panels[k]);
			if (a == (dragWidth+frame_bw_times_2) * (dragHeight+frame_bw_times_2)) {
			    /* completely fitted on source panel, move to target panel, keep relative location */
			    dragx = basex + (dragx - Lft(Scr->panels[k])) * (basew - frame_bw_times_2 - dragWidth)
					/ (AreaWidth(Scr->panels[k])  - frame_bw_times_2 - dragWidth);
			    dragy = basey + (dragy - Bot(Scr->panels[k])) * (baseh - frame_bw_times_2 - dragHeight)
					/ (AreaHeight(Scr->panels[k]) - frame_bw_times_2 - dragHeight);
			} else {
			    /* intersected various panels in source area */
#if 1
			    PlaceXY area;
			    area.x = basex;
			    area.y = basey;
			    area.width = basew;
			    area.height = baseh;
			    area.next = NULL;
			    if (FindEmptyArea(Scr->TwmRoot.next, tmp_win, &area, &area) == TRUE)
			    {
				/* found emtpy area large enough to completely fit the window */
				int x, y, b = Scr->TitleHeight + (int)(JunkBW);
				/* slightly off-centered: */
				x = (area.width  - dragWidth)  / 3;
				y = (area.height - dragHeight) / 2;
				/* tight placing: */
				if (y < b && x > b)
				    x = b;
				if (x < b && y > b)
				    y = b;
				/* loosen placing: */
				if (area.width  > 4*b + dragWidth  && x > 2*b)
				    x = 2*b;
				if (area.height > 4*b + dragHeight && y > 2*b)
				    y = 2*b;
				dragx = area.x + x;
				dragy = area.y + y;
			    }
			    else
#endif
			    {
				/* empty area not found, put centered onto target panel */
				dragx = basex + (basew - dragWidth  - frame_bw_times_2) / 2;
				dragy = basey + (baseh - dragHeight - frame_bw_times_2) / 2;
			    }
			}
		    } else {
			/* vertically not fitting onto target panel */
			if ((tmp_win->hints.flags & PWinGravity)
				&& (tmp_win->hints.win_gravity == SouthWestGravity
				    || tmp_win->hints.win_gravity == SouthGravity
				    || tmp_win->hints.win_gravity == SouthEastGravity))
			{
			    /* align to panel lower edge */
			    dragy = basey + (baseh - dragHeight - frame_bw_times_2);
			    basey = dragy;
			} else
			    /* align to panel upper edge */
			    dragy = basey;
			if (dx < 0 || basew < dx + dragWidth + frame_bw_times_2)
			    /* fitting, but out of screen, put horizontally centered */
			    dragx = basex + (basew - dragWidth  - frame_bw_times_2) / 2;
		    }
		} else {
		    /* horizontally not fitting onto target panel */
		    if ((tmp_win->hints.flags & PWinGravity)
			    && (tmp_win->hints.win_gravity == NorthEastGravity
				|| tmp_win->hints.win_gravity == EastGravity
				|| tmp_win->hints.win_gravity == SouthEastGravity))
		    {
			/* align to panel right edge */
			dragx = basex + (basew - dragWidth - frame_bw_times_2);
			basex = dragx;
		    } else
			/* align to panel left edge */
			dragx = basex;

		    /* check fitting vertically */
		    if (dragHeight + frame_bw_times_2 < baseh) {
			if (dy < 0 || baseh < dy + dragHeight + frame_bw_times_2)
			    /* fitting, but out of screen, put vertically centered */
			    dragy = basey + (baseh - dragHeight - frame_bw_times_2) / 2;
		    } else {
			/* no, vertically not fitting onto target panel */
			if ((tmp_win->hints.flags & PWinGravity)
				&& (tmp_win->hints.win_gravity == SouthWestGravity
				    || tmp_win->hints.win_gravity == SouthGravity
				    || tmp_win->hints.win_gravity == SouthEastGravity))
			{
			    /* align to panel lower edge */
			    dragy = basey + (baseh - dragHeight - frame_bw_times_2);
			    basey = dragy;
			} else
			    /* align to panel upper edge */
			    dragy = basey;
		    }
		}

		dx = dragx - basex;
		dy = dragy - basey;
	    }

#endif /*TILED_SCREEN*/

	    /* now finally treat horizontal geometry ('W' and 'X' of "WxH+X+Y"): */
	    if (((origMask & (XValue|WidthValue)) == (XValue|WidthValue)) && (origWidth > 0))
	    {
#if 0
		/* stepping/stretching reached panel edge, execute 'unzoom' */
		if ((tmp_win->zoomed != ZOOM_NONE)
			&& origx == 0 && (dx == 0 || dx + dragWidth + frame_bw_times_2 == basew))
		    goto unzoom;
#endif
		if (origMask & XNegative) { /** step/stretch to the left **/
		    if (origx != 0 && -origx*origWidth < dx) { /* "WxH-X+Y" (X != 0) */
			/* step/stretch inside panel area */
			if (flag == F_PANELGEOMETRYZOOM)
			    dragWidth -= origx*origWidth;
			dragx += origx*origWidth;
			/* keep window right edge fixed */
			JunkWidth = dragWidth;
			JunkHeight = dragHeight;
			ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
			if (dragWidth != JunkWidth)
			    dragx += dragWidth - JunkWidth;
		    } else { /* geometry is "WxH-0+Y", or */
			/* step/stretch to/across panel left edge */
			if (flag == F_PANELGEOMETRYZOOM)
			    dragWidth += dx;
			dragx = basex;
		    }
		} else { /** step/stretch to the right **/
		    if (origx != 0 && dx+dragWidth + origx*origWidth + frame_bw_times_2 < basew) { /* "WxH+X+Y" (X != 0) */
			/* step/stretch inside panel area */
			if (flag == F_PANELGEOMETRYZOOM)
			    dragWidth += origx*origWidth;
			else
			    dragx += origx*origWidth;
		    } else { /* geometry is "WxH+0+Y", or */
			/* step/stretch to/across panel right edge */
			if (flag == F_PANELGEOMETRYZOOM)
			    dragWidth = basew - dx - frame_bw_times_2;
			else
			    dragx = basew - dragWidth + basex - frame_bw_times_2;
			/* fix window right edge at panel edge */
			JunkWidth = dragWidth;
			JunkHeight = dragHeight;
			ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
			if (dragWidth != JunkWidth)
			    dragx += dragWidth - JunkWidth;
		    }
		}
	    }

	    /* treat vertical geometry ('H' and 'Y' of "WxH+X+Y"): */
	    if (((origMask & (YValue|HeightValue)) == (YValue|HeightValue)) && (origHeight > 0))
	    {
#if 0
		if ((tmp_win->zoomed != ZOOM_NONE)
			&& origy == 0 && (dy == 0 || dy + dragHeight + frame_bw_times_2 == baseh))
		    goto unzoom;
#endif
		if (origMask & YNegative) { /* step/stretch up */
		    if (origy != 0 && -origy*origHeight < dy) {
			if (flag == F_PANELGEOMETRYZOOM)
			    dragHeight -= origy*origHeight;
			dragy += origy*origHeight;
			/* keep window bottom edge fixed */
			JunkWidth = dragWidth;
			JunkHeight = dragHeight;
			ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
			if (dragHeight != JunkHeight)
			    dragy += dragHeight - JunkHeight;
		    } else {
			if (flag == F_PANELGEOMETRYZOOM)
			    dragHeight += dy;
			dragy = basey;
		    }
		} else { /* step/stretch down */
		    if (origy != 0 && dy+dragHeight + origy*origHeight + frame_bw_times_2 < baseh) {
			if (flag == F_PANELGEOMETRYZOOM)
			    dragHeight += origy*origHeight;
			else
			    dragy += origy*origHeight;
		    } else {
			if (flag == F_PANELGEOMETRYZOOM)
			    dragHeight = baseh - dy - frame_bw_times_2;
			else
			    dragy = baseh - dragHeight + basey - frame_bw_times_2;
			/* fix window bottom edge at panel edge */
			JunkWidth = dragWidth;
			JunkHeight = dragHeight;
			ConstrainSize (tmp_win, &JunkWidth, &JunkHeight);
			if (dragHeight != JunkHeight)
			    dragy += dragHeight - JunkHeight;
		    }
		}
	    }
            break;
	}

	tmp_win->zoomed = flag;
    }

    if (!Scr->NoRaiseResize)
        XRaiseWindow(dpy, tmp_win->frame);

    ConstrainSize(tmp_win, &dragWidth, &dragHeight);
    SetupWindow (tmp_win, dragx , dragy , dragWidth, dragHeight, -1);

#if 1
    if (mm == True && flag == F_PANELGEOMETRYMOVE)
	/* if mouse did/would fall inside the window, move mouse as well */
	XWarpPointer (dpy, None, tmp_win->frame, 0, 0, 0, 0, HotX, HotY);
#endif
}


void
fullgeomzoom (char *geometry_name, TwmWindow *tmp_win, int flag)
{
    int panel;
    char *name;
    char geom[200];

    strncpy (geom, geometry_name, sizeof(geom)-1); /*work on a copy*/
    geom[sizeof(geom)-1] = '\0';
    name = strchr (geom, '@');
    if (name != NULL)
	*name++ = '\0';
    panel = ParsePanelIndex (name);

    /*
     * precompute the mask and requested geometry for fullzoom()
     * as origMask, origx, origy, origWidth, origHeight
     */
    origx = origy = origWidth = origHeight = 0;
    origMask = XParseGeometry (geom, &origx, &origy,
			(unsigned int *)&origWidth, (unsigned int *)&origHeight);

    fullzoom (panel, tmp_win, flag);
}


void
SetFrameShape (TwmWindow *tmp)
{
    int fbw2 = 2 * tmp->frame_bw;

    /*
     * see if the titlebar needs to move
     */
    if (tmp->title_w.win) {
	int oldx = tmp->title_x, oldy = tmp->title_y;
	ComputeTitleLocation (tmp);
	if (oldx != tmp->title_x || oldy != tmp->title_y)
	  XMoveWindow (dpy, tmp->title_w.win, tmp->title_x, tmp->title_y);
    }

    /*
     * The frame consists of the shape of the contents window offset by
     * title_height or'ed with the shape of title_w.win (which is always
     * rectangular).
     */
    if (tmp->wShaped) {
	/*
	 * need to do general case
	 */
	XShapeCombineShape (dpy, tmp->frame, ShapeBounding,
			    0, tmp->title_height, tmp->w,
			    ShapeBounding, ShapeSet);
	if (tmp->title_w.win) {
	    XShapeCombineShape (dpy, tmp->frame, ShapeBounding,
				tmp->title_x + tmp->frame_bw,
				tmp->title_y + tmp->frame_bw,
				tmp->title_w.win, ShapeBounding,
				ShapeUnion);
	}
    } else {
	/*
	 * can optimize rectangular contents window
	 */
	if (tmp->squeeze_info) {
	    XRectangle  newBounding[2];
	    XRectangle  newClip[2];

	    /*
	     * Build the border clipping rectangles; one around title, one
	     * around window.  The title_[xy] field already have had frame_bw
	     * subtracted off them so that they line up properly in the frame.
	     *
	     * The frame_width and frame_height do *not* include borders.
	     */
	    /* border */
	    newBounding[0].x = tmp->title_x;
	    newBounding[0].y = tmp->title_y;
	    newBounding[0].width = tmp->title_width + fbw2;
	    newBounding[0].height = tmp->title_height;
	    newBounding[1].x = -tmp->frame_bw;
	    newBounding[1].y = Scr->TitleHeight;
	    newBounding[1].width = tmp->attr.width + fbw2;
	    newBounding[1].height = tmp->attr.height + fbw2;
	    XShapeCombineRectangles (dpy, tmp->frame, ShapeBounding, 0, 0,
				     newBounding, 2, ShapeSet, YXBanded);
	    /* insides */
	    newClip[0].x = tmp->title_x + tmp->frame_bw;
	    newClip[0].y = 0;
	    newClip[0].width = tmp->title_width;
	    newClip[0].height = Scr->TitleHeight;
	    newClip[1].x = 0;
	    newClip[1].y = tmp->title_height;
	    newClip[1].width = tmp->attr.width;
	    newClip[1].height = tmp->attr.height;
	    XShapeCombineRectangles (dpy, tmp->frame, ShapeClip, 0, 0,
				     newClip, 2, ShapeSet, YXBanded);
	} else {
	    (void) XShapeCombineMask (dpy, tmp->frame, ShapeBounding, 0, 0,
 				      None, ShapeSet);
	    (void) XShapeCombineMask (dpy, tmp->frame, ShapeClip, 0, 0,
				      None, ShapeSet);
	}
    }

    if (Scr->RoundedTitle == TRUE && tmp->title_height > 0) {
	XShapeCombineMask (dpy, tmp->frame, ShapeBounding,
			tmp->title_x, tmp->title_y,
			Scr->Tcorner.left, ShapeSubtract);
	XShapeCombineMask (dpy, tmp->frame, ShapeBounding,
			tmp->title_x+tmp->title_width+fbw2-Scr->Tcorner.width,
			tmp->title_y, Scr->Tcorner.right, ShapeSubtract);
	if (tmp->frame_bw != 0) {
	    XShapeCombineMask (dpy, tmp->frame, ShapeClip,
			tmp->title_x+tmp->frame_bw, tmp->title_y+tmp->frame_bw,
			Scr->Tcorner.left, ShapeSubtract);
	    XShapeCombineMask (dpy, tmp->frame, ShapeClip,
			tmp->title_x+tmp->frame_bw+tmp->title_width-Scr->Tcorner.width,
			tmp->title_y+tmp->frame_bw, Scr->Tcorner.right, ShapeSubtract);
	}
    }
}

/*
 * Squeezed Title:
 * 
 *                         tmp->title_x
 *                   0     |
 *  tmp->title_y   ........+--------------+.........  -+,- tmp->frame_bw
 *             0   : ......| +----------+ |....... :  -++
 *                 : :     | |          | |      : :   ||-Scr->TitleHeight
 *                 : :     | |          | |      : :   ||
 *                 +-------+ +----------+ +--------+  -+|-tmp->title_height
 *                 | +---------------------------+ |  --+
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | +---------------------------+ |
 *                 +-------------------------------+
 * 
 * 
 * Unsqueezed Title:
 * 
 *                 tmp->title_x
 *                 | 0
 *  tmp->title_y   +-------------------------------+  -+,tmp->frame_bw
 *             0   | +---------------------------+ |  -+'
 *                 | |                           | |   |-Scr->TitleHeight
 *                 | |                           | |   |
 *                 + +---------------------------+ +  -+
 *                 |-+---------------------------+-|
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | |                           | |
 *                 | +---------------------------+ |
 *                 +-------------------------------+
 * 
 * 
 * 
 * Dimensions and Positions:
 * 
 *     frame orgin                 (0, 0)
 *     frame upper left border     (-tmp->frame_bw, -tmp->frame_bw)
 *     frame size w/o border       tmp->frame_width , tmp->frame_height
 *     frame/title border width    tmp->frame_bw
 *     extra title height w/o bdr  tmp->title_height = TitleHeight + frame_bw
 *     title window height         Scr->TitleHeight
 *     title origin w/o border     (tmp->title_x, tmp->title_y)
 *     client origin               (0, Scr->TitleHeight + tmp->frame_bw)
 *     client size                 tmp->attr.width , tmp->attr.height
 * 
 * When shaping, need to remember that the width and height of rectangles
 * are really deltax and deltay to lower right handle corner, so they need
 * to have -1 subtracted from would normally be the actual extents.
 */
