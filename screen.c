/*
 * Teradesk. Copyright (c) 1993 - 2002  W. Klaren,
 *                         2002 - 2003  H. Robbers,
 *                         2003 - 2007  Dj. Vukovic
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
#include <stdlib.h>
#include <vdi.h>
#include <xdialog.h>
#include <mint.h>
#include <string.h>			/* for load_colours */
#include <library.h>
#include <xdialog.h>

#include "resource.h"
#include "desk.h"
#include "error.h"
#include "xfilesys.h"
#include "config.h"
#include "font.h"
#include "screen.h"
#include "window.h"


int palsize = 0;	/* new (as read from the file) palette size */
int *palette = 0; 	/* Pointer to current palette */



/*
 * Determine screen size by making an inquiry to the VDI
 */

void screen_size(void)
{
	int work_out[58];	

	vq_extnd(vdi_handle, 0, work_out);

	max_w = work_out[0] + 1;	/* Screen width (pixels)  */
	max_h = work_out[1] + 1;	/* Screen height (pixels) */
}


/* 
 * Clip een rechthoek op de grenzen van de desktop.
 * Als de rechthoek geheel buiten de desktop ligt, wordt
 * als resultaat FALSE teruggegeven. 
 */

boolean clip_desk(RECT *r)
{
	return xd_rcintersect(r, &screen_info.dsk, r);
}


void clipdesk_on(void)
{
	int clip_rect[4];

	xd_rect2pxy(&screen_info.dsk, clip_rect);
	vs_clip(vdi_handle, 1, clip_rect);
}


/*
 * Clear a rectangle by filling it with pattern and colour. Use v_bar(). 
 * (pattern and colour are those defined for window background)
 */

void pclear(RECT *r)
{
	boolean doo = options.win_pattern && options.win_colour;

	if(doo)
		clr_object(r, options.win_colour, options.win_pattern);
	else
		clr_object(r, 0, -1);

}


/*
 * Find if two rectangles intersect. Return TRUE if they do.
 */

boolean rc_intersect2(RECT *r1, RECT *r2)
{
	RECT r;

	return xd_rcintersect(r1, r2, &r);
}


/*
 * Determine whether a location is within a rectangle
 */

boolean inrect(int x, int y, RECT *r)
{
	if 
	(   
		x >= r->x
	    && x < (r->x + r->w)
	    && y >= r->y
	    && y < (r->y + r->h)
	)
		return TRUE;

	return FALSE;
}


/*
 * Set default attributes for drawing rectangles
 */

void set_rect_default(void)
{
	vswr_mode(vdi_handle, MD_XOR);
	vsl_color(vdi_handle, 1);
	vsl_ends(vdi_handle, 0, 0);
	vsl_type(vdi_handle, 7);
	vsl_udsty(vdi_handle, 0xCCCC);
	vsl_width(vdi_handle, 1);
}


/*
 * Draw a simple rectangle, defined by its diagonal points
 */

void draw_rect(int x1, int y1, int x2, int y2)
{
	xd_mouse_off();
	xd_clip_on(&screen_info.dsk);
	draw_xdrect(x1, y1, x2 - x1, y2 - y1);
	xd_clip_off();
	xd_mouse_on();
}


/*
 * Invert colours in a rectangle on the screen 
 */

void invert(RECT *r)
{
	MFDB mfdb;
	int pxy[8];

	xd_rect2pxy(r, pxy);
	xd_rect2pxy(r, &pxy[4]);
	mfdb.fd_addr = NULL;
	vro_cpyfm(vdi_handle, D_INVERT, pxy, &mfdb, &mfdb);
}


/* 
 * Move a part of the screen to a new location.
 */

void move_screen(RECT *dest, RECT *src)
{
	MFDB mfdb;
	int pxy[8];

	mfdb.fd_addr = NULL;
	xd_rect2pxy(src, pxy);
	xd_rect2pxy(dest, &pxy[4]);
	vro_cpyfm(vdi_handle, S_ONLY, pxy, &mfdb, &mfdb);
}


/* 
 * Set default text attributes from font data 
 * for subsequent writings to the screen.
 */

void set_txt_default(XDFONT *f)
{
	int dummy;

	xd_vswr_repl_mode();
	vst_font(vdi_handle, f->id);
	vst_rotation(vdi_handle, 0);
	vst_alignment(vdi_handle, 0, 5, &dummy, &dummy);
	xd_vst_point(f->size, &dummy);
	vst_color(vdi_handle, f->colour); 
	vst_effects(vdi_handle, f->effects);
}


