/* 
 * Xdialog Library. Copyright (c) 1993 - 2002  W. Klaren,
 *                                2002 - 2003  H. Robbers,
 *                                2003 - 2007  Dj. Vukovic
 *
 * This file is part of Teradesk.
 *
 * Teradesk is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Teradesk is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Teradesk; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */


#include <np_aes.h>
#include <vdi.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <library.h>
#include <multitos.h>

#include "xdialog.h"

extern int xd_vhandle;
extern int xe_mbshift;

#define ACC_WIND	19 /* from window.h !!! */


static WINDOW 
	*windows = NULL;		/* lijst met windows. */

WINDOW 
	*xw_deskwin = NULL; 			/* pointer to desktop window */

int
	xw_dosend = 1;					/* if 0, xw_send is disabled */

#ifdef __PUREC__
	extern int ap_id;		/* Defined in application. */
	#define aes_pid ap_id
#else
	#define aes_pid gl_apid
#endif


/*
 * Two dummy functions to be used as  WD_FUNC for ceratin window types 
 * in order to avoid some if-thens
 */

#pragma warn -par

void xw_nop1(WINDOW *w)
{

}

void xw_nop2(WINDOW *w, int i)
{

}

#pragma warn +par

/*
 * Send a message that passes rectangle size
 * All unused members of the message will be set to 0.
 * Sending is disabled if xw_dosend is set to 0.
 */


void xw_send_rect(WINDOW *w, int messid, int pid, RECT *area)
{
	if(xw_dosend)
	{
		int message[8], *messagep = message;

		*messagep++ = messid;
		*messagep++ = aes_pid;
		*messagep++ = 0;
		*messagep++ = w->xw_handle;
		*(RECT *)(messagep) = *area;
	
		appl_write(pid, 16, message);
	}
}


/*
 * Stuur een redraw boodschap naar een window.
 *
 * Parameters:
 *
 * w		- window,
 * area		- op nieuw te tekenen gebied.
 *
 * Opmerking : Om de funktie te kunnen gebruiken moet er een
 *			   globale variabele ap_id zijn, waarin de AES
 *			   applikatie id van het programma staat.
 */

void xw_send_redraw(WINDOW *w, int messid, RECT *area)
{
	xw_send_rect(w, messid, aes_pid, area);
}


/*
 * Send a message to this or other window owner.
 * All unused members of the message will be set to 0.
 * Sending is disabled if xw_dosend is set to 0.
 */

void xw_send(WINDOW *w, int messid)
{
	static const int dummy[] = {0, 0, 0, 0};


	xw_send_rect(w, messid, w->xw_ap_id, (RECT *)&dummy[0]);
}


/*
 * Funktie voor het vinden van een WINDOW structuur, aan de hand
 * van de window handle.
 * Find a window by its handle
 */

WINDOW *xw_hfind(int handle)
{
	WINDOW *w = windows;


	while (w)
	{
		if (w->xw_handle == handle)
			return (w);

		w = xw_next(w);
	}

	return (handle == 0) ? xw_deskwin : NULL;
}


/*
 * Funktie voor het vinden van een window dat zich op een bepaalde
 * positie bevindt.
 *
 * Parameters:
 *
 * x	- x coordinaat.
 * y	- y coordinaat.
 *
 * Resultaat : Pointer naar de WINDOW structuur van het window of
 *			   een NULL pointer als zich er geen window bevindt
 *			   op de opgegeven positie.
 */

WINDOW *xw_find(int x, int y)
{
	return xw_hfind(wind_find(x, y));
}


/*
 * Funktie voor het vinden van het bovenste window.
 *
 * Resultaat : Pointer naar de WINDOW structuur van het bovenste
 *			   window of NULL als er geen window geopend is.
 *
 * Opmerking : Dit levert altijd het bovenste window van de
 *			   applikatie, niet het werkelijke bovenste window,
 *			   dit kan ook van een accessory of van een andere
 *			   applikatie zijn.
 */

WINDOW *xw_top(void)
{
	WINDOW
		*w;

	int
		thandle;


	xw_get(NULL, WF_TOP, &thandle);		/* the real top window */

	/* If any known window is topped, return a pointer to it */

	if ((w = xw_hfind(thandle)) != NULL)
	{
		if ( w->xw_type == ACC_WIND ) 
			xw_note_top(w);

		return w;
	}

	/* No windows topped, search further */

	w = windows;

	while (w)
	{
		if ((w->xw_xflags & XWF_OPN) != 0)
			return w;		/* first teradesk's window */

		w = xw_next(w);
	}

	return xw_deskwin;		/* or the desktop, if none other */
}


