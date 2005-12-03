/*
 * Teradesk. Copyright (c) 1993, 1994, 2002  W. Klaren,
 *                               2002, 2003  H. Robbers,
 *                         2003, 2004, 2005  Dj. Vukovic
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


#include <np_aes.h>			/* modern */
#include <stdarg.h>
#include <stddef.h>
#include <vdi.h>
#include <xdialog.h> 

#include "desktop.h"
#include "error.h"
#include "stringf.h"


/* 
 * Obtain a pointer to a free-string in the resource,
 * identified by its index; handy for composing various texts.
 */

char *get_freestring( int stringid )
{
	OBSPEC s;
	rsrc_gaddr(R_STRING, stringid, &s);
	return s.free_string;
}


/* 
 * Retrieve a text string related to some errors, identified by error code.
 * Anything undefined is "TOS error #%d" 
 */

char *get_message(int error)
{
	static char buffer[80], *s;
	int msg;

	switch (error)
	{
	case EFILNF:
		msg = TFILNF;
		break;
	case EPTHNF:
		msg = TPATHNF;
		break;
	case EACCDN:
		msg = TACCDN;
		break;
	case ENSMEM:
		msg = TENSMEM;
		break;
	case EDSKFULL:
		msg = TDSKFULL;
		break;
	case ELOCKED:
		msg = TLOCKED;
		break;
	case EPLFMT:
		msg = TPLFMT;
		break;
	case EFNTL:
		msg = TFNTLNG;
		break;
	case EPTHTL:
		msg = TPTHTLNG;
		break;
	case EREAD:
		msg = TREAD;
		break;
	case EEOF:
		msg = TEOF;
		break;
	case EFRVAL:
		msg = TFRVAL;
		break;
	case ECOMTL:
		msg = TCMDTLNG;
		break;
	case XDNMWINDOWS:
		msg = MTMWIND;
		break;
	case XDVDI:
		msg = MVDIERR;
		break;
	default:
		s = get_freestring(TERROR);
		sprintf(buffer, s, error);
		return buffer;
	}
	s = get_freestring(msg);

	return s;
}



#if DEBUG

/* 
 * Display an alert box while debugging 
 */

int alert_msg(const char *string, ...)
{
	char alert[256];
	va_list argpoint;
	int button;
	sprintf(alert, "[1][ %s ][ Ok ]", string);
	va_start(argpoint, string);
	button = vaprintf(1, alert, argpoint);
	va_end(argpoint);

	return button;
}
 
#endif


/* 
 * Display an alert box identified by "message" (alert-box id.) and  
 * contining some texts; return button index 
 */

int alert_printf(int def, int message,...)
{
	va_list argpoint;
	int button;
	char *string;


	string = get_freestring(message);
	va_start(argpoint, message);
	button = vaprintf(def, string, argpoint);
	va_end(argpoint);

	return button;
}



/*
 * Display a single-button alert box (" ! " icon) with a text 
 * identified only by string-id "message";
 */

void alert_iprint( int message )
{
	alert_printf( 1, AGENALRT, get_freestring( message ) );
}


/*
 * Display a single-button alert box (" Stop " icon) with a text 
 * identified only by string-id "message", This is called
 * for fatal errors which will stop TeraDesk.
 */

void alert_abort( int message )
{
	alert_printf( 1, AFABORT, get_freestring( message ) );
}


/*
 * Display a two-buttons alert box (" ? " icon) with a text 
 * identified only by string id "message", 
 */

int alert_query( int message )
{
	return alert_printf( 1, AQUERY, get_freestring( message ) );
}


/* 
 * Display an alert box (" ! " icon, except in one case), 
 * text being identified only by error code.
 * Error code ENOMSG will not create an alert.
 * Anything undefined is "TOS error %d".
 * This routine ignores error codes >= 0.
 */

void xform_error(int error)
{
	/* Hopefully this optimization will work OK, hard to test it all now */

	if ( error == XDVDI )
		alert_abort( MVDIERR );	/* because of another icon here */
	else
		if (error < 0 && error != ENOMSG)
			alert_printf( 1, AGENALRT, get_message(error) );
}


/* 
 * Display an alert box identified by "message" (alert-box id.)
 * and additional error-message text idenified by "error" code 
 */

void hndl_error(int message, int error)
{
	if (error > EINVFN)
		return;

	alert_printf(1, message, get_message(error));
}


/* 
 * display an alert box (" ! " icon) for some file-related errors;
 * earlier, "msg" identified the alert-box form,
 * now it idenifies the first message text in AGFALERT alert box.
 * The alert box first displayes text "msg", then "file", then
 * the text associated to error code "error".
 */

int xhndl_error(int msg, int error, const char *file)
{
	int 
		button = 0, 
		txtid = 0;


	if ((error >= XFATAL) && (error <= XERROR))
		return error;
	else
	{
		if ((error < 0) && (error >= -17))
			return XFATAL;
		else
		{
			/* Display the appropriate alert-box */

			if ((error == EFILNF) || (error == EACCDN) || (error == ELOCKED))
				/* For: File not found, Access denied, File locked: */
				txtid = TSKIPABT;
			else if ( error != ENOMSG )
				/* For all the rest... */
				txtid = TABORT;

			if (txtid)
				button = alert_printf
				(
					1, AGFALERT, 
					get_freestring(msg), 
					file, 
					get_message(error), 
					get_freestring(txtid)
				);

			return (txtid == TSKIPABT && button == 1) ? XERROR : XABORT;
		}
	}
}



