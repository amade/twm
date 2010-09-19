/* $XFree86: xc/programs/twm/events.c,v 1.12 2001/12/14 20:01:06 dawes Exp $ */
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
 * $Xorg: events.c,v 1.4 2001/02/09 02:05:36 xorgcvs Exp $
 *
 * twm event handling
 *
 * 17-Nov-87 Thomas E. LaStrange		File created
 *
 ***********************************************************************/

#include <stdio.h>
#include "twm.h"
#include <X11/Xatom.h>
#include <X11/Xos.h>
#ifdef TWM_USE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#include "iconmgr.h"
#include "add_window.h"
#include "menus.h"
#include "events.h"
#include "resize.h"
#include "parse.h"
#include "gram.h"
#include "util.h"
#include "screen.h"
#include "icons.h"
#include "version.h"


#define MAX_X_EVENT 256
event_proc EventHandler[MAX_X_EVENT]; /* event handler jump table */
char *Action;
int Context = C_NO_CONTEXT;	/* current button press context */
TwmWindow *ButtonWindow;	/* button press window structure */
XEvent ButtonEvent;		/* button press event */
XEvent Event;			/* the current event */
TwmWindow *Tmp_win;		/* the current twm window */

Window DragWindow;		/* variables used in moving windows */
int origDragX;
int origDragY;
int DragX;
int DragY;
int DragWidth;
int DragHeight;
int CurrentDragX;
int CurrentDragY;

static int enter_flag;
static int ColortableThrashing;
static TwmWindow *enter_win, *raise_win;

static void remove_window_from_ring ( TwmWindow *tmp );
static void do_menu ( MenuRoot *menu, Window w );
static Bool HENQueueScanner ( Display *dpy, XEvent *ev, char *args );
static Bool HLNQueueScanner ( Display *dpy, XEvent *ev, char *args );
static void flush_expose ( Window w );
static Bool UninstallRootColormapQScanner ( Display *dpy, XEvent *ev, char *args );

int ButtonPressed = -1;
int Cancel = FALSE;

/** Used in HandleEnterNotify to remove border highlight from a window 
 * that has not recieved a LeaveNotify event because of a pointer grab 
 */
TwmWindow *UnHighLight_win = NULL;

void HighLightIconManager (TwmWindow *twm)
{
    if (UnHighLight_win != twm) {
	if (UnHighLight_win != NULL && UnHighLight_win->list != NULL)
	    NotActiveIconManager (UnHighLight_win->list);
	if (twm->list != NULL) {
	    ActiveIconManager (twm->list);
	    UnHighLight_win = twm;
	} else
	    UnHighLight_win = NULL;
    }
}

void UnHighLightIconManager (void)
{
    if (UnHighLight_win != NULL) {
	if (UnHighLight_win->list != NULL)
	    NotActiveIconManager (UnHighLight_win->list);
	UnHighLight_win = NULL;
    }
}


void SetRaiseWindow (TwmWindow *tmp)
{
    enter_flag = TRUE;
    enter_win = NULL;
    raise_win = tmp;
}

void AutoRaiseWindow (TwmWindow *tmp)
{
    XRaiseWindow (dpy, tmp->frame);
    XSync (dpy, False);
    SetRaiseWindow (tmp);
}



/**
 * initialize the event jump table.
 */
void
InitEvents()
{
    int i;


    ResizeWindow = (Window) 0;
    DragWindow = (Window) 0;
    enter_flag = FALSE;
    enter_win = raise_win = NULL;

    for (i = 0; i < MAX_X_EVENT; i++)
	EventHandler[i] = HandleUnknown;

    EventHandler[Expose] = HandleExpose;
    EventHandler[CreateNotify] = HandleCreateNotify;
    EventHandler[DestroyNotify] = HandleDestroyNotify;
    EventHandler[MapRequest] = HandleMapRequest;
    EventHandler[MapNotify] = HandleMapNotify;
    EventHandler[UnmapNotify] = HandleUnmapNotify;
    EventHandler[MotionNotify] = HandleMotionNotify;
    EventHandler[ButtonRelease] = HandleButtonRelease;
    EventHandler[ButtonPress] = HandleButtonPress;
    EventHandler[EnterNotify] = HandleEnterNotify;
    EventHandler[LeaveNotify] = HandleLeaveNotify;
    EventHandler[FocusIn] = HandleFocusChange;
    EventHandler[FocusOut] = HandleFocusChange;
    EventHandler[ConfigureRequest] = HandleConfigureRequest;
    EventHandler[ClientMessage] = HandleClientMessage;
    EventHandler[PropertyNotify] = HandlePropertyNotify;
    EventHandler[KeyRelease] = HandleKeyRelease;
    EventHandler[KeyPress] = HandleKeyPress;
    EventHandler[ColormapNotify] = HandleColormapNotify;
    EventHandler[VisibilityNotify] = HandleVisibilityNotify;
    if (HasShape)
	EventHandler[ShapeEventBase+ShapeNotify] = HandleShapeNotify;
#ifdef TWM_USE_XRANDR
    if (HasXrandr)
	EventHandler[XrandrEventBase+RRScreenChangeNotify] = HandleXrandrScreenChangeNotify;
#ifdef TWM_USE_XINERAMA
    else /*fallthrough*/
#endif
#endif
#ifdef TWM_USE_XINERAMA
    if (HasXinerama)
	EventHandler[ConfigureNotify] = HandleConfigureNotify;
#endif
}




Time lastTimestamp = CurrentTime;	/* until Xlib does this for us */

Bool StashEventTime (XEvent *ev)
{
    switch (ev->type) {
      case KeyPress:
      case KeyRelease:
	lastTimestamp = ev->xkey.time;
	return True;
      case ButtonPress:
      case ButtonRelease:
	lastTimestamp = ev->xbutton.time;
	return True;
      case MotionNotify:
	lastTimestamp = ev->xmotion.time;
	return True;
      case EnterNotify:
      case LeaveNotify:
	lastTimestamp = ev->xcrossing.time;
	return True;
      case PropertyNotify:
	lastTimestamp = ev->xproperty.time;
	return True;
      case SelectionClear:
	lastTimestamp = ev->xselectionclear.time;
	return True;
      case SelectionRequest:
	lastTimestamp = ev->xselectionrequest.time;
	return True;
      case SelectionNotify:
	lastTimestamp = ev->xselection.time;
	return True;
    }
    return False;
}



/**
 * return the window about which this event is concerned; this
 * window may not be the same as XEvent.xany.window (the first window listed
 * in the structure).
 */
Window WindowOfEvent (XEvent *e)
{
    /*
     * Each window subfield is marked with whether or not it is the same as
     * XEvent.xany.window or is different (which is the case for some of the
     * notify events).
     */
    switch (e->type) {
      case KeyPress:
      case KeyRelease:  return e->xkey.window;			     /* same */
      case ButtonPress:
      case ButtonRelease:  return e->xbutton.window;		     /* same */
      case MotionNotify:  return e->xmotion.window;		     /* same */
      case EnterNotify:
      case LeaveNotify:  return e->xcrossing.window;		     /* same */
      case FocusIn:
      case FocusOut:  return e->xfocus.window;			     /* same */
      case KeymapNotify:  return e->xkeymap.window;		     /* same */
      case Expose:  return e->xexpose.window;			     /* same */
      case GraphicsExpose:  return e->xgraphicsexpose.drawable;	     /* same */
      case NoExpose:  return e->xnoexpose.drawable;		     /* same */
      case VisibilityNotify:  return e->xvisibility.window;	     /* same */
      case CreateNotify:  return e->xcreatewindow.window;	     /* DIFF */
      case DestroyNotify:  return e->xdestroywindow.window;	     /* DIFF */
      case UnmapNotify:  return e->xunmap.window;		     /* DIFF */
      case MapNotify:  return e->xmap.window;			     /* DIFF */
      case MapRequest:  return e->xmaprequest.window;		     /* DIFF */
      case ReparentNotify:  return e->xreparent.window;		     /* DIFF */
      case ConfigureNotify:  return e->xconfigure.window;	     /* DIFF */
      case ConfigureRequest:  return e->xconfigurerequest.window;    /* DIFF */
      case GravityNotify:  return e->xgravity.window;		     /* DIFF */
      case ResizeRequest:  return e->xresizerequest.window;	     /* same */
      case CirculateNotify:  return e->xcirculate.window;	     /* DIFF */
      case CirculateRequest:  return e->xcirculaterequest.window;    /* DIFF */
      case PropertyNotify:  return e->xproperty.window;		     /* same */
      case SelectionClear:  return e->xselectionclear.window;	     /* same */
      case SelectionRequest: return e->xselectionrequest.requestor;  /* DIFF */
      case SelectionNotify:  return e->xselection.requestor;	     /* same */
      case ColormapNotify:  return e->xcolormap.window;		     /* same */
      case ClientMessage:  return e->xclient.window;		     /* same */
      case MappingNotify:  return None;
    }
    return None;
}



/**
 *      handle a single X event stored in global var Event
 * this routine for is for a call during an f.move
 */
