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
#include "keyboard.h"

/*Always remember: The Window type is just a long containing the window handle.*/
/*Heavy use of the GET_WINDOW macro is made here*/

/*Document-class: Imitator::X::XWindow
*This class allows interaction with the windows of an X server. There are some important terms 
*that have to be made clear: 
*[window] This means a window on the X server. 
*[screen] Each window resides on a screen. A screen is a physical monitor. Numbering starts with 0. 
*[display] A display is a collection of screens. If you have for example 4 monitors connected, you get 4 screens on 1 display. Numbering starts with 0. 
*[display_string] This is a string describing a screen on a display, of form <tt>":display.screen"</tt> (yes, it starts with a colon). 
*[root window] Every display has a root window, which is the parent of all windows visible on that screen (including the desktop window). 
*
*Every method in this class will raise XProtocolErrors if you try to operate on non-existant windows, e.g. trying to 
*retrieve a killed window's position. 
*
*All methods of this class that return strings return them encoded in UTF-8. However, it's assumed that your 
*X Server's locale is ISO-8859-1 (that's Latin-1), since I couldn't figure out how to query X for that information. 
*Change the value of XSTR_TO_RSTR (in xwindow.h) to your X Server's locale, than recompile this library if 
*it's incorrect. If you know how to obtain X's locale, please tell me at sutniuq$gmx:net. A patch would be nice, too. 
*
*The methods covering EWMH standard may be not available on every system. If such a method is called on a 
*system that doesn't support that part of EWMH or doesn't support EWMH at all, a NotImplementedError is raised. 
*The corresponding EWMH standard of a method is mentioned in it's _Remarks_ section. 
*/

/*******************Helper functions**************************/

/*
*This function retrieves the display of the calling 
*window. It's just shorthand for the two lines included in it. 
*/
static Display * get_win_display(VALUE self)
{
  Display * p_display;
  VALUE rstr;
  
  rstr = rb_ivar_get(self, rb_intern("@display_string"));
  p_display =  XOpenDisplay(StringValuePtr(rstr));
  return p_display;
}

/*
*This function checks whather the specified EWMH standard is supported 
*by the system's window manager. If not, it raises a NotImplementedError 
*exception. 
*/
static void check_for_ewmh(Display * p_display, const char * ewmh)
{
  Window root = XRootWindow(p_display, 0);
  Atom atom, actual_type, support_atom;
  int actual_format;
  unsigned long nitems, bytes;
  unsigned char * props;
  Atom * props2;
  int ret, i, result = 0;
  /*We don't use actual_type, actual_format and bytes. */
  
  /*To check wheather any EWMH is supported*/
  atom = XInternAtom(p_display, "_NET_SUPPORTED", False);
  /*To check if the specified EWMH is supported*/
  support_atom = XInternAtom(p_display, ewmh, False);
  
  /*GetWindowProperty(_NET_SUPPORTED) gives us an array of supported EWMH standards 
  *or fails if no EWMH is supported. 
  *Many great thanks to Jordan Sissel whose xdotool code 
  *showed me how this function works. */
  ret = XGetWindowProperty(p_display, root, atom, 0, (~0L), False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes, &props);
  if (ret != Success)
  {
    XSetErrorHandler(NULL); /*Ensure the connection...*/
    XCloseDisplay(p_display); /*...is properly closed. */
    rb_raise(rb_eNotImpError, "EWMH is not supported by this window manager!");
  }
  
  /*Cast props to an Atom array, otherwise we get BadAtom errors. */
  props2 = (Atom *) props;
  for(i = 0L; ((i < nitems) && (result == 0)); i++)
  {
    if(props2[i] == support_atom)
      result = 1; /*Found the atom.*/
  }
  XFree(props);
  
  if (result == 0)
  {
    XSetErrorHandler(NULL);
    XCloseDisplay(p_display);
    rb_raise(rb_eNotImpError, "EWMH '%s' is not supported by this window manager!", ewmh);
  }
}

/*************************Class methods***********************************/

/*
*call-seq: 
*  XWindow.xquery(display_string, window_id) ==> true
*
*<b>Don't call this method, it's used internally. </b>
*
*Raises a XProtocolError if the specified window doesn't exist on the specified +display_string+. 
*/
static VALUE cm_xquery(VALUE self, VALUE display_string, VALUE window_id)
{
  Display * p_display = XOpenDisplay(StringValuePtr(display_string));
  Window win = (Window)NUM2LONG(window_id);
  Window dummy, dummy2, *children = NULL;
  unsigned int child_num;
  
  XSetErrorHandler(handle_x_errors); /*Let Ruby handle the errors*/
  XQueryTree(p_display, win, &dummy, &dummy2, &children, &child_num);
  
  XSetErrorHandler(NULL); /*Let X handle the errors again*/
  
  XFree(children);
  XCloseDisplay(p_display);
  return Qtrue;
}

/*
*call-seq: 
*  XWindow.default_root_window() ==> aXWindow
*
*Returns the default root window of the default screen. 
*===Return value
*The default root window of the default screen as an XWindow object. 
*===Example
*  puts Imitator::X::XWindow.default_root_window.title #=> (null)
*/
static VALUE cm_default_root_window(VALUE self)
{
  Display * p_display = XOpenDisplay(NULL);
  Window root_win;
  VALUE args[1];
  
  root_win = XDefaultRootWindow(p_display);
  args[0] = LONG2NUM(root_win);
  
  XCloseDisplay(p_display);
  return rb_class_new_instance(1, args, XWindow);
}

