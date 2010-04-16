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
#include "x.h"
#include "xwindow.h"
#include "mouse.h"
#include "keyboard.h"
#include "clipboard.h"

VALUE Imitator;
VALUE X;
VALUE ProtocolError;
VALUE XError;

/*
*Document-module: Imitator
*Toplevel namespace of the Imitator libraries. 
*/

/*
*Document-module: Imitator::X
*The namespace of this library. 
*/

/*
*Document-class: Imitator::X::XProtocolError
*A Protocol Error as thrown by X. This class uses X's error messages. 
*/

/*
*Document-class: Imitator::X::XError
*Miscellaneous error messages caused by unexpected behaviour of X. 
*/

/***********************Helper functions***************************/
/*
*This function handles X server protocol errors. That allows us to throw Ruby 
*exceptions instead of having X terminate the program and print it's 
*own error message. 
*/
int handle_x_errors(Display *p_display, XErrorEvent *x_errevt)
{
  char msg[1000];
  
  XGetErrorText(p_display, x_errevt->error_code, msg, 1000);
  XCloseDisplay(p_display); /*Ensure the X Server connection is closed*/
  rb_raise(ProtocolError, msg); /*This is OK, I get the error message from X*/
  return 1;
}

/************************Init-Function****************************/

void Init_x(void)
{
  Imitator = rb_define_module("Imitator");
  X = rb_define_module_under(Imitator, "X");
  ProtocolError = rb_define_class_under(X, "XProtocolError", rb_eStandardError);
  XError = rb_define_class_under(X, "XError", rb_eStandardError);
  
  /*The version of this library. */
  rb_define_const(X, "VERSION", rb_str_new2("0.0.1"));
  
  /*Load the parts of Imitator for X*/
  Init_xwindow();
  Init_mouse();
  Init_keyboard();
  Init_clipboard();
}