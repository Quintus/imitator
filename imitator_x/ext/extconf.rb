#!/usr/bin/env ruby
#Encoding: UTF-8
=begin
--
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
++
=end
require "mkmf"

if ARGV.include?("-h") or ARGV.include?("--help")
  help =<<HELP
Creates a makefile for building Imitator for X. 
==Requirements
You need the following libraries in order to compile successfully: 
* X11 (X server)
* Xtst (XTest extension library, for sending input)
==Switches
* --help\t-h\tDisplays this help. 
* --with-X11-dir=DIR\tLook in DIR for the X server libs. 
* --with-Xtst-dir=DIR\tLook in DIR for the XTest lib. 

By default, the /usr/X11/lib, /usr/X11RC6/lib, /usr/openwin/lib and 
/usr/local/lib directories are searched for the X and XTest libraries. 
HELP
  puts help
  exit 1
end

dir_config("X11")
dir_config("XTst")

unless find_library("X11", "XOpenDisplay", "/usr/X11/lib", "/usr/X11RC6/lib", "/usr/openwin/lib", "/usr/local/lib")
  abort("Couldn't find X Server library!")
end
unless find_library("Xtst", "XTestFakeInput", "/usr/X11/lib", "/usr/X11RC6/lib", "/usr/openwin/lib", "/usr/local/lib")
  abort("Couldn't find XTest library!")
end

create_makefile("x")