/*
*call-seq: 
*  XWindow.exists?(window_id, screen = 0, display = 0) ==> true or false
*
*Checks if the given window ID exists on the given display and screen. 
*===Parameters
*[+window_id+] The window ID to check. 
*[+screen+] (0) The screen to check. 
*[+display+] (0) The display to check. 
*===Return value
*true or false. 
*===Example
*  root_win = Imitator::X::XWindow.default_root_window
*  puts Imitator::X::XWindow.exists?(root_win.window_id) #=> true
*  puts Imitator::X::XWindow.exists?(12345) #=> false
*===Remarks
*This method rescues a XProtocolError exception you'll see when running 
*under $DEBUG and get a +false+ result. 
*/
static VALUE cm_exists(int argc, VALUE argv[], VALUE self)
{
  VALUE window_id;
  VALUE screen;
  VALUE display;
  VALUE result;
  char display_string[100];
  char eval_str[1000];
  
  rb_scan_args(argc, argv, "12", &window_id, &screen, &display);
  
  /*Assign the display or default to 0*/
  if (NIL_P(display))
    display = INT2FIX(0);
  /*Assign the screen or default to 0*/
  if (NIL_P(screen))
    screen = INT2FIX(0);
  
  /*Get the display string, form ":display.screen"*/
  sprintf(display_string, ":%i.%i", FIX2INT(display), FIX2INT(screen));
  /*
  *eval is awful. rb_rescue too. I evaluate this string due to rb_rescue's inusability. 
  *This eval is quite safe, because before I put the two parts together in display_string, I convert 
  *screen and display into integers. 
  */
  sprintf(eval_str, "begin; Imitator::X::XWindow.xquery('%s', %i);true;rescue Imitator::X::XProtocolError;false;end", display_string, NUM2INT(window_id));
  result = rb_eval_string(eval_str);
  
  return result;
}

/*
*call-seq: 
*  XWindow.search(str , screen = 0 , display = 0) ==> anArray
*  XWindow.search(regexp , screen = 0 , display = 0 ) ==> anArray
*
*Searches for a special window title. 
*===Parameters
*[+str+] The title to look for. This will only match *exactly*. 
*[+regexp+] The title to look for, as a Regular Expression to match. 
*[+screen+] (0) The screen to look for the window. 
*[+display+] (0) The display to look for the screen. 
*===Return value
*An array containing the window IDs of all windows whose titles matched the string 
*or Regular Expression. This may be empty if nothing matches. 
*===Example
*  #Search for a window whose name is exactly "x-nautilus-desktop"
*  Imitator::X::XWindow.search("x-nautilus-desktop") #=> [33554464]
*  #Search for a window whose name contains the string "imitator". 
*  Imitator::X::XWindow.search(/imitator/) #=> [...]
*  #If a window isn't found, you get an empty array. 
*  Imitator::X::XWindow.search("nonexistant") #=> []
*===Remarks
*This method just searches the first children layer, i.e. the child windows of the root window. 
*You can't find windows in windows with this method. 
*/
static VALUE cm_search(int argc, VALUE argv[], VALUE self) /*title as string or regexp*/
{
  VALUE title, screen, display;
  char display_string[100];
  Display * p_display;
  Window root_win, parent_win, temp_win;
  Window * p_children;
  unsigned int num_children;
  XTextProperty xtext;
  char * p_title;
  int i;
  short is_regexp;
  VALUE result = rb_ary_new();
  
  rb_scan_args(argc, argv, "12", &title, &screen, &display);
  /*Check wheather we're operating on a Regular Expression or a String. Strings mean exact matching later on. */
  if (TYPE(title) == T_REGEXP)
    is_regexp = 1;
  else
    is_regexp = 0;
  /*Assign the display or default to 0*/
  if (NIL_P(display))
    display = INT2FIX(0);
  /*Assign the screen or default to 0*/
  if (NIL_P(screen))
    screen = INT2FIX(0);
  /*Get the display string, form ":display.screen"*/
  sprintf(display_string, ":%i.%i", FIX2INT(display), FIX2INT(screen));
  
  p_display = XOpenDisplay(display_string);
  XSetErrorHandler(handle_x_errors); /*Let Ruby handle the errors*/
  root_win = XDefaultRootWindow(p_display);
  
  XQueryTree(p_display, root_win, &root_win, &parent_win, &p_children, &num_children);
  
  if (p_children != NULL) /*This means there are child windows*/
  {
    for(i = 0; i < num_children; i++)
    {
      temp_win = *(p_children + i);
      XGetWMName(p_display, temp_win, &xtext); /*We want to match against the title later*/
      p_title = (char *) malloc(sizeof(char) * (xtext.nitems + 1)); /*Allocate one char more than neccassary, because we get an "invalid pointer" otherwise*/
      sprintf(p_title, "%s", xtext.value);
      
      if (is_regexp)
      {
        if (!NIL_P(rb_reg_match(title, XSTR_TO_RSTR(p_title)))) /*This performs the regexp match*/
          rb_ary_push(result, LONG2NUM(temp_win)); 
      }
      else /*Not using a regular expression*/
      {
        if (strcmp(StringValuePtr(title), p_title) == 0)
          rb_ary_push(result, LONG2NUM(temp_win));
      }
      XFree(xtext.value);
      free(p_title);
    }
    XFree(p_children);
  }
  
  XSetErrorHandler(NULL); /*Let X handle it's errors again*/
  XCloseDisplay(p_display);
  return result;
}

