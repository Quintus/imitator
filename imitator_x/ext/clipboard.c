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
#include "clipboard.h"

/*
*It was an awful lot of work to get into the X clipboard stuff. 
*I don't think I'll ever forget this, but just in case, here are the 
*important things: 
** What the X standard calls a "selection" is a clipboard. 
** An atom is for X what a symbol is for Ruby. Just an internal storage thing convertable to strings. 
** X maintains 3 clipboards: 
***PRIMARY (Atom: XA_PRIMARY): The middle-mouse-button-clipboard. Mildly deprecated. 
***SECONDARY (Atom: XA_SECONDARY): This clipboard isn't used by anyone. It's just there and stands around. 
*** CLIPBOARD (Atom: XInternAtom(p_display, "CLIPBOARD", True): The main clipboard. It acts as the Mac or Windows clipboard via [CTRL]+[C] and the like. 
**XConvertSelection() is sparely described in the X standard. For whatever reason, after 10 times reading it's documentation I 
*  did not understand what's actually _converted_ here. After I looked into xsel's and xclip's sources many times I finally got it: 
*  this damned function converts the clipboard data from X's internal storage to the specified encoding. The word "encoding" wasn't ever 
*  mentioned in the function's description, just "target atom". Why? It's a riddle, solve it if you like...
** XA_STRING is the atom for the ISO-8859-1 encoding. 
*  The atom for UTF-8 is: XInternAtom(p_display, "UTF8_STRING", True). 
*  The atom for binary and ASCII stuff is XA_TEXT. 
**XConvertSelection() fails for the CLIPBOARD selection if None is specified as the "property" parameter; just another thing not documented. 
*  It's absolutly... We Germans say "wurschtegal", I don't know how to translate this - it's unimportant in any way - what you pass in. Just use an atom you like, as e.g. XInternAtom(p_display, "DAMNED_FUNCTION", False). 
** If a X function wants a delay, you shouldn't pass in 0 if you want to make it executed immediately. Pass in CurrentTime instead. 
**Since X doesn't maintain a central clipboard application (but thank God, approaches are made for a shared library in order to simplify the interaction with the selections!) you must 
*  create a requestor window whose "xselection" property will get set. That works even if the window isn't mapped. 
** Data in the PRIMARY clipboard is set in that very moment you select text. You don't have to press any button or click any menu - it's just made. If you deselect the text, 
*  the PRIMARY clipboard's content goes away. 
** The requestor window will get a SelectionNotify event after a call to XConvertSelection(), regardless of that function's success. If XConvertSelection() failed, the SelectionNotify's 
*  "property" member will be None. 
*/

/*Document-module: Imitator::X::Clipboard
*Yeah, finally, this is the Clipboard module. During development I thought more than once I should stop 
*this, because to deal with X's selection functions is awkward. And still, the Clipboard.write method doesn't work 
*well, so don't rely on it (although Clipboard.read should do a good job). 
*Also, the methods defined here can't be used to set or access non-textual data. 
*
*So, here's what you need to know about X and the clipboard: 
*
** <b>X doesn't have clipboards. </b>
*  Yes, that's true! X does not use the term "clipboard", they're always called "selections". 
*
** <b>X has 3 selections. </b>
*  Every X server maintains three selections: The PRIMARY, the SECONDARY and the CLIPBOARD selection. 
*
** The <b>PRIMARY</b> selection is set when you select some text - really, you don't need anything else. Just 
*  open a terminal window, select some text and then press the middle mouse button (or your mouse wheel) 
*  and the selected text gets pasted. Of course this data can't survive the application that sets it. 
*
** The <b>SECONDARY</b> selection... well, it's used by nobody. Just ignore it. 
*
** The <b>CLIPBOARD</b> selection is to an extend of ~95% the one you're looking for. When you press [CTRL]+[C] 
*  or select text and then choose "copy" from a pull-down menu, your data is stored here. Depending on your 
*  system, the data stored here remains accessible even after the application that set it terminated. To achieve this, 
*  a program called a "clipboard manager" that is running in the background silently copies data applications store 
*  in the CLIPBOARD selection into it's own buffer and then acquires ownership of CLIPBOARD. 
*
*In conclusion, just remember that you should probably try the PRIMARY selection if CLIPBOARD doesn't contain your data. 
*
*This module operates on UTF-8-encoded strings; that means, you should pass UTF-8-encoded strings to Clipboard.write and 
*you get back UTF-8-encoded strings from Clipboard.read. 
*/