/*
 * Allocate space for a palette table, fill it and return a pointer to it.
 * Table format: 3 * int16 per colour (red, green, blue in promilles).
 * Return NULL if allocation is not successful.
 */

int *get_colours(void)
{
	int i, *colours, *h;

	palsize = xd_ncolours;

	if ((colours = malloc((long)xd_ncolours * 3L * sizeof(int))) != NULL)
	{
		h = colours;

		for (i = 0; i < xd_ncolours; i++)
		{
			vq_color(vdi_handle, i, 0, h);
			h += 3;
		}
	}

	return colours;
}


/*
 * Set colour palette from a palette table
 */

void set_colours(int *colours)
{
	int i, *h = colours;

	for (i = 0; i < palsize; i++)
	{
		vs_color(vdi_handle, i, h);
		h += 3;
	}
}


#if PALETTES

static const char *palide = "TeraDesk-pal";
extern char *palname;

typedef struct rgb
{
	int ind;
	int red;
	int green;
	int blue;
}RGB;

RGB cwork;

CfgNest rgb_config, pal_config;


static CfgEntry palette_root[] =
{
	{CFG_NEST, "palette", pal_config },
	{CFG_FINAL},
	{CFG_LAST}
};

static CfgEntry palette_table[] =
{
	{CFG_HDR,  "palette" },
	{CFG_BEG},
	{CFG_D,    "size", &palsize    },
	{CFG_NEST, "col",  rgb_config	 },
	{CFG_ENDG},
	{CFG_LAST}
};


static const CfgEntry colour_table[] =
{
	{CFG_HDR, "col"  },
	{CFG_BEG},
	{CFG_D | CFG_INHIB, "ind", &cwork.ind }, /* index is not essential, but accept it */
	{CFG_DDD, "rgb", &cwork.red },
	{CFG_END},
	{CFG_LAST}
};


/*
 * Load or save configuration for one colour
 */

static CfgNest rgb_config
{
	int 
		i,
		*p,
		nc = min(xd_ncolours, palsize),
		*thecolour = palette;

	if ( io == CFG_SAVE )
	{
		for ( i = 0; i < xd_ncolours; i++ )
		{
			cwork.ind = i;
			cwork.red =  *thecolour++;
			cwork.green = *thecolour++;
			cwork.blue =  *thecolour++; 

			*error = CfgSave(file, colour_table, lvl, CFGEMP);

			if (*error < 0)
				break;
		}

	}
	else
	{
		/* initialize rgb but NOT ind */

		cwork.red = 0;
		cwork.green = 0;
		cwork.blue = 0;

		*error = CfgLoad(file, colour_table, MAX_KEYLEN, lvl);

		if ( (*error == 0) && (cwork.ind < nc) )
		{
			p = &cwork.red;

			thecolour += 3 * cwork.ind; /* need not be in sequence */
			cwork.ind++;

			for ( i = 0; i < 3; i++ )
				*thecolour++ = min(*p++, 1000);
		}
	}
}


/*
 * Load or save palette configuration data
 */

static CfgNest pal_config
{
	palette = get_colours();
	cwork.ind = 0;

	/* 
	 * If space for the palette has been allocated, load data into it 
	 * (or save data from it, depending on io)
	 */

	if ( palette )
	{
		*error = handle_cfg(file, palette_table, lvl, CFGEMP, io, NULL, NULL );

		if ( io == CFG_LOAD )
		{
			if (palsize != xd_ncolours)
				alert_iprint(MECOLORS); /* warning */

			if (*error == 0)
				set_colours(palette);
		}

		free(palette);
	}
	else 
		*error = ENSMEM;
}
 

/*
 * Load or save complete colour palette configuration.
 * Palete file is opened, read/written to, and closed.
 * Note: temporary space for the palette is both allocated and freed here
 * Parameter "io" can be CFG_LOAD or CFG_SAVE.
 * Errors from handle_cfgfile() are ignored.
 */

void handle_colours(int io)
{
	if (options.vprefs & SAVE_COLOURS)	/* separate file "teradesk.pal" */
		handle_cfgfile( palname, palette_root, palide, io );
}


#endif		/* PALETTES */


