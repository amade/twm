/*****************************************************************************/
/*

Copyright 1989, 1998  The Open Group
Copyright 2005 Hitachi, Ltd.

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
 * $Xorg: twm.c,v 1.5 2001/02/09 02:05:37 xorgcvs Exp $
 *
 * twm - "Tom's Window Manager"
 *
 * 27-Oct-1987 Thomas E. LaStrange    File created
 * 10-Oct-1990 David M. Sternlicht    Storing saved colors on root
 * 19-Feb-2005 Julien Lafon           Handle print screens for unified Xserver
 ***********************************************************************/
/* $XFree86: xc/programs/twm/twm.c,v 3.13 2003/04/21 08:15:10 herrb Exp $ */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include "twm.h"
#include "icons.h"
#include "iconmgr.h"
#include "add_window.h"
#include "gc.h"
#include "parse.h"
#include "version.h"
#include "menus.h"
#include "events.h"
#include "util.h"
#include "gram.h"
#include "screen.h"
#include "parse.h"
#include "session.h"
#include <X11/Xproto.h>
#include <X11/Xatom.h>
#include <X11/SM/SMlib.h>
#include <X11/Xmu/Error.h>
#include <X11/Xmu/StdCmap.h>
#include <X11/extensions/sync.h>
#include <X11/Xlocale.h>
#include <X11/Xos.h>
#ifdef XPRINT
#include <X11/extensions/Print.h>
#endif /* XPRINT */
#ifdef TWM_USE_XINERAMA
#include <X11/extensions/Xinerama.h>
#endif
#ifdef TWM_USE_COMPOSITE
#include <X11/extensions/Xcomposite.h>
#endif

XtAppContext appContext;	/* Xt application context */
XtSignalId si;

Display *dpy = NULL;		/* which display are we talking to */
Window ResizeWindow;		/* the window we are resizing */

int MultiScreen = TRUE;		/* try for more than one screen? */
int NoPrintscreens = False;     /* ignore special handling of print screens? */
int NumScreens;			/* number of screens in ScreenList */
int HasShape;			/* server supports shape extension? */
int ShapeEventBase, ShapeErrorBase;
int HasSync;			/* server supports SYNC extension? */
int SyncEventBase, SyncErrorBase;
#ifdef TWM_USE_XINERAMA
int HasXinerama = True;		/* server supports XINERAMA extension? */
#endif
#ifdef TWM_USE_XRANDR
int HasXrandr;			/* server supports XRANDR extension? */
int XrandrEventBase, XrandrErrorBase;
#endif
#ifdef TWM_USE_COMPOSITE
static int XcompositeOpcode;
static int CatchCompositeError ( Display *dpy, XErrorEvent *event );
#endif
ScreenInfo **ScreenList;	/* structures for each screen */
ScreenInfo *Scr = NULL;		/* the cur and prev screens */
int PreviousScreen;		/* last screen that we were on */
int FirstScreen;		/* TRUE ==> first screen of display */
int PrintErrorMessages = 0;	/* controls error messages */
static int RedirectError;	/* TRUE ==> another window manager running */
static int TwmErrorHandler ( Display *dpy, XErrorEvent *event );	/* for settting RedirectError */
static int CatchRedirectError ( Display *dpy, XErrorEvent *event );	/* for everything else */
static SIGNAL_T sigHandler(int);
char Info[INFO_LINES][INFO_SIZE];		/* info strings to print */
int InfoLines;
char *InitFile = NULL;

Cursor UpperLeftCursor;		/* upper Left corner cursor */
Cursor RightButt;
Cursor MiddleButt;
Cursor LeftButt;

XContext TwmContext;		/* context for twm windows */
XContext MenuContext;		/* context for all menu windows */
XContext IconManagerContext;	/* context for all window list windows */
XContext ScreenContext;		/* context to get screen data */
XContext ColormapContext;	/* context for colormap operations */

XClassHint NoClass;		/* for applications with no class */

XGCValues Gcv;

char *Home;			/* the HOME environment variable */
int HomeLen;			/* length of Home */
int ParseError;			/* error parsing the .twmrc file */

int HandlingEvents = FALSE;	/* are we handling events yet? */

Window JunkRoot;		/* junk window */
Window JunkChild;		/* junk window */
int JunkX;			/* junk variable */
int JunkY;			/* junk variable */
unsigned int JunkWidth, JunkHeight, JunkBW, JunkDepth, JunkMask;

char *ProgramName;
int Argc;
char **Argv;

Bool RestartPreviousState = False;	/* try to restart in previous state */

unsigned long black, white;

Atom TwmAtoms[11];

#ifdef TWM_USE_OPACITY
Atom _XA_NET_WM_WINDOW_OPACITY;
#endif

Bool use_fontset;		/* (I18N, utf8) use XFontSet-related functions or not */

TwmWindow *Focus;		/* the twm window that has keyboard focus */
short FocusRoot;		/* is the input focus on the root ? */

#ifdef TWM_USE_SLOPPYFOCUS
short SloppyFocus;		/* TRUE if global sloppy focus in effect */
#endif
int RecoverStolenFocusAttempts;	/* number of attempts twm should try to return focus if some client stole it */
int RecoverStolenFocusTimeout;	/* time in milliseconds twm should fight for focus if some client stole it */

/* don't change the order of these strings */
static char* atom_names[11] = {
    "_MIT_PRIORITY_COLORS",
    "WM_CHANGE_STATE",
    "WM_STATE",
    "WM_COLORMAP_WINDOWS",
    "WM_PROTOCOLS",
    "WM_TAKE_FOCUS",
    "WM_SAVE_YOURSELF",
    "WM_DELETE_WINDOW",
    "SM_CLIENT_ID",
    "WM_CLIENT_LEADER",
    "WM_WINDOW_ROLE"
};

#ifdef XPRINT
/* |hasExtension()| and |IsPrintScreen()| have been stolen from
 * xc/programs/xdpyinfo/xdpyinfo.c */
static
Bool hasExtension(Display *dpy, char *extname)
{
  int    num_extensions,
         i;
  char **extensions;
  extensions = XListExtensions(dpy, &num_extensions);
  for (i = 0; i < num_extensions &&
         (strcmp(extensions[i], extname) != 0); i++);
  XFreeExtensionList(extensions);
  return i != num_extensions;
}

static
Bool IsPrintScreen(Screen *s)
{
    Display *dpy = XDisplayOfScreen(s);
    int      i;

    /* Check whether this is a screen of a print DDX */
    if (hasExtension(dpy, XP_PRINTNAME)) {
        Screen **pscreens;
        int      pscrcount;

        pscreens = XpQueryScreens(dpy, &pscrcount);
        for( i = 0 ; (i < pscrcount) && pscreens ; i++ ) {
            if (s == pscreens[i]) {
                return True;
            }
        }
        XFree(pscreens);                      
    }
    return False;
}
#endif /* XPRINT */

/***********************************************************************
 *
 *  Procedure:
 *	main - start of twm
 *
 ***********************************************************************
 */