/*
*call-seq: 
*  XWindow.from_title(str [, screen = 0 [, display = 0 ] ] ) ==> aXWindow
*  XWindow.from_title(regexp [, screen = 0 [, display = 0 ] ] ) ==> aXWindow
*
*Creates a new XWindow object by passing on the given parameters to XWindow.search and 
*using the first found window ID to make the XWindow object. 
*===Parameters
*See the XWindow.search parameter description. 
*===Return value
*The first matching window as a XWindow object. 
*===Raises
*[ArgumentError] No matching window was found. 
*===Example
*  #Get the first found window which name includes the string "imitator". 
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*/
static VALUE cm_from_title(int argc, VALUE argv[], VALUE self)
{
  VALUE args[1];
  
  args[0] = rb_ary_entry(cm_search(argc, argv, self), 0);
  if (NIL_P(args[0]))
    rb_raise(rb_eArgError, "No matching window found!");
  
  return rb_class_new_instance(1, args, XWindow);
}

/*
*call-seq: 
*  XWindow.from_focused( [screen = 0 [, display = 0 ] ] ) ==> aXWindow
*
*Creates a new XWindow from the actually focused window. 
*===Parameters
*[screen] (0) The screen to look for the window. 
*[display] (0) The display to look for the screen. 
*===Return value
*The window having the input focus. 
*===Example
*  xwin = Imitator::X::XWindow.from_focused
*===Remarks
*This method is not reliable, since it's likely to find one of those 
*invisible "InputOnly" windows. Have a look at XWindow.from_active 
*for a more reliable variant. 
*/
static VALUE cm_from_focused(int argc, VALUE argv[], VALUE self)
{
  VALUE screen, display;
  Display * p_display;
  char display_string[100];
  Window win;
  int revert;
  VALUE result;
  VALUE args[1];
  
  rb_scan_args(argc, argv, "02", &screen, &display);
  
  /*Assign the display or default to 0*/
  if (NIL_P(display))
    display = INT2FIX(0);
  /*Assign the screen or default to 0*/
  if (NIL_P(screen))
    screen = INT2FIX(0);
  /*Get the display string, form ":display.screen"*/
  sprintf(display_string, ":%i.%i", FIX2INT(display), FIX2INT(screen));
  
  p_display = XOpenDisplay(display_string);
  XSetErrorHandler(handle_x_errors);
  
  XGetInputFocus(p_display, &win, &revert);
  args[0] = LONG2NUM(win);
  result = rb_class_new_instance(1, args, XWindow);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*call-seq: 
*  XWindow.from_active( [screen = 0 [, display = 0 ] ] ) ==> aXWindow
*
*Creates a new XWindow from the currently active window. 
*===Parameters
*[screen] (0) The screen to look for the window. 
*[display] (0) The display to look for the screen. 
*===Return value
*The found window as a XWindow object. 
*===Raises
*[NotImplementedError] The EWMH standard _NET_ACTIVE_WINDOW isn't supported. 
*[XError] Failed to retrieve the active window from the X server. Did you specify a correct screen and display?
*===Example
*  xwin = Imitator::X::XWindow.from_active
*  #Active window of screen 3
*  xwin = Imitator::X::XWindow.from_active(3)
*  #Active window of screen 0 on display 2
*  xwin = Imitator::X::XWindow.from_active(0, 2)
*===Remarks
*This method is part of the EWMH standard _NET_ACTIVE_WINDOW. 
*
*If you can choose between this method and XWindow.from_focused, choose this one, 
*because it doesn't find invisible windows. 
*/
static VALUE cm_from_active(int argc, VALUE argv[], VALUE self)
{
  Display * p_display;
  VALUE screen, display, result;
  VALUE args[1];
  char display_string[100];
  Atom atom, actual_type;
  Window root, active_win;
  int actual_format;
  unsigned long nitems, bytes;
  unsigned char * prop;
  
  rb_scan_args(argc, argv, "02", &screen, &display);
  /*Assign the display or default to 0*/
  if (NIL_P(display))
    display = INT2FIX(0);
  /*Assign the screen or default to 0*/
  if (NIL_P(screen))
    screen = INT2FIX(0);
  /*Get the display string, form ":display.screen"*/
  sprintf(display_string, ":%i.%i", FIX2INT(display), FIX2INT(screen));
  
  p_display = XOpenDisplay(display_string);
  XSetErrorHandler(handle_x_errors);
  check_for_ewmh(p_display, "_NET_ACTIVE_WINDOW");
  
  atom = XInternAtom(p_display, "_NET_ACTIVE_WINDOW", False);
  root = XDefaultRootWindow(p_display);
  
  XGetWindowProperty(p_display, root, atom, 0, (~0L), False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes, &prop);
  if (nitems > 0)
    active_win = *((Window *) prop); /*We got a Window*/
  else /*Shouldn't be the case*/
    rb_raise(XError, "Couldn't retrieve the active window for some reason!");
  args[0] = LONG2NUM(active_win);
  result = rb_class_new_instance(1, args, XWindow);
  
  XFree(prop);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*call-seq: 
*  XWindow.wait_for_window( str [, screen = 0 [, display = 0 ] ] ) ==> aXWindow
*  XWindow.wait_for_window(regexp [, screen = 0 [, display = 0 ] ] ) ==> aXWindow
*
*Pauses execution until a window matching the given criteria is found. 
*===Parameters
*[+str+] The *exact* title of the window you want to to wait for. 
*[+regexp+] A Regular Expression matching the window title you want to wait for. 
*[+screen+] (0) The screen the window will be mapped to. 
*[+display+] (0) The display the window will be mapped to. 
*===Return value
*The XWindow object of the matching window. 
*===Example
*  #Wait until a window with "gedit" in it's title exists
*  gedit_win = Imitator::X::XWindow.wait_for_window(/gedit/)
*/
static VALUE cm_wait_for_window(int argc, VALUE argv[], VALUE self)
{
  VALUE rstr;
  VALUE rscreen;
  VALUE rdisplay;
  VALUE args[3];
  
  rb_scan_args(argc, argv, "12", &rstr, &rscreen, &rdisplay);
  args[0] = rstr;
  args[1] = rscreen;
  args[2] = rdisplay;
  
  /*Hang around until we see a window matching the cirteria*/
  while (RTEST(rb_funcall(cm_search(3, args, self), rb_intern("empty?"), 0)))
    rb_thread_sleep(1);
  
  /*Wait another second to be sure that the window can be worked with*/
  rb_thread_sleep(1);
  
  return cm_from_title(3, args, self);
}

/*
*call-seq: 
*  XWindow.wait_for_window_termination( str [, screen = 0 [, display = 0 ] ] ) ==> nil
*  XWindow.wait_for_window_termination( regexp [, streen = 0 [, display = 0 ] ] ) ==> nil
*
*Pauses execution flow until *every* window matching the given criteria disappeared. 
*===Parameters
*[+str+] The window's title. This must match *excatly*. 
*[+regexp+] The window's title, as a Regular Expression to match. 
*[+screen+] (0) The screen the window resides on. 
*[+display+] (0) The screen's display. 
*===Return value
*nil. 
*===Example
*  #Wait until all gedit windows are closed
*  Imitator::X::XWindow.wait_for_window_termination(/gedit/)
*  #Wait until Firefox on screen 1 closes
*  Imitator::X::XWindow.wait_for_window_termination(/Mozilla Firefox/, 1)
*===Remarks
*If you want to wait for a specific window's termination and you already have 
*a XWindow object for that one, you can also use the following code which is 
*probably shorter: 
*  #... (Get your XWindow instance somewhere)
*  sleep 0.1 while xwin.exists?
*This causes Ruby to check every tenth of a second to check if the reference 
*hold by +xwin+ is still valid, and if not, the loop exits and program flow continues. 
*/
static VALUE cm_wait_for_window_termination(int argc, VALUE argv[], VALUE self)
{
  VALUE rstr;
  VALUE rscreen;
  VALUE rdisplay;
  VALUE args[3];
  
  rb_scan_args(argc, argv, "12", &rstr, &rscreen, &rdisplay);
  args[0] = rstr;
  args[1] = rscreen;
  args[2] = rdisplay;
  
  /*Hang around until every window matching the criteria has gone*/
  while (!RTEST(rb_funcall(cm_search(3, args, self), rb_intern("empty?"), 0)))
    rb_thread_sleep(1);
  
  /*Wait another second to be sure that the window can't be worked with*/
  rb_thread_sleep(1);
  
  return Qnil;
}
/****************************Instance methods*************************************/

/*
*call-seq: 
*  XWindow.new(window_id, screen = 0, display = 0) ==> aXWindow
*
*Creates a new XWindow object which holds a pseudo reference to a real window. 
*===Parameters
*[+window_id+] The ID of the window to get a reference to. 
*[+screen+] (0) The number of the screen the window is shown on. 
*[+display+] (0) The number of the display that contains the screen the window is mapped to. 
*===Return value
*A brand new XWindow object. 
*===Raises
*[XProtocolError] The window ID wasn't found (on the specified screen and/or display). 
*===Example
*  #Assume 12345 is an existing window on the default screen and display
*  xwin = Imitator::X::XWindow.new(12345)
*  #Or on screen 2 on display 0
*  xwin = Imitator::X::XWindow.new(12345, 2)
*  #Or on screen 3 on display 1 (you almost never need this)
*  xwin = Imitator::X::XWindow.new(12345, 3, 1)
*/
static VALUE m_initialize(VALUE argc, VALUE argv[], VALUE self)
{
  VALUE window_id;
  VALUE screen;
  VALUE display;
  VALUE display_string = rb_class_new_instance(0, 0, rb_cString);
  
  rb_scan_args(argc, argv, "12", &window_id, &screen, &display);
  
  rb_str_concat(display_string, rb_str_new2(":")); /*Ommit hostname, we're only operating locally*/
  /*Assign the display or default to 0*/
  if (NIL_P(display))
    rb_str_concat(display_string, rb_str_new2("0")); 
  else
    rb_str_concat(display_string, display);
  rb_str_concat(display_string, rb_str_new2(".")); /*Screen delimiter*/
  /*Assign the screen or default to 0*/
  if (NIL_P(screen))
    rb_str_concat(display_string, rb_str_new2("0"));
  else
    rb_str_concat(display_string, screen);
  
  rb_ivar_set(self, rb_intern("@window_id"), window_id);
  rb_ivar_set(self, rb_intern("@display_string"), display_string);
  return self;
}

/*
*Human-readable description of form <tt><Imitator::X::XWindow 'window_title' (window_id_as_hex)></tt>. 
*/
static VALUE m_inspect(VALUE self)
{
  char str[1000];
  VALUE rstr;
  VALUE handle;
  
  rstr = rb_funcall(self, rb_intern("title"), 0);
  handle = rb_funcall(rb_ivar_get(self, rb_intern("@window_id")), rb_intern("to_s"), 1, INT2NUM(16));
  sprintf(str, "<Imitator::X::XWindow '%s' (0x%s)>", StringValuePtr(rstr), StringValuePtr(handle));
  rstr = rb_enc_str_new(str, strlen(str), rb_utf8_encoding());
  return rstr;
}

/*
*Returns the window's title. 
*===Return value
*The window's title. 
*===Example
*  #Get the root window's title. This is not very useful, it's always "(null)". 
*  puts Imitator::X::XWindow.default_root_window.title #=> (null)
*/
static VALUE m_title(VALUE self)
{
  Display * p_display;
  Window win;
  XTextProperty xtext;
  char * cp;
  VALUE rstr;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  win = NUM2LONG(rb_ivar_get(self, rb_intern("@window_id")));
  XGetWMName(p_display, win, &xtext);
  cp = malloc(sizeof(char) * xtext.nitems);
  sprintf(cp, "%s", xtext.value);
  rstr = XSTR_TO_RSTR(cp);
  
  XFree(xtext.value);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return rstr;
}

/*
*Returns the window_id. 
*===Return value
*The window ID as an integer. 
*===Example
*  #Get the root window's ID. 
*  puts Imitator::X::XWindow.default_root_window.window_id #=> 316
*/
static VALUE m_window_id(VALUE self)
{
  return rb_ivar_get(self, rb_intern("@window_id"));
}

/*
*Returns the root window of the screen that holds this window. 
*===Return value
*The root window of the screen holding this window, as a XWindow. 
*===Example
*  #Get a window with "imitator" in it's title and...
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  #...get the root window of the screen it's mapped to. 
*  xwin.root_win #=> <Imitator::X::XWindow '(null)' (0x13c)>
*/
static VALUE m_root_win(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  XWindowAttributes xattr;
  VALUE args[1];
  VALUE rroot_win;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XGetWindowAttributes(p_display, win, &xattr);
  args[0] = LONG2NUM(xattr.root);
  rroot_win = rb_class_new_instance(1, args, XWindow);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return rroot_win;
}

/*
*Returns this window's parent window. 
*===Return value
*This window's parent window as a XWindow object. 
*===Example
*  xwin = Imitator:X::XWindow.from_title(/imitator/)
*  xwin.parent #=> <Imitator::X::XWindow '(null)' (0x13c)>
*/
static VALUE m_parent(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  Window root_win, parent;
  Window * p_children;
  unsigned int nchildren;
  VALUE args[1];
  VALUE result;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XQueryTree(p_display, win, &root_win, &parent, &p_children, &nchildren);
  args[0] = LONG2NUM(parent);
  result = rb_class_new_instance(1, args, XWindow);
  
  XFree(p_children);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*Returns the children of this window. 
*===Return value
*The window IDs of the children of this window as integers. 
*===Example
*  #Get a list of all windows that are searched by XWindow.search
*  Imitator::X::XWindow.default_root_window.children #=> [...]
*===Remarks
*I orginally wanted to return an array of XWindow obejcts, but everytime 
*I tried to #inspect the created array, I got a segfault, memory corruption or 
*malloc failed assertion. When only inspecting parts of the array, everything goes fine. 
*So, don't try to map the list returned by this method into an array and then inspect it. 
*E-mail me at sutniuq$gmx:net if you know a solution... See also the commented-out source 
*in the for loop inside this function. 
*/
static VALUE m_children(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  Window root_win, parent;
  Window * p_children;
  unsigned int num_children;
  int i;
  //VALUE args[1];
  VALUE result = rb_ary_new();
  //VALUE rtemp;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XQueryTree(p_display, win, &root_win, &parent, &p_children, &num_children);
  for(i = 0;i < num_children; i++)
  {
    //printf("%lu\n", *(p_children + i));
    //args[0] = LONG2NUM(*(p_children + i));
    //rtemp = rb_class_new_instance(1, args, XWindow);
    //rb_ary_push(result, rtemp); /*Why causes this a segfault??? AND WHY ONLY WHEN INSPECTING THE WHOLE ARRAY?????*/
    rb_ary_push(result, LONG2NUM(*(p_children + i)));
  }
  
  XFree(p_children);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*Returns true if +self+ is a root window. 
*===Return value
*true or false. 
*===Example
*  root = Imitator::X::XWindow.default_root_window
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  root.root_win? #=> true
*  xwin.root_win? #=> false
*/
static VALUE m_is_root_win(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  Window root_win, parent;
  Window * p_children;
  unsigned int num_children;
  VALUE result;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XQueryTree(p_display, win, &root_win, &parent, &p_children, &num_children);
  
  if (parent == 0)
    result = Qtrue;
  else
    result = Qfalse;
  
  XFree(p_children);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*Returns the window's current position. 
*===Return value
*The position as a two-element array of form <tt>[x, y]</tt>. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  p xwin.position #=> [3, 47]
*/
static VALUE m_position(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  XWindowAttributes xattr;
  VALUE pos = rb_ary_new();
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XGetWindowAttributes(p_display, win, &xattr);
  rb_ary_push(pos, INT2NUM(xattr.x));
  rb_ary_push(pos, INT2NUM(xattr.y));
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return pos;
}

/*
*Return the window's current size. 
*===Return value
*The window's size as a two-element array of form <tt>[width, height]</tt>. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  p xwin.size #=> [801, 551]
*/
static VALUE m_size(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  XWindowAttributes xattr;
  VALUE size = rb_ary_new();
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XGetWindowAttributes(p_display, win, &xattr);
  rb_ary_push(size, INT2NUM(xattr.width));
  rb_ary_push(size, INT2NUM(xattr.height));
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return size;
}

/*
*Determines wheather or not +self+ is visible on the screen. 
*===Return value
*true or false. 
*===Example
*  Imitator::X::XWindow.from_title(/imitator/).visible? #=> true
*===Remarks
*Generally invisible, so-called InputOnly-windows, are treated as 
*invisible and return false. If you want to check the map state of such 
*a window, use #mapped?. 
*
*This method returns also false if an ancestor is unmapped. 
*/
static VALUE m_is_visible(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  XWindowAttributes xattr;
  VALUE result;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XGetWindowAttributes(p_display, win, &xattr);
  if (xattr.class == InputOnly)
    result = Qfalse; /*Input-only windows are always invisible*/
  else
  {
    if (xattr.map_state == IsUnmapped || xattr.map_state == IsUnviewable)
      result = Qfalse; /*Only mapped windows are visible (their ancestors must be mapped, too)*/
    else
      result = Qtrue;
  }
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*Checkes wheather or not +self+ is mapped to the screen. 
*===Return value
*true or false. 
*===Example
*  Imitator::X::XWindow.from_title(/imitator/).mapped? #=> true
*===Remarks
*This method doesn't give reliable information about a window's 
*visibility, because it returns true for InputOnly-windows which 
*are generally invisible. If you don't want this behaviour, use #visible?. 
*
*This method returns also false if an ancestor is unmapped. 
*/
static VALUE m_is_mapped(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  XWindowAttributes xattr;
  VALUE result;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XGetWindowAttributes(p_display, win, &xattr);
  if (xattr.map_state == IsUnmapped || xattr.map_state == IsUnviewable)
    result = Qfalse;
  else
    result = Qtrue;
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*call-seq: 
*  move(x, y) ==> anArray
*
*Moves +self+ to the specified position. 
*===Parameters
*[+x+] The goal X coordinate. 
*[+y+] The goal Y coordinate. 
*===Return value
*The window's new position. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  #Move the window to (100|100)
*  xwin.move(100, 100) #=> [103, 123]
*  xwin.pos #=> [103, 123] #See remarks
*===Remarks
*It's impossible to set a window exactly to that coordinate you want. It seems, 
*that X sets and retrieves a window's position at the upper-left coordinate of a window's client area, 
*that is, beyond the border and the title bar. Therefore, to make an exact position movement, you would have 
*to subtract the width of those elements. Unfortunally, aXWindowAttributes.border_width always returns 0 on my 
*system. :(
*
*Also, this function can't move the window off the screen. If you try to, the window will 
*be moved as near to the screen's edge as possible. 
*/
static VALUE m_move(VALUE self, VALUE rx, VALUE ry)
{
  Display * p_display;
  Window win = GET_WINDOW;
  int x = NUM2INT(rx);
  int y = NUM2INT(ry);
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XMoveWindow(p_display, win, x, y);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return m_position(self);
}

/*
*call-seq: 
*  resize(width, height) ==> anArray
*
*Resizes a window. 
*===Parameters
*[+width+] The desired width, in pixels. 
*[+height+] The desired height, in pixels. 
*===Return value
*The new window size. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  #Resize the window to 400x400px
*  xwin.resize(400, 400) #=> [400, 400]
*  xwin.size #=> [400, 400]
*===Remarks
*Some windows have a minumum width value set. If a windows has, you can't change it's size 
*to a smaller value than that one. Also, it's impossible to make a window larger than the screen. 
*In any of those cases, the window will be resized to the minimum/maximum acceptable value. 
*/
static VALUE m_resize(VALUE self, VALUE rwidth, VALUE rheight)
{
  Display * p_display;
  Window win = GET_WINDOW;
  unsigned int width = NUM2UINT(rwidth);
  unsigned int height = NUM2UINT(rheight);
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XResizeWindow(p_display, win, width, height);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return m_size(self);
}

/*
*Raises a window to the top, but doesn't give it the input focus. 
*===Return value
*nil. 
*===Example
*  wxin = Imitator::X::XWindow.from_title(/imitator/)
*  xwin.raise_win
*===Remarks
*This method isn't named "raise", because that would conflict with the 
*Kernel module's "raise" method. You can define an alias if you want to. 
*
*Instead of combining this method with #focus you may should have a 
*look at #activate. 
*/
static VALUE m_raise_win(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XRaiseWindow(p_display, win);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*Gives +self+ the input focus, but doesn't bring it to the front. 
*===Return value
*nil. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  xwin.focus
*===Remarks
*Instead of combining this method with #raise_win you may should 
*have a look at #activate. 
*/
static VALUE m_focus(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XSetInputFocus(p_display, win, RevertToNone, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*Makes +self+ lose the input focus. The window will continue looking focused. 
*===Return value
*nil. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  #Try to type after this, your input won't get anywhere: 
*  xwin.focus;sleep 2;xwin.unfocus
*/
static VALUE m_unfocus(VALUE self)
{
  Display * p_display;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XSetInputFocus(p_display, None, RevertToNone, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*Maps a window to the screen, i.e. make it visible. Only possible after a previous 
*call to #unmap. 
*===Return value
*nil. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  p xwin.visible? #=> true
*  p xwin.size #=> [804, 550]
*  xwin.unmap
*  p xwin.visible? #=> false
*  p xwin.size #=> [804, 550]
*  xwin.map
*  p xwin.visible? #=> true
*  p xwin.size #=> [804, 550]
*/
static VALUE m_map(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XMapWindow(p_display, win);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*Unmaps a window from the screen, i.e. make it invisible. The window <b>is not</b> deleted. 
*===Return value
*nil. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  xwin.unmap
*===Remarks
*Perhaps it's not a good idea to unmap the root window...
*/
static VALUE m_unmap(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XUnmapWindow(p_display, win);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*Activates a window, that is, bring it to the front and give it the input focus. 
*===Return value
*nil. 
*===Raises
*[NotImplementedError] The EWMH standard _NET_ACTIVE_WINDOW is not supported. 
*===Example
*  Imitator::X::XWindow.from_title(/imitator/).activate
*===Remarks
*This method is part of the EWMH standard _NET_ACTIVE_WINDOW. It's usually more reliable 
*than #raise_win combined with #focus. 
*/
static VALUE m_activate(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  XEvent xevt;
  XWindowAttributes xattr;
  Window root;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  check_for_ewmh(p_display, "_NET_ACTIVE_WINDOW");
  /*We're going to notify the root window*/
  XGetWindowAttributes(p_display, win, &xattr);
  root = xattr.root;
  
  xevt.type = ClientMessage; /*It's a message for a client*/
  xevt.xclient.display = p_display;
  xevt.xclient.window = win;
  xevt.xclient.message_type = XInternAtom(p_display, "_NET_ACTIVE_WINDOW", False); /*Activate request*/
  xevt.xclient.format = 32; /*Using 32-bit messages*/
  xevt.xclient.data.l[0] = 2L;
  xevt.xclient.data.l[1] = CurrentTime;
  
  /*Actually send the event; this has to happen to all child windows of the target window. */
  XSendEvent(p_display, root, False, SubstructureNotifyMask | SubstructureRedirectMask, &xevt);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*Gets a window process' process identification number (PID). 
*===Return value
*The PID of the window process. 
*===Raises
*[XError] Failed to retrieve the window's PID. Either your window manager does not support EWMH at all, _NET_WM_PID in special, or your queried window hasn't set this property (see _Remarks_). 
*===Example
*  puts  Imitator::X::XWindow.from_title(/imitator/).pid #=> 4478
*===Remarks
*This method is part of the EWMH standard _NET_WM_PID. 
*
*This method may fail even if the window manager supports the _NET_WM_PID standard, beacuse 
*the use of _NET_WM_PID isn't enforced. Some programs don't use it, and those will cause this 
*method to fail. The most prominent example is the default root window, which doesn't have the 
*_NET_WM_PID property set (at least on my Ubuntu Karmic). 
*/
static VALUE m_pid(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  Atom actual_type;
  int obtain_prop, actual_format, pid;
  unsigned long nitems;
  unsigned long bytes;
  unsigned char * property;
  VALUE result;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  //check_for_ewmh(p_display, "_NET_WM_PID"); /*For some unknown reason, this always fails*/
  obtain_prop = XInternAtom(p_display, "_NET_WM_PID", True);
  
  XGetWindowProperty(p_display, win, obtain_prop, 0, 1000000, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes, &property);
  if (property == None)
    rb_raise(XError, "Could not get _NET_WM_PID attribute!");
  
  /*OK, got the property. Compute the PID. I don't know why this works, I 
  *just saw the arithmetic somewhere on the Internet. */
  pid = property[1] * 256;
  pid += property[0];
  result = INT2NUM(pid);
  
  XFree(property);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return result;
}

/*
*Requests the X server to destroy +self+. 
*===Return value
*nil. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  xwin.kill
*===Remarks
*This method requests the X server to send a DestroyNotify event to 
*+self+. A window may choose not to react on this (although this is quite 
*seldom). 
*/
static VALUE m_kill(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XDestroyWindow(p_display, win);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*Forces the X server to close the connection to +self+, effectively 
*destroying the window. 
*===Return value
*nil. 
*===Example
*  Imitator::X::XWindow.from_title(/imitator/).kill!
*===Remarks
*From all window closing methods, this should be the most reliable one. 
*If a window's connection to the X server was closed, the window's process 
*will crash in nearly all cases. Even if not, the window is not usable anymore. 
*/
static VALUE m_bang_kill(VALUE self)
{
  Display * p_display;
  Window win = GET_WINDOW;
  
  p_display = get_win_display(self);
  XSetErrorHandler(handle_x_errors);
  
  XKillClient(p_display, win);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*call-seq: 
*  kill_process( [ term = true ] ) ==> nil
*
*Kills a window's process. 
*===Parameters
*[+term+] (true) If true, SIGTERM is sent to the window process; if false or nil, SIGKILL is sent. 
*===Return value
*nil. 
*===Raises
*[XError] _NET_WM_PID is unsupported or the target window does not have the _NET_WM_PID property set. 
*===Example
*  #Send SIGTERM to a window process
*  Imitator::X::XWindow.from_title(/imitator/).kill_process
*  #Send SIGKILL to a window process
*  Imitator::X::XWindow.from_title(/imitator/).kill_process(false)
*===Remarks
*This method is part of the EWMH standard _NET_WM_PID. 
*
*This method is by far the most aggressive variant to destroy a window, 
*especially if +term+ is false, but since not every window has set the _NET_WM_PID property, 
*it may fail. See also #close, #kill and #kill!, also have a look at #pid. 
*/
static VALUE m_kill_process(int argc, VALUE argv[], VALUE self)
{
  VALUE rterm;
  int signal = SIGTERM;
  
  rb_scan_args(argc, argv, "01", &rterm);
  
  if (rterm == Qfalse || rterm == Qnil)
    signal = SIGKILL;
  
  kill(NUM2INT(m_pid(self)), signal);
  return Qnil;
}

/*
*Closes +self+ by activating it and then sending [ALT]+[F4]. 
*===Return value
*nil. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  xwin.close
*===Remarks
*This method doesn't force a window to close. If you have unsaved data, 
*a program may ask you to save instead of closing. Have a look at the 
*various kill methods to achieve this. 
*/
static VALUE m_close(VALUE self)
{
  m_activate(self);
  rb_funcall(Keyboard, rb_intern("key"), 1, rb_enc_str_new("Alt+F4", strlen("Alt+F4"), rb_utf8_encoding()));
  return Qnil;
}

/*
*Checks weather +self+ exists or not by calling XWindow.exists? with 
*the information of this object. 
*===Return value
*true or false. 
*===Example
*  xwin = Imitator::X::XWindow.from_title(/imitator/)
*  puts xwin.exists? #=> true
*  xwin.kill
*  puts xwin.exists? #=> false
*/
static VALUE m_exists(VALUE self)
{
  VALUE args[3];
  VALUE rmatch_data, screen, display;
  
  rmatch_data = rb_funcall(rb_ivar_get(self, rb_intern("@display_string")), rb_intern("match"), 1, rb_reg_new("\\:(\\d+?)\\.(\\d+?)", strlen("\\:(\\d+?)\\.(\\d+?)"), 0));
  display = rb_funcall(rb_reg_nth_match(1, rmatch_data), rb_intern("to_i"), 0);
  screen = rb_funcall(rb_reg_nth_match(2, rmatch_data), rb_intern("to_i"), 0);
  args[0] = rb_ivar_get(self, rb_intern("@window_id"));
  args[1] = screen;
  args[2] = display;
  return cm_exists(3, args, XWindow);
}

/*
*call-seq: 
*  xwin.eql?( other_xwin ) ==> true or false
*  xwin == other_xwin ==> true or false
*
*Checks wheather or not two XWindow objects are the same. Two 
*Xwindow objects are the same if they refer to the same window id. 
*===Parameters
*[+other_xwin+] The window to compare with. 
*===Return value
*true or false. 
*===Example
*  xwin1 = Imitator::X::XWindow.from_title(/imitator/)
*  xwin2 = Imitator::X::XWindow.from_title(/imitat/)
*  xwin3 = Imitator::X::XWindow.from_active
*  p xwin.window_id #=> 69206019
*  p xwin2.window_id #=> 69206019
*  p xwin3.window_id #=> 73401172
*  p(xwin1 == xwin2) #=> true
*  p(xwin1 == xwin3) #=> false
*/
static VALUE m_is_equal_to(VALUE self, VALUE other)
{
  VALUE self_id = rb_ivar_get(self, rb_intern("@window_id"));
  VALUE other_id = rb_ivar_get(other, rb_intern("@window_id"));
  
  return rb_funcall(self_id, rb_intern("=="), 1, other_id);
}
/***********************Init-Function*******************************/

void Init_xwindow(void)
{
  XWindow = rb_define_class_under(X, "XWindow", rb_cObject);
  
  rb_define_singleton_method(XWindow, "default_root_window", cm_default_root_window, 0);
  rb_define_singleton_method(XWindow, "exists?", cm_exists, -1);
  rb_define_singleton_method(XWindow, "xquery", cm_xquery, 2); /* :nodoc: */
  rb_define_singleton_method(XWindow, "search", cm_search, -1);
  rb_define_singleton_method(XWindow, "from_title", cm_from_title, -1);
  rb_define_singleton_method(XWindow, "from_focused", cm_from_focused, -1);
  rb_define_singleton_method(XWindow, "from_active", cm_from_active, -1);
  rb_define_singleton_method(XWindow, "wait_for_window", cm_wait_for_window, -1);
  rb_define_singleton_method(XWindow, "wait_for_window_termination", cm_wait_for_window_termination, -1);
  
  rb_define_method(XWindow, "initialize", m_initialize, -1);
  rb_define_method(XWindow, "inspect", m_inspect, 0);
  rb_define_method(XWindow, "title", m_title, 0);
  rb_define_method(XWindow, "window_id", m_window_id, 0);
  rb_define_method(XWindow, "root_win", m_root_win, 0);
  rb_define_method(XWindow, "parent", m_parent, 0);
  rb_define_method(XWindow, "children", m_children, 0);
  rb_define_method(XWindow, "root_win?", m_is_root_win, 0);
  rb_define_method(XWindow, "position", m_position, 0);
  rb_define_method(XWindow, "size", m_size, 0);
  rb_define_method(XWindow, "visible?", m_is_visible, 0);
  rb_define_method(XWindow, "mapped?", m_is_mapped, 0);
  rb_define_method(XWindow, "move", m_move, 2);
  rb_define_method(XWindow, "resize", m_resize, 2);
  rb_define_method(XWindow, "raise_win", m_raise_win, 0);
  rb_define_method(XWindow, "focus", m_focus, 0);
  rb_define_method(XWindow, "unfocus", m_unfocus, 0);
  rb_define_method(XWindow, "map", m_map, 0);
  rb_define_method(XWindow, "unmap", m_unmap, 0);
  rb_define_method(XWindow, "activate", m_activate, 0);
  rb_define_method(XWindow, "pid", m_pid, 0);
  rb_define_method(XWindow, "kill", m_kill, 0);
  rb_define_method(XWindow, "kill!", m_bang_kill, 0);
  rb_define_method(XWindow, "kill_process", m_kill_process, -1);
  rb_define_method(XWindow, "close", m_close, 0);
  rb_define_method(XWindow, "exists?", m_exists, 0);
  rb_define_method(XWindow, "eql?", m_is_equal_to, 1);
  
  rb_define_alias(XWindow, "to_s", "title");
  rb_define_alias(XWindow, "to_i", "window_id");
  rb_define_alias(XWindow, "pos", "position");
  rb_define_alias(XWindow, "==", "eql?");
}