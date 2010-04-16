/*********************************************************************************
Imitator for X is a library allowing you to fake input to systems using X11. 
Copyright � 2010 Marvin G�lker

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
#ifndef IMITATOR_KEYBOARD_HEADER
#define IMITATOR_KEYBOARD_HEADER

#define RUBY_UTF8_STR(str) rb_enc_str_new(str, strlen(str), rb_utf8_encoding())

VALUE Keyboard;

void Init_keyboard(void);

#endif