int
main(int argc, char *argv[])
{
    Window *children;
    unsigned int nchildren;
    int i, j;
    char *display_name = NULL;
    unsigned long valuemask;	/* mask for create windows */
    XSetWindowAttributes attributes;	/* attributes for create windows */
    int numManaged, firstscrn, lastscrn, scrnum;
    int PrintVersionAndExit = FALSE;
    int class = -1, depth = -1;
    int zero = 0;
    char *restore_filename = NULL;
    char *client_id = NULL;
    char *loc;

    static struct { char *name; int class; } visuals [] = {
			{"StaticGray",  StaticGray},
			{"GrayScale",   GrayScale},
			{"StaticColor", StaticColor},
			{"PseudoColor", PseudoColor},
			{"TrueColor",   TrueColor},
			{"DirectColor", DirectColor}
		    };

#ifdef TWM_USE_XFT
    short xft_available = 0;	/* use Xft if 'EnableXftFontRenderer' in .twmrc */
#endif

#ifdef TWM_USE_RENDER
    short xrender_available = TRUE;
#ifdef TWM_USE_COMPOSITE
    short xcomposite_available = FALSE;
#endif
#endif

    ProgramName = argv[0];
    Argc = argc;
    Argv = argv;

    for (i = 1; i < argc; i++) {
	if (argv[i][0] == '-') {
	    switch (argv[i][1]) {
	      case 'd':				/* -display dpy */
		if (strcmp(&argv[i][1], "depth") == 0) { /* -depth 32 */
		    if (++i >= argc) goto usage;
		    if (sscanf (argv[i], " %d", &depth) != 1 || depth < 1)
			goto usage;
		    continue;
		}
		if (strcmp(&argv[i][1], "display")) goto usage;
		if (++i >= argc) goto usage;
		display_name = argv[i];
		continue;
	      case 's':				/* -single */
		MultiScreen = FALSE;
		continue;
#ifdef XPRINT
	      case 'n':				/* -noprint */
		if (strcmp(&argv[i][1], "noprint")) goto usage;
		NoPrintscreens = True;
		continue;
#endif /* XPRINT */
	      case 'f':				/* -file twmrcfilename */
		if (++i >= argc) goto usage;
		InitFile = argv[i];
		continue;
	      case 'v':
		if (strcmp(&argv[i][1], "visual") == 0) { /* -visual TrueColor */
		    if (++i >= argc)
			goto usage;
		    for (j = 0; j < sizeof(visuals)/sizeof(visuals[0]); ++j)
			if (strcmp(&argv[i][0], visuals[j].name) == 0) {
			    class = visuals[j].class;
			    break;
			}
		    if (j == sizeof(visuals)/sizeof(visuals[0]))
			goto usage;
		    continue;
		}
		if (strcmp(&argv[i][1], "v") == 0 || strcmp(&argv[i][1], "verbose") == 0) { /* -verbose */
		    PrintErrorMessages++; /* specified multiply counts up! */
		    continue;
		}
		goto usage;
	      case 'V':				/* -Version */
		PrintVersionAndExit = TRUE;
		continue;
	      case 'c':				/* -clientId */
		if (strcmp(&argv[i][1], "clientId")) goto usage;
		if (++i >= argc) goto usage;
		client_id = argv[i];
		continue;
	      case 'r':				/* -restore */
#ifdef TWM_USE_RENDER
		if (strcmp(&argv[i][1], "render") == 0) { /* -render */
		    xrender_available = FALSE;
		    continue;
		}
#endif
		if (strcmp(&argv[i][1], "restore")) goto usage;
		if (++i >= argc) goto usage;
		restore_filename = argv[i];
		continue;
	      case 'q':				/* -quiet */
		PrintErrorMessages = 0;
		continue;
#ifdef TWM_USE_XFT
	      case 't':				/* -truetype */
		if (strcmp(&argv[i][1], "truetype") == 0) {
		    if (xft_available >= 0)
			xft_available = -1; /* don't use Xft regardless of .twmrc */
		    else
			xft_available = +1; /* (-truetype -truetype) use Xft  -"- */
		    continue;
		}
		goto usage;
#endif
#ifdef TWM_USE_XINERAMA
	      case 'x':				/* -xinerama */
		if (strcmp(&argv[i][1], "xinerama") == 0) {
		    HasXinerama = False;
		    continue;
		}
		goto usage;
#endif
	    }
	}
      usage:
	fprintf (stderr,
		 "usage:  %s [-display dpy] [-f file] [-s] [-q] [-v] [-V]"
                 " [-visual {StaticGray, GrayScale, StaticColor, PseudoColor, TrueColor, DirectColor} ]"
                 " [-depth bpp ]"
#ifdef TWM_USE_XFT
                 " [-truetype]"
#endif
#ifdef TWM_USE_RENDER
                 " [-render]"
#endif
#ifdef TWM_USE_XINERAMA
                 " [-xinerama]"
#endif
#ifdef XPRINT
                 " [-noprint]"
#endif /* XPRINT */
                 " [-clientId id] [-restore file]\n",
		 ProgramName);
	exit (1);
    }

    if (PrintErrorMessages || (PrintVersionAndExit == TRUE)) {
#ifdef __DATE__
	fprintf (stderr, "%s: This is TWM, version \"%s\", build date %s.\n",
		ProgramName, Version, __DATE__);
#else
	fprintf (stderr, "%s: This is TWM, version \"%s\".\n", ProgramName, Version);
#endif
#ifdef TWM_SOURCECODE_ID_STRING
	fprintf (stderr, "%s: (Source code ID is \"%s\".)\n", ProgramName, TWM_SOURCECODE_ID_STRING);
#endif
	if (PrintVersionAndExit == TRUE)
	    exit (0);
    }

    loc = setlocale(LC_ALL, "");
    if (!loc || !strcmp(loc, "C") || !strcmp(loc, "POSIX") ||
	!XSupportsLocale()) {
	 use_fontset = False;
    } else {
	 use_fontset = True;
    }

#define newhandler(sig) \
    if (signal (sig, SIG_IGN) != SIG_IGN) (void) signal (sig, sigHandler)

    
    newhandler (SIGINT);
    newhandler (SIGHUP);
    newhandler (SIGQUIT);
    newhandler (SIGTERM);

#undef newhandler

    Home = getenv("HOME");
    if (Home != NULL) {
    	char *temp_p;

	/*
	 * Make a copy of Home because the string returned by getenv() can be
	 * overwritten by some POSIX.1 and ANSI-C implementations of getenv()
	 * when further calls to getenv() are made
	 */

	temp_p = strdup(Home);
	Home = temp_p;
    }
#ifdef HAVE_PWD_H
    else {
	struct passwd *pw;
	/*
	 * If ${HOME} is not set get login directory from password file.
	 */
	pw = getpwuid (getuid());
	if (pw != NULL)
	    Home = strdup (pw->pw_dir);
    }
#endif

    if (Home == NULL)
	Home = "./";

    HomeLen = strlen(Home);

    NoClass.res_name = NoName;
    NoClass.res_class = NoName;

    XtToolkitInitialize ();
    appContext = XtCreateApplicationContext ();

    si = XtAppAddSignal(appContext, Done, NULL);
    
    if (!(dpy = XtOpenDisplay (appContext, display_name, "twm", "twm",
	NULL, 0, &zero, NULL))) {
	fprintf (stderr, "%s:  unable to open display \"%s\"\n",
		 ProgramName, XDisplayName(display_name));
	exit (1);
    }

    if (fcntl(ConnectionNumber(dpy), F_SETFD, 1) == -1) {
	fprintf (stderr, 
		 "%s:  unable to mark display connection as close-on-exec\n",
		 ProgramName);
	exit (1);
    }

    if (PrintErrorMessages)
	fprintf (stderr, "%s: X%d server (vendor \"%s\", release %d) is available.\n",
		ProgramName, XProtocolVersion(dpy), XServerVendor(dpy), XVendorRelease(dpy));

    if (restore_filename)
	ReadWinConfigFile (restore_filename);

    HasSync = XSyncQueryExtension(dpy,  &SyncEventBase, &SyncErrorBase);
    if (PrintErrorMessages) {
	if (HasSync == True) {
	    if (!XSyncInitialize(dpy, &JunkX, &JunkY))
		JunkX = JunkY = -1;
	    fprintf (stderr, "%s: %s extension (version %d.%d) is available.\n",
		    ProgramName, SYNC_NAME, JunkX, JunkY);
	} else
	    fprintf (stderr, "%s: %s extension is not available.\n",
		    ProgramName, SYNC_NAME);
    }

    HasShape = XShapeQueryExtension (dpy, &ShapeEventBase, &ShapeErrorBase);
    if (PrintErrorMessages) {
	if (HasShape == True) {
	    if (!XShapeQueryVersion(dpy, &JunkX, &JunkY))
		JunkX = JunkY = -1;
	    fprintf (stderr, "%s: Shape extension (version %d.%d) is available.\n",
		    ProgramName, JunkX, JunkY);
	} else
	    fprintf (stderr, "%s: Shape extension is not available.\n",
		    ProgramName);
    }

#ifdef TWM_USE_XFT
    if (xft_available >= 0) {
	if ((FcTrue == XftInit(0)) && (FcTrue == XftInitFtLibrary())) {
	    if (PrintErrorMessages) {
		i = XftGetVersion();
		fprintf (stderr,
			"%s: Xft subsystem (runtime version %d.%d.%d) is available.\n",
			ProgramName, i / 10000, (i / 100) % 100, i % 100);
	    }
	} else {
	    xft_available = -1;
	    if (PrintErrorMessages) {
		i = XftVersion;
		fprintf (stderr,
		    "%s: Xft subsystem (compiletime version %d.%d.%d) is not available, using core fonts.\n",
		    ProgramName, i / 10000, (i / 100) % 100, i % 100);
	    }
	}
    }
#endif

#ifdef TWM_USE_XINERAMA
    if (HasXinerama == True) {
	if (XineramaQueryExtension(dpy, &JunkX, &JunkY) != True)
	    HasXinerama = False;
	else if (!XineramaQueryVersion(dpy, &JunkX, &JunkY))
	    HasXinerama = False;
	else if (XineramaIsActive (dpy) != True)
		HasXinerama = False;
	if (PrintErrorMessages) {
	    if (HasXinerama == True)
		fprintf (stderr, "%s: Xinerama extension (version %d.%d) is available.\n",
			ProgramName, JunkX, JunkY);
	    else
		fprintf (stderr, "%s: Xinerama extension is not available.\n",
			ProgramName);
	}
    }
#endif

#ifdef TWM_USE_XRANDR
    HasXrandr = XRRQueryExtension (dpy, &XrandrEventBase, &XrandrErrorBase);
    if (PrintErrorMessages) {
	if (HasXrandr == True) {
	    if (!XRRQueryVersion(dpy, &JunkX, &JunkY))
		JunkX = JunkY = -1;
	    fprintf (stderr, "%s: %s extension (version %d.%d) is available.\n",
		    ProgramName, RANDR_NAME, JunkX, JunkY);
	} else
	    fprintf (stderr, "%s: %s extension is not available.\n",
		    ProgramName, RANDR_NAME);
    }
#endif

#ifdef TWM_USE_RENDER
    if (xrender_available == TRUE) {
	if (True == XRenderQueryExtension(dpy, &JunkX, &JunkY) && XRenderQueryVersion(dpy, &JunkX, &JunkY))
	{
	    if (PrintErrorMessages)
		fprintf (stderr, "%s: %s extension (version %d.%d) is available.\n",
			ProgramName, RENDER_NAME, JunkX, JunkY);
	}
	else
	{
	    if (PrintErrorMessages)
		fprintf (stderr, "%s: %s extension is not available.\n",
			ProgramName, RENDER_NAME);
	    xrender_available = FALSE;
	}
    }
#ifdef TWM_USE_COMPOSITE
    if (xrender_available == TRUE) {
	if (True == XQueryExtension(dpy, COMPOSITE_NAME, &XcompositeOpcode, &JunkX, &JunkY)
	    && True == XCompositeQueryExtension(dpy, &JunkX, &JunkY) && XCompositeQueryVersion(dpy, &JunkX, &JunkY))
	{
	    xcomposite_available = TRUE;
	    if (PrintErrorMessages)
		fprintf (stderr, "%s: %s extension (version %d.%d) is available.\n",
			ProgramName, COMPOSITE_NAME, JunkX, JunkY);
	}
	else
	{
	    if (PrintErrorMessages)
		fprintf (stderr, "%s: %s extension is not available.\n",
			ProgramName, COMPOSITE_NAME);
	}
    }
#endif
#endif

    TwmContext = XUniqueContext();
    MenuContext = XUniqueContext();
    IconManagerContext = XUniqueContext();
    ScreenContext = XUniqueContext();
    ColormapContext = XUniqueContext();

    (void) XInternAtoms(dpy, atom_names, sizeof TwmAtoms / sizeof TwmAtoms[0],
			False, TwmAtoms);

#ifdef TWM_USE_OPACITY
    _XA_NET_WM_WINDOW_OPACITY = XInternAtom (dpy, "_NET_WM_WINDOW_OPACITY", False);
#endif

    /* Set up the per-screen global information. */

    NumScreens = ScreenCount(dpy);

    if (MultiScreen)
    {
	firstscrn = 0;
	lastscrn = NumScreens - 1;
    }
    else
    {
	firstscrn = lastscrn = DefaultScreen(dpy);
    }

    InfoLines = 0;

    /* for simplicity, always allocate NumScreens ScreenInfo struct pointers */
    ScreenList = (ScreenInfo **) calloc (NumScreens, sizeof (ScreenInfo *));
    if (ScreenList == NULL)
    {
	fprintf (stderr, "%s: Unable to allocate memory for screen list, exiting.\n",
		 ProgramName);
	exit (1);
    }
    numManaged = 0;
    PreviousScreen = DefaultScreen(dpy);
    FirstScreen = TRUE;
    for (scrnum = firstscrn ; scrnum <= lastscrn; scrnum++)
    {
#ifdef XPRINT
        /* Ignore print screens to avoid that users accidentally warp on a
         * print screen (which are not visible on video displays) */
        if ((!NoPrintscreens) && IsPrintScreen(XScreenOfDisplay(dpy, scrnum)))
        {
	    fprintf (stderr, "%s:  skipping print screen %d\n",
		     ProgramName, scrnum);
            continue;
        }
#endif /* XPRINT */

        /* Make sure property priority colors is empty */
        XChangeProperty (dpy, RootWindow(dpy, scrnum), _XA_MIT_PRIORITY_COLORS,
			 XA_CARDINAL, 32, PropModeReplace, NULL, 0);
	RedirectError = FALSE;
	XSetErrorHandler(CatchRedirectError);
	XSelectInput(dpy, RootWindow (dpy, scrnum),
	    ColormapChangeMask | EnterWindowMask | PropertyChangeMask | 
	    SubstructureRedirectMask | KeyPressMask | FocusChangeMask |
#ifdef TWM_USE_XINERAMA
#ifdef TWM_USE_XRANDR
	    (HasXrandr != True && HasXinerama == True ? StructureNotifyMask : 0) |
#else
	    (HasXinerama == True ? StructureNotifyMask : 0) |
#endif
#endif
	    ButtonPressMask | ButtonReleaseMask);
	XSync(dpy, 0);
	XSetErrorHandler(TwmErrorHandler);

	if (RedirectError)
	{
	    fprintf (stderr, "%s:  another window manager is already running.",
		     ProgramName);
	    if (MultiScreen && NumScreens > 0)
		fprintf(stderr, " on screen %d?\n", scrnum);
	    else
		fprintf(stderr, "?\n");
	    continue;
	}

	numManaged ++;

	/* Note:  ScreenInfo struct is calloc'ed to initialize to zero. */
	Scr = ScreenList[scrnum] = 
	    (ScreenInfo *) calloc(1, sizeof(ScreenInfo));
  	if (Scr == NULL)
  	{
  	    fprintf (stderr, "%s: unable to allocate memory for ScreenInfo structure for screen %d.\n",
  		     ProgramName, scrnum);
  	    continue;
  	}

	Scr->screen = scrnum;
	Scr->Root = RootWindow (dpy, scrnum);
	XSaveContext (dpy, Scr->Root, ScreenContext, (caddr_t) Scr);

	Scr->MyDisplayWidth = DisplayWidth (dpy, scrnum);
	Scr->MyDisplayHeight = DisplayHeight (dpy, scrnum);
	Scr->MaxWindowWidth = 32767 - Scr->MyDisplayWidth;
	Scr->MaxWindowHeight = 32767 - Scr->MyDisplayHeight;

#ifdef TILED_SCREEN
	Scr->panel_names = NULL;
	Scr->panels = NULL;
	Scr->npanels = 0;
#endif

#ifdef TWM_USE_XRANDR
	if (HasXrandr == True)
	{
	    XEvent e;
	    XRRSelectInput (dpy, Scr->Root, RRScreenChangeNotifyMask);
	    XSync (dpy, False);
	    while (XCheckTypedWindowEvent(dpy, Scr->Root,
			XrandrEventBase+RRScreenChangeNotify, &e) == True)
	    {
		XRRUpdateConfiguration (&e);
#if defined DEBUG_EVENTS || 1
		fprintf (stderr,
			"twm.c -- main(): Dropping RRScreenChangeNotify on screen %d\n",
			Scr->screen);
#endif
	    }
	    GetXrandrTilesGeometries (Scr);
	}
#endif

#ifdef TWM_USE_XINERAMA
	if (HasXinerama == True && Scr->panels == NULL)
	    GetXineramaTilesGeometries (Scr);
#endif

#ifdef TILED_SCREEN
	/* prerequisite: Scr->MyDisplayWidth, Scr->MyDisplayHeight are initialised: */
	Scr->use_panels = ComputeTiledAreaBoundingBox (Scr); /* prior to parsing .twmrc! */
#endif

	Scr->StdCmapInfo.head = Scr->StdCmapInfo.tail = Scr->StdCmapInfo.mru = NULL;
	Scr->StdCmapInfo.mruindex = 0;
	LocateStandardColormaps();

	/*
	 * root-window default colormap:    Scr->TwmRoot.cmaps.cwins[0]->colormap->c
	 * twm colormap (installed as std): Scr->TwmRoot.cmaps.cwins[1]->colormap->c
	 */
	Scr->TwmRoot.cmaps.cwins = (ColormapWindow **) malloc (2 * sizeof(ColormapWindow *));
	Scr->TwmRoot.cmaps.number_cwins = 1;
	Scr->d_visual = NULL;

	if (!((class == -1 && depth == -1)
	    || (class == DefaultVisual(dpy, scrnum)->class && depth == -1)
	    || (class == -1 && depth == DefaultDepth(dpy, scrnum))
	    || (class == DefaultVisual(dpy, scrnum)->class && depth == DefaultDepth(dpy, scrnum))))
	{
	    int cls = class;
	    int dpt = depth;
	    Scr->d_visual = FindVisual (scrnum, &cls, &dpt);
	    if (Scr->d_visual == NULL) {
		fprintf (stderr,
			"%s: Requested visual/depth not found on X%d-screen %d, fallback to '%s' at depth %d.\n",
			ProgramName, XProtocolVersion(dpy), scrnum, visuals[DefaultVisual (dpy, scrnum)->class].name,
			DefaultDepth (dpy, scrnum));
	    } else {
		XWindowAttributes att;
		Scr->d_depth = dpt;
		/* store root-window colormap: */
		XGetWindowAttributes (dpy, Scr->Root, &att);
		Scr->TwmRoot.cmaps.cwins[0] = CreateColormapWindowAttr (Scr->Root, &att, True, False);
		/* attempt to create a standard colormap for use by twm: */
		att.colormap = None;
		if (XmuVisualStandardColormaps(dpy, scrnum, Scr->d_visual->visualid, Scr->d_depth, /*replace*/False, /*retain*/True))
		{
		    StdCmap *tmp;
		    LocateStandardColormaps(); /* reload standard colormaps */
		    for (tmp = Scr->StdCmapInfo.head; tmp != NULL; tmp = tmp->next)
			for (i = 0; i < tmp->nmaps; ++i)
			    if (tmp->maps[i].visualid == Scr->d_visual->visualid) {
				att.colormap = tmp->maps[i].colormap;
				goto oo;
			    }
		    oo:;
		}
		if (att.colormap == None) /* create a private colormap for twm */
		    att.colormap = XCreateColormap (dpy, Scr->Root, Scr->d_visual, AllocNone);
		Scr->TwmRoot.cmaps.cwins[1] = CreateColormapWindowAttr (Scr->Root, &att, True, False);
		Scr->TwmRoot.cmaps.number_cwins++;
		Scr->TwmRoot.cmaps.scoreboard = calloc (1, ColormapsScoreboardLength(&Scr->TwmRoot.cmaps));
		if (PrintErrorMessages)
		    fprintf (stderr, "%s: Visual class '%s' (VisualID 0x0%lx, Colormap 0x0%lx) at depth %d is available on X%d-screen %d.\n",
				ProgramName, visuals[Scr->d_visual->class].name, (long)(Scr->d_visual->visualid),
				(long)(Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c),
				Scr->d_depth, XProtocolVersion(dpy), scrnum);
	    }
	}

	if (Scr->d_visual == NULL)
	{
	    Scr->d_depth = DefaultDepth (dpy, scrnum);
	    Scr->d_visual = DefaultVisual (dpy, scrnum);
	    Scr->TwmRoot.cmaps.cwins[0] = CreateColormapWindow (Scr->Root, True, False);
	}

	Scr->cmapInfo.cmaps = NULL;
	Scr->cmapInfo.maxCmaps = MaxCmapsOfScreen(ScreenOfDisplay (dpy, Scr->screen));
	Scr->cmapInfo.root_pushes = 0;
	Scr->TwmRoot.cmaps.cwins[0]->visibility = VisibilityPartiallyObscured;
	InstallWindowColormaps (0, &Scr->TwmRoot);

	if (Scr->d_depth == 1)
	    Scr->Monochrome = MONOCHROME;
	else if (Scr->d_visual->class == GrayScale || Scr->d_visual->class == StaticGray)
	    Scr->Monochrome = GRAYSCALE;
	else
	    Scr->Monochrome = COLOR;

	/* setup default colors */
	Scr->FirstTime = TRUE;
	GetColor(Scr->Monochrome, &black, "black");
	GetColor(Scr->Monochrome, &white, "white");
	Scr->Black = black;
	Scr->White = white;

#ifdef TWM_USE_RENDER
	Scr->XCompMgrRunning = FALSE;
	DragWindow = 0x0; /* dirty-dancing: shape-off trick due to xcompmgr/Shape in gram.y */
#ifdef TWM_USE_COMPOSITE
	if (xcomposite_available == TRUE)
	{
	    void *xerr;
	    JunkMask = 0;
	    xerr = XSetErrorHandler (CatchCompositeError);
	    XCompositeRedirectSubwindows (dpy, Scr->Root, CompositeRedirectManual);
	    XSync (dpy, False);
	    if (JunkMask == 0) { /* we didn't run the 'CatchCompositeError()' handler? */
		XCompositeUnredirectSubwindows (dpy, Scr->Root, CompositeRedirectManual);
		if (PrintErrorMessages)
		    fprintf (stderr, "%s: %s manager is not available on X%d-screen %d.\n",
			    ProgramName, COMPOSITE_NAME, XProtocolVersion(dpy), Scr->screen);
	    } else {
		Scr->XCompMgrRunning = TRUE;
		if (PrintErrorMessages)
		    fprintf (stderr, "%s: %s manager is available on X%d-screen %d.\n",
			    ProgramName, COMPOSITE_NAME, XProtocolVersion(dpy), Scr->screen);
	    }
	    XSetErrorHandler (xerr);
	}
	if (Scr->XCompMgrRunning == TRUE) {
	    if (Scr->d_depth != 32) /*alpha-channel support?*/
	    {
		Scr->XCompMgrRunning = FALSE;
		DragWindow = 0x1; /* dirty-dancing: shape-off trick due to xcompmgr/Shape in gram.y */
	    }
	}
#endif
	Scr->XRcol32Clear.red = Scr->XRcol32Clear.green = Scr->XRcol32Clear.blue = Scr->XRcol32Clear.alpha = 0x0000;
	Scr->XRcol32Fill.red = Scr->XRcol32Fill.green = Scr->XRcol32Fill.blue = Scr->XRcol32Fill.alpha = 0xffff;
	Scr->PicAttr.graphics_exposures = False;
	Scr->PicAttr.subwindow_mode = IncludeInferiors;
	Scr->PicAttr.clip_mask = None;
	Scr->FormatRGB = NULL;
	if (ScreenDepthSupported (8) == True) {
	    Scr->FormatA = PictStandardA8;
	    Scr->DepthA = 8;
	} else if (ScreenDepthSupported (4) == True) {
	    Scr->FormatA = PictStandardA4;
	    Scr->DepthA = 4;
	} else if (ScreenDepthSupported (1) == True) {
	    Scr->FormatA = PictStandardA1;
	    Scr->DepthA = 1;
	} else {
	    xrender_available = FALSE;  /* turn off XRender, no depth supporting alpha available */
	    Scr->DepthA = 0;
	}
#endif

	/* initialize list pointers, remember to put an initialization
	 * in InitVariables also
	 */
	Scr->BorderColorL = NULL;
	Scr->IconBorderColorL = NULL;
	Scr->IconBitmapColorL = NULL;
	Scr->BorderTileForegroundL = NULL;
	Scr->BorderTileBackgroundL = NULL;
	Scr->TitleForegroundL = NULL;
	Scr->TitleBackgroundL = NULL;
	Scr->TitleButtonForegroundL = NULL;
	Scr->TitleButtonBackgroundL = NULL;
	Scr->TitleHighlightForegroundL = NULL;
	Scr->TitleHighlightBackgroundL = NULL;
	Scr->IconForegroundL = NULL;
	Scr->IconBackgroundL = NULL;
	Scr->NoTitle = NULL;
	Scr->MakeTitle = NULL;
	Scr->DecorateTransientsL = NULL;
	Scr->NoDecorateTransientsL = NULL;
	Scr->AutoRaise = NULL;
	Scr->IconNames = NULL;
	Scr->NoHighlight = NULL;
	Scr->NoStackModeL = NULL;
	Scr->NoTitleHighlight = NULL;
	Scr->DontIconify = NULL;
	Scr->IconMgrNoShow = NULL;
	Scr->IconMgrShow = NULL;
	Scr->IconifyByUn = NULL;
	Scr->IconManagerFL = NULL;
	Scr->IconManagerBL = NULL;
	Scr->IconMgrs = NULL;
	Scr->ShowIconManagerL = NULL;
	Scr->StartIconified = NULL;
	Scr->ClientBorderWidthL = NULL;
	Scr->NoClientBorderWidthL = NULL;
	Scr->SqueezeTitleL = NULL;
	Scr->DontSqueezeTitleL = NULL;
	Scr->WindowRingL = NULL;
	Scr->NoWindowRingL = NULL;
	Scr->WarpCursorL = NULL;
	Scr->NoWarpCursorL = NULL;
	Scr->RandomPlacementL = NULL;
	Scr->NoRandomPlacementL = NULL;
	/* remember to put an initialization in InitVariables also
	 */

	Scr->TBInfo.nleft = Scr->TBInfo.nright = 0;
	Scr->TBInfo.head = NULL;
	Scr->TBInfo.border = 1;
	Scr->TBInfo.width = 0;
	Scr->TBInfo.leftx = 0;
	Scr->TBInfo.titlex = 0;

	Scr->XORvalue = (((unsigned long) 1) << Scr->d_depth) - 1;

	if (FirstScreen)
	{
#ifdef TWM_USE_SLOPPYFOCUS
	    SloppyFocus = FALSE;
#endif
	    RecoverStolenFocusAttempts = 0; /* zero value means no focus recovery attempts are made */
	    RecoverStolenFocusTimeout = -1; /* negative value means no focus recovery attempts are made */
	    FocusRoot = TRUE;
	    Focus = NULL;
	    SetFocus ((TwmWindow *)NULL, CurrentTime);

	    /* define cursors */

	    NewFontCursor(&UpperLeftCursor, "top_left_corner");
	    NewFontCursor(&RightButt, "rightbutton");
	    NewFontCursor(&LeftButt, "leftbutton");
	    NewFontCursor(&MiddleButt, "middlebutton");
	}

	Scr->iconmgr.geometry = "";
	Scr->iconmgr.x = 0;
	Scr->iconmgr.y = 0;
	Scr->iconmgr.width = 150;
	Scr->iconmgr.height = 5;
	Scr->iconmgr.next = NULL;
	Scr->iconmgr.prev = NULL;
	Scr->iconmgr.lasti = &(Scr->iconmgr);
	Scr->iconmgr.first = NULL;
	Scr->iconmgr.last = NULL;
	Scr->iconmgr.active = NULL;
	Scr->iconmgr.hilited = NULL;
	Scr->iconmgr.scr = Scr;
	Scr->iconmgr.columns = 1;
	Scr->iconmgr.reversed = FALSE;
	Scr->iconmgr.Xnegative = FALSE;
	Scr->iconmgr.Ynegative = FALSE;
	Scr->iconmgr.count = 0;
	Scr->iconmgr.cur_columns = 0;
	Scr->iconmgr.cur_rows = 0;
	Scr->iconmgr.name = "TWM";
	Scr->iconmgr.icon_name = "Icons";
	Scr->ActiveIconMgr = NULL;

	Scr->IconDirectory = NULL;

	Scr->IconMgrIcon = None;
	Scr->siconifyPm = None;
	Scr->pullPm = None;
	Scr->hilitePm = None;
	Scr->borderPm = None;
	Scr->tbpm.xlogo = None;
	Scr->tbpm.resize = None;
	Scr->tbpm.question = None;
	Scr->tbpm.menu = None;
	Scr->tbpm.delete = None;

#ifdef TWM_USE_XFT
	Scr->use_xft = xft_available;
#endif

	InitVariables();
	CreateGCs();
	InitMenus();

#ifdef TWM_USE_RENDER
	Scr->use_xrender = FALSE;
#endif

	/* Parse it once for each screen. */
	ParseTwmrc(InitFile);

#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE) {
	    if (xrender_available == TRUE)
		Scr->FormatRGB = XRenderFindVisualFormat (dpy, Scr->d_visual);
	    else
		Scr->use_xrender = FALSE;
	}
#endif

	assign_var_savecolor(); /* storeing pixels for twmrc "entities" */
	if (Scr->SqueezeTitle == -1) Scr->SqueezeTitle = FALSE;
	Scr->IconRegionEntryMaxWidth = FindIconRegionEntryMaxWidth (Scr);

	if (!Scr->HaveFonts) CreateFonts();
	InitTitlebarCorners ();
	MakeMenus();

#if 1
	if (Scr->FramePadding > Scr->TitleBarFont.height) { /*special case*/
	    Scr->TitleHeight = Scr->FramePadding;
	    Scr->FramePadding = (Scr->TitleHeight - Scr->TitleBarFont.height) / 2;
	}
	else
#endif
	{
	    Scr->TitleHeight = Scr->TitleBarFont.height + Scr->FramePadding * 2;
	    /* make title height be odd so buttons look nice and centered */
	    if (!(Scr->TitleHeight & 1)) Scr->TitleHeight++;
	}
	Scr->TitleBarFont.y += Scr->FramePadding;

	InitTitlebarButtons ();		/* menus are now loaded! */

	XGrabServer(dpy);
	XSync(dpy, 0);

	nchildren = 0;
	children = NULL;
	if (XQueryTree(dpy, Scr->Root, &JunkRoot, &JunkChild, &children, &nchildren))
	    /*
	     * weed out icon windows
	     */
	    if (children != NULL)
		for (i = 0; i < nchildren; i++)
		    if (children[i] != None) {
			XWMHints * wmhintsp = XGetWMHints (dpy, children[i]);
			if (wmhintsp) {
			    if (wmhintsp->flags & IconWindowHint)
				for (j = 0; j < nchildren; j++)
				    if (children[j] == wmhintsp->icon_window) {
					children[j] = None;
					break;
				    }
			    XFree (wmhintsp);
			}
		    }

	if (Scr->NoIconManagers == FALSE) {
	    CreateIconManagers();
	    Scr->iconmgr.twm_win->icon = TRUE;
	}

	/*
	 * map all of the non-override windows
	 */
	if (children != NULL)
	{
	    for (i = 0; i < nchildren; i++)
		if (children[i] != None && MappedNotOverride(children[i]))
		{
		    XUnmapWindow (dpy, children[i]);
		    SimulateMapRequest (children[i]);
		}
	    XFree (children);
	}

	if (Scr->NoIconManagers == FALSE) {
	    if (Scr->ShowIconManager == TRUE
		    || (Scr->ShowIconManagerL != NULL
			&& LookInList(Scr->ShowIconManagerL, Scr->iconmgr.twm_win->full_name, &Scr->iconmgr.twm_win->class) != NULL)) {
		if (Scr->iconmgr.count > 0) {
		    SetMapStateProp (Scr->iconmgr.twm_win, NormalState);
		    XMapWindow (dpy, Scr->iconmgr.w);
		    XMapWindow (dpy, Scr->iconmgr.twm_win->frame);
		}
		Scr->iconmgr.twm_win->icon = FALSE;
	    }
	}

	XUngrabServer(dpy);

	attributes.border_pixel = Scr->BorderColor;
	attributes.background_pixel = Scr->DefaultC.back;
	attributes.event_mask = (ExposureMask | ButtonPressMask |
				 KeyPressMask | ButtonReleaseMask);
	attributes.backing_store = NotUseful;
	attributes.override_redirect = True;
	attributes.colormap = Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c;
	attributes.cursor = XCreateFontCursor (dpy, XC_hand2);
	valuemask = (CWColormap | CWBorderPixel | CWBackPixel | CWEventMask |
		     CWBackingStore | CWOverrideRedirect | CWCursor);

/*#ifdef TWM_USE_OPACITY*/
#ifdef TWM_USE_RENDER
	if (Scr->use_xrender == TRUE)
	    valuemask &= ~CWBackPixel;
#endif
/*#endif*/

	Scr->InfoWindow.win = XCreateWindow (dpy, Scr->Root, 0, 0,
					 (unsigned int) 5, (unsigned int) 5,
					 Scr->BorderWidth,
					 Scr->d_depth,
					 (unsigned int) CopyFromParent,
					 Scr->d_visual,
					 valuemask, &attributes);

	Scr->SizeStringWidth = MyFont_TextWidth (&Scr->SizeFont,
					   " 8888 x 8888 ", 13);
	valuemask = (CWColormap | CWBorderPixel | CWBackPixel | CWOverrideRedirect | CWBitGravity);
	attributes.bit_gravity = NorthWestGravity;
	Scr->SizeWindow.win = XCreateWindow (dpy, Scr->Root, 0, 0,
					 (unsigned int) Scr->SizeStringWidth,
					 (unsigned int) (Scr->SizeFont.height +
							 SIZE_VINDENT*2),
					 Scr->BorderWidth,
					 Scr->d_depth,
					 (unsigned int) CopyFromParent,
					 Scr->d_visual,
					 valuemask, &attributes);

#ifdef TWM_USE_RENDER
	CopyPixelToXRenderColor (Scr->DefaultC.back, &Scr->XRcolDefaultB);
#ifdef TWM_USE_OPACITY
	Scr->XRcolDefaultB.alpha = Scr->InfoOpacity * 257;
	Scr->XRcolDefaultB.red = Scr->XRcolDefaultB.red * Scr->XRcolDefaultB.alpha / 0xffff; /* premultiply */
	Scr->XRcolDefaultB.green = Scr->XRcolDefaultB.green * Scr->XRcolDefaultB.alpha / 0xffff;
	Scr->XRcolDefaultB.blue = Scr->XRcolDefaultB.blue * Scr->XRcolDefaultB.alpha / 0xffff;
	if (Scr->use_xrender == FALSE)
#endif
#endif
#ifdef TWM_USE_OPACITY
	    SetWindowOpacity (Scr->InfoWindow.win, Scr->InfoOpacity);
#endif

#ifdef TWM_USE_XFT
	if (Scr->use_xft > 0) {
	    Scr->InfoWindow.xft = MyXftDrawCreate (Scr->InfoWindow.win);
	    Scr->SizeWindow.xft = MyXftDrawCreate (Scr->SizeWindow.win);
	    CopyPixelToXftColor (Scr->DefaultC.fore, &Scr->DefaultC.xft);
	}
#endif

	FirstScreen = FALSE;
    	Scr->FirstTime = FALSE;
    } /* for */

    if (numManaged == 0) {
	if (MultiScreen && NumScreens > 0)
	  fprintf (stderr, "%s:  unable to find any unmanaged %sscreens.\n",
		   ProgramName, NoPrintscreens?"":"video ");
	exit (1);
    }