/*
 * Return a pointer to the bottom window
 */

WINDOW *xw_bottom(void)
{
	WINDOW
		*w;

	int
		bhandle;


	xw_get(NULL, WF_BOTTOM, &bhandle);		/* the real bottom window */

	/* If any known window is the bottom one, return a pointer to it */

	if ((w = xw_hfind(bhandle)) != NULL)
	{
		if ( w->xw_type == ACC_WIND ) 
			xw_note_bottom(w);

		return w;
	}

	/* No windows found, search further */

	w = xw_last();

	return w;
}


/*
 * Funktie die kijkt of een bepaalde WINDOW structuur, inderdaad
 * (nog) bestaat.
 *
 * Parameters:
 *
 * w	- de target WINDOW structuur.
 *
 * Resultaat : 1 als t voorkomt in de window lijst
 *			   0 in alle andere gevallen
 */

int xw_exist( WINDOW *w )
{
	WINDOW *h = windows;


	while (h)
	{
		if (h == w)
			return 1;

		h = xw_next(h);
	}

	if (xw_deskwin && (w == xw_deskwin))
		return 1;

	return 0;
}


/*
 * Funktie die het eerste window uit de windowlijst teruggeeft.
 *
 * Resultaat: NULL als er geen window geopend is, anders een pointer
 *			  naar het eerste window uit de windowlijst.
 */

WINDOW *xw_first(void)
{
	return windows;
}


/*
 * Funktie die het laatste window uit de windowlijst teruggeeft.
 *
 * Resultaat: NULL als er geen window geopend is, anders een pointer
 *			  naar het laatste window uit de windowlijst.
 */

WINDOW *xw_last(void)
{
	WINDOW *w = NULL;

	if (windows)
	{
		w = windows;

		while (xw_next(w) != NULL)
			w = xw_next(w);
	}

	return w;
}


/*
 * Notify that a window is the top one.
 * Nothing is done if the window is already topped.
 */

void xw_note_top(WINDOW *w)
{
	if (w && w != windows)
	{
		if (xw_next(w))
			w->xw_next->xw_prev = xw_prev(w);

		w->xw_prev->xw_next = xw_next(w);
		windows->xw_prev = w;
		w->xw_prev = NULL;
		w->xw_next = windows;
		windows = w;

		if (w->xw_func->wd_top != 0L)
			w->xw_func->wd_top(w);
	}
}


/*
 * Note that a window is the bottom one
 */

void xw_note_bottom(WINDOW *w)
{
	WINDOW *lw = xw_last();

	if (!(w == lw || lw == windows || lw))
	{
		if (xw_next(w))
			w->xw_next->xw_prev = xw_prev(w);

		if (w->xw_prev)
			w->xw_prev->xw_next = xw_next(w);

		lw->xw_next = w;
		w->xw_prev = lw;
		w->xw_next = NULL;
	}
}


/*
 * Set a window to top or bottom
 */

static void xw_set_topbot(WINDOW *w, int wf)
{
	void
		(*notef)(WINDOW *w);

	int
		msg;


	if(wf == WF_TOP)
	{
		msg = WM_TOPPED;
		notef = xw_note_top;
	}
	else
	{
		msg = WM_BOTTOMED;
		xw_bottom();
		notef = xw_note_bottom;
	}

	if(w->xw_type == ACC_WIND)
		xw_send(w, msg);
	else 
		wind_set(w->xw_handle, wf, w->xw_handle);

	notef(w);
}

/*
 * Funktie voor het cyclen van windows. De funktie maakt van het
 * onderste window het bovenste.
 */

void xw_cycle(void)
{
	WINDOW 
		*lw = xw_last();

	/* Set it as top window */

	if ((lw != NULL) && (lw != windows))
		xw_set_topbot(lw, WF_TOP);
}


/*
 * Funktie voor het bepalen van de rechthoek om de menubalk
 * van een window.
 */

static void xw_bar_rect(WINDOW *w, RECT *r)
{
	xd_objrect(w->xw_menu, w->xw_bar, r);
	r->h += 1;
}


/*
 * Funktie die de positie van de menubalk zet op de positie van
 * het werkgebied van het window.
 */

static void xw_set_barpos(WINDOW *w)
{
	OBJECT *menu = w->xw_menu;

	if (menu)
	{
		menu->ob_x = w->xw_work.x;
		menu->ob_y = w->xw_work.y; 
		menu[w->xw_bar].ob_width = w->xw_work.w;
	}
}


/*
 * Funktie die de menubalk in een window opnieuw tekent,
 * bijvoorbeeld na een redraw event.
 */

