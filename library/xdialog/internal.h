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

#ifndef __XD_INTERNAL_H__
#define __XD_INTERNAL_H__

#ifndef __PUREC__
#define cdecl
#endif

/* Vlaggen voor xd_redraw() */

#define XD_RDIALOG	0x1
#define XD_RCURSOR	0x2

/* 
 * Maximum size of scrolled editable text should be a little less
 * than the size of VLNAME
 */

#define XD_MAX_SCRLED sizeof(VLNAME) - 1

/* Vlaggen voor xd_form_button() */

#define FMD_FORWARD		0
#define FMD_BACKWARD	1
#define FMD_DEFLT		2

#define MAXKEYS		48

/* Macro's for 3D-buttons. */

#define AES3D_1			0x200
#define AES3D_2			0x400

#define GET_3D(flags)	(flags & (AES3D_1 | AES3D_2))
#define IS_ACT(flags)	(GET_3D(flags) == (AES3D_1 | AES3D_2))
#define IS_IND(flags)	(GET_3D(flags) == AES3D_1)
#define IS_BG(flags)	(GET_3D(flags) == AES3D_2)

/* Defines for internal fill style. */

#ifndef __PUREC__
#define FIS_HOLLOW		0
#define FIS_SOLID		1
#define FIS_PATTERN		2
#define FIS_HATCH		3
#define FIS_USER		4
#endif

/* Macro to check if an extended user-defined structure is used. */

#define IS_XUSER(userblk)	(userblk->ub_parm == (long)userblk)

/* Extended user-defined structure. */

typedef struct xuserblk
{
	int cdecl (*ub_code)(PARMBLK *parmblock);
	struct xuserblk *ub_parm;		/* Pointer to itself */
	int ob_type;					/* Original object type */
	int ob_flags;					/* Original object flags */
	OBSPEC ob_spec;					/* Original object specifier */
	union
	{ 
		long ptr;					/* a handy pointer to anything */
		int ob_shift;				/* For scrledit: position of letterbox. */
		struct
		{
			int pattern;			/* background fill pattern */
			int colour;				/* background fill colour */
		} fill;
	}uv;
} XUSERBLK;

typedef struct xdobjdata
{
	struct xdobjdata *next;
} XDOBJDATA;

typedef struct
{
	int key;
	int object;
} KINFO;

typedef struct
{
	XW_INTVARS;
	XDINFO *xd_info;
	int nkeys;
	KINFO kinfo[MAXKEYS];
} XD_NMWINDOW;

typedef struct
{
	int id;

/* currently unused
	int type;
*/
	int size;
	int colour;
	int effects;
	int cw;
	int ch;
} XDFONT;

extern int
	xd_vhandle,
	xd_nplanes,
	xd_ncolours,
	xd_npatterns,
	xd_nfills,
	xd_fnt_w,
	xd_fnt_h,
	xd_pix_height,
	xd_posmode,
	xd_min_timer,
	aes_flags,
	xd_fdo_flag;

extern const int
	ckeytab[];

extern const char *xd_prgname;
extern void *(*xd_malloc) (size_t size);
extern void (*xd_free) (void *block);
extern XDOBJDATA *xd_objdata;
extern XDINFO *xd_dialogs;		/* Lijst met modale dialoogboxen. */
extern XDINFO *xd_nmdialogs;	/* Lijst met niet modale dialoogboxen. */
extern OBJECT *xd_menu;
extern RECT xd_screen, xd_desk;
extern XDFONT xd_regular_font, xd_small_font;

extern void set_linedef(int colour);
extern int xd_movebutton(OBJECT *tree);
extern int xd_abs_curx(OBJECT *tree, int object, int curx);
extern void xd_cursor_on(XDINFO *info);
extern void xd_cursor_off(XDINFO *info);
extern int xd_hndlmessage(int *message, int flag);
extern int xd_scan_messages(int flag, int *mes);
extern void xd_redraw(XDINFO *info, int start, int depth, RECT *area, int flags);
extern XDINFO *xd_find_dialog(WINDOW *w);
extern int xd_form_button(XDINFO *info, int object, int clicks, int *result);
extern int xd_find_obj(OBJECT *tree, int start, int which);
extern void xd_edit_init(XDINFO *info, int object, int curx);
extern void xd_edit_end(XDINFO *info);
extern void xd_calcpos(XDINFO *info, XDINFO *prev, int pmode);
extern int xd_edit_char(XDINFO *info, int key);
extern int xd_form_keybd(XDINFO *info, int kobnext, int kchar, int *knxtobject, int *knxtchar);
extern int xd_find_key(OBJECT *tree, KINFO *kinfo, int nk, int key);
extern int xd_set_keys(OBJECT *tree, KINFO *kinfo);
extern void xw_closeall(void);
extern void xd_xuserdef(OBJECT *object, XUSERBLK *userblk, int cdecl(*code)(PARMBLK *parmblock));

#endif
