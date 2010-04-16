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
#ifndef IMITATOR_CLIPBOARD_HEADER
#define IMITATOR_CLIPBOARD_HEADER

/*All the macros defined here assume that you have an open X server connection stored 
*in a variable p_display. */

/*UTF-8 X-Encoding atom*/
#define UTF8_ATOM XInternAtom(p_display, "UTF8_STRING", True)

/*Atom for the CLIPBOARD selection*/
#define CLIPBOARD_ATOM XInternAtom(p_display, "CLIPBOARD", True)

/*CLIPBOARD_MANAGER atom*/
#define CLIPBOARD_MANAGER_ATOM XInternAtom(p_display, "CLIPBOARD_MANAGER", True)

/*ATOM_PAIR atom*/
#define ATOM_PAIR_ATOM XInternAtom(p_display, "ATOM_PAIR", True)

/*SAVE_TARGETS atom*/
#define SAVE_TARGETS_ATOM XInternAtom(p_display, "SAVE_TARGETS", True)

/*TARGET_SIZES atom*/
#define TARGET_SIZES_ATOM XInternAtom(p_display, "TARGET_SIZES", True)

/*TARGETS atom*/
#define TARGETS_ATOM XInternAtom(p_display, "TARGETS", True)

/*MULTIPLE request atom*/
#define MULTIPLE_ATOM XInternAtom(p_display, "MULTIPLE", True)

/*Atom for storing properties used by this library*/
#define IMITATOR_X_CLIP_ATOM XInternAtom(p_display, "IMITATOR_X_CLIP", False)

/*In order to work with the X selections, we need a window. 
*This macro just creates a simple, unmapped window that is used for 
*the X selection interaction*/
#define CREATE_REQUESTOR_WIN XCreateSimpleWindow(p_display, XDefaultRootWindow(p_display), 0, 0, 1, 1, 0, 0, 0)

VALUE Clipboard;
void Init_clipboard(void);

#endif