void xw_redraw_menu(WINDOW *w, int object, RECT *r)
{
	OBJECT
		*menu;

	RECT
		r1, r2, in;

	int
		pxy[4];


	menu = w->xw_menu;

	/* don't redraw in iconified window */

	if ( menu && ((w->xw_xflags & XWF_ICN) == 0) )
	{
		xd_objrect(menu, object, &r1);
		if (object == w->xw_bar)
			r1.h += 1;

		/* Begin en eind coordinaten van lijn onder de menubalk. */

		pxy[0] = w->xw_work.x;
		pxy[1] = pxy[3] = w->xw_work.y + r1.h - 1;
		pxy[2] = w->xw_work.x + w->xw_work.w - 1;

		if (xd_rcintersect(r, &r1, &r1))
		{
			xd_begupdate();
			xd_mouse_off();
			xd_vswr_trans_mode();
			set_linedef(1);

			xw_getfirst(w, &r2);

			while (r2.w != 0 && r2.h != 0)
			{
				if (xd_rcintersect(&r1, &r2, &in))
				{
					objc_draw(menu, w->xw_bar, MAX_DEPTH, in.x, in.y, in.w, in.h);
					xd_clip_on(&in);
					v_pline(xd_vhandle, 2, pxy);
					xd_clip_off();
				}
				xw_getnext(w, &r2);
			}

			xd_mouse_on();
			xd_endupdate();
		}
	}
}


/*
 * Funktie die de ouder objecten van de menubalk en van de 
 * pulldown menu's bepaalt.
 */

static void xw_find_objects(OBJECT *menu, int *bar, int *boxes)
{
	*bar = menu->ob_head;
	*boxes = menu->ob_tail;
}


/*
 * Funktie voor het tekenen van een pulldown menu.
 */

static void xw_menu_draw(OBJECT *menu, int item, RECT *box)
{
	objc_draw(menu, item, MAX_DEPTH, box->x, box->y, box->w, box->h);
}


/*
 * Funktie voor het selecteren en deselecteren van een item
 * in een pulldown menu.
 */

static void xw_menu_change(OBJECT *menu, int item, int select, RECT *box)
{
	int newstate = menu[item].ob_state;

	newstate = (select) ? newstate | SELECTED : newstate & ~SELECTED;
	objc_change(menu, item, 0, box->x, box->y, box->w, box->h, newstate, 1);
}


/*
 * Funktie voor het kopieren van een deel van het scherm van of
 * naar een buffer (voor het redden van het scherm onder een
 * pulldown menu.
 */

static void xw_copy_screen(MFDB *dest, MFDB *src, int *pxy)
{
	xd_mouse_off();
	vro_cpyfm(xd_vhandle, S_ONLY, pxy, src, dest);
	xd_mouse_on();
}


/*
 * Funktie voor het afhandelen van muis klikken in de menubalk
 * van een window.
 */

