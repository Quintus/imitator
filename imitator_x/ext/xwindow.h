/*********************************************************************************
Imitator for X is a library allowing you to fake input to systems using X11. 
Copyright © 2010 Marvin Gülker

This file is part of Imitator for X.

Imitator for X is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Imitator for X is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with Imitator for X.  If not, see <http://www.gnu.org/licenses/>.
*********************************************************************************/
#ifndef IMITATOR_XWINDOW_HEADER
#define IMITATOR_XWINDOW_HEADER

/*Gets the window ID of self and converts it to a Window. A Window is just a long. */
#define GET_WINDOW (Window) NUM2LONG(rb_ivar_get(self, rb_intern("@window_id")))

/*Converts a string returned by a X function (cp), a char *, to a ruby string 
*(encoded in ISO-88591; I couldn't find out how to get the locale encoding out of X, 
*so you may have to change this if ISO-8859-1 isn't your X Server's locale) and then 
*to an UTF-8-encoded string. */
#define XSTR_TO_RSTR(cp) rb_str_export_to_enc(rb_enc_str_new(cp, strlen(cp), rb_enc_find("ISO-8859-1")), rb_utf8_encoding())

/*Imitator::X::XWindow*/
VALUE XWindow;

/*XWindow initialization function*/
void Init_xwindow(void);

#endif