/*
*call-seq: 
*  Clipboard.read(selection = :clipboard) ==> aString
*
*Reads the value of the specified +clipboard+. 
*===Parameters
*[+selection+] The selection to read from. One of <tt>:clipboard</tt>, <tt>:primary</tt> and <tt>:secondary</tt>. 
*===Return value
*A string containing the selection's content or an empty string if there is no data in the selection. 
*===Raises
*[ArgumentError] Incorrect +selection+ argument given. 
*[XError] Could not retrieve the selection data for some reason. It's likely that there's data in the selection, but it isn't textual data; such as an image, for example. 
*===Example
*  puts Imitator::X::Clipboard.read(:clipboard) #=> I love Ruby!
*/
static VALUE m_read(int argc, VALUE argv[], VALUE self)
{
  Display * p_display;
  Atom clipboard, actual_type;
  Window win;
  XEvent xevt;
  int actual_format;
  unsigned long nitems, bytes_after_return;
  unsigned char * property;
  char * cp;
  VALUE rclipboard, result;
  
  rb_scan_args(argc, argv, "01", &rclipboard);
  
  if (NIL_P(rclipboard))
    rclipboard = ID2SYM(rb_intern("clipboard"));
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  rclipboard = rb_funcall(rclipboard, rb_intern("to_s"), 0); /*Symbol -> String*/
  rclipboard = rb_funcall(rclipboard, rb_intern("upcase"), 0); /*String -> STRING*/
  clipboard = XInternAtom(p_display, StringValuePtr(rclipboard), True); /*STRING -> Atom(STRING)*/
  if (clipboard == None)
    rb_raise(rb_eArgError, "Invalid clipboard specified!");
  
  /*Check wheather there is a clipboard owner, and if not, just return an empty string. */
  if (XGetSelectionOwner(p_display, clipboard) == None)
    return rb_str_new2("");
  
  /*Create a window for selection interaction*/
  win = CREATE_REQUESTOR_WIN;
  
  /*Order the selection content from the current selection owner*/
  XConvertSelection(p_display, clipboard, UTF8_ATOM, IMITATOR_X_CLIP_ATOM, win, CurrentTime);
  /*Wait until our requestor window gets the message that it has received the selection content*/
  for (;;)
  {
    XNextEvent(p_display, &xevt);
    if (xevt.type == SelectionNotify)
      break;
  }
  if (xevt.xselection.property == None)
      rb_raise(XError, "Could not retrieve selection (XConvertSelection() failed)! Is there non-text data in the clipboard?");
  
  /*Read the selection property of our requestor window, just in order to get the selection content's size. We don't read the actual data here. */
  XGetWindowProperty(xevt.xselection.display, xevt.xselection.requestor, xevt.xselection.property, 0, 0, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after_return, &property);
  cp = (char *) malloc(bytes_after_return); /*Nice - we already get the number of bytes to allocate from X*/
  /*This is the exciting moment. Reads the data from our requestor window. */
  XGetWindowProperty(xevt.xselection.display, xevt.xselection.requestor, xevt.xselection.property, 0, bytes_after_return, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes_after_return, &property);
  /*Now copy it away from X, since X wants to XFree() it's data*/
  strcpy(cp, (char *) property);
  /*Convert it to a Ruby UTF-8 string*/
  result = rb_enc_str_new(cp, strlen(cp), rb_utf8_encoding());
  
  /*Clean-up actions*/
  XFree(property);
  free(cp);
  XDestroyWindow(p_display, win); /*We don't need the window anymore, a new request will create a new window*/
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  
  return result;
}