static int xw_do_menu(WINDOW *w, int x, int y)
{
	OBJECT
		*menu;

	RECT
		r,
		box;

	MFDB
		bmfdb,
		smfdb;

	long
		mem;

	int
		pxy[8],
		title,
		otitle,
		item,
		p,
		i,
		c,
		exit_mstate,
		stop,
		draw;


	menu = w->xw_menu;

	/* If no menu, or if window is iconified, return */

	if (menu == NULL || ((w->xw_xflags & XWF_ICN) != 0) )
		return FALSE;

	xw_bar_rect(w, &r);

	if (xd_inrect(x, y, &r) == FALSE)
		return FALSE;

	p = menu[w->xw_bar].ob_head;

	exit_mstate = (xe_button_state() & 1) ? 0 : 1;

	if (((title = objc_find(menu, p, MAX_DEPTH, x, y)) >= 0) && (menu[title].ob_type == G_TITLE))
	{
		xd_begupdate();
		xd_begmctrl();

		item = -1;
		stop = FALSE;

		do
		{
			menu[title].ob_state |= SELECTED;
			xw_redraw_menu(w, title, &r);

			i = menu[p].ob_head;
			c = 0;

			/* Zoek welke titel geselekteerd is */

			while (i != title)
			{
				i = menu[i].ob_next;
				c++;
			}

			i = menu[w->xw_mparent].ob_head;

			/* Zoek de bijbehorende box */

			while (c > 0)
			{
				i = menu[i].ob_next;
				c--;
			}

			xd_objrect(menu, i, &box);

			box.x -= 1;
			box.y -= 1;
			box.w += 2;
			box.h += 2;

			mem = xd_initmfdb(&box, &bmfdb);

			otitle = title;

			if ((bmfdb.fd_addr = (*xd_malloc)(mem)) == NULL)
				stop = TRUE;
			else
			{
				if ((draw = xd_rcintersect(&box, &xd_desk, &box)) == TRUE)
				{
					xd_rect2pxy(&box, pxy);
					pxy[4] = 0;
					pxy[5] = 0;
					pxy[6] = box.w - 1;
					pxy[7] = box.h - 1;
					smfdb.fd_addr = NULL;

					xw_copy_screen(&bmfdb, &smfdb, pxy);
					xw_menu_draw(menu, i, &box);
				}

				do
				{
					int mx, my, oitem, dummy;

					oitem = item;

					stop = xe_mouse_event(exit_mstate, &mx, &my, &dummy);

					if ((title = objc_find(menu, p, MAX_DEPTH, mx, my)) < 0)
					{
						title = otitle;

						if (((item = objc_find(menu, i, MAX_DEPTH, mx, my)) >= 0) && (menu[item].ob_state & DISABLED))
							item = -1;
					}
					else
					{
						item = -1;
						if (exit_mstate != 0)
							stop = FALSE;
					}

					if (item != oitem)
					{
						if (oitem >= 0)
							xw_menu_change(menu, oitem, FALSE, &box);
						if (item >= 0)
							xw_menu_change(menu, item, TRUE, &box);
					}
				}
				while ((title == otitle) && (stop == FALSE));

				if (item >= 0)
					menu[item].ob_state &= ~SELECTED;

				if (draw)
				{
					pxy[0] = 0;
					pxy[1] = 0;
					pxy[2] = box.w - 1;
					pxy[3] = box.h - 1;

					xd_rect2pxy(&box, &pxy[4]);
					smfdb.fd_addr = NULL;
					xw_copy_screen(&smfdb, &bmfdb, pxy);
				}

				(*xd_free)(bmfdb.fd_addr);
			}

			if (item < 0)
			{
				menu[otitle].ob_state &= ~SELECTED;
				xw_redraw_menu(w, otitle, &r);
			}
		}
		while (stop == FALSE);

		/* Wacht tot muisknop wordt losgelaten. */

		while (xe_button_state() & 1);

		xd_endmctrl();
		xd_endupdate();

		if (item >= 0)
			w->xw_func->wd_hndlmenu(w, title, item);
	}

	return TRUE;
}


/*
 * Funktie voor het plaatsen of verwijderen van een marker voor
 * een menupunt.
 *
 * Parameters:
 *
 * w		- window,
 * item		- index van menupunt,
 * check	- 0 = verwijder marker, 1 = plaats marker.
 */

void xw_menu_icheck(WINDOW *w, int item, int check)
{
	unsigned int *s = (unsigned int *)&(w->xw_menu[item].ob_state);

	if (check == 0)
		*s &= ~CHECKED;
	else
		*s |= CHECKED;
}



/* These functions are curently unused in TeraDesk

/*
 * Funktie voor het enablen of disablen van een menupunt.
 *
 * Parameters:
 *
 * w		- window,
 * item		- index van menupunt,
 * enable	- 0 = disable, 1 = enable.
 */

void xw_menu_ienable(WINDOW *w, int item, int enable)
{
	unsigned int *s = (unsigned int *)&(w->xw_menu[item].ob_state);

	if (enable == 0)
		*s |= DISABLED;
	else
		*s &= ~DISABLED;
}


/*
 * Funktie voor het veranderen van de tekst van een menupunt.
 *
 * Parameters:
 *
 * w		- window,
 * item		- index van menupunt,
 * text		- nieuwe tekst.
 */

void xw_menu_text(WINDOW *w, int item, const char *text)
{
	w->xw_menu[item].ob_spec.free_string = (char *)text;
}

*/


/*
 * Funktie voor het selecteren en deselecteren van een menutitel.
 *
 * Parameters:
 *
 * w		- window,
 * item		- index van menupunt,
 * normal	- 0 = selecteer, 1 = deselecteer.
 */

void xw_menu_tnormal(WINDOW *w, int item, int normal)
{
	RECT r;
	unsigned int *s = (unsigned int *)&(w->xw_menu[item].ob_state);


	if (normal == 0)
		*s |= SELECTED;
	else
		*s &= ~SELECTED;

	xw_bar_rect(w, &r);
	xw_redraw_menu(w, item, &r);
}

#if !__USE_MACROS
/*
 * Bepaal het type van een window.
 */

int xw_type(WINDOW *w)
{
	return w->xw_type;
}


/*
 * Bepaal de window handle van een window.
 */

int xw_handle(WINDOW *w)
{
	return w->xw_handle;
}


/*
 * Return a  pointer to the next window 
 */

WINDOW *xw_next(WINDOW *w)
{
	return w->xw_next;
}