#if defined TWM_USE_XFT && defined XFT_DEBUG
    if (Scr->use_xft > 0) /* execute "XFT_DEBUG=512 twm -v" */
	XftMemReport ();  /* compile libXft with "_X_EXPORT void XftMemReport (void)" */
#endif

#ifdef TWM_ENABLE_STOLENFOCUS_RECOVERY
    /* measure X11-server roundtrip and estimate 'RecoverStolenFocusTimeout': */
    if (RecoverStolenFocusAttempts == 0) /* '0' means no recovery attempts */
	RecoverStolenFocusTimeout = -1;
    else if (RecoverStolenFocusAttempts == 1)
	RecoverStolenFocusTimeout = 0; /* timeout '0' means try once */
    if (RecoverStolenFocusAttempts > 1)
    {
	struct timeval start, stop;
	long long roundtrip;
	XImage *image;

	XSync (dpy, False);
	X_GETTIMEOFDAY (&start);

	image = XGetImage (dpy, RootWindow(dpy, DefaultScreen(dpy)), 0, 0, 1, 1, ~0, ZPixmap);
	if (image)
	    XDestroyImage (image);
	XSync (dpy, False);

	image = XGetImage (dpy, RootWindow(dpy, DefaultScreen(dpy)), 0, DisplayHeight(dpy, DefaultScreen(dpy))-1, 1, 1, ~0, ZPixmap);
	if (image)
	    XDestroyImage (image);
	XSync (dpy, False);

	image = XGetImage (dpy, RootWindow(dpy, DefaultScreen(dpy)), DisplayWidth(dpy, DefaultScreen(dpy))-1, 0, 1, 1, ~0, ZPixmap);
	if (image)
	    XDestroyImage (image);
	XSync (dpy, False);

	image = XGetImage (dpy, RootWindow(dpy, DefaultScreen(dpy)), DisplayWidth(dpy, DefaultScreen(dpy))-1, DisplayHeight(dpy, DefaultScreen(dpy))-1, 1, 1, ~0, ZPixmap);
	if (image)
	    XDestroyImage (image);
	XSync (dpy, False);

	X_GETTIMEOFDAY (&stop);
	if (stop.tv_usec < start.tv_usec)
	{
	    stop.tv_usec += 1000000;
	    stop.tv_sec -= 1;
	}
	roundtrip = (long long)(stop.tv_usec - start.tv_usec) + (long long)(stop.tv_sec - start.tv_sec) * 1000000;

	/* set focus recovery timeout (milliseconds) proportional to X11-server roundtrip (microseconds): */
	RecoverStolenFocusTimeout = 20;    /* fast */
	if (roundtrip > (long long)(500))
	    RecoverStolenFocusTimeout = 100; /* slower */
	if (roundtrip > (long long)(2000))
	    RecoverStolenFocusTimeout = 500; /* even slower */
	if (roundtrip > (long long)(5000))
	    RecoverStolenFocusTimeout = 900; /* slowest */
	if (PrintErrorMessages)
	    fprintf(stderr, "%s: X%d-server roundtrip calibration %ld microseconds, lost focus recovery timeout := %d milliseconds (or %d attempts).\n",
		ProgramName, XProtocolVersion(dpy), (long)roundtrip, RecoverStolenFocusTimeout, RecoverStolenFocusAttempts);
    }
