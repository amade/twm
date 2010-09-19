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
/* $XFree86: xc/programs/twm/icons.h,v 1.4 2001/01/17 23:45:06 dawes Exp $ */

/**********************************************************************
 *
 * $Xorg: icons.h,v 1.4 2001/02/09 02:05:36 xorgcvs Exp $
 *
 * Icon releated definitions
 *
 * 10-Apr-89 Tom LaStrange        Initial Version.
 *
 **********************************************************************/

#ifndef ICONS_H
#define ICONS_H

typedef struct IconRegion
{
    struct IconRegion	*next;
    int			x, y, w, h;
    int			grav1, grav2;
    int			stepx, stepy;	/* allocation granularity */
    struct IconEntry	*entries;
} IconRegion;

typedef struct IconEntry
{
    struct IconEntry	*next;
    int			x, y, w, h;
    TwmWindow		*twm_win;
    short 		used;
}IconEntry;

extern int FindIconRegionEntryMaxWidth ( struct ScreenInfo *scr );
extern int roundUp ( int v, int multiple );
extern void PlaceIcon ( TwmWindow *tmp_win, int def_x, int def_y, 
		       int *final_x, int *final_y );
extern void IconUp ( TwmWindow *tmp_win );
extern void IconDown ( TwmWindow *tmp_win );
extern void AddIconRegion ( char *geom, int grav1, int grav2, 
			   int stepx, int stepy );
extern void CreateIconWindow ( TwmWindow *tmp_win, int def_x, int def_y );
extern void SetIconWindowHintForFrame ( TwmWindow *tmp_win, Window icon_window );

extern Visual * FindVisual ( int screen, int *class, int *depth );
extern Pixmap CreateIconWMhintsPixmap ( TwmWindow *tmp_win, int *depth );
extern void ComputeIconSize ( TwmWindow *tmp_win, int *bm_x, int *bm_y );
extern Window CreateIconBMWindow ( TwmWindow *tmp_win, int x, int y, Pixmap pm, int pm_depth );
extern void SetIconShapeMask ( TwmWindow *tmp_win );

#endif /* ICONS_H */