/*
 * Return a pointer to the previous window
 */

WINDOW *xw_prev(WINDOW *w)
{
	return w->xw_prev;
}

#endif


/*
 * Funktie voor het afhandelen van een button event. Als er voor
 * een bepaald window geen handler is geinstalleerd of als het
 * window van een ander applicatie is, dan wordt FALSE teruggegeven.
 *
 * Parameters:
 *
 * x			- x coordinaat muis event,
 * y			- y coordinaat muis event,
 * n			- n aantal muisklik's,
 * buttonstate	- toestand muisbuttons bij event,
 * keystate		- toestand SHIFT, CONTROL en ALTERNATE toetsen.
 */

int xw_hndlbutton(int x, int y, int n, int bstate, int kstate)
{
	WINDOW *w;

	if ((w = xw_find(x, y)) != NULL)
	{
		if (!xw_do_menu(w, x, y))
		{
			if (w->xw_func->wd_hndlbutton != 0L)
			{
				w->xw_func->wd_hndlbutton(w, x, y, n, bstate, kstate);
				return TRUE;
			}
		}
		else
			return TRUE;
	}

	return FALSE;
}


/*
 * Funktie voor het afhandelen van een keyboard event. Als er voor
 * een bepaald window geen handler is geinstalleerd of als het
 * bovenste window van een ander applicatie is, dan wordt FALSE
 * teruggegeven.
 *
 * Parameters:
 *
 * scancode	- scancode van de toets, in Xdialog formaat,
 * keystate	- toestand SHIFT, CONTROL en ALTERNATE toetsen.
 */

int xw_hndlkey(int scancode, int keystate)
{
	WINDOW *w = xw_top();

	if (w != NULL)
	{
		if ( scancode == 0x2217 ) /* hardcoded ^W from AV-protocol client */
		{
			xw_cycle();
			return TRUE;
		}		
		else if (w->xw_func->wd_hndlkey != 0)
			return w->xw_func->wd_hndlkey(w, scancode, keystate);
	}

	return FALSE;
}


/*
 * Iconify a window.
 */

void xw_iconify(WINDOW *w, RECT *r)
{
	/* Set window to iconified state and find new work area */

	xw_set(w, WF_ICONIFY, r); /* wind_set() then wind_get() */ 

	w->xw_xflags |= XWF_ICN;
}


/*
 * Uniconify a window; it will revert to the size and position
 * it had before iconification.
 */

void xw_uniconify(WINDOW *w, RECT *r)
{
	/* Uniconify window and find new work area */

	xw_set(w, WF_UNICONIFY, r);

	w->xw_xflags &= ~XWF_ICN;
}


/*
 * Funktie voor het afhandelen van een message event.
 *
 * Parameters:
 *
 * message	- ontvangen message
 */

int xw_hndlmessage(int *message)
{
	WD_FUNC *func, *wwfunc;
	WINDOW *w, *ww;
	int *m4 = &message[4];

	/* Process only messages for an existing window */

	if ((w = xw_hfind(message[3])) == NULL)
		return FALSE;

	/* If a windowed dialog is opened, override the topping function */

	ww = w;

	if(xd_dialogs && !( xd_nmdialogs && w == xd_nmdialogs->window ))
		ww = xd_dialogs->window;

	/* 
	 * Which functions apply to this window ? 
	 * Take care that all possible functions are defined for
	 * all window types. Use dummy functions from xwindow.c
	 * when no action is required.
	 */

	func = w->xw_func;
	wwfunc = ww->xw_func;

	/* Do what is needed */

	switch (message[0])
	{
		case WM_REDRAW:
		{
			xw_redraw_menu(w, w->xw_bar, (RECT *)m4);
			func->wd_redraw(w, (RECT *)m4);
			break;
		}
		case WM_CLOSED:
		{
			if(xd_dialogs)
			{
				if(w != xd_dialogs->window)
				{
					bell();
					xw_set(xd_dialogs->window, WF_TOP);
				}
				else
					return FALSE; /* closer in dialog window handled differently */
			}	
			func->wd_closed(w, 0);
			break;
		}
		case WM_FULLED:
		{
			func->wd_fulled(w, xe_mbshift);
			break;
		}
		case WM_ARROWED:
		{
			/* a wheeled mouse can send this even if there are no arrow widgets */
			func->wd_arrowed(w, *m4);
			break;
		}
		case WM_HSLID:
		{
			func->wd_hslider(w, *m4);
			break;
		}
		case WM_VSLID:
		{
			func->wd_vslider(w, *m4);
			break;
		}
		case WM_SIZED:
		{
			func->wd_sized(w, (RECT *)m4);
			break;
		}
		case WM_MOVED:
		{
			func->wd_moved(w, (RECT *)m4);
			break;
		}
		case WM_TOPPED:
		{
			wwfunc->wd_topped(ww);
			break;
		}
		case WM_NEWTOP:
		{
			wwfunc->wd_newtop(ww);
			break;
		}
		case WM_ONTOP:
		case WM_UNTOPPED:
		{
			xw_top(); /* just find the new top window ? */
			break;
		}
		case WM_BOTTOMED:
		{
			func->wd_bottomed(w);
			break;
		}
		case WM_ICONIFY:
		{
			func->wd_iconify(w, (RECT *)m4);
			break;
		}
		case WM_UNICONIFY:
		{
			func->wd_uniconify(w, (RECT *)m4);
			break;
		}
		default :
		{
			return FALSE;
		}
	}

	return TRUE;
}