#else
    RecoverStolenFocusTimeout = -1; /* here we disable stolen-focus recovery altogether */
#endif

    (void) ConnectToSessionManager (client_id);

    RestartPreviousState = False;
    HandlingEvents = TRUE;
    InitEvents();

#if 0
    /* raise initial "welcome" window */
    if (PrintErrorMessages) {
	Scr = FindPointerScreenInfo();
	if (Scr != NULL) {
	    Identify ((TwmWindow *) NULL);
	    XSync (dpy, False);
	}
    }
#endif

    HandleEvents();
    exit(0);
}

/**
 * initialize twm variables
 */
void
InitVariables()
{
    FreeList(&Scr->BorderColorL);
    FreeList(&Scr->IconBorderColorL);
    FreeList(&Scr->IconBitmapColorL);
    FreeList(&Scr->BorderTileForegroundL);
    FreeList(&Scr->BorderTileBackgroundL);
    FreeList(&Scr->TitleForegroundL);
    FreeList(&Scr->TitleBackgroundL);
    FreeList(&Scr->TitleButtonForegroundL);
    FreeList(&Scr->TitleButtonBackgroundL);
    FreeList(&Scr->TitleHighlightForegroundL);
    FreeList(&Scr->TitleHighlightBackgroundL);
    FreeList(&Scr->IconForegroundL);
    FreeList(&Scr->IconBackgroundL);
    FreeList(&Scr->IconManagerFL);
    FreeList(&Scr->IconManagerBL);
    FreeList(&Scr->IconMgrs);
    FreeList(&Scr->ShowIconManagerL);
    FreeList(&Scr->NoTitle);
    FreeList(&Scr->MakeTitle);
    FreeList(&Scr->DecorateTransientsL);
    FreeList(&Scr->NoDecorateTransientsL);
    FreeList(&Scr->AutoRaise);
    FreeList(&Scr->IconNames);
    FreeList(&Scr->NoHighlight);
    FreeList(&Scr->NoStackModeL);
    FreeList(&Scr->NoTitleHighlight);
    FreeList(&Scr->DontIconify);
    FreeList(&Scr->IconMgrNoShow);
    FreeList(&Scr->IconMgrShow);
    FreeList(&Scr->IconifyByUn);
    FreeList(&Scr->StartIconified);
    FreeList(&Scr->IconManagerHighlightL);
    FreeList(&Scr->ClientBorderWidthL);
    FreeList(&Scr->NoClientBorderWidthL);
    FreeList(&Scr->SqueezeTitleL);
    FreeList(&Scr->DontSqueezeTitleL);
    FreeList(&Scr->WindowRingL);
    FreeList(&Scr->NoWindowRingL);
    FreeList(&Scr->WarpCursorL);
    FreeList(&Scr->NoWarpCursorL);
    FreeList(&Scr->RandomPlacementL);
    FreeList(&Scr->NoRandomPlacementL);

    NewFontCursor(&Scr->FrameCursor, "top_left_arrow");
    NewFontCursor(&Scr->TitleCursor, "top_left_arrow");
    NewFontCursor(&Scr->IconCursor, "top_left_arrow");
    NewFontCursor(&Scr->IconMgrCursor, "top_left_arrow");
    NewFontCursor(&Scr->MoveCursor, "fleur");
    NewFontCursor(&Scr->ResizeCursor, "fleur");
    NewFontCursor(&Scr->MenuCursor, "sb_left_arrow");
    NewFontCursor(&Scr->ButtonCursor, "hand2");
    NewFontCursor(&Scr->WaitCursor, "watch");
    NewFontCursor(&Scr->SelectCursor, "dot");
    NewFontCursor(&Scr->DestroyCursor, "pirate");

    Scr->Ring = NULL;
    Scr->RingLeader = NULL;

    Scr->DefaultC.fore = black;
    Scr->DefaultC.back = white;
    Scr->BorderColor = black;
    Scr->BorderTileC.fore = black;
    Scr->BorderTileC.back = white;
    Scr->TitleC.fore = black;
    Scr->TitleC.back = white;
    Scr->TitleButtonC.fore = ((Pixel)(~0)); /* UNUSED_PIXEL, more than 24 bits */
    Scr->TitleButtonC.back = ((Pixel)(~0));
    Scr->TitleHighlightC.fore = ((Pixel)(~0));
    Scr->TitleHighlightC.back = ((Pixel)(~0));
    Scr->MenuC.fore = black;
    Scr->MenuC.back = white;
    Scr->MenuTitleC.fore = black;
    Scr->MenuTitleC.back = white;
    Scr->MenuShadowColor = black;
    Scr->MenuBorderColor = black;
    Scr->IconC.fore = black;
    Scr->IconC.back = white;
    Scr->IconBorderColor = black;
    Scr->IconBitmapColor = ((Pixel)(~0));
    Scr->PointerForeground.pixel = black;
    XQueryColor(dpy, Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c,
		&Scr->PointerForeground);
    Scr->PointerBackground.pixel = white;
    XQueryColor(dpy, Scr->TwmRoot.cmaps.cwins[Scr->TwmRoot.cmaps.number_cwins-1]->colormap->c,
		&Scr->PointerBackground);
    Scr->IconManagerC.fore = black;
    Scr->IconManagerC.back = white;
    Scr->IconManagerHighlight = black;

    Scr->FramePadding = 2;		/* values that look "nice" on */
    Scr->TitlePadding = 8;		/* 75 and 100dpi displays */
    Scr->ButtonIndent = 1;
    Scr->SizeStringOffset = 0;
    Scr->BorderWidth = BW;
    Scr->IconBorderWidth = BW;
    Scr->MenuBorderWidth = BW;
    Scr->UnknownWidth = 0;
    Scr->UnknownHeight = 0;
    Scr->NumAutoRaises = 0;
    Scr->NoDefaults = FALSE;
    Scr->UsePPosition = PPOS_OFF;
    Scr->WarpCursor = FALSE;
    Scr->ForceIcon = FALSE;
    Scr->NoGrabServer = FALSE;
    Scr->NoRaiseMove = FALSE;
    Scr->NoRaiseResize = FALSE;
    Scr->NoRaiseDeicon = FALSE;
    Scr->NoRaiseWarp = FALSE;
    Scr->DontMoveOff = FALSE;
    Scr->DoZoom = FALSE;
    Scr->TitleFocus = TRUE;
    Scr->NoTitlebar = FALSE;
    Scr->DecorateTransients = FALSE;
    Scr->IconifyByUnmapping = FALSE;
    Scr->ShowIconManager = FALSE;
    Scr->IconManagerDontShow =FALSE;
    Scr->BackingStore = TRUE;
    Scr->SaveUnder = TRUE;
    Scr->RandomPlacement = FALSE;
    Scr->OpaqueMove = FALSE;
    Scr->Highlight = TRUE;
    Scr->StackMode = TRUE;
    Scr->TitleHighlight = TRUE;
    Scr->MoveDelta = 1;		/* so that f.deltastop will work */
    Scr->ZoomCount = 8;
    Scr->SortIconMgr = FALSE;
    Scr->Shadow = TRUE;
    Scr->InterpolateMenuColors = FALSE;
    Scr->NoIconManagers = FALSE;
    Scr->ClientBorderWidth = FALSE;
    Scr->SqueezeTitle = -1;
    Scr->RoundedTitle = FALSE;
    Scr->FirstRegion = NULL;
    Scr->LastRegion = NULL;
    Scr->FirstTime = TRUE;
    Scr->HaveFonts = FALSE;		/* i.e. not loaded yet */
    Scr->CaseSensitive = TRUE;
    Scr->WarpUnmapped = FALSE;
    Scr->ShowXUrgencyHints = FALSE;
    Scr->ZoomFunc = ZOOM_NONE;
    Scr->ZoomPanel = 0; /* 'current panel', see ParsePanelIndex() */
    Scr->ShapedIconLabels = FALSE;
    Scr->ShapedIconPixmaps = FALSE;
    Scr->IconLabelOffsetX = 0;
    Scr->IconLabelOffsetY = 0;
    Scr->FilledIconMgrHighlight = FALSE;
    Scr->ShapedIconMgrLabels = FALSE;
    Scr->IconMgrLabelOffsetX = 0;
    Scr->IconMgrLabelOffsetY = 0;

    /* setup default fonts; overridden by defaults from system.twmrc */
#define DEFAULT_NICE_FONT "variable"
#define DEFAULT_FAST_FONT "fixed"

    Scr->TitleBarFont.font = NULL;
    Scr->TitleBarFont.fontset = NULL;
    Scr->TitleBarFont.name = DEFAULT_NICE_FONT;
    Scr->MenuTitleFont.font = NULL;
    Scr->MenuTitleFont.fontset = NULL;
    Scr->MenuTitleFont.name = DEFAULT_NICE_FONT;
    Scr->MenuFont.font = NULL;
    Scr->MenuFont.fontset = NULL;
    Scr->MenuFont.name = DEFAULT_NICE_FONT;
    Scr->IconFont.font = NULL;
    Scr->IconFont.fontset = NULL;
    Scr->IconFont.name = DEFAULT_NICE_FONT;
    Scr->SizeFont.font = NULL;
    Scr->SizeFont.fontset = NULL;
    Scr->SizeFont.name = DEFAULT_FAST_FONT;
    Scr->IconManagerFont.font = NULL;
    Scr->IconManagerFont.fontset = NULL;
    Scr->IconManagerFont.name = DEFAULT_NICE_FONT;
    Scr->DefaultFont.font = NULL;
    Scr->DefaultFont.fontset = NULL;
    Scr->DefaultFont.name = DEFAULT_FAST_FONT;

#ifdef TWM_USE_OPACITY
    Scr->MenuOpacity = 255;		/* 0 = transparent ... 255 = opaque */
    Scr->IconOpacity = 255;
    Scr->IconMgrOpacity = 255;
    Scr->InfoOpacity = 255;
#endif

#ifdef TWM_USE_XFT
    Scr->TitleBarFont.xft = NULL;
    Scr->MenuTitleFont.xft = NULL;
    Scr->MenuFont.xft = NULL;
    Scr->IconFont.xft = NULL;
    Scr->SizeFont.xft = NULL;
    Scr->IconManagerFont.xft = NULL;
    Scr->DefaultFont.xft = NULL;
    CopyPixelToXftColor (Scr->White, &Scr->XftWhite);
    CopyPixelToXftColor (Scr->Black, &Scr->XftBlack);
#endif

#ifdef TWM_USE_XRANDR
    Scr->RRScreenChangeRestart = FALSE;
    Scr->RRScreenSizeChangeRestart = FALSE;
#endif
}