/*
*call-seq: 
*  Clipboard.write( text ) ==> text
*
*Writes +text+ to the CLIPBOARD selection. 
*===Parameters
*[+text+] The text to store. 
*===Return value
*The +text+ argument. 
*===Raises
*[XError] No clipboard manager available, or it doesn't signal it's existance via CLIPBOARD_MANAGER. Also indicates failure in acquiring ownership of the CLIPBOARD selection or failure in setting the clipboard manager as it's owner. 
*===Example
*  Imitator::X::Clipboard.write("I love Ruby")
*  puts Imitator::X::Clipboard.read #=> I love Ruby
*===Remarks
*This method doesn't work well in all cases. It works on my Ubuntu Karmic machine, but didn't work 
*with Xubuntu and openSUSE when I tested it in VirtualBox. Therefore, don't rely on this method. 
*/
static VALUE m_write(VALUE self, VALUE rtext)
{
  Display * p_display;
  Window win, clipboard_owner;
  XEvent xevt, xevt2;
  Atom targets[6], target_sizes[12], save_targets[2];
  VALUE rtext_utf8, rtext_iso_latin1;
  int utf8_len, iso_latin1_len;
  
  /*Get neccessary information*/
  rtext_utf8 = rb_str_export_to_enc(rtext, rb_utf8_encoding());
  rtext_iso_latin1 = rb_str_export_to_enc(rtext, rb_enc_find("ISO-8859-1"));
  /*rtext_utf8.bytes.to_a.length - otherwise we lose data due to multibyte characters*/
  utf8_len = NUM2INT(rb_funcall(rb_funcall(rb_funcall(rtext_utf8, rb_intern("bytes"), 0), rb_intern("to_a"), 0), rb_intern("length"), 0));
  iso_latin1_len = NUM2INT(rb_funcall(rtext_iso_latin1, rb_intern("length"), 0));
  
  /*Open default display*/
  p_display = XOpenDisplay(NULL);
  /*Let Ruby handle protocol errors*/
  XSetErrorHandler(handle_x_errors);
  /*This are the TARGETS we support*/
  targets[0] = TARGETS_ATOM;
  targets[1] = UTF8_ATOM;
  targets[2] = XA_STRING;
  targets[3] = XInternAtom(p_display, "TIMESTAMP", True); /*TODO: Implement this request*/
  targets[4] = SAVE_TARGETS_ATOM;
  targets[5] = TARGET_SIZES_ATOM;
  /*These are the target's sizes*/
  target_sizes[0] = TARGETS_ATOM;
  target_sizes[1] = 6 * sizeof(Atom);
  
  target_sizes[2] = UTF8_ATOM;
  target_sizes[3] = utf8_len;
  
  target_sizes[4] = XA_STRING;
  target_sizes[5] = iso_latin1_len;
  
  target_sizes[6] = XInternAtom(p_display, "TIMESTAMP", True);
  target_sizes[7] = -1;
  
  target_sizes[8] = SAVE_TARGETS_ATOM;
  target_sizes[9] = -1;
  
  target_sizes[10] = TARGET_SIZES_ATOM;
  target_sizes[11] = 12 * sizeof(Atom);
  /*We want our data have stored as UTF-8*/
  save_targets[0] = UTF8_ATOM;
  save_targets[1] = XA_STRING;
  if (CLIPBOARD_MANAGER_ATOM == None)
  {
    XSetErrorHandler(NULL);
    XCloseDisplay(p_display);
    rb_raise(XError, "No clipboard manager available!");
  }
  
  /*Create a window for copying into CLIPBOARD*/
  win = CREATE_REQUESTOR_WIN;
  
  /*Make sure that there's a clipboard manager which can process our request*/
  if ( (clipboard_owner = XGetSelectionOwner(p_display, CLIPBOARD_MANAGER_ATOM)) == None)
  {
    XDestroyWindow(p_display, win);
    XSetErrorHandler(NULL);
    XCloseDisplay(p_display);
    rb_raise(XError, "No owner for the CLIPBOARD_MANAGER selection!");
  }
  
  /*Get control of the CLIPBOARD*/
  XSetSelectionOwner(p_display, CLIPBOARD_ATOM, win, CurrentTime);
  if (XGetSelectionOwner(p_display, CLIPBOARD_ATOM) != win)
  {
    XDestroyWindow(p_display, win);
    XSetErrorHandler(NULL);
    XCloseDisplay(p_display);
    rb_raise(XError, "Could not acquire ownership of the CLIPBOARD selection!");
  }
  
  /*Our application "needs to exit"*/
  XChangeProperty(p_display, win, IMITATOR_X_CLIP_ATOM, XA_ATOM, 32, PropModeReplace, (unsigned char *) save_targets, 1);
  XConvertSelection(p_display, CLIPBOARD_MANAGER_ATOM, SAVE_TARGETS_ATOM, IMITATOR_X_CLIP_ATOM, win, CurrentTime);
  for (;;)
  {
    XNextEvent(p_display, &xevt);
    if (xevt.type == SelectionRequest) /*selection-related event*/
    {
      /*This is for all anserwing events the same (except "not supported")*/
      xevt2.xselection.type = SelectionNotify;
      xevt2.xselection.display = xevt.xselectionrequest.display;
      xevt2.xselection.requestor = xevt.xselectionrequest.requestor;
      xevt2.xselection.selection = xevt.xselectionrequest.selection;
      xevt2.xselection.target = xevt.xselectionrequest.target;
      xevt2.xselection.time = xevt.xselectionrequest.time;
      xevt2.xselection.property = xevt.xselectionrequest.property;
      
      /*Handle individual selection requests*/
      if (xevt.xselectionrequest.target == XInternAtom(p_display, "TARGETS", True)) /*TARGETS information requested*/
      {
        /*Write supported TARGETS into the requestor*/
        XChangeProperty(p_display, xevt.xselectionrequest.requestor, xevt.xselectionrequest.property, XA_ATOM, 32, PropModeReplace, (unsigned char *) targets, 6);
        /*Notify the requestor we have set its requested property*/
        XSendEvent(p_display, xevt.xselectionrequest.requestor, False, NoEventMask, &xevt2);
      }
      else if (xevt.xselectionrequest.target == TARGET_SIZES_ATOM)
      {
        /*Answer how much data we want to store*/
        XChangeProperty(p_display, xevt.xselectionrequest.requestor, xevt.xselectionrequest.property, ATOM_PAIR_ATOM, 32, PropModeReplace, (unsigned char *) target_sizes, 6);
        /*Notify the requestor we have set its requested property*/
        XSendEvent(p_display, xevt.xselectionrequest.requestor, False, NoEventMask, &xevt2);
      }
      else if (xevt.xselectionrequest.target == MULTIPLE_ATOM) /*Makes multiple requests at once*/
      { /*I know, allocating variables in inner blocks is bad style, but I didn't want to 
        *declare this amount of variables at the function's top, beacuse that would decrease readability*/
        Atom actual_type, requested = None;
        int actual_format, i;
        unsigned long nitems, bytes;
        unsigned char * prop;
        Atom * wanted_atoms;
        /*See how much data is there and allocate this amount*/
        XGetWindowProperty(p_display, xevt.xselectionrequest.requestor, xevt.xselectionrequest.property, 0, 0, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes, &prop);
        wanted_atoms = (Atom *) malloc(sizeof(Atom) * nitems);
        /*Now get the data and copy it to our variable*/
        XGetWindowProperty(p_display, xevt.xselectionrequest.requestor, xevt.xselectionrequest.property, 0, 1000000, False, AnyPropertyType, &actual_type, &actual_format, &nitems, &bytes, &prop);
        memcpy(wanted_atoms, prop, sizeof(Atom) * nitems);
        /*Now handle each single request by it's own*/
        for(i = 0;i < nitems; i++)
        {
          if (requested == None) /*This means we'll get a target atom*/
            requested = wanted_atoms[i];
          else /*This means we'll get a property atom*/
          {
            /*OK, I see. I could have made the event handling a separate function and then call it recursively here, 
            *but since I only support two targets that's unneccessary I think*/
            if (requested == UTF8_ATOM)
              XChangeProperty(p_display, xevt.xselectionrequest.requestor, wanted_atoms[i], requested, 8, PropModeReplace, (unsigned char *) StringValuePtr(rtext_utf8), utf8_len);
            else if (requested == XA_STRING)
              XChangeProperty(p_display, xevt.xselectionrequest.requestor, wanted_atoms[i], requested, 8, PropModeReplace, (unsigned char *) StringValuePtr(rtext_iso_latin1), iso_latin1_len);
            else /*Not supported request*/
              XChangeProperty(p_display, xevt.xselectionrequest.requestor, wanted_atoms[i], requested, 32, PropModeReplace, (unsigned char *) None, 1);
            requested = None; /*The next iteration will be a target atom again*/
          }
        }
        /*Notify the requestor we have finished*/
        XSendEvent(p_display, xevt.xselectionrequest.requestor, False, NoEventMask, &xevt2);
        /*Free the allocated memory*/
        free(wanted_atoms);
        XFree(prop);
      }
      //~ else if (xevt.xselectionrequest.target == UTF8_ATOM || xevt.xselectionrequest.target == XA_STRING) /*UTF-8 or ASCII requested*/
      else if (xevt.xselectionrequest.target == UTF8_ATOM)
      {
        /*Write the string into the requestor*/
        XChangeProperty(p_display, xevt.xselectionrequest.requestor, xevt.xselectionrequest.property, xevt.xselectionrequest.target, 8, PropModeReplace, (unsigned char *) StringValuePtr(rtext_utf8), utf8_len);
        /*Notify the requestor we've finished*/
        XSendEvent(p_display, xevt.xselectionrequest.requestor, False, NoEventMask, &xevt2);
      }
      else if (xevt.xselectionrequest.target == XA_STRING)
      {
        XChangeProperty(p_display, xevt.xselectionrequest.requestor, xevt.xselectionrequest.property, xevt.xselectionrequest.target, 8, PropModeReplace, (unsigned char *) StringValuePtr(rtext_iso_latin1), iso_latin1_len);
        XSendEvent(p_display, xevt.xselectionrequest.requestor, False, NoEventMask, &xevt2);
      }
      else /*No supported request. SAVE_TARGETS is included here, since it's only a marker*/
      {
        /*Notify the requestor we don't support what it wants*/
        xevt2.xselection.target = None; /*This indicates we don't support what it wants*/
        XSendEvent(p_display, xevt.xselectionrequest.requestor, False, NoEventMask, &xevt2);
      }
    }
    else if (xevt.type == SelectionNotify) /*OK, our request to the clipboard manager has completed*/
    {
      if (xevt.xselection.property == None) /*Ooops - conversion failed, we're still the owner of CLIPBOARD*/
      {
        XDestroyWindow(p_display, win);
        XSetErrorHandler(NULL);
        XCloseDisplay(p_display);
        rb_raise(XError, "Unable to request the clipboard manager to acquire the CLIPBOARD selection!");
      }
      else if (xevt.xselection.property == IMITATOR_X_CLIP_ATOM) /*Success - we're out of responsibility now and can safely exit*/
        break;
    }
  }
  
  /*Cleanup actions*/
  XDestroyWindow(p_display, win);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  
  return rtext;
}