/*
 * Vervanger van wind_set(). De funktie corrigeert de grootte van
 * het werkgebied voor een eventueel aanwezige menubalk. Voor
 * WF_CURRXYWH moet als parameter een pointer naar RECT structuur
 * worden opgegeven, in plaats van vier integers.
 */

void xw_set(WINDOW *w, int field,...)
{
	int p1, p2, p3, p4;
	RECT *r;
	va_list p;

	va_start(p, field);

	switch (field)
	{
		case WF_ICONIFY: 
		case WF_UNICONIFY:
		case WF_CURRXYWH:
		{
			/* call wind_get() here because rectangle can be -1, -1, -1, -1 */
			r = va_arg(p, RECT *);
			w->xw_size = *r;
			wind_set(w->xw_handle, field, w->xw_size.x,
					 w->xw_size.y, w->xw_size.w, w->xw_size.h);
			wind_get(w->xw_handle, WF_CURRXYWH, &w->xw_size.x,
					 &w->xw_size.y, &w->xw_size.w, &w->xw_size.h);
			wind_get(w->xw_handle, WF_WORKXYWH, &w->xw_work.x,
					 &w->xw_work.y, &w->xw_work.w, &w->xw_work.h);
			xw_set_barpos(w); /* irelevant but harmless in an iconified window */
			break;
		}
		case WF_TOP:
		case WF_BOTTOM:
		{
			xw_set_topbot(w, field);
			break;
		}
		default:
		{
			p1 = va_arg(p, int);
			p2 = va_arg(p, int);
			p3 = va_arg(p, int);
			p4 = va_arg(p, int);
			wind_set(w->xw_handle, field, p1, p2, p3, p4);
		}
	}

	va_end(p);
}


/*
 * Set window size (short call)
 * This routine also makes a wind_get() to get new size 
 * of the work area
 */

void xw_setsize(WINDOW *w, RECT *size)
{
	xw_set(w, WF_CURRXYWH, size);
}
 

/*
 * Get window size (short)
 */

void xw_getsize(WINDOW *w, RECT *size)
{
	xw_get(w, WF_CURRXYWH, size);
}


/*
 * Vervanger van wind_get(). Voor het opvragen van informatie over
 * de desktop achtergrond kan een NULL pointer opgegeven worden
 * voor 'w'. De funktie corrigeert de grootte van het werkgebied
 * voor een eventueel aanwezige menubalk. Voor WF_FIRSTXYWH,
 * WF_NEXTXYWH, WF_FULLXYWH, WF_PREVXYWH, WF_WORKXYWH en WF_CURRXYWH
 * moet als parameter een pointer naar RECT structuur worden
 * opgegeven, in plaats van vier pointers naar integers.
 * Beware that, if window handle is not 0, this function for some cases 
 * just returns data from the buffer and does not make any inquiry to the AES.
 * Only WF_* codes up to 32 are supported (see also xw_get_argtab[])
 */

void xw_get(WINDOW *w, int field,...)
{
	RECT *r;
	int *parm[4], dummy, i, parms, handle = 0;
	va_list p;

	va_start(p, field);

	if(w)
		handle = w->xw_handle;

	switch (field)
	{
		case WF_WORKXYWH:
		{
			if (handle != 0)
			{
				r = va_arg(p, RECT *);
				*r = w->xw_work;
	
				if (w->xw_menu != NULL)
				{
					int height = w->xw_menu[w->xw_bar].ob_height + 1;
	
					r->y += height;
					r->h -= height;
				}
				break;
			}
		}
		case WF_CURRXYWH:
		{
			if (handle != 0)
			{
				r = va_arg(p, RECT *);
				*r = w->xw_size;
				break;
			}
		}
		case WF_FIRSTXYWH:
		case WF_NEXTXYWH:
		case WF_FULLXYWH:
		case WF_PREVXYWH:
		case WF_ICONIFY:
		{
			r = va_arg(p, RECT *);
			wind_get(handle, field, &r->x, &r->y, &r->w, &r->h);
			break;
		}
		default:
		{
			parms = wind_get_nargs(field);
	
			for (i = 0; i < parms; i++)
				parm[i] = va_arg(p, int *);
			for (;i < 4;i++)
				parm[i] = &dummy;
	
			wind_get(handle, field, parm[0], parm[1], parm[2], parm[3]);
		}
	}
	va_end(p);
}