void
CreateFonts ()
{
    /*
     * Record runtime drawing screen back-pointer
     * (not always drawing to screen pointed to by "Scr"):
     */
    Scr->TitleBarFont.scr = Scr;
    Scr->MenuTitleFont.scr = Scr;
    Scr->MenuFont.scr = Scr;
    Scr->IconFont.scr = Scr;
    Scr->SizeFont.scr = Scr;
    Scr->IconManagerFont.scr = Scr;
    Scr->DefaultFont.scr = Scr;

    GetFont(&Scr->TitleBarFont);
    GetFont(&Scr->MenuTitleFont);
    GetFont(&Scr->MenuFont);
    GetFont(&Scr->IconFont);
    GetFont(&Scr->SizeFont);
    GetFont(&Scr->IconManagerFont);
    GetFont(&Scr->DefaultFont);
    Scr->HaveFonts = TRUE;
}

void
RestoreWithdrawnLocation (TwmWindow *tmp)
{
    int gravx, gravy;
    unsigned int bw, mask;
    XWindowChanges xwc;

    if (XGetGeometry (dpy, tmp->w, &JunkRoot, &xwc.x, &xwc.y, 
		      &JunkWidth, &JunkHeight, &bw, &JunkDepth)) {

	GetGravityOffsets (tmp, &gravx, &gravy);
	if (gravy < 0) xwc.y -= tmp->title_height;

	if (bw != tmp->old_bw) {
	    int xoff, yoff;

	    if (!tmp->ClientBorderWidth) {
		xoff = gravx;
		yoff = gravy;
	    } else {
		xoff = 0;
		yoff = 0;
	    }

	    xwc.x -= (xoff + 1) * tmp->old_bw;
	    xwc.y -= (yoff + 1) * tmp->old_bw;
	}
	if (!tmp->ClientBorderWidth) {
	    xwc.x += gravx * tmp->frame_bw;
	    xwc.y += gravy * tmp->frame_bw;
	}

	mask = (CWX | CWY);
	if (bw != tmp->old_bw) {
	    xwc.border_width = tmp->old_bw;
	    mask |= CWBorderWidth;
	}

	XConfigureWindow (dpy, tmp->w, mask, &xwc);

	if (tmp->wmhints && (tmp->wmhints->flags & IconWindowHint)) {
	    XUnmapWindow (dpy, tmp->wmhints->icon_window);
	}

    }
}