/*
*call-seq: 
*  Clipboard.clear( *selections = :clipboard ) ==> nil
*
*Clears the specified selection(s). 
*===Parameters
*[<tt>*selections</tt>] (<tt>:clipboard</tt>) Specifies one or more of <tt>:primary</tt>, <tt>:secondary</tt> and/or <tt>:clipboard</tt>. 
*===Return value
*nil. 
*===Raises
*[XError] Invalid selection specified. 
*===Example
*  #Clear the CLIPBOARD's contents
*  Imitator::X::Clipboard.clear
*  #Clear PRIMARY
*  Imitator::X::Clipboard.clear(:primary)
*  #Clear PRIMARY and CLIPBOARD
*  Imitator::X::Clipboard.clear(:primary, :clipboard)
*/
VALUE m_clear(VALUE self, VALUE args)
{
  VALUE rtemp;
  Display * p_display;
  int length, i;
  Atom selection;
  
  if (RTEST(rb_funcall(args, rb_intern("empty?"), 0)))
    rb_ary_push(args, ID2SYM(rb_intern("clipboard")));
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  length = NUM2INT(rb_funcall(args, rb_intern("length"), 0));
  for(i = 0; i < length; i++)
  {
    rtemp = rb_funcall(rb_ary_entry(args, i), rb_intern("upcase"), 0);
    selection = XInternAtom(p_display, StringValuePtr(rtemp), True);
    if (selection == None)
    {
      XSetErrorHandler(NULL);
      XCloseDisplay(p_display);
      rb_raise(XError, "Invalid selection specified!");
    }
    XSetSelectionOwner(p_display, selection, None, CurrentTime);
  }
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/********************Init function**********************/

void Init_clipboard(void)
{
  Clipboard = rb_define_module_under(X, "Clipboard");
  rb_define_module_function(Clipboard, "read", m_read, -1);
  rb_define_module_function(Clipboard, "write", m_write, 1);
  rb_define_module_function(Clipboard, "clear", m_clear, -2);
}