/*
 * Shorter form of xw_get to obtain WF_WORKXYWH
 * and save some bytes at an expense in speed.
 * This is the most often form of xw_get in TeraDesk.
 * Beware that, if window handle is not 0, this routine does not
 * make any inquiry from the AES but just copies the data from
 * the window structure (height may be modified by the window menu height).
 */

void xw_getwork(WINDOW *w, RECT *size)
{
	xw_get(w, WF_WORKXYWH, size);
}


/*
 * Similar to above, but obtain WF_FIRSTXYWH; this routine -does-
 * always make an inquiry to the AES.
 */

void xw_getfirst(WINDOW *w, RECT *size)
{
	xw_get(w, WF_FIRSTXYWH, size);
}


/*
 * Similar to above, but obtain WF_NEXTXYWH
 * As this would be used only in loops, benefit may  be doubtful,
 * because of the speed penalty. Use with caution.
 */

void xw_getnext(WINDOW *w, RECT *size)
{
	xw_get(w, WF_NEXTXYWH, size);
}


/*
 * Vervanger van wind_calc(), die rekening houdt met een eventuele
 * menubalk.
 * Argument w_ctype can be eithe WC_WORK or WC_BORDER; in the first case
 * work area height is calculated from overall height; in the second case
 * it is the opposite.
 */
 
void xw_calc(int w_ctype, int w_flags, RECT *input, RECT *output, OBJECT *menu)
{
	int height;

	wind_calc(w_ctype, w_flags, input->x, input->y, input->w, input->h,
			  &output->x, &output->y, &output->w, &output->h);

	if (menu)
	{
		height = menu[menu->ob_head].ob_height + 1;

		if(w_ctype != WC_WORK)
			height = -height;

		output->y += height;
		output->h -= height;
	}
}


/*
 * Funktie voor het bepalen van het aantal OBJECT structuren in een
 * object boom.
 */

static int xw_tree_size(OBJECT *menu)
{
	int i = 0;

	if (menu)
	{
		while ((menu[i].ob_flags & LASTOB) == 0)
			i++;

		i++;
	}

	return i;
}


/*
 * Funktie voor het alloceren van geheugen voor een nieuw window.
 * Tevens wordt een kopie gemaakt van de menubalk.
 * Note: all unspecified structure elements are set to zero
 */

static WINDOW *xw_add(size_t size, OBJECT *menu)
{
	WINDOW *w;
	size_t msize;

	msize = (size_t)xw_tree_size(menu) * sizeof(OBJECT);

	if ((w = (*xd_malloc)(size + msize)) == NULL)
		return NULL;

	memclr(w, size);

	if (menu)
	{
		/* Menu follows immediately after window memory area */

		w->xw_menu = (OBJECT *) &(((char*)w)[size]);
		memcpy(w->xw_menu, menu, msize);
		xw_find_objects(menu, &w->xw_bar, &w->xw_mparent);
	}

	return w;
}


/*
 * Funktie voor het creeren van een window.
 *
 * Parameters:
 *
 * type				- type van het window,
 * functions		- pointer naar een structuur met pointers naar
 *					  event handlers van het window,
 * flags			- window flags van het window;
 *					  for an ACC_WIND pass an externally obtained handle here;
 * msize			- maximum grootte van het window,
 * wd_struct_size	- grootte van de window structuur,
 * menu				- pointer naar de object boom van de menubalk
 *					  van het window of NULL.
 */

WINDOW *xw_create(int type, WD_FUNC *functions, int flags,
				  RECT *msize, size_t wd_struct_size, OBJECT *menu, int *error)
{
	WINDOW *w;

	/* Allocate memory for the window structure and zero it */

	if ((w = xw_add(wd_struct_size, menu)) == NULL)
	{
		*error = XDNSMEM;
		return NULL;
	}

	if ( type != ACC_WIND )
	{
		if ((w->xw_handle = wind_create(flags, msize->x, msize->y,
									msize->w, msize->h)) < 0)
		{
			(*xd_free)(w); /* release memory */
			*error = XDNMWINDOWS;
			return NULL;
		}
	}
	else
		w->xw_handle = flags; /* ONLY for acc window */

	w->xw_type = type;
	w->xw_flags = flags;
	w->xw_func = functions;

	/* Update pointers to previous and next windows */

	if (windows)
		windows->xw_prev = w;

	w->xw_next = windows;
	windows = w;
	*error = 0;

	return w;
}