void 
Reborder (Time time)
{
    TwmWindow *tmp;			/* temp twm window structure */
    int scrnum;

    /* put a border back around all windows */

    XGrabServer (dpy);
    for (scrnum = 0; scrnum < NumScreens; scrnum++)
    {
	if ((Scr = ScreenList[scrnum]) == NULL)
	    continue;

	InstallWindowColormaps (0, &Scr->TwmRoot);	/* force reinstall */
	for (tmp = Scr->TwmRoot.next; tmp != NULL; tmp = tmp->next)
	{
	    /* unzoom to preserve size only if 'ZoomState' statement is in .twmrc: */
	    if (Scr->ZoomFunc != ZOOM_NONE && tmp->zoomed != ZOOM_NONE)
	    {
		if (Scr->ZoomFunc == tmp->zoomed && tmp->icon == FALSE) /* honour 'IconicState' */
		    SetMapStateProp (tmp, ZoomState); /* record 'ZoomState' in _XA_WM_STATE */

		fullzoom (Scr->ZoomPanel, tmp, /* bypass zoom special case: */
				(tmp->zoomed = (tmp->zoomed==F_PANELGEOMETRYZOOM
				    || tmp->zoomed==F_PANELGEOMETRYMOVE ? ZOOM_NONE : tmp->zoomed)));
	    }

	    RestoreWithdrawnLocation (tmp);
	    XMapWindow (dpy, tmp->w);
	}
    }

    XUngrabServer (dpy);
    SetFocus ((TwmWindow*)NULL, time);

#if defined TWM_USE_XFT && defined XFT_DEBUG
    for (scrnum = 0; scrnum < NumScreens; scrnum++)
	if (ScreenList[scrnum] != NULL && ScreenList[scrnum]->use_xft > 0) {
	    XftMemReport ();
	    break;
	}
#endif
}