Bool DispatchEvent2 ()
{
    Window w = Event.xany.window;
    StashEventTime (&Event);

    if (XFindContext (dpy, w, TwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
      Tmp_win = NULL;

    if (XFindContext (dpy, w, ScreenContext, (caddr_t *)&Scr) == XCNOENT) {
	Scr = FindScreenInfo (WindowOfEvent (&Event));
    }

    if (!Scr) return False;

    if (menuFromFrameOrWindowOrTitlebar && Event.type == Expose)
      HandleExpose();

    if (!menuFromFrameOrWindowOrTitlebar && Event.type>= 0 && Event.type < MAX_X_EVENT) {
	(*EventHandler[Event.type])();
    }

    return True;
}

/**
 * handle a single X event stored in global var Event
 */
Bool DispatchEvent ()
{
    Window w = Event.xany.window;
    StashEventTime (&Event);

    if (XFindContext (dpy, w, TwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
      Tmp_win = NULL;

    if (XFindContext (dpy, w, ScreenContext, (caddr_t *)&Scr) == XCNOENT) {
	Scr = FindScreenInfo (WindowOfEvent (&Event));
    }

    if (!Scr) return False;

    if (Event.type>= 0 && Event.type < MAX_X_EVENT) {
	(*EventHandler[Event.type])();
    }

    return True;
}



/**
 * handle X events
 */
void
HandleEvents()
{
    while (TRUE)
    {
	if (enter_flag && !QLength(dpy)) {
	    if (enter_win && enter_win != raise_win) {
		AutoRaiseWindow (enter_win);  /* sets enter_flag T */
	    } else {
		enter_flag = FALSE;
	    }
	}
	if (ColortableThrashing && !QLength(dpy) && Scr) {
	    InstallWindowColormaps(ColormapNotify, (TwmWindow *) NULL);
	}
	WindowMoved = FALSE;
	XtAppNextEvent(appContext, &Event);
	if (Event.type>= 0 && Event.type < MAX_X_EVENT)
	    (void) DispatchEvent ();
	else
	    XtDispatchEvent (&Event);
    }
}



/**
 * colormap notify event handler.
 *
 * This procedure handles both a client changing its own colormap, and
 * a client explicitly installing its colormap itself (only the window
 * manager should do that, so we must set it correctly).
 *
 */
void
HandleColormapNotify()
{
    XColormapEvent *cevent = (XColormapEvent *) &Event;
    ColormapWindow *cwin, **cwins;
    TwmColormap *cmap;
    int lost, won, n, number_cwins;

    if (XFindContext(dpy, cevent->window, ColormapContext, (caddr_t *)&cwin) == XCNOENT)
	return;
    cmap = cwin->colormap;

    if (cevent->new)
    {
	if (XFindContext(dpy, cevent->colormap, ColormapContext,
			 (caddr_t *)&cwin->colormap) == XCNOENT)
	    cwin->colormap = CreateTwmColormap(cevent->colormap);
	else
	    cwin->colormap->refcnt++;

	cmap->refcnt--;

	if (cevent->state == ColormapUninstalled)
	    cmap->state &= ~CM_INSTALLED;
	else
	    cmap->state |= CM_INSTALLED;

	if (cmap->state & CM_INSTALLABLE)
	    InstallWindowColormaps(ColormapNotify, (TwmWindow *) NULL);

	if (cmap->refcnt == 0)
	{
	    XDeleteContext(dpy, cmap->c, ColormapContext);
	    free((char *) cmap);
	}

	return;
    }

    if (cevent->state == ColormapUninstalled &&
	(cmap->state & CM_INSTALLABLE))
    {
	if (!(cmap->state & CM_INSTALLED))
	    return;
	cmap->state &= ~CM_INSTALLED;

	if (!ColortableThrashing)
	{
	    ColortableThrashing = TRUE;
	    XSync(dpy, 0);
	}

	if (cevent->serial >= Scr->cmapInfo.first_req)
	{
	    number_cwins = Scr->cmapInfo.cmaps->number_cwins;

	    /*
	     * Find out which colortables collided.
	     */

	    cwins = Scr->cmapInfo.cmaps->cwins;
	    for (lost = won = -1, n = 0;
		 (lost == -1 || won == -1) && n < number_cwins;
		 n++)
	    {
#if 1
		if (won == -1 && cwins[n]->w == cevent->window)
		{
		    won = n;  /* colormap-focused client changes its colormap */
		    ColortableThrashing = FALSE; /* don't thrash this installation */
		    continue;
		}
#endif
		if (lost == -1 && cwins[n] == cwin)
		{
		    lost = n;	/* This is the window which lost its colormap */
		    continue;
		}

		if (won == -1 &&
		    cwins[n]->colormap->install_req == cevent->serial)
		{
		    won = n;	/* This is the window whose colormap caused */
		    continue;	/* the de-install of the previous colormap */
		}
	    }

	    /*
	    ** Cases are:
	    ** Both the request and the window were found:
	    **		One of the installs made honoring the WM_COLORMAP
	    **		property caused another of the colormaps to be
	    **		de-installed, just mark the scoreboard.
	    **
	    ** Only the request was found:
	    **		One of the installs made honoring the WM_COLORMAP
	    **		property caused a window not in the WM_COLORMAP
	    **		list to lose its map.  This happens when the map
	    **		it is losing is one which is trying to be installed,
	    **		but is getting getting de-installed by another map
	    **		in this case, we'll get a scoreable event later,
	    **		this one is meaningless.
	    **
	    ** Neither the request nor the window was found:
	    **		Somebody called installcolormap, but it doesn't
	    **		affect the WM_COLORMAP windows.  This case will
	    **		probably never occur.
	    **
	    ** Only the window was found:
	    **		One of the WM_COLORMAP windows lost its colormap
	    **		but it wasn't one of the requests known.  This is
	    **		probably because someone did an "InstallColormap".
	    **		The colormap policy is "enforced" by re-installing
	    **		the colormaps which are believed to be correct.
	    */

	    if (won != -1) {
		if (lost != -1)
		{
		    /* lower diagonal index calculation */
		    if (lost > won)
			n = lost*(lost-1)/2 + won;
		    else
			n = won*(won-1)/2 + lost;
		    Scr->cmapInfo.cmaps->scoreboard[n] = 1;
		} else
		{
		    /*
		    ** One of the cwin installs caused one of the cwin
		    ** colormaps to be de-installed, so I'm sure to get an
		    ** UninstallNotify for the cwin I know about later.
		    ** I haven't got it yet, or the test of CM_INSTALLED
		    ** above would have failed.  Turning the CM_INSTALLED
		    ** bit back on makes sure we get back here to score
		    ** the collision.
		    */
		    cmap->state |= CM_INSTALLED;
		}
	    } else if (lost != -1) {
		InstallWindowColormaps(ColormapNotify, (TwmWindow *) NULL);
	    }
	}
    }

    else if (cevent->state == ColormapUninstalled)
	cmap->state &= ~CM_INSTALLED;

    else if (cevent->state == ColormapInstalled)
	cmap->state |= CM_INSTALLED;
}



/**
 * visibility notify event handler.
 *
 * This routine keeps track of visibility events so that colormap
 * installation can keep the maximum number of useful colormaps
 * installed at one time.
 *
 */
void
HandleVisibilityNotify()
{
    XVisibilityEvent *vevent = (XVisibilityEvent *) &Event;
    ColormapWindow *cwin;
    TwmColormap *cmap;

    if (XFindContext(dpy, vevent->window, ColormapContext, (caddr_t *)&cwin) == XCNOENT)
	return;
    
    /*
     * when Saber complains about retreiving an <int> from an <unsigned int>
     * just type "touch vevent->state" and "cont"
     */
    cmap = cwin->colormap;
    if ((cmap->state & CM_INSTALLABLE) &&
	vevent->state != cwin->visibility &&
	(vevent->state == VisibilityFullyObscured ||
	 cwin->visibility == VisibilityFullyObscured) &&
	cmap->w == cwin->w) {
	cwin->visibility = vevent->state;
	InstallWindowColormaps(VisibilityNotify, (TwmWindow *) NULL);
    } else
	cwin->visibility = vevent->state;
}



/**
 * key release event handler
 */
void
HandleKeyRelease()
{
    XFlush (dpy); /* speedup keybinding processing completion */
}


int MovedFromKeyPress = False;

/**
 * key press event handler
 */
void
HandleKeyPress()
{
    KeySym ks;
    FuncKey *key;
    int len;
    unsigned int modifier;

    if (InfoLines) XUnmapWindow(dpy, Scr->InfoWindow.win);
    Context = C_NO_CONTEXT;

    if (Event.xany.window == Scr->Root)
	Context = C_ROOT;
    if (Tmp_win)
    {
	if (Event.xany.window == Tmp_win->title_w.win)
	    Context = C_TITLE;
	if (Event.xany.window == Tmp_win->w)
	    Context = C_WINDOW;
	if (Event.xany.window == Tmp_win->icon_w.win)
	    Context = C_ICON;
	if (Event.xany.window == Tmp_win->frame)
	    Context = C_FRAME;
	if (Tmp_win->list && Event.xany.window == Tmp_win->list->w.win)
	    Context = C_ICONMGR;
	if (Tmp_win->list && Event.xany.window == Tmp_win->list->icon)
	    Context = C_ICONMGR;
    }

    modifier = (Event.xkey.state & mods_used);
    ks = XLookupKeysym((XKeyEvent *) &Event, /* KeySyms index */ 0);
    for (key = Scr->FuncKeyRoot.next; key != NULL; key = key->next)
    {
 	if (key->keysym == ks &&
	    key->mods == modifier &&
	    (key->cont == Context || key->cont == C_NAME))
	{
	    /* weed out the functions that don't make sense to execute
	     * from a key press 
	     */
	    if (key->func == F_RESIZE)
		return;
            /* special case for F_MOVE/F_FORCEMOVE activated from a keypress */
            if (key->func == F_MOVE || key->func == F_FORCEMOVE)
                MovedFromKeyPress = True;

	    if (key->cont != C_NAME)
	    {
		ExecuteFunction(key->func, key->action, Event.xany.window,
		    Tmp_win, &Event, Context, FALSE);
		XUngrabPointer(dpy, CurrentTime);
		return;
	    }
	    else
	    {
		int matched = FALSE;
		len = strlen(key->win_name);

		/* try and match the name first */
		for (Tmp_win = Scr->TwmRoot.next; Tmp_win != NULL;
		    Tmp_win = Tmp_win->next)
		{
		    if (!strncmp(key->win_name, Tmp_win->name, len))
		    {
			matched = TRUE;
			ExecuteFunction(key->func, key->action, Tmp_win->frame,
			    Tmp_win, &Event, C_FRAME, FALSE);
			XUngrabPointer(dpy, CurrentTime);
		    }
		}

		/* now try the res_name */
		if (!matched)
		for (Tmp_win = Scr->TwmRoot.next; Tmp_win != NULL;
		    Tmp_win = Tmp_win->next)
		{
		    if (!strncmp(key->win_name, Tmp_win->class.res_name, len))
		    {
			matched = TRUE;
			ExecuteFunction(key->func, key->action, Tmp_win->frame,
			    Tmp_win, &Event, C_FRAME, FALSE);
			XUngrabPointer(dpy, CurrentTime);
		    }
		}

		/* now try the res_class */
		if (!matched)
		for (Tmp_win = Scr->TwmRoot.next; Tmp_win != NULL;
		    Tmp_win = Tmp_win->next)
		{
		    if (!strncmp(key->win_name, Tmp_win->class.res_class, len))
		    {
			matched = TRUE;
			ExecuteFunction(key->func, key->action, Tmp_win->frame,
			    Tmp_win, &Event, C_FRAME, FALSE);
			XUngrabPointer(dpy, CurrentTime);
		    }
		}
		if (matched)
		    return;
	    }
	}
    }

    /* if we get here, no function key was bound to the key.  Send it
     * to the client if it was in a window we know about.
     *
     * Besides modifier keys e.g. Shift_L, Control_L, Alt_L and the like
     * these may be "usual" keys as well typed while mouse hovering over
     * icons or icon manager iconified entries (only if FocusRoot == TRUE).
     */
    if (Tmp_win && Scr->TitleFocus)
    {
        if (Event.xany.window == Tmp_win->icon_w.win ||
	    Event.xany.window == Tmp_win->frame ||
	    Event.xany.window == Tmp_win->title_w.win ||
	    (Tmp_win->list && (Event.xany.window == Tmp_win->list->w.win)))
        {
            Event.xkey.window = Tmp_win->w;
            XSendEvent(dpy, Tmp_win->w, False, KeyPressMask, &Event);
        }
    }
}



void
free_window_names (TwmWindow *tmp, Bool nukefull, Bool nukename, Bool nukeicon)
{
/*
 * XXX - are we sure that nobody ever sets these to another constant (check
 * twm windows)?
 */
    if (tmp->name == tmp->full_name) nukefull = False;
    if (tmp->icon_name == tmp->name) nukename = False;

    if (nukefull && tmp->full_name) free (tmp->full_name);
    if (nukename && tmp->name) free (tmp->name);
    if (nukeicon && tmp->icon_name) free (tmp->icon_name);
    return;
}



void 
free_cwins (TwmWindow *tmp)
{
    int i;
    TwmColormap *cmap;

    if (tmp->cmaps.number_cwins) {
	for (i = 0; i < tmp->cmaps.number_cwins; i++) {
	     if (--tmp->cmaps.cwins[i]->refcnt == 0) {
		cmap = tmp->cmaps.cwins[i]->colormap;
		if (--cmap->refcnt == 0) {
		    XDeleteContext(dpy, cmap->c, ColormapContext);
		    free((char *) cmap);
		}
		XDeleteContext(dpy, tmp->cmaps.cwins[i]->w, ColormapContext);
		free((char *) tmp->cmaps.cwins[i]);
	    }
	}
	free((char *) tmp->cmaps.cwins);
	if (tmp->cmaps.number_cwins > 1) {
	    free(tmp->cmaps.scoreboard);
	    tmp->cmaps.scoreboard = NULL;
	}
	tmp->cmaps.number_cwins = 0;
    }
}



static void
RaiseXUrgencyHintWindow (TwmWindow *tmp)
{
    int x, y;

    InfoLines = 0;
    (void) sprintf(Info[InfoLines++], "XUrgencyHint:");
    (void) sprintf(Info[InfoLines++], "%s", tmp->name);

    if (tmp->mapped) {
	x = tmp->frame_x + tmp->frame_width/2;
	y = tmp->frame_y;
    } else
	x = y = 0;

    RaiseInfoWindow (x, y);
}

static void
CheckXUrgencyHint (TwmWindow *tmp)
{
    if (tmp->wmhints && (tmp->wmhints->flags & XUrgencyHint))
	if (Scr->ShowXUrgencyHints == TRUE)
	    RaiseXUrgencyHintWindow (tmp);
}



/**
 * property notify event handler
 */
void
HandlePropertyNotify()
{
    char *name = NULL;

    /* watch for standard colormap changes */
    if (Event.xproperty.window == Scr->Root) {
	XStandardColormap *maps = NULL;
	int nmaps;

	switch (Event.xproperty.state) {
	  case PropertyNewValue:
	    if (XGetRGBColormaps (dpy, Scr->Root, &maps, &nmaps, 
				  Event.xproperty.atom)) {
		/* if got one, then replace any existing entry */
		InsertRGBColormap (Event.xproperty.atom, maps, nmaps, True);
	    }
	    return;

	  case PropertyDelete:
	    RemoveRGBColormap (Event.xproperty.atom);
	    return;
	}
    }

    if (!Tmp_win) return;		/* unknown window */

#define MAX_NAME_LEN 200L		/* truncate to this many */
#define MAX_ICON_NAME_LEN 200L		/* ditto */

    switch (Event.xproperty.atom) {
      case XA_WM_NAME:
	if (!I18N_FetchName(Tmp_win->w, &name)) return;
	free_window_names (Tmp_win, True, True, False);

	Tmp_win->full_name = strdup(name ? name : NoName);
	Tmp_win->name = strdup(name ? name : NoName);
	if (name) free(name);

	Tmp_win->nameChanged = 1;

	Tmp_win->name_width = MyFont_TextWidth (&Scr->TitleBarFont,
					  Tmp_win->name,
					  strlen (Tmp_win->name));

	SetupWindow (Tmp_win, Tmp_win->frame_x, Tmp_win->frame_y,
		     Tmp_win->frame_width, Tmp_win->frame_height, -1);

	if (Tmp_win->title_w.win) XClearArea(dpy, Tmp_win->title_w.win, 0,0,0,0, True);

	/*
	 * if the icon name is NoName, set the name of the icon to be
	 * the same as the window 
	 */
	if (Tmp_win->icon_name == NoName) {
	    Tmp_win->icon_name = Tmp_win->name;
	    RedoIconName();
	}
	break;

      case XA_WM_ICON_NAME:
	if (!I18N_GetIconName(Tmp_win->w, &name)) return;
	free_window_names (Tmp_win, False, False, True);
	Tmp_win->icon_name = strdup(name ? name : NoName);
	if (name) free(name);

	RedoIconName();
	break;

      case XA_WM_HINTS:
	if (Tmp_win->wmhints) XFree ((char *) Tmp_win->wmhints);
	Tmp_win->wmhints = XGetWMHints(dpy, Event.xany.window);

#if 1
	/* Turn off 'initial_state' flag as we are mapped, this confuses iconify/deiconify handling */
	if (Tmp_win->wmhints)
	    Tmp_win->wmhints->flags &= ~StateHint;
#endif

	if (Tmp_win->wmhints && (Tmp_win->wmhints->flags & WindowGroupHint))
	  Tmp_win->group = Tmp_win->wmhints->window_group;

	CheckXUrgencyHint (Tmp_win);

	if (Tmp_win->icon_not_ours && Tmp_win->wmhints &&
	    !(Tmp_win->wmhints->flags & IconWindowHint)) {
	    /* IconWindowHint was formerly on, now off; revert
	    // to a default icon */
	    int icon_x = 0, icon_y = 0;
	    XGetGeometry (dpy, Tmp_win->icon_w.win, &JunkRoot,
			  &icon_x, &icon_y,
			  &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth);
	    XSelectInput (dpy, Tmp_win->icon_w.win, None);
	    XDeleteContext (dpy, Tmp_win->icon_w.win, TwmContext);
	    XDeleteContext (dpy, Tmp_win->icon_w.win, ScreenContext);
	    CreateIconWindow(Tmp_win, icon_x, icon_y);
	    break;
	}

	if (!Tmp_win->forced && Tmp_win->wmhints &&
	    Tmp_win->wmhints->flags & IconWindowHint) {
	    if (Tmp_win->icon_w.win) {
	    	int icon_x, icon_y;

		/*
		 * There's already an icon window.
		 * Try to find out where it is; if we succeed, move the new
		 * window to where the old one is.
		 */
		if (XGetGeometry (dpy, Tmp_win->icon_w.win, &JunkRoot, &icon_x,
		  &icon_y, &JunkWidth, &JunkHeight, &JunkBW, &JunkDepth)) {
		    /*
		     * Move the new icon window to where the old one was.
		     */
		    XMoveWindow(dpy, Tmp_win->wmhints->icon_window, icon_x,
		      icon_y);
		}

		/*
		 * If the window is iconic, map the new icon window.
		 */
		if (Tmp_win->icon)
		    XMapWindow(dpy, Tmp_win->wmhints->icon_window);

		/*
		 * Now, if the old window isn't ours, unmap it, otherwise
		 * just get rid of it completely.
		 */
		if (Tmp_win->icon_not_ours) {
		    if (Tmp_win->icon_w.win != Tmp_win->wmhints->icon_window)
			XUnmapWindow(dpy, Tmp_win->icon_w.win);
		} else {
#ifdef TWM_USE_XFT
		    if (Scr->use_xft > 0)
			MyXftDrawDestroy(Tmp_win->icon_w.xft);
#endif
#ifdef TWM_USE_RENDER
		    if (Tmp_win->PicIconB != None) {
			XRenderFreePicture (dpy, Tmp_win->PicIconB);
			Tmp_win->PicIconB = None;
		    }
		    if (Tmp_win->PenIconB != None) {
			XRenderFreePicture (dpy, Tmp_win->PenIconB);
			Tmp_win->PenIconB = None;
		    }
#endif
		    XDestroyWindow(dpy, Tmp_win->icon_w.win);
		}

		XDeleteContext(dpy, Tmp_win->icon_w.win, TwmContext);
		XDeleteContext(dpy, Tmp_win->icon_w.win, ScreenContext);

		/*
		 * The new icon window isn't our window, so note that fact
		 * so that we don't treat it as ours.
		 */
		Tmp_win->icon_not_ours = TRUE;

		/*
		 * Now make the new window the icon window for this window,
		 * and set it up to work as such (select for key presses
		 * and button presses/releases, set up the contexts for it,
		 * and define the cursor for it).
		 */
		Tmp_win->icon_w.win = Tmp_win->wmhints->icon_window;
		XSelectInput (dpy, Tmp_win->icon_w.win,
		  KeyPressMask | ButtonPressMask | ButtonReleaseMask);
		XSaveContext(dpy, Tmp_win->icon_w.win, TwmContext, (caddr_t)Tmp_win);
		XSaveContext(dpy, Tmp_win->icon_w.win, ScreenContext, (caddr_t)Scr);
		XDefineCursor(dpy, Tmp_win->icon_w.win, Scr->IconCursor);
		SetIconWindowHintForFrame (Tmp_win, Tmp_win->icon_w.win);
	    }
	}

	if (Tmp_win->icon_w.win && !Tmp_win->forced)
	{
	    int pm_depth;
	    Pixmap pm;

	    pm = CreateIconWMhintsPixmap (Tmp_win, &pm_depth);
	    if (pm != None)
	    {
		if (Tmp_win->icon_bm_w)
		    XDestroyWindow(dpy, Tmp_win->icon_bm_w);
		Tmp_win->icon_bm_w = CreateIconBMWindow (Tmp_win, 0, 0, pm, pm_depth);
		XFreePixmap (dpy, pm);
		RedoIconName();
	    }
	}
	break;

      case XA_WM_NORMAL_HINTS:
	GetWindowSizeHints (Tmp_win);
	break;

      default:
	if (Event.xproperty.atom == _XA_WM_COLORMAP_WINDOWS) {
	    FetchWmColormapWindows (Tmp_win);	/* frees old data */
	    break;
	} else if (Event.xproperty.atom == _XA_WM_PROTOCOLS) {
	    FetchWmProtocols (Tmp_win);
	    break;
	}
#ifdef TWM_USE_OPACITY
	else if ((Event.xproperty.atom == _XA_NET_WM_WINDOW_OPACITY)
			&& Event.xproperty.window == Tmp_win->w)
	    switch (Event.xproperty.state) {
	    case PropertyNewValue:
		PropagateWindowOpacity (Tmp_win);
		break;
	    case PropertyDelete:
		XDeleteProperty (dpy, Tmp_win->frame, _XA_NET_WM_WINDOW_OPACITY);
		break;
	    }
#endif
	break;
    }
}



/**
 * procedure to re-position the icon window and name
 */
void
RedoIconName()
{
    int x, y;

    if (Tmp_win->list)
    {
	if (Scr->SortIconMgr)
	    SortIconManager(Tmp_win->list->iconmgr);
	else
	    SetIconManagerLabelShapeMask(Tmp_win->list);
	    /* let the expose event cause the repaint */
	    XClearArea(dpy, Tmp_win->list->w.win, 0,0,0,0, True);
    }

    if (Tmp_win->icon_w.win == (Window) 0)
	return;

    if (Tmp_win->icon_not_ours)
	return;

    ComputeIconSize (Tmp_win, &x, &y);

    XResizeWindow(dpy, Tmp_win->icon_w.win, Tmp_win->icon_w_width,
	Tmp_win->icon_w_height);

    if (Tmp_win->icon_bm_w)
    {
	XMoveWindow(dpy, Tmp_win->icon_bm_w, x, y);
	XMapWindow(dpy, Tmp_win->icon_bm_w);
    }

    SetIconShapeMask(Tmp_win);

    if (Tmp_win->icon)
    {
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE) {
	    if (Tmp_win->icon_on == TRUE)
		if (Scr->XCompMgrRunning == FALSE) {
		    /*
		     * We have no CWBackPixel set, so force "clean screen" hack.
		     * (If icon width changes due to icon_name change, we would
		     * actually have to deiconify+iconify to ensure correct
		     * icon positioning!)
		     */
		    XUnmapWindow (dpy, Tmp_win->icon_w.win);
		    XMapWindow (dpy, Tmp_win->icon_w.win);
		    XFlush (dpy);
		}
	}
	else
#endif
	    XClearArea(dpy, Tmp_win->icon_w.win, 0, 0, 0, 0, True);
    }
}



/**
 *client message event handler
 */
void
HandleClientMessage()
{
    if (Event.xclient.message_type == _XA_WM_CHANGE_STATE)
    {
	if (Tmp_win != NULL)
	{
	    if (Event.xclient.data.l[0] == IconicState && !Tmp_win->icon)
	    {
		XEvent button;

		XQueryPointer( dpy, Scr->Root, &JunkRoot, &JunkChild,
			      &(button.xmotion.x_root),
			      &(button.xmotion.y_root),
			      &JunkX, &JunkY, &JunkMask);

		ExecuteFunction(F_ICONIFY, NULLSTR, Event.xany.window,
		    Tmp_win, &button, FRAME, FALSE);
		XUngrabPointer(dpy, CurrentTime);
	    }
	}
    }
    else if (Tmp_win != NULL && Tmp_win->iconmgr == FALSE /* message for the 'frame' or 'icon' */
	    && (Tmp_win->frame == Event.xclient.window || Tmp_win->icon_w.win == Event.xclient.window)
	    && Event.xclient.message_type == _XA_WM_PROTOCOLS)
    {
	if (Event.xclient.data.l[0] == _XA_WM_DELETE_WINDOW)
	    SendDeleteWindowMessage (Tmp_win, LastTimestamp());
	else if (Event.xclient.data.l[0] == _XA_WM_SAVE_YOURSELF)
	    SendSaveYourselfMessage (Tmp_win, LastTimestamp());
    }
}



/**
 * expose event handler
 */
void
HandleExpose()
{
    MenuRoot *tmp;
    if (XFindContext(dpy, Event.xany.window, MenuContext, (caddr_t *)&tmp) == 0)
    {
	PaintMenu(tmp, &Event);
	return;
    }

    if (Event.xexpose.count != 0)
	return;

    if (Event.xany.window == Scr->InfoWindow.win && InfoLines)
    {
	int i;
	int height;

#ifdef TWM_USE_SPACING
	height = 120*Scr->DefaultFont.height/100; /*baselineskip 1.2*/
#else
	height = Scr->DefaultFont.height+2;
#endif

#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE)
	{
	    Picture pic_win = XRenderCreatePicture (dpy, Scr->InfoWindow.win, Scr->FormatRGB,
							CPGraphicsExposure, &Scr->PicAttr);
	    XGetGeometry (dpy, Scr->InfoWindow.win, &JunkRoot, &JunkX, &JunkY, &JunkWidth, &JunkHeight,
							&JunkBW, &JunkDepth);
	    if (Scr->XCompMgrRunning == TRUE)
		XRenderFillRectangle (dpy, PictOpSrc, pic_win, &Scr->XRcol32Clear, 0, 0, JunkWidth, JunkHeight);
	    XRenderFillRectangle (dpy, PictOpOver, pic_win, &Scr->XRcolDefaultB, 0, 0, JunkWidth, JunkHeight);
	    XRenderFreePicture (dpy, pic_win);
	}
	else
#endif
	    XClearArea (dpy, Scr->InfoWindow.win, 0, 0, 0, 0, False);

	for (i = 0; i < InfoLines; i++)
	{
	    MyFont_DrawString (&Scr->InfoWindow, &Scr->DefaultFont, &Scr->DefaultC,
#ifdef TWM_USE_SPACING
		    Scr->DefaultFont.height/2, (i*height) + Scr->DefaultFont.height,
#else
		    5, (i*height) + Scr->DefaultFont.y,
#endif
		    Info[i], strlen(Info[i]));
	}
    } 
    else if (Tmp_win != NULL)
    {
	if (Event.xany.window == Tmp_win->title_w.win)
	{
	    MyFont_DrawImageStringEllipsis (&Tmp_win->title_w, &Scr->TitleBarFont,
		&Tmp_win->TitleC, Scr->TBInfo.titlex, Scr->TitleBarFont.y,
		Tmp_win->name, strlen (Tmp_win->name),
		Tmp_win->title_width - Scr->TBInfo.titlex - Scr->TBInfo.rightoff - Scr->TitlePadding);
	}
	else if (Event.xany.window == Tmp_win->icon_w.win)
	{
	    int l;
#ifdef TWM_USE_RENDER
	    if (Scr->use_xrender == TRUE)
	    {
		Picture pic_win = XRenderCreatePicture (dpy, Tmp_win->icon_w.win, Scr->FormatRGB,
							CPGraphicsExposure, &Scr->PicAttr);
		if (Scr->XCompMgrRunning == TRUE)
		    XRenderFillRectangle (dpy, PictOpSrc, pic_win, &Scr->XRcol32Clear,
					    0, 0, Tmp_win->icon_w_width, Tmp_win->icon_w_height);
		XRenderComposite (dpy, PictOpOver, Tmp_win->PenIconB, Tmp_win->PicIconB, pic_win, 0, 0, 0, 0,
					    0, 0, Tmp_win->icon_w_width, Tmp_win->icon_w_height);
		XRenderFreePicture (dpy, pic_win);
	    }
#endif
#ifdef TWM_USE_SPACING
	    l = Tmp_win->icon_w_width - Scr->IconFont.height;
#else
	    l = Tmp_win->icon_w_width - 6;
#endif
	    MyFont_DrawImageStringEllipsis (&Tmp_win->icon_w, &Scr->IconFont,
		&Tmp_win->IconC, Tmp_win->icon_x, Tmp_win->icon_y,
		Tmp_win->icon_name, strlen (Tmp_win->icon_name), l);
	}
	else if (Tmp_win->list && Tmp_win->list->w.win == Event.xany.window)
	{
#ifdef TWM_USE_RENDER
	    if (Scr->use_xrender == TRUE)
	    {
		Picture pen, pic_win;
		pic_win = XRenderCreatePicture (dpy, Tmp_win->list->iconmgr->twm_win->frame, Scr->FormatRGB,
						(CPGraphicsExposure|CPSubwindowMode), &Scr->PicAttr);
		if (Scr->XCompMgrRunning == TRUE)
		    XRenderFillRectangle (dpy, PictOpSrc, pic_win, &Scr->XRcol32Clear,
					Tmp_win->list->x, Tmp_win->list->y+Tmp_win->list->iconmgr->twm_win->title_height,
					Tmp_win->list->width, Tmp_win->list->height);

		if (Scr->Highlight && (Tmp_win->list->iconmgr->hilited == Tmp_win->list && Scr->FilledIconMgrHighlight == TRUE))
		    pen = Tmp_win->list->PenIconMgrH;
		else
		    pen = Tmp_win->list->PenIconMgrB;

		XRenderComposite (dpy, PictOpOver, pen, Tmp_win->list->PicIconMgrB, pic_win, 0, 0, 0, 0,
					Tmp_win->list->x, Tmp_win->list->y+Tmp_win->list->iconmgr->twm_win->title_height,
					Tmp_win->list->width, Tmp_win->list->height);
		XRenderFreePicture (dpy, pic_win);
		XClearArea (dpy, Tmp_win->list->icon, 0, 0, 0, 0, False); /* repaint "iconified" knob */
	    }
	    else
#endif
		XClearArea (dpy, Tmp_win->list->w.win, 0, 0, 0, 0, False);

	    MyFont_DrawStringEllipsis (&Tmp_win->list->w,
		    &Scr->IconManagerFont, &Tmp_win->list->IconManagerC,
#ifdef TWM_USE_SPACING
		    Scr->iconmgr_textx, Scr->IconManagerFont.y
			+ (Tmp_win->list->height-Scr->IconManagerFont.height)/2,
#else
		    Scr->iconmgr_textx, Scr->IconManagerFont.y+4,
#endif
		    Tmp_win->icon_name, strlen(Tmp_win->icon_name),
		    Tmp_win->list->width - Scr->iconmgr_textx - Scr->IconManagerFont.ellen/2);
	    DrawIconManagerBorder (Tmp_win->list);
	} 
    }
    flush_expose (Event.xany.window);
}



static void remove_window_from_ring (TwmWindow *tmp)
{
    TwmWindow *prev = tmp->ring.prev, *next = tmp->ring.next;

    if (enter_win == tmp) {
	enter_flag = FALSE;
	enter_win = NULL;
    }
    if (raise_win == Tmp_win) raise_win = NULL;

    /*
     * 1. Unlink window
     * 2. If window was only thing in ring, null out ring
     * 3. If window was ring leader, set to next (or null)
     */
    if (prev) prev->ring.next = next;
    if (next) next->ring.prev = prev;
    if (Scr->Ring == tmp) 
      Scr->Ring = (next != tmp ? next : (TwmWindow *) NULL);

    if (!Scr->Ring || Scr->RingLeader == tmp) Scr->RingLeader = Scr->Ring;
}

void DiscardWindowEvents (Window w, long exceptions)
{
    XEvent e;

    /* drop events of all types not including 'exceptions': */
    while (True == XCheckWindowEvent (dpy, w, ~exceptions, &e)) {
	continue;
    }
}



/**
 * DestroyNotify event handler
 */
void
HandleDestroyNotify()
{
    int i;

    /*
     * Warning, this is also called by HandleUnmapNotify; if it ever needs to
     * look at the event, HandleUnmapNotify will have to mash the UnmapNotify
     * into a DestroyNotify.
     */

    if (Tmp_win == NULL)
	return;

    XDeleteContext(dpy, Tmp_win->w, TwmContext);
    XDeleteContext(dpy, Tmp_win->w, ScreenContext);
    XDeleteContext(dpy, Tmp_win->frame, TwmContext);
    XDeleteContext(dpy, Tmp_win->frame, ScreenContext);
    if (Tmp_win->icon_w.win)
    {
	XDeleteContext(dpy, Tmp_win->icon_w.win, TwmContext);
	XDeleteContext(dpy, Tmp_win->icon_w.win, ScreenContext);
    }
    if (Tmp_win->title_height)
    {
	int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;
	XDeleteContext(dpy, Tmp_win->title_w.win, TwmContext);
	XDeleteContext(dpy, Tmp_win->title_w.win, ScreenContext);
	if (Tmp_win->hilite_w)
	{
	    XDeleteContext(dpy, Tmp_win->hilite_w, TwmContext);
	    XDeleteContext(dpy, Tmp_win->hilite_w, ScreenContext);
	}
	if (Tmp_win->titlebuttons) {
	    for (i = 0; i < nb; i++) {
		XDeleteContext (dpy, Tmp_win->titlebuttons[i].window,
				TwmContext);
		XDeleteContext (dpy, Tmp_win->titlebuttons[i].window,
				ScreenContext);
	    }
        }
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    MyXftDrawDestroy (Tmp_win->title_w.xft);
#endif
    }

    if (Scr->cmapInfo.cmaps == &Tmp_win->cmaps)
	InstallWindowColormaps(DestroyNotify, &Scr->TwmRoot);

    /*
     * TwmWindows contain the following pointers
     * 
     *     1.  full_name
     *     2.  name
     *     3.  icon_name
     *     4.  wmhints
     *     5.  class.res_name
     *     6.  class.res_class
     *     7.  list
     *     8.  iconmgrp
     *     9.  cwins
     *     10. titlebuttons
     *     11. window ring
     */
    if (Tmp_win->gray) XFreePixmap (dpy, Tmp_win->gray);

    XDestroyWindow(dpy, Tmp_win->frame);
    XSync (dpy, False);
    DiscardWindowEvents (Tmp_win->w, (/*SubstructureNotifyMask|*/StructureNotifyMask)); /*drop Enter/Leave etc*/
    DiscardWindowEvents (Tmp_win->frame, SubstructureRedirectMask); /* -"- */

    if (Tmp_win->title_height) /*drop Expose etc*/
	DiscardWindowEvents (Tmp_win->title_w.win, NoEventMask);

    if (Tmp_win->icon_w.win && !Tmp_win->icon_not_ours) {
#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0)
	    MyXftDrawDestroy(Tmp_win->icon_w.xft);
#endif
#ifdef TWM_USE_RENDER
	if (Tmp_win->PicIconB != None)
	    XRenderFreePicture (dpy, Tmp_win->PicIconB);
	if (Tmp_win->PenIconB != None)
	    XRenderFreePicture (dpy, Tmp_win->PenIconB);
#endif
	XDestroyWindow(dpy, Tmp_win->icon_w.win);
	DiscardWindowEvents (Tmp_win->icon_w.win, NoEventMask);
	IconDown (Tmp_win);
    }

    RemoveIconManager(Tmp_win);					/* 7 */

    Tmp_win->prev->next = Tmp_win->next;
    if (Tmp_win->next != NULL)
	Tmp_win->next->prev = Tmp_win->prev;
    if (Tmp_win->auto_raise) Scr->NumAutoRaises--;

    free_window_names (Tmp_win, True, True, True);		/* 1, 2, 3 */
    if (Tmp_win->wmhints)					/* 4 */
      XFree ((char *)Tmp_win->wmhints);
    if (Tmp_win->class.res_name && Tmp_win->class.res_name != NoName)  /* 5 */
      XFree ((char *)Tmp_win->class.res_name);
    if (Tmp_win->class.res_class && Tmp_win->class.res_class != NoName) /* 6 */
      XFree ((char *)Tmp_win->class.res_class);
    free_cwins (Tmp_win);				/* 9 */
    if (Tmp_win->titlebuttons)					/* 10 */
      free ((char *) Tmp_win->titlebuttons);
    remove_window_from_ring (Tmp_win);				/* 11 */

    if (UnHighLight_win == Tmp_win)
	UnHighLight_win = NULL;

    free((char *)Tmp_win);
}



void
HandleCreateNotify()
{
#ifdef DEBUG_EVENTS
    fprintf(stderr, "events.c -- HandleCreateNotify(): win = 0x%08lx\n", (long)(Event.xcreatewindow.window));
    Bell(XkbBI_Info,0,Event.xcreatewindow.window);
    XSync(dpy, 0);
#endif
}



/**
 *	HandleMapRequest - MapRequest event handler
 */
void
HandleMapRequest()
{
    int state;

    Event.xany.window = Event.xmaprequest.window;
    state = XFindContext(dpy, Event.xany.window, TwmContext, (caddr_t *)&Tmp_win);
    if (state == XCNOENT)
	Tmp_win = NULL;

    /* If the window has never been mapped before ... */
    if (Tmp_win == NULL)
    {
	/* Add decorations. */
	Tmp_win = AddWindow(Event.xany.window, FALSE, (IconMgr *) NULL);
	if (Tmp_win == NULL)
	    return;

#ifdef TWM_USE_OPACITY
	PropagateWindowOpacity (Tmp_win);
#endif
    }
    else
    {
	/*
	 * If the window has been unmapped by the client, it won't be listed
	 * in the icon manager.  Add it again, if requested.
	 */
	if (Tmp_win->list == NULL)
	    (void) AddIconManager (Tmp_win);
    }

    /* If it's not merely iconified, and we have hints, use them. */
    if ((Tmp_win->icon == FALSE)
	    /* use WM_STATE if enabled */
	&& ((RestartPreviousState == TRUE && GetWMState(Tmp_win->w, &state, &JunkChild) == True)
	    || (Tmp_win->wmhints && (Tmp_win->wmhints->flags & StateHint)
			&& (state=Tmp_win->wmhints->initial_state) == state)))
    {
	int zoom_save;

	switch (state)
	{
	    case ZoomState:
		if (Scr->ZoomFunc != ZOOM_NONE)
		    fullzoom (Scr->ZoomPanel, Tmp_win, Scr->ZoomFunc);
	    case NormalState:
	    case InactiveState:
#if DontCareState != WithdrawnState
	    case DontCareState:
#endif
		XMapWindow (dpy, Tmp_win->w);
		XMapWindow (dpy, Tmp_win->frame);
		SetMapStateProp (Tmp_win, NormalState);
		break;

	    case IconicState:
#if DontCareState == WithdrawnState
	    case WithdrawnState:
#endif
		zoom_save = Scr->DoZoom;
		Scr->DoZoom = FALSE;
		Iconify (Tmp_win, 0, 0);
		Scr->DoZoom = zoom_save;
		CheckXUrgencyHint (Tmp_win);
	    default:
		return;
	}
    }
    else
	/* If no hints, or currently an icon, just "deiconify" */
	DeIconify (Tmp_win);

    SetRaiseWindow (Tmp_win);

    CheckXUrgencyHint (Tmp_win);

#ifdef TWM_USE_SLOPPYFOCUS
    if (HandlingEvents == TRUE && ((SloppyFocus == TRUE) || (FocusRoot == TRUE)))
#else
    if (HandlingEvents == TRUE && FocusRoot == TRUE) /* only warp if f.focus is not active */
#endif
	if ((Scr->WarpCursorL != NULL) || (Scr->NoWarpCursorL != NULL) || (Scr->WarpCursor == TRUE))
	{
	    /* warp to the transient window if top-level client has focus: */
	    if (Tmp_win->transient && Focus != NULL) {
		if ((Tmp_win->transientfor == Focus->w) || (Tmp_win->group == Focus->group))
		    WarpToWindow (Tmp_win);
	    } else if (Scr->WarpCursorL != NULL) {
		if (LookInList(Scr->WarpCursorL, Tmp_win->full_name, &Tmp_win->class) != NULL)
		    WarpToWindow (Tmp_win);
	    } else if (Scr->NoWarpCursorL != NULL) {
		if (LookInList(Scr->NoWarpCursorL, Tmp_win->full_name, &Tmp_win->class) == NULL)
		    WarpToWindow (Tmp_win);
	    } else if (Scr->WarpCursor == TRUE)
		WarpToWindow (Tmp_win);
	}
}



void SimulateMapRequest (w)
    Window w;
{
    Event.xmaprequest.window = w;
    HandleMapRequest ();
}



/**
 *	MapNotify event handler
 */
void
HandleMapNotify()
{
    if (Tmp_win == NULL)
	return;

    /*
     * Need to do the grab to avoid race condition of having server send
     * MapNotify to client before the frame gets mapped; this is bad because
     * the client would think that the window has a chance of being viewable
     * when it really isn't.
     */
    XGrabServer (dpy);
    if (Tmp_win->icon_w.win)
	XUnmapWindow(dpy, Tmp_win->icon_w.win);
    if (Tmp_win->title_w.win)
	XMapSubwindows(dpy, Tmp_win->title_w.win);
    XMapSubwindows(dpy, Tmp_win->frame);
    if (Focus != Tmp_win && Tmp_win->hilite_w)
	XUnmapWindow(dpy, Tmp_win->hilite_w);

    XMapWindow(dpy, Tmp_win->frame);
    XUngrabServer (dpy);
    XFlush (dpy);
    Tmp_win->mapped = TRUE;
    Tmp_win->icon = FALSE;
    Tmp_win->icon_on = FALSE;
}



/**
 * UnmapNotify event handler
 */
void
HandleUnmapNotify()
{
    int dstx, dsty, nchild;
    Window *child;
    Bool state;

    /*
     * The July 27, 1988 ICCCM spec states that a client wishing to switch
     * to WithdrawnState should send a synthetic UnmapNotify with the
     * event field set to (pseudo-)root, in case the window is already
     * unmapped (which is the case for twm for IconicState).  Unfortunately,
     * we looked for the TwmContext using that field, so try the window
     * field also.
     */
    if (Tmp_win == NULL)
    {
	Event.xany.window = Event.xunmap.window;
	if (XFindContext(dpy, Event.xany.window,
	    TwmContext, (caddr_t *)&Tmp_win) == XCNOENT)
	    Tmp_win = NULL;
    }

    if (Tmp_win == NULL || (!Tmp_win->mapped && !Tmp_win->icon))
	return;

    if (Tmp_win == Focus)
	FocusOnRoot();

    /*
     * The program may have unmapped the client window, from either
     * NormalState or IconicState.  Handle the transition to WithdrawnState.
     *
     * We need to reparent the window back to the root (so that twm exiting 
     * won't cause it to get mapped) and then throw away all state (pretend 
     * that we've received a DestroyNotify).
     */

    XGrabServer (dpy);

    /* check if the client window exists: */
    state = False;
    nchild = 0;
    child = NULL;
    if (XQueryTree (dpy, Tmp_win->frame, &JunkRoot, &JunkChild, &child, &nchild))
	if (child != NULL) {
	    while (nchild-- > 0)
		if (child[nchild] == Tmp_win->w /*Event.xunmap.window*/) {
		    state = True;
		    XTranslateCoordinates (dpy, Tmp_win->w /*Event.xunmap.window*/,
				Tmp_win->attr.root, 0, 0, &dstx, &dsty, &JunkChild);
		    break;
		}
	    XFree (child);
	}

    if (state == True)
    {
	XEvent ev;
	Bool reparented = XCheckTypedWindowEvent (dpy,
					Tmp_win->w /*Event.xunmap.window*/,
					ReparentNotify, &ev);
	SetMapStateProp (Tmp_win, WithdrawnState);
	if (reparented) {
	    if (Tmp_win->old_bw) XSetWindowBorderWidth (dpy,
					Tmp_win->w /*Event.xunmap.window*/,
					Tmp_win->old_bw);
	    if (Tmp_win->wmhints && (Tmp_win->wmhints->flags & IconWindowHint))
	      XUnmapWindow (dpy, Tmp_win->wmhints->icon_window);
	} else {
	    XReparentWindow (dpy, Tmp_win->w /*Event.xunmap.window*/,
				Tmp_win->attr.root,
				dstx, dsty);
	    RestoreWithdrawnLocation (Tmp_win);
	}
	XRemoveFromSaveSet (dpy, Tmp_win->w /*Event.xunmap.window*/);
	XSelectInput (dpy, Tmp_win->w /*Event.xunmap.window*/, NoEventMask);
	HandleDestroyNotify ();		/* do not need to mash event before */
    }
    else /* window no longer exists and we'll get a destroy notify */
    {
#if 0
	XUnmapWindow (dpy, Tmp_win->frame);
	XSync (dpy, False);
	DiscardWindowEvents (Tmp_win->w, (SubstructureNotifyMask|StructureNotifyMask));
	DiscardWindowEvents (Tmp_win->frame, SubstructureRedirectMask);
	Tmp_win->mapped = FALSE; /* avoid FocusIn/Out, Enter/Leave events */
	Tmp_win->icon = TRUE;
	Tmp_win->icon_on = FALSE;
#endif
    }

    XUngrabServer (dpy);
    XFlush (dpy);
}



/**
 * MotionNotify event handler
 */
void
HandleMotionNotify()
{
    if (ResizeWindow != (Window) 0)
    {
	XQueryPointer( dpy, Event.xany.window,
	    &(Event.xmotion.root), &JunkChild,
	    &(Event.xmotion.x_root), &(Event.xmotion.y_root),
	    &(Event.xmotion.x), &(Event.xmotion.y),
	    &JunkMask);

	/* Set WindowMoved appropriately so that f.deltastop will
	   work with resize as well as move. */
	if (abs (Event.xmotion.x - ResizeOrigX) >= Scr->MoveDelta
	    || abs (Event.xmotion.y - ResizeOrigY) >= Scr->MoveDelta)
	  WindowMoved = TRUE;

	XFindContext(dpy, ResizeWindow, TwmContext, (caddr_t *)&Tmp_win);
	DoResize(Event.xmotion.x_root, Event.xmotion.y_root, Tmp_win);
    }
}



/**
 * ButtonRelease event handler
 */
void
HandleButtonRelease()
{
    int xl, yt, w, h;
    unsigned mask;

    if (InfoLines) 		/* delete info box on 2nd button release  */
      if (Context == C_IDENTIFY) {
	XUnmapWindow(dpy, Scr->InfoWindow.win);
	InfoLines = 0;
	Context = C_NO_CONTEXT;
      }

    if (DragWindow != None)
    {
	MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);

	XFindContext(dpy, DragWindow, TwmContext, (caddr_t *)&Tmp_win);
	if (DragWindow == Tmp_win->frame)
	{
	    xl = Event.xbutton.x_root - DragX - Tmp_win->frame_bw;
	    yt = Event.xbutton.y_root - DragY - Tmp_win->frame_bw;
	    w = DragWidth + 2 * Tmp_win->frame_bw;
	    h = DragHeight + 2 * Tmp_win->frame_bw;
	}
	else
	{
	    xl = Event.xbutton.x_root - DragX - Scr->IconBorderWidth;
	    yt = Event.xbutton.y_root - DragY - Scr->IconBorderWidth;
	    w = DragWidth + 2 * Scr->IconBorderWidth;
	    h = DragHeight + 2 * Scr->IconBorderWidth;
	}

	if (ConstMove)
	{
	    if (ConstMoveDir == MOVE_HORIZ)
		yt = ConstMoveY;

	    if (ConstMoveDir == MOVE_VERT)
		xl = ConstMoveX;

	    if (ConstMoveDir == MOVE_NONE)
	    {
		yt = ConstMoveY;
		xl = ConstMoveX;
	    }
	}
	
	if (Scr->DontMoveOff && MoveFunction != F_FORCEMOVE)
	    EnsureRectangleOnScreen (&xl, &yt, w, h);

	CurrentDragX = xl;
	CurrentDragY = yt;
	if (DragWindow == Tmp_win->frame)
	  SetupWindow (Tmp_win, xl, yt,
		       Tmp_win->frame_width, Tmp_win->frame_height, -1);
	else
	    XMoveWindow (dpy, DragWindow, xl, yt);

	if (!Scr->NoRaiseMove && !Scr->OpaqueMove)    /* opaque already did */
	    XRaiseWindow(dpy, DragWindow);

	if (!Scr->OpaqueMove)
	    UninstallRootColormap();
	else
	    XSync(dpy, 0);

	if (Scr->NumAutoRaises) {
	    enter_flag = TRUE;
	    enter_win = NULL;
	    raise_win = ((DragWindow == Tmp_win->frame && !Scr->NoRaiseMove)
			 ? Tmp_win : NULL);
	}

	DragWindow = (Window) 0;
	ConstMove = FALSE;
    }

    if (ResizeWindow != (Window) 0)
    {
	EndResize();
    }

    if (ActiveMenu != NULL && RootFunction == 0)
    {
	if (ActiveItem != NULL)
	{
	    int func = ActiveItem->func;
	    Action = ActiveItem->action;
	    switch (func) {
	      case F_MOVE:
	      case F_FORCEMOVE:
		ButtonPressed = -1;
		break;
	      case F_WARPTOSCREEN:
		XUngrabPointer(dpy, CurrentTime);
		/* fall through */
	      case F_CIRCLEUP:
	      case F_CIRCLEDOWN:
	      case F_REFRESH:
		PopDownMenu();
		break;
	      default:
		break;
	    }
	    ExecuteFunction(func, Action,
		ButtonWindow ? ButtonWindow->frame : None,
		ButtonWindow, &Event/*&ButtonEvent*/, Context, TRUE);
	    Context = C_NO_CONTEXT;
	    ButtonWindow = NULL;

	    /* if we are not executing a defered command, then take down the
	     * menu
	     */
	    if (RootFunction == 0)
	    {
		PopDownMenu();
	    }
	}
	else
	    PopDownMenu();
    }

    mask = (Button1Mask|Button2Mask|Button3Mask|Button4Mask|Button5Mask);
    switch (Event.xbutton.button)
    {
	case Button1: mask &= ~Button1Mask; break;
	case Button2: mask &= ~Button2Mask; break;
	case Button3: mask &= ~Button3Mask; break;
	case Button4: mask &= ~Button4Mask; break;
	case Button5: mask &= ~Button5Mask; break;
    }

    if (RootFunction != 0 ||
	ResizeWindow != None ||
	DragWindow != None)
	ButtonPressed = -1;

    if (RootFunction == 0 &&
	(Event.xbutton.state & mask) == 0 &&
	DragWindow == None &&
	ResizeWindow == None)
    {
	XUngrabPointer(dpy, CurrentTime);
	XUngrabServer(dpy);
	XFlush(dpy);
	EventHandler[EnterNotify] = HandleEnterNotify;
	EventHandler[LeaveNotify] = HandleLeaveNotify;
	ButtonPressed = -1;
	if (DownIconManager)
	{
	    DownIconManager->down = FALSE;
	    if (Scr->Highlight) DrawIconManagerBorder(DownIconManager);
	    DownIconManager = NULL;
	}
	Cancel = FALSE;
    }
}



/**
 *
 * \param menu menu to pop up
 * \param w    invoking window, or None
 */
static void 
do_menu (MenuRoot *menu, Window w)
{
    int x = Event.xbutton.x_root;
    int y = Event.xbutton.y_root;
    Bool center;

    if (!Scr->NoGrabServer)
	XGrabServer(dpy);
    if (w) {
	int h = Scr->TBInfo.width - Scr->TBInfo.border;
	Window child;

	(void) XTranslateCoordinates (dpy, w, Scr->Root, 0, h, &x, &y, &child);
	center = False;
    } else {
	center = True;
    }
    if (PopUpMenu (menu, x, y, center)) {
	UpdateMenu();
    } else {
	Bell(XkbBI_MinorError,0,w);
    }
}



/**
 * ButtonPress event handler
 */
void
HandleButtonPress()
{
    unsigned int modifier;
    Cursor cur;

    /* too much code relies on this assumption */
    if (Event.xbutton.button > MAX_BUTTONS)
	return;

    /* pop down the menu, if any */
    if (ActiveMenu != NULL)
	PopDownMenu();

    XSync(dpy, 0);			/* XXX - remove? */

#if 1
/*
 * Lets suppose we have info-window mapped.  Further, lets suppose
 * we have a menu bound to the title bar tied to some mouse button.
 * Now press this button, the menu gets mapped. Don't release this
 * button but press some other one.  We execute this handler here
 * again, and few lines above we take the menu down. Then we enter
 * the 'if' statement below and grab the pointer. Next, releasing
 * one of the buttons we execute HandleButtonRelease() above and
 * bypass all code, the 'mask' calculation doesn't result in releasing
 * the grab we obtain below.  So only killing twm releases the grab.
 *
 * Until a proper solution to this problem is found lets suppose we
 * have no info window mapped.
 */
    if (ButtonPressed != -1)
#else
    if (ButtonPressed != -1 && !InfoLines) /* want menus if we have info box */
#endif
    {
	/* we got another butt press in addition to one still held
	 * down, we need to cancel the operation we were doing
	 */
	Cancel = TRUE;
	CurrentDragX = origDragX;
	CurrentDragY = origDragY;
	if (!menuFromFrameOrWindowOrTitlebar) {
	  if (Scr->OpaqueMove && DragWindow != None) {
	    XMoveWindow (dpy, DragWindow, origDragX, origDragY);
	  } else {
	    MoveOutline(Scr->Root, 0, 0, 0, 0, 0, 0);
	  }
	}
	XUnmapWindow(dpy, Scr->SizeWindow.win);
	if (!Scr->OpaqueMove)
	    UninstallRootColormap();
	ResizeWindow = None;
	DragWindow = None;
	cur = LeftButt;
	if (Event.xbutton.button == Button2)
	    cur = MiddleButt;
	else if (Event.xbutton.button >= Button3)
	    cur = RightButt;

	XGrabPointer(dpy, Scr->Root, True,
	    ButtonReleaseMask | ButtonPressMask,
	    GrabModeAsync, GrabModeAsync,
	    Scr->Root, cur, CurrentTime);

	return;
    }
    else
	ButtonPressed = Event.xbutton.button;

    if (ResizeWindow != None ||
	DragWindow != None  ||
	ActiveMenu != NULL)
	return;

    /* check the title bar buttons */
    if (Tmp_win && Tmp_win->title_height && Tmp_win->titlebuttons)
    {
	register int i;
	register TBWindow *tbw;
	int nb = Scr->TBInfo.nleft + Scr->TBInfo.nright;

	for (i = 0, tbw = Tmp_win->titlebuttons; i < nb; i++, tbw++) {
	    if (Event.xany.window == tbw->window) {
		if (tbw->info->func == F_MENU) {
		    Context = C_TITLE;
		    ButtonEvent = Event;
		    ButtonWindow = Tmp_win;
		    do_menu (tbw->info->menuroot, tbw->window);
		} else {
		    ExecuteFunction (tbw->info->func, tbw->info->action,
				     Event.xany.window, Tmp_win, &Event,
				     C_TITLE, FALSE);
		}
		return;
	    }
	}
    }

    Context = C_NO_CONTEXT;

    if (Event.xany.window == Scr->InfoWindow.win)
      Context = C_IDENTIFY;

    if (Event.xany.window == Scr->Root)
	Context = C_ROOT;
    if (Tmp_win)
    {
	if (Tmp_win->list && RootFunction != 0 &&
	    (Event.xany.window == Tmp_win->list->w.win ||
		Event.xany.window == Tmp_win->list->icon))
	{
	    Tmp_win = Tmp_win->list->iconmgr->twm_win;
	    XTranslateCoordinates(dpy, Event.xany.window, Tmp_win->w,
		Event.xbutton.x, Event.xbutton.y, 
		&JunkX, &JunkY, &JunkChild);

	    Event.xbutton.x = JunkX;
	    Event.xbutton.y = JunkY - Tmp_win->title_height;
	    Event.xany.window = Tmp_win->w;
	    Context = C_WINDOW;
	}
	else if (Event.xany.window == Tmp_win->title_w.win)
	{
	    Context = C_TITLE;
	}
	else if (Event.xany.window == Tmp_win->w) 
	{
	    printf("ERROR! ERROR! ERROR! YOU SHOULD NOT BE HERE!!!\n");
	    Context = C_WINDOW;
	}
	else if (Event.xany.window == Tmp_win->icon_w.win)
	{
	    Context = C_ICON;
	}
	else if (Event.xany.window == Tmp_win->frame)
	{
	    /* since we now place a button grab on the frame instead
             * of the window, (see GrabButtons() in add_window.c), we
             * need to figure out where the pointer exactly is before
             * assigning Context.  If the pointer is on the application
             * window we will change the event structure to look as if
             * it came from the application window.
	     */
	    if (Event.xbutton.subwindow == Tmp_win->w) {
	      Event.xbutton.window = Tmp_win->w;
              Event.xbutton.y -= Tmp_win->title_height;
/*****
              Event.xbutton.x -= Tmp_win->frame_bw;
*****/
	      Context = C_WINDOW;
	    }
            else Context = C_FRAME;
	}
	else if (Tmp_win->list &&
	    (Event.xany.window == Tmp_win->list->w.win ||
		Event.xany.window == Tmp_win->list->icon))
	{
	    Tmp_win->list->down = TRUE;
	    if (Scr->Highlight) DrawIconManagerBorder(Tmp_win->list);
	    DownIconManager = Tmp_win->list;
	    Context = C_ICONMGR;
	}
    }

    /* this section of code checks to see if we were in the middle of
     * a command executed from a menu
     */
    if (RootFunction != 0)
    {
	if (Event.xany.window == Scr->Root)
	{
	    /* if the window was the Root, we don't know for sure it
	     * it was the root.  We must check to see if it happened to be
	     * inside of a client that was getting button press events.
	     */
	    XTranslateCoordinates(dpy, Scr->Root, Scr->Root,
		Event.xbutton.x, 
		Event.xbutton.y, 
		&JunkX, &JunkY, &Event.xany.window);

	    if (Event.xany.window == 0 ||
		(XFindContext(dpy, Event.xany.window, TwmContext,
			      (caddr_t *)&Tmp_win) == XCNOENT))
	    {
		RootFunction = 0;
		Bell(XkbBI_MinorError,0,Event.xany.window);
		return;
	    }

	    XTranslateCoordinates(dpy, Scr->Root, Event.xany.window,
		Event.xbutton.x, 
		Event.xbutton.y, 
		&JunkX, &JunkY, &JunkChild);

	    Event.xbutton.x = JunkX;
	    Event.xbutton.y = JunkY;
	    Context = C_WINDOW;
	}

	/* make sure we are not trying to move an identify window */
	if (Event.xany.window != Scr->InfoWindow.win)
{
#ifdef DEBUG_HOTBUTTONS
	    if (PrintErrorMessages) {
		extern char *return_keyword(), *return_context(), *return_modifiers();
		fprintf (stderr, "HandleButtonPress(): Executing Button%d modifiers '%s' RootFunction '%s (%d)' arg = '%s' for context '%s'\n", Event.xbutton.button, return_modifiers(Event.xbutton.state), return_keyword(RootFunction), RootFunction, (Action?Action:""), return_context(Context));
	  }
#endif
	  ExecuteFunction(RootFunction, Action, Event.xany.window,
			  Tmp_win, &Event, Context, FALSE);
}

	RootFunction = 0;
	return;
    }

    ButtonEvent = Event;
    ButtonWindow = Tmp_win;

    /* if we get to here, we have to execute a function or pop up a 
     * menu
     */
    modifier = (Event.xbutton.state & mods_used);

    if (Context == C_NO_CONTEXT)
	return;

    RootFunction = 0;
    if (Scr->Mouse[Event.xbutton.button][Context][modifier].func == F_MENU)
    {
	do_menu (Scr->Mouse[Event.xbutton.button][Context][modifier].menu,
		 (Window) None);
    }
    else if (Scr->Mouse[Event.xbutton.button][Context][modifier].func != 0)
    {
	Action = Scr->Mouse[Event.xbutton.button][Context][modifier].item ?
	    Scr->Mouse[Event.xbutton.button][Context][modifier].item->action : NULL;
#if defined DEBUG_HOTBUTTONS && 1 /*on/off*/
	if (PrintErrorMessages) {
	    extern char *return_keyword(), *return_context(), *return_modifiers();
	    fprintf (stderr, "HandleButtonPress(): Triggering Button%d pressed-mod '%s (twm-mod 0x0%x, twm-used = 0x0%x)' function '%s (%d)' arg = '%s' for context '%s' (frame=0x0%lx client=0x0%lx)\n", Event.xbutton.button, return_modifiers(Event.xbutton.state), modifier, mods_used, return_keyword(Scr->Mouse[Event.xbutton.button][Context][modifier].func), Scr->Mouse[Event.xbutton.button][Context][modifier].func, (Action?Action:""), return_context(Context), (long)(Tmp_win?Tmp_win->frame:-1), (long)(Tmp_win?Tmp_win->w:-1));
	}
#endif
	ExecuteFunction(Scr->Mouse[Event.xbutton.button][Context][modifier].func,
	    Action, Event.xany.window, Tmp_win, &Event, Context, FALSE);
    }
    else if (Scr->DefaultFunction.func != 0)
    {
	if (Scr->DefaultFunction.func == F_MENU)
	{
	    do_menu (Scr->DefaultFunction.menu, (Window) None);
	}
	else
	{
	    Action = Scr->DefaultFunction.item ?
		Scr->DefaultFunction.item->action : NULL;
	    ExecuteFunction(Scr->DefaultFunction.func, Action,
	       Event.xany.window, Tmp_win, &Event, Context, FALSE);
	}
    }
}



/**
 * EnterNotify event handler
 */
void
HandleEnterNotify()
{
    MenuRoot *mr;

    /*
     * if we aren't in the middle of menu processing
     */
    if (!ActiveMenu) {
	/*
	 * We're not interested in pseudo Enter/Leave events generated
	 * from grab initiations.
	 */
	if (Event.xcrossing.mode == NotifyGrab || Event.xcrossing.detail == NotifyPointer)
	    return;

	/*
	 * if entering root window, restore twm default colormap so that 
	 * titlebars are legible
	 */
	if (Event.xcrossing.window == Scr->Root) {
	    if (FocusRoot == TRUE)
		InstallWindowColormaps(EnterNotify, &Scr->TwmRoot);
	    return;
	}

	/*
	 * We're not interested in Enter events while coming
	 * from subwindows (client 'w', 'title_w').
	 */
	if (Event.xcrossing.detail == NotifyInferior)
	    return;

	/*
	 * if we have an event for a specific one of our windows
	 */
	if (Tmp_win != NULL)
	{
	    int inicon = WinIsIconMgrEntry (Event.xcrossing.window, Tmp_win);

	    if ((Event.xcrossing.window == Tmp_win->frame) || inicon)
		SetBorder (Tmp_win, True);

	    /*
	     * If currently in PointerRoot mode (indicated by FocusRoot), then
	     * focus on this window
	     */

#ifdef TWM_USE_SLOPPYFOCUS
	    /*
	     * If both SloppyFocus and TitleFocus are 'TRUE' we need to ensure the client
	     * colormap gets installed as we enter the client window.  Focus is already
	     * there (since the 'Enter'-event into the frame), resulting in skipping the
	     * relevant part of the below 'if' statement, therefore unfortunately
	     * a special case handling here is necessary.
	     */
	    if (SloppyFocus == TRUE && Tmp_win == Focus
		    && Scr->TitleFocus == TRUE && Event.xcrossing.window == Tmp_win->w)
	    {
		InstallWindowColormaps (EnterNotify, Tmp_win);
	    }

	    else if ((SloppyFocus == TRUE && Tmp_win != Focus) || (FocusRoot == TRUE))
#else
	    if (FocusRoot == TRUE)
#endif
	    {
		int input = (AcceptsInput(Tmp_win) || ExpectsTakeFocus(Tmp_win));

		if (Tmp_win->mapped == TRUE)
		{
		    /*
		     * If entering the frame or the icon manager, then do 
		     * "window activation things":
		     *
		     *     1.  install frame colormap
		     *     2.  focus on client window to forward typing
		     *     2a. same as 2 but for icon mgr w/with NoTitlebar on.
		     *     3.  send WM_TAKE_FOCUS if requested
		     */

		    if ((Event.xcrossing.window == Tmp_win->frame) || inicon)
		    {
#if 1
			if (Scr->TwmRoot.cmaps.number_cwins > 1)
			{
			    if (Event.xcrossing.window == Tmp_win->frame
				&& 0 < Tmp_win->title_height && Event.xcrossing.y < Tmp_win->title_height)
			    {
				/* we entered frame over title-bar, load twm colormap */
				int i = Scr->TwmRoot.cmaps.number_cwins-1;
				ColormapWindow *cwins = Scr->TwmRoot.cmaps.cwins[0];
				/* bump screen/twm colormap: */
				Scr->TwmRoot.cmaps.cwins[0] = Scr->TwmRoot.cmaps.cwins[i];  Scr->TwmRoot.cmaps.cwins[i] = cwins;
				Scr->TwmRoot.cmaps.cwins[0]->colormap->state = Scr->TwmRoot.cmaps.cwins[i]->colormap->state = 0;
				InstallWindowColormaps (EnterNotify, &Scr->TwmRoot);
				/* unbump screen/twm colormap: */
				Scr->TwmRoot.cmaps.cwins[i] = Scr->TwmRoot.cmaps.cwins[0];  Scr->TwmRoot.cmaps.cwins[0] = cwins;
			    }
#if 1
			    else if (inicon) /* we are over its iconmanager entry, load client colormap */
				InstallWindowColormaps (EnterNotify, Tmp_win);
#endif
			}
			else
#endif
			    InstallWindowColormaps (EnterNotify, &Scr->TwmRoot); /* 1 */

			if (input && (Scr->TitleFocus == TRUE))
			{
			    if (AcceptsInput(Tmp_win)) /* 2, 2a */
				SetFocus (Tmp_win, Event.xcrossing.time);
			    else if (Tmp_win->hilite_w)
				/* ...no FocusIn event coming: */
				XMapWindow (dpy, Tmp_win->hilite_w);

			    if (ExpectsTakeFocus(Tmp_win)) /* 3 */
				SendTakeFocusMessage (Tmp_win, Event.xcrossing.time);
#ifdef TWM_USE_SLOPPYFOCUS
			    if (SloppyFocus == TRUE)
				FocusRoot = FALSE;
#endif
			    Focus = Tmp_win;
			    HighLightIconManager (Focus);
			}
			else /* client accepts no input, or TitleFocus is FALSE; in iconmgr or not */
			{
#ifdef TWM_USE_SLOPPYFOCUS
			    if (FocusRoot == FALSE) {
				UnHighLightIconManager();
				FocusOnRoot();
			    }
#endif
			    if (Tmp_win->list != NULL)
				SetIconMgrEntryActive (Tmp_win->list);
			}
		    }
		    else if (Event.xcrossing.window == Tmp_win->w)
		    {
			if (input && (Scr->TitleFocus == FALSE))
			{
#ifdef TWM_USE_SLOPPYFOCUS
			    if (SloppyFocus == TRUE)
			    {
				FocusRoot = FALSE;
				if (AcceptsInput(Tmp_win))
				    SetFocus (Tmp_win, Event.xcrossing.time);
			    }
			    else /*fallthrough*/
#endif
			    if (ExpectsTakeFocus(Tmp_win)) {
				if (ExpectsInput(Tmp_win))
				    SetFocus (Tmp_win, Event.xcrossing.time);
				SendTakeFocusMessage (Tmp_win, Event.xcrossing.time);
			    } else if (Tmp_win->hilite_w)
				/* ...no FocusIn event coming: */
				XMapWindow (dpy, Tmp_win->hilite_w);

			    Focus = Tmp_win;
			    HighLightIconManager (Focus);
			}
			/*
			 * If we are entering the application window,
			 * install its colormap(s).
			 */
			InstallWindowColormaps (EnterNotify, Tmp_win);
		    }
		}
		else /* ...client not mapped: */
		{
		    if (inicon)
		    {
			/* Mouse over *iconified* entry in iconmgr: */
			if (input)
			{
#ifdef TWM_USE_SLOPPYFOCUS
			    if (FocusRoot == FALSE) {
				UnHighLightIconManager();
				FocusOnRoot();
			    }
#endif
			    if (Scr->TitleFocus == TRUE)
				HighLightIconManager (Tmp_win);
			    else
				SetIconMgrEntryActive (Tmp_win->list);
			}
			else
			{
			    SetIconMgrEntryActive (Tmp_win->list);
			}
		    } else if (Tmp_win->wmhints != NULL
			    && Tmp_win->wmhints->icon_window == Event.xcrossing.window)
			InstallWindowColormaps (EnterNotify, Tmp_win);
		}
	    }

	    /*
	     * If this window is to be autoraised, mark it so
	     */
	    if (Tmp_win->auto_raise) {
		enter_win = Tmp_win;
		if (enter_flag == FALSE)
		    AutoRaiseWindow (Tmp_win);
	    } else if (enter_flag && raise_win == Tmp_win)
	      enter_win = Tmp_win;
#if 1
	    else
		/* iconmgr special treatment: raise now: */
		if (inicon && Tmp_win->list->iconmgr->twm_win->auto_raise)
		    XRaiseWindow (dpy, Tmp_win->list->iconmgr->twm_win->frame);
#endif

	    /*
	     * set ring leader
	     */
	    if (Tmp_win->ring.next && (!enter_flag || raise_win == enter_win))
	      Scr->RingLeader = Tmp_win;

	    XFlush (dpy);
	    return;
	}				/* end if Tmp_win */
    }					/* end if !ActiveMenu */

    /*
     * Find the menu that we are dealing with now; punt if unknown
     */
    if (XFindContext (dpy, Event.xcrossing.window, MenuContext, (caddr_t *)&mr) != XCSUCCESS)
	return;

    mr->entered = TRUE;
    if (ActiveMenu && mr == ActiveMenu->prev && RootFunction == 0) {
	if (Scr->Shadow) XUnmapWindow (dpy, ActiveMenu->shadow);
	XUnmapWindow (dpy, ActiveMenu->w.win);
	ActiveMenu->mapped = UNMAPPED;
	UninstallRootColormap ();
	if (ActiveItem) {
	    ActiveItem->state = 0;
	    PaintEntry (ActiveMenu, ActiveItem,  False);
	}
	ActiveItem = NULL;
	ActiveMenu = mr;
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE) {
	    if (Scr->Shadow && Scr->XCompMgrRunning == FALSE) {
		extern void RepaintTranslucentShadow (MenuRoot*);
		/* trick cleanup menuwindow semitransparent shadow area */
		XUnmapWindow (dpy, mr->shadow);
		XSync (dpy, False);
		XMapWindow (dpy, mr->shadow);
		RepaintTranslucentShadow (mr);
	    }
	}
#endif
	MenuDepth--;
    }
}



/**
 * LeaveNotify event handler
 */
void
HandleLeaveNotify()
{
    /*
     * We're not interested in pseudo Enter/Leave events generated
     * from grab initiations and terminations.
     */
    if (Event.xcrossing.mode != NotifyNormal || Event.xcrossing.detail == NotifyPointer)
	return;

    /*
     * We're not interested in Leave events while going
     * into subwindows (client 'w', 'title_w').
     */
    if (Event.xcrossing.detail == NotifyInferior)
	return;

    if (Tmp_win != NULL)
    {
	int inicon = WinIsIconMgrEntry (Event.xcrossing.window, Tmp_win);

	if ((Event.xcrossing.window == Tmp_win->frame) || inicon)
	    SetBorder (Tmp_win, False);

	if (FocusRoot == TRUE)
	{
	    int input = AcceptsInput (Tmp_win);

	    if ((Event.xcrossing.window == Tmp_win->frame) || inicon)
	    {
		if (input)
		{
		    if ((Scr->TitleFocus == TRUE) || inicon)
		    {
			SetFocus ((TwmWindow *) NULL, Event.xcrossing.time);
			if (Tmp_win->hilite_w && ExpectsTakeFocus(Tmp_win))
			    XUnmapWindow (dpy, Tmp_win->hilite_w);
			UnHighLightIconManager();
			Focus = NULL;
		    }
		}
	    }
	    else if (Event.xcrossing.window == Tmp_win->w)
	    {
		if (input)
		{
		    if (Scr->TitleFocus == FALSE)
		    {
			/* Mouse leaving client window in 'PointerRoot' mode: */
			if (ExpectsInput(Tmp_win) && ExpectsTakeFocus(Tmp_win))
			    SetFocus ((TwmWindow *) NULL, Event.xcrossing.time);
			else if (Tmp_win->hilite_w)
			    /* ...no FocusOut event coming: */
			    XUnmapWindow (dpy, Tmp_win->hilite_w);
			UnHighLightIconManager();
			Focus = NULL;
		    }
		}

		InstallWindowColormaps (LeaveNotify, &Scr->TwmRoot);
	    }
	}

	if (Scr->RingLeader && Scr->RingLeader == Tmp_win
	    && Event.xcrossing.window == (Scr->TitleFocus == TRUE ? Tmp_win->frame : Tmp_win->w))
	{
	    if (!inicon) {
		/* only save mouse location if leaving from inside the client area: */
		if (Event.xcrossing.x > 0 && Event.xcrossing.x < Tmp_win->frame_width-1
		    && Event.xcrossing.y > 0 && Event.xcrossing.y < Tmp_win->frame_height-1)
		{
		    Tmp_win->ring.curs_x = Event.xcrossing.x;
		    Tmp_win->ring.curs_y = Event.xcrossing.y;
		}
	    }
	    Scr->RingLeader = (TwmWindow *) NULL;
	}

	XFlush (dpy);
    }
}



/**
 *	FocusChange event handler
 */
void
HandleFocusChange()
{
    if (Event.xfocus.detail != NotifyInferior && Event.xfocus.detail != NotifyPointer)
    {
	if (Tmp_win == NULL)
	{
	    if (Event.xfocus.type == FocusIn
		&& (Event.xfocus.detail == NotifyDetailNone || Event.xfocus.detail == NotifyPointerRoot))
	    {
		/*
		 * Some (override_redirect or windowless?) X11-client attempts to turn off focus.
		 * It is essential for twm to not allow this as it deactivates the keyboard (and
		 * all twm-bound "hotkeys".
		 * If someone sets focus to "NotifyPointerRoot", it is not that fatal and let it
		 * be so.
		 * Further, if some override_redirect client (or some client on some
		 * non-twm-managed screen) hijacks focus, we cannot do anything about this.
		 */
		if (Event.xfocus.detail == NotifyDetailNone)
		    FocusOnRoot();
		else {
#if 1		    /* some servers are buggy (XDarwin on PPC) */
		    XGetInputFocus (dpy, &JunkChild, &JunkX);
		    if (JunkChild != PointerRoot)
			return;
#endif
		    FocusedOnRoot();
		}
		UnHighLightIconManager();
	    }
	}
	else
	{
	    if (Event.xfocus.type == FocusIn)
	    {
		static Window thf = None;

		if (Focus != Tmp_win || (FocusRoot == TRUE && Event.xfocus.window == Tmp_win->frame))
		{
		    int f;
		    if (Event.xfocus.window == Tmp_win->frame)
		    {
			if (Event.xfocus.detail == NotifyNonlinear /* || Event.xfocus.detail == NotifyNonlinearVirtual */ || Event.xfocus.detail == NotifyAncestor)
			{
			    /*
			     * Accept focus change because probably some external entity
			     * assigned focus to the 'frame' window; simply redirect it
			     * to the client 'w'.  Don't resist.
			     */
			    if (AcceptsInput (Tmp_win)) {
				/* don't trigger FocusOut for 'frame', do trigger 'FocusIn' for 'w' */
				XWindowAttributes a;
				XGetWindowAttributes (dpy, Tmp_win->frame, &a);
				XSelectInput (dpy, Tmp_win->frame, a.your_event_mask & ~FocusChangeMask);
				SetFocus (Tmp_win, CurrentTime);
				XSelectInput (dpy, Tmp_win->frame, a.your_event_mask);
				if (ExpectsTakeFocus(Tmp_win))
				    SendTakeFocusMessage (Tmp_win, lastTimestamp);
				FocusRoot = FALSE; /* enter F_FOCUS mode */
			    }
			}
			return;
		    }
#ifdef TWM_ENABLE_STOLENFOCUS_RECOVERY
		    else if (GloballyActive (Tmp_win))
		    { /* accept focus change into a ICCCM "Globally Active" client */ }
		    else if (RecoverStolenFocusTimeout >= 0)
		    {
			/*
			 * This shouldn't happen but some clients "grab" focus unexpectedly,
			 * so update TWM's knowledge where the focus actually is.
			 *
			 * Accept focus transfer if it happens inside a window group
			 * or to/from a "transient-for" window.
			 *
			 * Accept focus transfer too, if the mouse appears to be inside
			 * the client grabbing the focus -- physical mouse transfer,
			 * Enter/Leave and FocusIn/FocusOut are not always synchronised?
			 */
			if (Event.xfocus.mode == NotifyNormal || Event.xfocus.mode == NotifyWhileGrabbed)
			{
			    /* attn: we accept unexpected focus changes by NotifyGrab/NotifyUngrab */
			    f = (Tmp_win->frame_bw << 1);
			    if ( ((Focus == NULL) || !IS_RELATED_WIN(Focus,Tmp_win))
#if 0				/* check if mouse is over the greedy client, obscured or visible: */
				&&
				(False == XQueryPointer(dpy, Tmp_win->frame, &JunkRoot, &JunkChild, &JunkX, &JunkY, &HotX, &HotY, &JunkMask)
				    || HotX < 0 || HotX >= Tmp_win->frame_width + f || HotY < 0 || HotY >= Tmp_win->frame_height + f)
			/*	&&
				(Tmp_win->list == NULL || Tmp_win->list->iconmgr->twm_win->mapped == FALSE
				|| False == XTranslateCoordinates(dpy, Tmp_win->list->iconmgr->scr->Root, Tmp_win->list->w.win, JunkX, JunkY, &HotX, &HotY, &JunkChild)
				    || HotX < 0 || HotX >= Tmp_win->list->width || HotY < 0 || HotY >= Tmp_win->list->height) */
#endif
			    )
			    {
				auto struct timeval clk;
				auto long stamp1;
				static long stamp0 = 0; /* milliseconds measure */
				static int attempts = 0;

				X_GETTIMEOFDAY (&clk);
				stamp1 = (long)(clk.tv_sec) * 1000 + (long)(clk.tv_usec) / 1000;

				/* attempt to return focus: */
				if (thf != Tmp_win->frame)
				{
				    thf = Tmp_win->frame;
				    stamp0 = stamp1;
				    attempts = 0;
				}

				if (stamp1 - stamp0 <= RecoverStolenFocusTimeout && attempts < RecoverStolenFocusAttempts)
				{
				    attempts++;

				    if (Focus == NULL)
				    {
					/* PointerRoot mode */
					SetFocus ((TwmWindow *) NULL, CurrentTime);
				    }
				    else
				    {
					/* prohibit triggering FocusIn event */
					XWindowAttributes a;
					XGetWindowAttributes (dpy, Focus->w, &a);
					XSelectInput (dpy, Focus->w, a.your_event_mask & ~FocusChangeMask);
					SetFocus (Focus, CurrentTime);
					XSelectInput (dpy, Focus->w, a.your_event_mask);
					if (ExpectsTakeFocus(Focus))
					    SendTakeFocusMessage (Focus, lastTimestamp);
					if (Focus->hilite_w)
					    XMapWindow (dpy, Focus->hilite_w);
				    }
#ifdef DEBUG_STOLENFOCUS
				    if (PrintErrorMessages)
				    {
					if (stamp1 != stamp0)
					    fprintf (stderr, "HandleFocusChange(1,s=%lu,F=%x,C=%lx{%d,%d}): From '%s' (w=0x0%lx,f=%x) to '%s' (w=0x0%lx,f=%x), %d. attempt to return (%ld milliseconds later).\n",
						Event.xfocus.serial,
						((Scr->TitleFocus==TRUE?512:256)|(SloppyFocus==TRUE?32:16)|(FocusRoot==TRUE?2:1)),
						(long)JunkChild, HotX, HotY,
						(Focus?Focus->name:"NULL"),
						(long)(Focus?Focus->w:None),
						(Focus?(((Focus->protocols&DoesWmTakeFocus)?32:16)|((!Focus->wmhints||Focus->wmhints->input)?2:1)):0),
						Tmp_win->name, (long)(Tmp_win->w),
						(Tmp_win?(((Tmp_win->protocols&DoesWmTakeFocus)?32:16)|((!Tmp_win->wmhints||Tmp_win->wmhints->input)?2:1)):0),
						attempts, (stamp1-stamp0));
#if 1
					else
					    fprintf (stderr, "HandleFocusChange(0,s=%lu,F=%x,C=%lx{%d,%d}): From '%s' (w=0x0%lx,f=%x) to '%s' (w=0x0%lx,f=%x), attempting to return.\n",
						Event.xfocus.serial,
						((Scr->TitleFocus==TRUE?512:256)|(SloppyFocus==TRUE?32:16)|(FocusRoot==TRUE?2:1)),
						(long)JunkChild, HotX, HotY,
						(Focus?Focus->name:"NULL"),
						(long)(Focus?Focus->w:None),
						(Focus?(((Focus->protocols&DoesWmTakeFocus)?32:16)|((!Focus->wmhints||Focus->wmhints->input)?2:1)):0),
						Tmp_win->name, (long)(Tmp_win->w),
						(Tmp_win?(((Tmp_win->protocols&DoesWmTakeFocus)?32:16)|((!Tmp_win->wmhints||Tmp_win->wmhints->input)?2:1)):0));
#endif
				    }
#endif
				    return;
				}
				/* focus recovery time expired or attempts exhausted; give up, let focus go */
#if 1
				if (PrintErrorMessages)
				{
				    int x, y;
				    InfoLines = 0;
				    (void) sprintf(Info[InfoLines++], "Lost focus alert:");
				    if (Focus) {
					(void) sprintf(Info[InfoLines++], "Focus hijacked by '%s'.", Tmp_win->name);
					if (Focus->mapped) {
					    x = Focus->frame_x + Focus->frame_width/2;
					    y = Focus->frame_y;
					} else
					    x = y = 0;
				    } else {
					if (Tmp_win->mapped) {
					    (void) sprintf(Info[InfoLines++], "Focus hijacked here.", Tmp_win->name);
					    x = Tmp_win->frame_x + Tmp_win->frame_width/2;
					    y = Tmp_win->frame_y;
					} else {
					    (void) sprintf(Info[InfoLines++], "Focus hijacked by '%s'.", Tmp_win->name);
					    x = y = 0;
					}
				    }
				    RaiseInfoWindow (x, y);
				}
#endif
#ifdef DEBUG_STOLENFOCUS
				if (PrintErrorMessages)
				    fprintf (stderr, "HandleFocusChange(2,s=%lu,F=%x): From '%s' (w=0x0%lx,f=%x) to '%s' (w=0x0%lx,f=%x), giving away (%ld milliseconds later).\n",
					Event.xfocus.serial,
					((Scr->TitleFocus==TRUE?512:256)|(SloppyFocus==TRUE?32:16)|(FocusRoot==TRUE?2:1)),
					(Focus?Focus->name:"NULL"),
					(long)(Focus?Focus->w:None),
					(Focus?(((Focus->protocols&DoesWmTakeFocus)?32:16)|((!Focus->wmhints||Focus->wmhints->input)?2:1)):0),
					Tmp_win->name, (long)(Tmp_win->w),
					(Tmp_win?(((Tmp_win->protocols&DoesWmTakeFocus)?32:16)|((!Tmp_win->wmhints||Tmp_win->wmhints->input)?2:1)):0),
					(stamp1-stamp0));
#endif
			    }
			} /* NotifyNormal */
		    } /* RecoverStolenFocusTimeout >= 0 */
#endif
		    f = FocusRoot;
		    FocusedOnClient (Tmp_win); /*only decorate etc, preserve focus state variable*/
		    FocusRoot = f;
		    HighLightIconManager (Tmp_win);
		} /* Focus != Tmp_win */

		if (Tmp_win->hilite_w /* (remember: server delivered FocusIn!) && AcceptsInput (Tmp_win) */)
		    XMapWindow (dpy, Tmp_win->hilite_w);

		thf = None;
	    }
	    else /* FocusOut */
	    {
		if (Tmp_win->hilite_w)
		    XUnmapWindow (dpy, Tmp_win->hilite_w);
	    }
	} /* Tmp_win ?= NULL */
    }
}



/**
 *	HandleConfigureRequest - ConfigureRequest event handler
 */
void
HandleConfigureRequest()
{
    XWindowChanges xwc;
    unsigned long xwcm;
    int x, y, width, height, bw;
    int gravx, gravy;
    XConfigureRequestEvent *cre = &Event.xconfigurerequest;

#ifdef DEBUG_EVENTS
    fprintf(stderr, "events.c -- HandleConfigureRequest(): win=0x%08lx\n", (long)(cre->window));
    if (cre->value_mask & CWX)
	fprintf(stderr, "  x = %d\n", cre->x);
    if (cre->value_mask & CWY)
	fprintf(stderr, "  y = %d\n", cre->y);
    if (cre->value_mask & CWWidth)
	fprintf(stderr, "  width = %d\n", cre->width);
    if (cre->value_mask & CWHeight)
	fprintf(stderr, "  height = %d\n", cre->height);
    if (cre->value_mask & CWBorderWidth)
	fprintf(stderr, "  border = %d\n", cre->border_width);
    if (cre->value_mask & CWSibling)
	fprintf(stderr, "  above = 0x%x\n", cre->above);
    if (cre->value_mask & CWStackMode)
	fprintf(stderr, "  stack = %d\n", cre->detail);
#endif

    /*
     * Event.xany.window is Event.xconfigurerequest.parent, so Tmp_win will
     * be wrong
     */
    Event.xany.window = cre->window;	/* mash parent field */
    if (XFindContext (dpy, cre->window, TwmContext, (caddr_t *) &Tmp_win) == XCNOENT)
	Tmp_win = NULL;
    else if ((Tmp_win->iconmgr == TRUE) || (Tmp_win->icon_w.win == cre->window)) {
	/* prohibit moving iconmgr and icon windows completely off X11-screen */
	if ((cre->x + (cre->width + (cre->border_width<<1)) <= 0) || (cre->x >= Scr->MyDisplayWidth))
	    cre->value_mask &= ~CWX;
	if ((cre->y + (cre->height + (cre->border_width<<1)) <= 0) || (cre->y >= Scr->MyDisplayHeight))
	    cre->value_mask &= ~CWY;
    }


    /*
     * According to the July 27, 1988 ICCCM draft, we should ignore size and
     * position fields in the WM_NORMAL_HINTS property when we map a window.
     * Instead, we'll read the current geometry.  Therefore, we should respond
     * to configuration requests for windows which have never been mapped.
     */
    if (!Tmp_win || Tmp_win->icon_w.win == cre->window) {
	xwcm = cre->value_mask &
	    (CWX | CWY | CWWidth | CWHeight | CWBorderWidth);
	xwc.x = cre->x;
	xwc.y = cre->y;
	xwc.width = cre->width;
	xwc.height = cre->height;
	xwc.border_width = cre->border_width;
	XConfigureWindow(dpy, Event.xany.window, xwcm, &xwc);
	return;
    }

    if ((cre->value_mask & CWStackMode) && Tmp_win->stackmode) {
	TwmWindow *otherwin;

	xwc.sibling = (((cre->value_mask & CWSibling) &&
			(XFindContext (dpy, cre->above, TwmContext,
				       (caddr_t *) &otherwin) == XCSUCCESS))
		       ? otherwin->frame : cre->above);
	xwc.stack_mode = cre->detail;
	XConfigureWindow (dpy, Tmp_win->frame, 
			  cre->value_mask & (CWSibling | CWStackMode), &xwc);
    }


    /* Don't modify frame_XXX fields before calling SetupWindow! */
    x = Tmp_win->frame_x;
    y = Tmp_win->frame_y;
    width = Tmp_win->frame_width;
    height = Tmp_win->frame_height;
    bw = Tmp_win->frame_bw;

    /*
     * Section 4.1.5 of the ICCCM states that the (x,y) coordinates in the
     * configure request are for the upper-left outer corner of the window.
     * This means that we need to adjust for the additional title height as
     * well as for any border width changes that we decide to allow.  The
     * current window gravity is to be used in computing the adjustments, just
     * as when initially locating the window.  Note that if we do decide to 
     * allow border width changes, we will need to send the synthetic 
     * ConfigureNotify event.
     */
    GetGravityOffsets (Tmp_win, &gravx, &gravy);

    if (cre->value_mask & CWBorderWidth) {
	int bwdelta = cre->border_width - Tmp_win->old_bw;  /* posit growth */
	if (bwdelta && Tmp_win->ClientBorderWidth) {  /* if change allowed */
	    x += gravx * bwdelta;	/* change default values only */
	    y += gravy * bwdelta;	/* ditto */
	    bw = cre->border_width;
	    if (Tmp_win->title_height) height += bwdelta;
	    x += (gravx < 0) ? bwdelta : -bwdelta;
	    y += (gravy < 0) ? bwdelta : -bwdelta;
	}
	Tmp_win->old_bw = cre->border_width;  /* for restoring */
    }

    if (cre->value_mask & CWX) {	/* override even if border change */
	if (cre->window == Tmp_win->frame)
	    x = cre->x; /* some external entity reconfigured the frame, let be so */
	else
	    x = cre->x - bw;
    }
    if (cre->value_mask & CWY) {
	if (cre->window == Tmp_win->frame)
	    y = cre->y;
	else
	    y = cre->y - ((gravy < 0) ? 0 : Tmp_win->title_height) - bw;
    }

    if (cre->value_mask & CWWidth) {
	width = cre->width;
    }
    if (cre->value_mask & CWHeight) {
	if (cre->window == Tmp_win->frame)
	    height = cre->height;
	else
	    height = cre->height + Tmp_win->title_height;
    }

    if (width != Tmp_win->frame_width || height != Tmp_win->frame_height)
	Tmp_win->zoomed = ZOOM_NONE;

    /*
     * SetupWindow (x,y) are the location of the upper-left outer corner and
     * are passed directly to XMoveResizeWindow (frame).  The (width,height)
     * are the inner size of the frame.  The inner width is the same as the 
     * requested client window width; the inner height is the same as the
     * requested client window height plus any title bar slop.
     */
    if (cre->window == Tmp_win->frame) {
	if (cre->value_mask & (CWWidth | CWHeight))
	    ConstrainSize (Tmp_win, &width, &height);
	SetupFrame (Tmp_win, x, y, width, height, bw, True);
	if (Tmp_win->iconmgr) {
	    Tmp_win->iconmgrp->width = width;
	    PackIconManager (Tmp_win->iconmgrp);
	}
    }
    else
	SetupWindow (Tmp_win, x, y, width, height, bw);
}



/**
 * shape notification event handler
 */
void
HandleShapeNotify ()
{
    XShapeEvent	    *sev = (XShapeEvent *) &Event;

    if (Tmp_win == NULL)
	return;
    if (sev->kind != ShapeBounding)
	return;
    if (!Tmp_win->wShaped && sev->shaped) {
	XShapeCombineMask (dpy, Tmp_win->frame, ShapeClip, 0, 0, None,
			   ShapeSet);
    }
    Tmp_win->wShaped = sev->shaped;
    SetFrameShape (Tmp_win);
}



/**
 * root-window configuration change notification event handler
 */
#if defined TWM_USE_XINERAMA || defined TWM_USE_XRANDR
static void RootChangeNotify (int width, int height)
{
    if (Scr->RRScreenChangeRestart == TRUE
	    || (Scr->RRScreenSizeChangeRestart == TRUE
		&& (Scr->MyDisplayWidth != width || Scr->MyDisplayHeight != height)))
    {
	/* prepare a faked button event: */
	XEvent e = Event;
	e.xbutton.type = ButtonPress;
	e.xbutton.button = Button1;
	e.xbutton.time = CurrentTime;

	/* initiate "f.restart": */
	ExecuteFunction (F_RESTART, NULLSTR, Scr->Root, NULL, &e, C_ROOT, FALSE);
    }
    else
    {
	/* Scr->MyDisplayWidth, Scr->MyDisplayHeight must be updated: */
	Scr->MyDisplayWidth = width;
	Scr->MyDisplayHeight = height;
	Scr->use_panels = FALSE;
#ifdef TWM_USE_XRANDR
	if (HasXrandr == True) {
	    if (GetXrandrTilesGeometries (Scr) == TRUE)
		Scr->use_panels = ComputeTiledAreaBoundingBox (Scr);
	}
#ifdef TWM_USE_XINERAMA
	else /*fallthrough*/
#endif
#endif
#ifdef TWM_USE_XINERAMA
	if (HasXinerama == True) {
	    if (GetXineramaTilesGeometries (Scr) == TRUE)
		Scr->use_panels = ComputeTiledAreaBoundingBox (Scr);
	}
#endif
    }
}
#endif

#ifdef TWM_USE_XINERAMA
void
HandleConfigureNotify ()
{
    if (Event.xconfigure.window == Scr->Root) {
#ifdef TWM_USE_XRANDR
	XRRUpdateConfiguration (&Event);
#endif
	RootChangeNotify (Event.xconfigure.width, Event.xconfigure.height);
    }
}
#endif

#ifdef TWM_USE_XRANDR
/**
 * XRANDR screen change notification event handler
 */
void
HandleXrandrScreenChangeNotify ()
{
    XEvent e = Event;
    /* drop any further RR-events for other panels (one is enough): */
    while (XCheckTypedWindowEvent(dpy, Scr->Root, XrandrEventBase+RRScreenChangeNotify, &e) == True)
	continue;
    if (XRRUpdateConfiguration (&e) != 1)
	fprintf (stderr, "%s: HandleXrandrScreenChangeNotify(): Not understanding Xrandr-event type = %d\n",
		ProgramName, e.xany.type);
    RootChangeNotify (XDisplayWidth(dpy, Scr->screen), XDisplayHeight(dpy, Scr->screen));
}
#endif



/**
 * unknown event handler
 */
void
HandleUnknown()
{
#ifdef DEBUG_EVENTS
    fprintf(stderr, "events.c -- HandleUnknown(): type = %d\n", Event.type);
#endif
}



/**
 * checks to see if the window is a transient.
 *
 *  \return TRUE  if window is a transient
 *  \return FALSE if window is not a transient
 *
 *	\param w the window to check
 */
int
Transient(Window w, Window *propw)
{
    return (XGetTransientForHint(dpy, w, propw) ? TRUE : FALSE);
}



/**
 * get ScreenInfo struct associated with a given window
 *
 *  \return ScreenInfo struct
 *  \param  w the window
 */
ScreenInfo *
FindPointerScreenInfo (void)
{
    int scrnum;

    XQueryPointer (dpy, /*dummy*/DefaultRootWindow(dpy), &JunkRoot, &JunkChild,
		    &JunkX, &JunkY, &HotX, &HotY, &JunkMask);
    for (scrnum = 0; scrnum < NumScreens; scrnum++) {
	if (ScreenList[scrnum] != NULL && ScreenList[scrnum]->Root == JunkRoot)
	    return ScreenList[scrnum];
    }
    return NULL;
}

ScreenInfo *
FindDrawableScreenInfo (Drawable d)
{
    if (d != None && XGetGeometry(dpy, d, &JunkRoot, &JunkX, &JunkY, &JunkWidth, &JunkHeight,
			&JunkBW, &JunkDepth)) {
	int scrnum;
	for (scrnum = 0; scrnum < NumScreens; scrnum++)
	    if (ScreenList[scrnum] != NULL && ScreenList[scrnum]->Root == JunkRoot)
		return ScreenList[scrnum];
    }
    return NULL;
}

ScreenInfo *
FindWindowScreenInfo (XWindowAttributes *attr)
{
    int scrnum;

    for (scrnum = 0; scrnum < NumScreens; scrnum++) {
	if (ScreenList[scrnum] != NULL
		&& ScreenOfDisplay(dpy, ScreenList[scrnum]->screen) == attr->screen)
	    return ScreenList[scrnum];
    }
    return NULL;
}

ScreenInfo *
FindScreenInfo (Window w)
{
    XWindowAttributes attr;

    attr.screen = NULL;
    if (w != None && XGetWindowAttributes(dpy, w, &attr)) {
	return FindWindowScreenInfo (&attr);
    }
    return NULL;
}



static void flush_expose (w)
    Window w;
{
    XEvent dummy;

				/* SUPPRESS 530 */
    while (XCheckTypedWindowEvent (dpy, w, Expose, &dummy)) ;
}



/**
 * install the colormaps for one twm window.
 *
 *  \param type  type of event that caused the installation
 *  \param tmp   for a subset of event types, the address of the
 *		  window structure, whose colormaps are to be installed.
 */
void
InstallWindowColormaps (int type, TwmWindow *tmp)
{
    int i, j, n, number_cwins, state;
    ColormapWindow **cwins, *cwin, **maxcwin = NULL;
    TwmColormap *cmap;
    char *row, *scoreboard;

    switch (type) {
    case EnterNotify:
    case LeaveNotify:
    case DestroyNotify:
    default:
	/* Save the colormap to be loaded for when force loading of
	 * root colormap(s) ends.
	 */
	Scr->cmapInfo.pushed_window = tmp;
	/* Don't load any new colormap if root colormap(s) has been
	 * force loaded.
	 */
	if (Scr->cmapInfo.root_pushes)
	    return;
	/* Don't reload the currend window colormap list.
	 */
	if (Scr->cmapInfo.cmaps == &tmp->cmaps && (tmp != &Scr->TwmRoot || Scr->TwmRoot.cmaps.number_cwins == 1))
	    return;
	if (Scr->cmapInfo.cmaps)
	    for (i = Scr->cmapInfo.cmaps->number_cwins,
		 cwins = Scr->cmapInfo.cmaps->cwins; i-- > 0; cwins++)
		(*cwins)->colormap->state &= ~CM_INSTALLABLE;
	Scr->cmapInfo.cmaps = &tmp->cmaps;
	break;
    
    case PropertyNotify:
    case VisibilityNotify:
    case ColormapNotify:
	break;
    }

    number_cwins = Scr->cmapInfo.cmaps->number_cwins;
    cwins = Scr->cmapInfo.cmaps->cwins;
    scoreboard = Scr->cmapInfo.cmaps->scoreboard;

    ColortableThrashing = FALSE; /* in case installation aborted */

    state = CM_INSTALLED;

      for (i = n = 0; i < number_cwins; i++) {
	cwin = cwins[i];
	cmap = cwin->colormap;
	cmap->state |= CM_INSTALLABLE;
	cmap->state &= ~CM_INSTALL;
	cmap->w = cwin->w;
      }
      for (i = n = 0; i < number_cwins; i++) {
  	cwin = cwins[i];
  	cmap = cwin->colormap;
	if (cwin->visibility != VisibilityFullyObscured &&
	    n < Scr->cmapInfo.maxCmaps) {
	    row = scoreboard + (i*(i-1)/2);
	    for (j = 0; j < i; j++)
		if (row[j] && (cwins[j]->colormap->state & CM_INSTALL))
		    break;
	    if (j != i)
		continue;
	    n++;
	    maxcwin = &cwins[i];
	    state &= (cmap->state & CM_INSTALLED);
	    cmap->state |= CM_INSTALL;
	}
    }

    Scr->cmapInfo.first_req = NextRequest(dpy);

    for ( ; n > 0 && maxcwin >= cwins; maxcwin--) {
	cmap = (*maxcwin)->colormap;
	if (cmap->state & CM_INSTALL) {
	    cmap->state &= ~CM_INSTALL;
	    if (!(state & CM_INSTALLED)) {
		cmap->install_req = NextRequest(dpy);
		XInstallColormap(dpy, cmap->c);
	    }
	    cmap->state |= CM_INSTALLED;
	    n--;
	}
    }
}



/** \fn InstallRootColormap
 *  \fn UninstallRootColormap
 *
 * Force (un)loads root colormap(s)
 *
 *	   These matching routines provide a mechanism to insure that
 *	   the root colormap(s) is installed during operations like
 *	   rubber banding or menu display that require colors from
 *	   that colormap.  Calls may be nested arbitrarily deeply,
 *	   as long as there is one UninstallRootColormap call per
 *	   InstallRootColormap call.
 *
 *	   The final UninstallRootColormap will cause the colormap list
 *	   which would otherwise have be loaded to be loaded, unless
 *	   Enter or Leave Notify events are queued, indicating some
 *	   other colormap list would potentially be loaded anyway.
 */
void
InstallRootColormap()
{
    TwmWindow *tmp;
    if (Scr->cmapInfo.root_pushes == 0) {
	/*
	 * The saving and restoring of cmapInfo.pushed_window here
	 * is a slimy way to remember the actual pushed list and
	 * not that of the root window.
	 */
	tmp = Scr->cmapInfo.pushed_window;
#if 1
	if (Scr->TwmRoot.cmaps.number_cwins > 1 && ActiveMenu != NULL)
	{
	    /* load twm colormap only if dealing with menus, not on resize/move: */
	    int i = Scr->TwmRoot.cmaps.number_cwins-1;
	    ColormapWindow *cwins = Scr->TwmRoot.cmaps.cwins[0];
	    /* bump screen/twm colormap: */
	    Scr->TwmRoot.cmaps.cwins[0] = Scr->TwmRoot.cmaps.cwins[i];  Scr->TwmRoot.cmaps.cwins[i] = cwins;
	    Scr->TwmRoot.cmaps.cwins[0]->colormap->state = Scr->TwmRoot.cmaps.cwins[i]->colormap->state = 0;
	    InstallWindowColormaps (EnterNotify, &Scr->TwmRoot);
	    /* unbump screen/twm colormap: */
	    Scr->TwmRoot.cmaps.cwins[i] = Scr->TwmRoot.cmaps.cwins[0];  Scr->TwmRoot.cmaps.cwins[0] = cwins;
	}
	else
#endif
	    InstallWindowColormaps(0, &Scr->TwmRoot);
	Scr->cmapInfo.pushed_window = tmp;
    }
    Scr->cmapInfo.root_pushes++;
}



static Bool
UninstallRootColormapQScanner(Display *dpy, XEvent *ev, char *args)
{
    if (!*args) {
	if (ev->type == EnterNotify) {
	    if (ev->xcrossing.mode != NotifyGrab)
		*args = 1;
	} else if (ev->type == LeaveNotify) {
	    if (ev->xcrossing.mode == NotifyNormal)

		*args = 1;
	}
    }
    return (False);
}


void
UninstallRootColormap()
{
    char args;
    XEvent dummy;

    if (Scr->cmapInfo.root_pushes)
	Scr->cmapInfo.root_pushes--;
    
    if (!Scr->cmapInfo.root_pushes) {
	/*
	 * If we have subsequent Enter or Leave Notify events,
	 * we can skip the reload of pushed colormaps.
	 */
	XSync (dpy, 0);
	args = 0;
	(void) XCheckIfEvent(dpy, &dummy, UninstallRootColormapQScanner, &args);

	if (!args)
	    InstallWindowColormaps(0, Scr->cmapInfo.pushed_window);
    }
}

#if defined DEBUG_EVENTS
void
dumpevent (XEvent *e)
{
    char *name = NULL;

    switch (e->type) {
      case KeyPress:  name = "KeyPress"; break;
      case KeyRelease:  name = "KeyRelease"; break;
      case ButtonPress:  name = "ButtonPress"; break;
      case ButtonRelease:  name = "ButtonRelease"; break;
      case MotionNotify:  name = "MotionNotify"; break;
      case EnterNotify:  name = "EnterNotify"; break;
      case LeaveNotify:  name = "LeaveNotify"; break;
      case FocusIn:  name = "FocusIn"; break;
      case FocusOut:  name = "FocusOut"; break;
      case KeymapNotify:  name = "KeymapNotify"; break;
      case Expose:  name = "Expose"; break;
      case GraphicsExpose:  name = "GraphicsExpose"; break;
      case NoExpose:  name = "NoExpose"; break;
      case VisibilityNotify:  name = "VisibilityNotify"; break;
      case CreateNotify:  name = "CreateNotify"; break;
      case DestroyNotify:  name = "DestroyNotify"; break;
      case UnmapNotify:  name = "UnmapNotify"; break;
      case MapNotify:  name = "MapNotify"; break;
      case MapRequest:  name = "MapRequest"; break;
      case ReparentNotify:  name = "ReparentNotify"; break;
      case ConfigureNotify:  name = "ConfigureNotify"; break;
      case ConfigureRequest:  name = "ConfigureRequest"; break;
      case GravityNotify:  name = "GravityNotify"; break;
      case ResizeRequest:  name = "ResizeRequest"; break;
      case CirculateNotify:  name = "CirculateNotify"; break;
      case CirculateRequest:  name = "CirculateRequest"; break;
      case PropertyNotify:  name = "PropertyNotify"; break;
      case SelectionClear:  name = "SelectionClear"; break;
      case SelectionRequest:  name = "SelectionRequest"; break;
      case SelectionNotify:  name = "SelectionNotify"; break;
      case ColormapNotify:  name = "ColormapNotify"; break;
      case ClientMessage:  name = "ClientMessage"; break;
      case MappingNotify:  name = "MappingNotify"; break;
    }

    if (HasShape && e->type == (ShapeEventBase+ShapeNotify))
	name = "ShapeNotify";

#ifdef TWM_USE_XRANDR
    if (HasXrandr && e->type == (XrandrEventBase+RRScreenChangeNotify))
	name = "RRScreenChangeNotify";
#endif

    if (name) {
	fprintf (stderr, "event:  %s, %d remaining\n", name, QLength(dpy));
    } else {
	fprintf (stderr, "unknown event %d, %d remaining\n", e->type, QLength(dpy));
    }
}
#endif /* DEBUG_EVENTS */