/*
 * Funktie voor het openen van een window.
 * A new window will be added as the first one in the list, and topped.
 *
 * Parameters:
 *
 * w	- te openen window (moet eerst gecreerd worden met xw_create),
 * size	- initiele grootte van het window.
 */

void xw_open(WINDOW *w, RECT *size)
{
	wind_open(w->xw_handle, size->x, size->y, size->w, size->h);

	w->xw_size = *size;

	wind_get(w->xw_handle, WF_WORKXYWH, &w->xw_work.x,
			 &w->xw_work.y, &w->xw_work.w, &w->xw_work.h);

	xw_set_barpos(w);

	w->xw_xflags |= XWF_OPN;

	if (w->xw_func->wd_top != 0L)
		w->xw_func->wd_top(w);
}


/*
 * Funktie voor het verwijderen van een window uit de window lijst
 * en het vrijgeven van gealloceerd geheugen.
 */

static void xw_rem(WINDOW *w)
{
	/* Take care of the next and the previous */

	if (xw_prev(w) == NULL)
		/* This was the first window; now, the next one is the first */
		windows = xw_next(w);
	else
		/* This is not the first one; mark the next window as the next from the previous */
		w->xw_prev->xw_next = xw_next(w);

	if (xw_next(w) != NULL)
		/* Mark the previous window in the next one */
		w->xw_next->xw_prev = xw_prev(w);

	/* Release memory for this window */

	(*xd_free)(w);
}


/*
 * Close a window. If it is an accessory window, opened
 * thorugh AV_PROTOCOL, send that application a message to
 * close the window
 *
 * Parameters:
 *
 * window	- te sluiten window.
 */

void xw_close(WINDOW *w)
{
	WINDOW *tw;

	if (xw_exist(w))
	{	
		if ( w->xw_type == ACC_WIND )
			xw_send(w, WM_CLOSED);
		else
			wind_close(w->xw_handle);

		w->xw_xflags &= ~(XWF_ICN | XWF_OPN); /* not iconified or open anymore */	

		if ( (tw = xw_top()) != NULL)
		{
			if (tw->xw_func->wd_top != 0L)
				tw->xw_func->wd_top(tw);
		}
		else if ((xw_deskwin != NULL) && (xw_deskwin->xw_func->wd_top != 0L))
			xw_deskwin->xw_func->wd_top(tw);
	}
}


/*
 * Funktie voor het deleten van een window.
 *
 * Parameters:
 *
 * window	- te deleten window.
 */

void xw_delete(WINDOW *w)
{
	if (xw_exist(w))
	{
		if ( w->xw_type != ACC_WIND )
			wind_delete(w->xw_handle);
		xw_rem(w);
	}
}


/* currently not used in TeraDesk

/*
 * Close and delete a window
 */

void xw_closedelete(WINDOW *w)
{
	xw_close(w);
	xw_delete(w);
}

*/


/*
 * Function for opening the desktop root window.
 *
 * type				- type van het window,
 * functions		- pointer naar een structuur met pointers naar
 *					  event handlers van het window,
 * wd_struct_size	- grootte van de window structuur,
 */

WINDOW *xw_open_desk(int type, WD_FUNC *functions,
					 size_t wd_struct_size, int *error)
{
	WINDOW *w;

	if ((w = (*xd_malloc)(wd_struct_size)) == NULL)
	{
		*error = XDNSMEM;
		return NULL;
	}

	/* All items not explicitely specified below are zeroed */

	memclr(w, wd_struct_size);

	w->xw_type = type;
	w->xw_xflags |= XWF_OPN;
	w->xw_func = functions;

	xw_deskwin = w; /* globally available */

	*error = 0;

	xw_getwork(NULL, &w->xw_size);
	w->xw_work = w->xw_size;

	return w;
}


/*
 * Function for closing the desktop window.
 * Note that xd_free must permit free(NULL)
 */

void xw_close_desk(void)
{
	(*xd_free)(xw_deskwin);
	xw_deskwin = NULL;
}


/*
 * Function for closing (and deleting) all windows, incl. desktop
 */

void xw_closeall(void)
{
	while(windows)
	{
		if ((windows->xw_xflags & XWF_OPN) != 0)
			xw_close(windows);

		xw_delete(windows);
	}

	xw_close_desk();
}

