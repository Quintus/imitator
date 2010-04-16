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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <X11/extensions/XTest.h>
#include "ruby.h"
#include "ruby/encoding.h"

#ifndef IMITATOR_X_HEADER
#define IMITATOR_X_HEADER

/*Maps XProtocolErrors to Ruby errors*/
int handle_x_errors(Display *p_display, XErrorEvent *x_errevt);
/*Main initialization function*/
void Init_x(void);
/*Imitator module*/
VALUE Imitator;
/*Imitator::X*/
VALUE X;
/*Imitator::X::XProtocolError*/
VALUE ProtocolError;
/*Imitator::X::XError*/
VALUE XError;

#endif