static SIGNAL_T 
sigHandler(int sig)
{
    XtNoticeSignal(si);
    SIGNAL_RETURN;
}

/**
 * cleanup and exit twm
 */
void
Done(XtPointer client_data, XtSignalId *si)
{
    if (dpy) 
    {
	Reborder(CurrentTime);
	XCloseDisplay(dpy);
    }
    exit(0);
}


/*
 * Error Handlers.  If a client dies, we'll get a BadWindow error (except for
 * GetGeometry which returns BadDrawable) for most operations that we do before
 * manipulating the client's window.
 */

Bool ErrorOccurred = False;
XErrorEvent LastErrorEvent;

static int
AfterRaiseInfoWindow (Display *d)
{
    XSetAfterFunction (dpy, NULL); /*turn off*/
    if (InfoLines > 0)
    {
#if 0
	int i;
	for (i = 0; i < NumScreens; ++i)
	    if ((Scr = ScreenList[i]) != NULL)
		break;
#else
	Scr = FindPointerScreenInfo();
#endif
	if (Scr != NULL)
	{
	    int x, y;
	    y = InfoLines * (120*Scr->DefaultFont.height/100) / 2;
#if 1
	    x = MyFont_TextWidth (&Scr->DefaultFont, Info[0], strlen(Info[0])) / 2;
#else
	    x = 2*y;
#endif
#ifdef TILED_SCREEN
	    if (Scr->use_panels == TRUE) {
		int i = FindNearestPanelToMouse();
		x += Lft(Scr->panels[i]);
		y += Bot(Scr->panels[i]);
	    }
#endif
	    RaiseInfoWindow (2*Scr->DefaultFont.height + x, y);
	}
    }
    return 0;
}

static int 
TwmErrorHandler(Display *dpy, XErrorEvent *event)
{
    LastErrorEvent = *event;
    ErrorOccurred = True;

    if (PrintErrorMessages)
    {
	time_t moment = time ((time_t*)(0));
	fprintf (stderr, "%s:  %s", ProgramName,
		asctime(localtime(&moment)));
#if 1
	if (PrintErrorMessages == 2)
	{
	    int pipefd[2];
	    FILE *fout;
	    if (pipe(pipefd) == 0)
	    {
		fout = fdopen (pipefd[1], "w");
		if (fout)
		{
		    char buf[1024], *fin = buf;
		    int i, cnt;

		    XmuPrintDefaultErrorMessage (dpy, event, fout);
		    if (fclose (fout))
			fprintf (stderr, "%s: fclose() failed in TwmErrorHandler().\n", ProgramName);

		    i = cnt = 0;
		    do {
			i = read (pipefd[0], fin+cnt, sizeof(buf)-1-cnt);
			if (0 < i)
			    cnt += i;
		    } while (0 < i && cnt < sizeof(buf));
		    if (!(cnt < sizeof(buf)-1))
			cnt = sizeof(buf)-1;
		    fin[cnt] = '\0';
		    if (close (pipefd[0]))
			fprintf (stderr, "%s: close (pipefd[0]) failed in TwmErrorHandler(1).\n", ProgramName);

		    if (0 < cnt)
		    {
			fprintf (stderr, "%s", fin), fflush (stderr);
#if 1
			InfoLines = 0;
			while (0 < cnt) {
			    i = 0;
			    while (0 < cnt && (*fin) != '\0' && (*fin) != '\n' && (*fin) != '\r') {
				Info[InfoLines][i++] = *fin++;
				cnt--;
			    }
			    Info[InfoLines][i] = '\0';
/*fprintf(stderr, "TwmErrorHandler() LINE %d '%s'\n", InfoLines, Info[InfoLines]), fflush (stderr);*/
			    fin++;
			    cnt--;
			    InfoLines++;
			}
			Info[InfoLines++][0] = '\0';
			/*
			 * Xlib documentation explicitly states not to call any Xlib-functions
			 * which generate X11-protocol requests or look for input events from
			 * inside an error handler.  Therefore we cannot call RaiseInfoWindow()
			 * here because it does exactly this and we are at risk of crashing twm.
			 * So lets install the 'after-function' which raises info window on
			 * continuing completion of this event (which triggered the current
			 * X11-error).  Being asynchronous, we can run this error handler here
			 * multiple times before the 'after-function' gets executed; which
			 * means we'll see the last error occurred.
			 */
			XSetAfterFunction (dpy, AfterRaiseInfoWindow);
#endif
		    }
		    else if (cnt < 0)
			fprintf (stderr, "%s: read() failed in TwmErrorHandler().\n", ProgramName);
		} else {
		    fprintf (stderr, "%s: fdopen() failed in TwmErrorHandler().\n", ProgramName);
		    if (close (pipefd[0]))
			fprintf (stderr, "%s: close (pipefd[0]) failed in TwmErrorHandler(2).\n", ProgramName);
		    if (close (pipefd[1]))
			fprintf (stderr, "%s: close (pipefd[1]) failed in TwmErrorHandler().\n", ProgramName);
		}
	    } else
		fprintf (stderr, "%s: pipe() failed in TwmErrorHandler().\n", ProgramName);
	}
	else
#endif
	{
	    XmuPrintDefaultErrorMessage (dpy, event, stderr);
	}
    }
    return 0;
}


static int 
CatchRedirectError(Display *dpy, XErrorEvent *event)
{
    RedirectError = TRUE;
    LastErrorEvent = *event;
    ErrorOccurred = True;
    return 0;
}

#ifdef TWM_USE_COMPOSITE
static int
CatchCompositeError(Display *dpy, XErrorEvent *event)
{
    if (event->request_code == XcompositeOpcode)
	if (event->minor_code == X_CompositeRedirectSubwindows)
	    JunkMask = ~JunkMask;
    return 0;
}
#endif
