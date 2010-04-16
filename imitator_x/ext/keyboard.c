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
#include "keyboard.h"

/*
*When coding this file, I found out that the easiest way 
*to get the KeySym of a character is just to encode it in UTF-8
*and then call String#ord on it. That directly gives the value of 
*the XK_<your_char_here> constants. 
*
*The XTEST extension cannot fake special keys directly (sadly). 
*In order to fake keystrokes like those for uppercase characters
*you have to to specify the full needed keystroke sequence 
*(this explains much of the complexity of the code here), i.e. 
*[Shift]+[T] is required to get an uppercase letter T. */

/*Document-module: Imitator::X::Keyboard
*This module allows interaction with the keyboard. Via the 
*Keyboard.key method you can send key combinations à la 
*[CTRL]+[ALT]+[DEL], and by using Keyboard.simulate you 
*can fake whole textual data. Please note that you cannot simulate keystrokes 
*for characters that you cannot type manually on your keyboard. 
*
*Keyboard.simulate requires one further note, though. Since there 
*are many characters out there which aren't created by the same 
*keystrokes all over the world, I had to think about how I can implement 
*this best, without needing to test my code on every single locale existing 
*somewhere in the world (take the at sign @ for instance: On my German keyboard 
*I'd press [ALT_GR]+[Q] to get it, in Switzerland it would be [ALT_GR]+[2]). 
*Finally I created a file "imitator_x_special_chars.yml" in Imitator for X's gem 
*directory that maps the special characters to their key combinations. You are 
*encouraged to change the mapping temporarily by modifying the Keyboard::SPECIAL_CHARS 
*hash or permanently by changing the contents of that file. If you do so, you 
*may should think about sending me an email to sutniuq<>gmx<>net and attach 
*your "imitator_x_special_chars.yml" file and mention your locale - sometime 
*in the future I may be able to set up a locale selector then that automatically 
*chooses the right key combinations. 
*
*The best way to find out how keys are generated is to run the following command 
*and then press the wanted key: 
*  xev | grep keysym
*
*==Note to debuggers
*When you're debugging Imitator for X and you don't want to 
*use the "imitator_x_special_chars.yml" file of an already installed gem 
*you should set the global variable $imitator_x_charfile_path to the 
*path of the file you want to use before you require "imitator/x". 
*/

/********************Helper functions***********************/

/*This function returns the KeyCode for the Ruby string specified by 
*+rkey+. It first tries to convert it directly to a KeySym, and if that fails, 
*it looks into the ALIASES hash (a constant of Keyboard) if there is an alias 
*defined. If so, the KeySym for the alias is used. If not, the connection to the 
*X server is closed and a XError is thrown. 
*Afterwards, the KeySym is converted to the desired KeyCode. 
*/
KeyCode get_keycode(Display * p_display, VALUE rkey)
{
  KeySym sym;
  KeyCode code;
  VALUE ralias;
  
  /*First try direct conversion*/
  sym = XStringToKeysym(StringValuePtr(rkey));
  if (sym == NoSymbol) /*If direct conversion failed*/
  {
    /*Look into the ALIASES hash for an alias*/
    ralias = rb_hash_lookup(rb_const_get(Keyboard, rb_intern("ALIASES")), rkey);
    if (NIL_P(ralias)) /*If no alias found*/
    {
      XSetErrorHandler(NULL);
      XCloseDisplay(p_display);
      rb_raise(XError, "Invalid key '%s'!", StringValuePtr(rkey));
    }
    else /*Use the alias for direct conversion*/
      code = get_keycode(p_display, ralias); /*The hash only stores valid keysym names; otherwise an endless recursion would occur here*/
  }
  else
    code = XKeysymToKeycode(p_display, sym);
  return code;
}

/*Converts a text into a command list. Returns an array of form 
*  [[keycode, bool], [keycode, bool], ...]
*where +keycode+ is the key to press or release and +bool+ indicates 
*wheather the key should be pressed or released. 
*Note that everything is returned as a Ruby object. */
VALUE convert_rtext_to_rkeycodes(Display * p_display, VALUE rtext)
{
  VALUE rtokens, rcommands, rcommands2, rtemp, rkeycodes, rkeycode, rentry, rparts, rkeysym;
  int length, length2, i, j;
  
  rtokens = rb_str_split(rtext, ""); /*Every single letter has to be processed by it's own*/
  length = NUM2INT(rb_funcall(rtokens, rb_intern("length"), 0));
  
  rcommands = rb_ary_new();
  /*First, replace all characters that need modkey presses 
  *with the full keypress sequence, as specified in the SPECIAL_CHARS hash*/
  for(i = 0;i < length;i++)
  {
    rtemp = rb_ary_entry(rtokens, i);
    if ( RTEST(rentry = rb_hash_lookup(rb_const_get(Keyboard, rb_intern("SPECIAL_CHARS")), rtemp)) )
      rb_ary_push(rcommands, rentry);
    else /*We don't need a replacement*/
      rb_ary_push(rcommands, rtemp);
  }
  
  rcommands2 = rb_ary_new();
  /*Second, replace the keypress and keypress sequences with command arrays as 
  *[38, true]
  *(this means press "a" down on my Ubuntu Karmic 64-bit machine). */
  for(i = 0;i < length;i++)
  {
    rtemp = rb_ary_entry(rcommands, i);
    if (NUM2INT(rb_funcall(rtemp, rb_intern("length"), 0)) > 1) /*Character that need modkey press*/
    {
      /*Each char is separated from others by the + sign, so split up there*/
      rparts = rb_str_split(rtemp, "+");
      length2 = NUM2INT(rb_funcall(rparts, rb_intern("length"), 0));
      
      /*Convert each character to it's corresponding keycode*/
      rkeycodes = rb_ary_new();
      for(j = 0; j < length2; j++)
        rb_ary_push(rkeycodes, INT2NUM(get_keycode(p_display, rb_ary_entry(rparts, j))));
      
      /*Insert the keypress events in the rcommands2 array. */
      for(j = 0; j < length2; j++)
      {
        rentry = rb_ary_new();
        rb_ary_push(rentry, rb_ary_entry(rkeycodes, j));
        rb_ary_push(rentry, Qtrue);
        rb_ary_push(rcommands2, rentry);
      }
      
      /*Insert the keyrelease events in the rcommands2 array, but in reverse 
      *order*/
      for(j = length2 - 1; j >= 0; j--)
      {
        rentry = rb_ary_new();
        rb_ary_push(rentry, rb_ary_entry(rkeycodes, j));
        rb_ary_push(rentry, Qfalse);
        rb_ary_push(rcommands2, rentry);
      }
    }
    else /*Normal key*/
    {
      /*X stores the keysyms by the value of the unicode codepoint of a 
      *character - Ruby easily allows us to access the unicode codepoint of a 
      *character by using the String#ord method on an UTF-8-encoded character. */
      rkeysym = rb_funcall(rtemp, rb_intern("ord"), 0);
      /*Convert the keysym to the local keycode*/
      rkeycode = INT2NUM(XKeysymToKeycode(p_display, NUM2INT(rkeysym)));
      
      /*Down*/
      rentry = rb_ary_new();
      rb_ary_push(rentry, rkeycode);
      rb_ary_push(rentry, Qtrue);
      rb_ary_push(rcommands2, rentry);
      /*Up*/
      rentry = rb_ary_new();
      rb_ary_push(rentry, rkeycode);
      rb_ary_push(rentry, Qfalse);
      rb_ary_push(rcommands2, rentry);
    }
  }
  return rcommands2;
}

/********************Module functions**********************/

/*
*call-seq: 
*  Keyboard.key(str) ==> anArray
*
*Simulates one single key press. This method can also be used to send 
*combinations like [CTRL]+[A] or [CTRL]+[ALT]+[DEL]. 
*===Parameters
*[+str+] The keypress to simulate. In order to press more than one key, separate them by a plus + sign. 
*===Return value
*An array of how the +str+ has been interpreted. That means, +str+ split by plus. 
*===Example
*  #Sends a single lower a
*  Imitator::X::Keyboard.key("a")
*  #Sends a single upper A
*  Imitator::X::Keyboard.key("Shift+a")
*  #Sends [CTRL]+[L]
*  Imitator::X::Keyboard.key("Ctrl+l")
*  #Sends [CTRL]+[ALT]+[DEL]
*  Imitator::X::Keyboard.key("Ctrl+Alt+Delete")
*  #Sends character ä
*  Imitator::X::Keyboard.key("adiaeresis")
*/
static VALUE m_key(VALUE self, VALUE keystr)
{
  Display * p_display;
  KeyCode * keycodes;
  VALUE keys;
  int arylen, i;
  
  /*Split the keystr into single key names*/
  keys = rb_str_split(keystr, "+");
  /*Prepare conversion into an array of KeyCodes*/
  arylen = NUM2INT(rb_funcall(keys, rb_intern("length"), 0));
  keycodes = (KeyCode *) malloc(sizeof(KeyCode) * arylen);
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  /*Convert the array of Ruby key names into one of KeyCodes*/
  for(i = 0;i < arylen;i++)
    keycodes[i] = get_keycode(p_display, rb_ary_entry(keys, i));
  
  /*Press all buttons down*/
  for(i = 0;i < arylen;i++)
    XTestFakeKeyEvent(p_display, keycodes[i], True, CurrentTime);
  
  /*Release all buttons in reverse order*/
  for(i = arylen - 1; i >= 0; i--)
    XTestFakeKeyEvent(p_display, keycodes[i], False, CurrentTime);
  
  /*Cleanup actions*/
  free(keycodes);
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return keys;
}

/*
*call-seq: 
*  Keyboard.simulate( text [, raw = false ] ) ==> aString
*
*Simulates the given sequence of characters as keypress and keyrelease 
*events to the X server. 
*===Parameters
*[+text+] The characters whose keystrokes you want to simulate. 
*[+raw+] If true, escape sequences via { and } are ignored. See _Remarks_. 
*===Return value
*The interpreted string. That is, the +text+ you passed in with minor modifications 
*as ASCII TAB replaced by {TAB}. 
*===Raises
*[XError] Invalid key name in escape sequence or ASCII Tab or Newline found in raw string. 
*===Example
*  #Simulate [A], [B] and [C] keystrokes
*  Imitator::X::Keyboard.simulate("abc")
*  #Simulate [A], [SHIFT]+[B] and [C] keystrokes
*  Imitator::X::Keyboard.simulate("aBc")
*  #Simulate [A], [ESC] and [B] keystrokes
*  Imitator::X::Keyboard.simulate("a{ESC}b")
*===Remarks
*The +text+ parameter may contain special escape sequences which are included in braces 
*{ and }. These are ignored if the +raw+ parameter is set to true, otherwise they cause the 
*following keys to be pessed (and released, of course): 
*  Escape sequence | Resulting keystroke
*  ================+=========================
*  {Shift}         | [SHIFT_L]
*  ----------------+-------------------------
*  {Alt}           | [ALT_L]
*  ----------------+-------------------------
*  {AltGr}         | [ALT_GR]
*  ----------------+-------------------------
*  {Ctrl}          | [CTRL_L]
*  ----------------+-------------------------
*  {Win}           | [WIN_L]
*  ----------------+-------------------------
*  {Super}         | [WIN_L]
*  ----------------+-------------------------
*  {CapsLock}      | [CAPS_LOCK}
*  ----------------+-------------------------
*  {Caps_Lock}     | [CAPS_LOCK]
*  ----------------+-------------------------
*  {ScrollLock}    | [SCROLL_LOCK]
*  ----------------+-------------------------
*  {Scroll_Lock}   | [SCROLL_LOCK]
*  ----------------+-------------------------
*  {NumLock}       | [NUM_LOCK]
*  ----------------+-------------------------
*  {Num_Lock}      | [NUM_LOCK]
*  ----------------+-------------------------
*  {Del}           | [DEL]
*  ----------------+-------------------------
*  {DEL}           | [DEL]
*  ----------------+-------------------------
*  {Delete}        | [DEL]
*  ----------------+-------------------------
*  {BS}            | [BACKSPACE]
*  ----------------+-------------------------
*  {BackSpace}     | [BACKSPACE]
*  ----------------+-------------------------
*  {Enter}         | [RETURN]
*  ----------------+-------------------------
*  {Return}        | [RETURN]
*  ----------------+-------------------------
*  {Tab}           | [TAB]
*  ----------------+-------------------------
*  {TabStop}       | [TAB]
*  ----------------+-------------------------
*  {PUp}           | [PRIOR] (page up)
*  ----------------+-------------------------
*  {Prior}         | [PRIOR] (page up)
*  ----------------+-------------------------
*  {PDown}         | [NEXT] (page down)
*  ----------------+-------------------------
*  {Next}          | [NEXT] (page down)
*  ----------------+-------------------------
*  {Pos1}          | [HOME]
*  ----------------+-------------------------
*  {Home}          | [HOME]
*  ----------------+-------------------------
*  {End}           | [END]
*  ----------------+-------------------------
*  {Insert}        | [INS]
*  ----------------+-------------------------
*  {Ins}           | [INS]
*  ----------------+-------------------------
*  {INS}           | [INS]
*  ----------------+-------------------------
*  {Pause}         | [PAUSE]
*  ----------------+-------------------------
*  {Menu}          | [MENU]
*  ----------------+-------------------------
*  {Esc}           | [ESC]
*  ----------------+-------------------------
*  {ESC}           | [ESC]
*  ----------------+-------------------------
*  {Escape}        | [ESC]
*  ----------------+-------------------------
*  {F1}..{F12}     | [F1]..[F12]
*  ----------------+-------------------------
*  {KP_0}..{KP_9}  | [0]..[9] (keypad)
*  ----------------+-------------------------
*  {KP_Enter}      | [ENTER] (keypad)
*  ----------------+-------------------------
*  {KP_Add}        | [+] (keypad)
*  ----------------+-------------------------
*  {KP_Subtract}   | [-] (keypad)
*  ----------------+-------------------------
*  {KP_Multiply}   | [*] (keypad)
*  ----------------+-------------------------
*  {KP_Divide}     | [/] (keypad)
*  ----------------+-------------------------
*  {KP_Separator}  | [,] (keypad)
*/
static VALUE m_simulate(int argc, VALUE argv[], VALUE self)
{
  VALUE rtext, rraw, rcommands, rcommand, rtokens, rscanner, rtemp, rlast_post;
  VALUE args[2];
  int raw = 0;
  int i, length;
  KeyCode keycode;
  Display * p_display;
  
  rb_scan_args(argc, argv, "11", &rtext, &rraw);
  if (RTEST(rraw))
    raw = 1;
  
  /*Ensure we're working with UTF-8-encoded strings*/
  rtext = rb_str_export_to_enc(rtext, rb_utf8_encoding());
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  if (raw) /*Raw string - no special keypresses*/
  {
    /*Convert the string into a sequence of keypress and keyrelease commands*/
    rcommands = convert_rtext_to_rkeycodes(p_display, rtext);
    length = NUM2INT(rb_funcall(rcommands, rb_intern("length"), 0));
    /*Execute the commands*/
    for(i = 0;i < length; i++)
    {
      rcommand = rb_ary_entry(rcommands, i);
      keycode = NUM2INT(rb_ary_entry(rcommand, 0));
      if (RTEST(rb_ary_entry(rcommand, 1))) /*Keypress*/
        XTestFakeKeyEvent(p_display, keycode, True, CurrentTime);
      else /*Keyrelease*/
        XTestFakeKeyEvent(p_display, keycode, False, CurrentTime);
    }
  }
  else /*With special keypresses in braces { and }. */
  {
    /*Ensure that ASCII newline and ASCII tab are treated correctly*/
    rtext = rb_obj_dup(rtext); /*We don't want to change the original string*/
    rb_funcall(rtext, rb_intern("gsub!"), 2, RUBY_UTF8_STR("\n"), RUBY_UTF8_STR("{Return}"));
    rb_funcall(rtext, rb_intern("gsub!"), 2, RUBY_UTF8_STR("\t"), RUBY_UTF8_STR("{Tab}"));
    /*We create a command array here, of form [ [string, bool], [string, bool] ], 
    *where the +string+s are the characters to simulate and +bool+ indicates 
    *wheather +string+ is one special key press or a sequence of normal ones. */
    rtokens = rb_ary_new();
    rlast_post = rtext; /*Needed for the case that no {...}-sequences are in the string*/
    args[0] = rtext; /*We need only one argument here*/
    /*Create a StringScanner object for tokenizing the input string*/
    rscanner = rb_class_new_instance(1, args, rb_const_get(rb_cObject, rb_intern("StringScanner")));
    /*Now scan until no special characters can be found anymore*/
    while ( RTEST(rb_funcall(rscanner, rb_intern("scan_until"), 1, rb_reg_new("(.*?){(\\w+?)}", 13, 0))) )
    {
      /*Get the characters before the {...}-sequence and mark them as normal text*/
      rtemp = rb_ary_new();
      rb_ary_push(rtemp, rb_funcall(rscanner, rb_intern("[]"), 1, INT2FIX(1)));
      rb_ary_push(rtemp, Qfalse);
      rb_ary_push(rtokens, rtemp);
      
      /*Get the characters in the {...}-sequence and mark them as one special char*/
      rtemp = rb_ary_new();
      rb_ary_push(rtemp, rb_funcall(rscanner, rb_intern("[]"), 1, INT2FIX(2)));
      rb_ary_push(rtemp, Qtrue);
      rb_ary_push(rtokens, rtemp);
      
      /*Reset rlast_post; this ensures that we don't ignore text beyond the last {...}-sequence*/
      rlast_post = rb_funcall(rscanner, rb_intern("post_match"), 0);
    }
    /*Append text beyond the last {...}-sequence to the command queue if there's any*/
    if (!RTEST(rb_funcall(rlast_post, rb_intern("empty?"), 0)))
    {
      rtemp = rb_ary_new();
      rb_ary_push(rtemp, rlast_post);
      rb_ary_push(rtemp, Qfalse);
      rb_ary_push(rtokens, rtemp);
    }
    
    /*Now execute the command queue*/
    length = NUM2INT(rb_funcall(rtokens, rb_intern("length"), 0));
    for(i = 0;i < length;i++)
    {
      /*Get the next command, of form [str, bool] where str is the string to simulate and bool wheather it's a special char or not*/
      rtemp = rb_ary_entry(rtokens, i);
      if (RTEST(rb_ary_entry(rtemp, 1))) /*This means we got a special character*/
      {
        keycode = get_keycode(p_display, rb_ary_entry(rtemp, 0));
        XTestFakeKeyEvent(p_display, keycode, True, CurrentTime);
        XTestFakeKeyEvent(p_display, keycode, False, CurrentTime);
      }
      else /*Just normal text*/
      {
        args[0] = rb_ary_entry(rtemp, 0);
        args[1] = Qtrue;
        /*Recursively call this method, sending the normal text with the "raw" parameter set to true*/
        m_simulate(2, args, self);
      }
      /*Ensure that the event(s) got processed before we send more - 
      *otherwise it could happen that in a sequence "ab{BS}c" the [A], [B] and [C] 
      *keypresses are executed before the [BackSpace] press. */
      XSync(p_display, False);
    }
  }
  
  /*Cleanup actions*/
  XSetErrorHandler(handle_x_errors);
  XCloseDisplay(p_display);
  return rtext;
}

/*
*call-seq: 
*  Keyboard.delete( del = false ) ==> nil
*
*Simulates a delete keystroke. 
*===Parameters
*[+del+] (false) If true, [DEL] is simulated. Otherwise, [BACKSPACE]. 
*===Return value
*nil. 
*===Example
*  #Send a [BACKSPACE] keystroke
*  Imitator::X::Keyboard.delete
*  #Send a [DEL] keystroke
*  Imitator::X::Keyboard.delete(true)
*/
static VALUE m_delete(int argc, VALUE argv[], VALUE self)
{
  Display * p_display;
  VALUE del;
  KeyCode keycode;
  
  rb_scan_args(argc, argv, "01", &del);
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  if (RTEST(del))
    keycode = XKeysymToKeycode(p_display, XStringToKeysym("Delete"));
  else
    keycode = XKeysymToKeycode(p_display, XStringToKeysym("BackSpace"));
  
  XTestFakeKeyEvent(p_display, keycode, True, CurrentTime);
  XTestFakeKeyEvent(p_display, keycode, False, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*call-seq: 
*  Keyboard.down( key ) ==> nil
*
*Holds the specified key down. 
*===Parameters
*[+key+] The key to hold down. This cannot be a key combination. 
*===Return value
*nil. 
*===Raises
*[XError] Invalid key specified. 
*===Example
*  #Hold the [A] key for one second down
*  Imitator::X::Keyboard.down("a")
*  sleep 1
*  Imitator::X::Keyboard.up("a")
*  #Hold [SHIFT]+[A] one second down. Since you cannot 
*  #send key combinations with this method, we need more than one call. 
*  Imitator::X::Keyboard.down("Shift")
*  Imitator::X::Keyboard.down("a")
*  sleep 1
*  Imitator::X::Keyboard.up("a") #The release order doesn't matter, but...
*  Imitator::X::Keyboard.up("Shift") #...I like this more. 
*===Remarks
*Don't forget to release the key sometime...
*/
static VALUE m_down(VALUE self, VALUE key)
{
  Display * p_display;
  KeyCode keycode;
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  keycode = get_keycode(p_display, key);
  XTestFakeKeyEvent(p_display, keycode, True, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*call-seq: 
*  Keyboard.up( key ) ==> nil
*
*Releases the specified key.  
*===Parameters
*[+key+] The key to release. This cannot be a key combination. 
*===Return value
*nil. 
*===Raises
*[XError] Invalid key specified. 
*===Example
*  #Hold the [A] key for one second down
*  Imitator::X::Keyboard.down("a")
*  sleep 1
*  Imitator::X::Keyboard.up("a")
*  #Hold [SHIFT]+[A] one second down. Since you cannot 
*  #send key combinations with this method, we need more than one call. 
*  Imitator::X::Keyboard.down("Shift")
*  Imitator::X::Keyboard.down("a")
*  sleep 1
*  Imitator::X::Keyboard.up("a") #The release order doesn't matter, but...
*  Imitator::X::Keyboard.up("Shift") #...I like this more. 
*===Remarks
*You may want to call Keyboard.down before calling this method?
*/
static VALUE m_up(VALUE self, VALUE key)
{
  Display * p_display;
  KeyCode keycode;
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  keycode = get_keycode(p_display, key);
  XTestFakeKeyEvent(p_display, keycode, False, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*call-seq: 
*  Keyboard.method_missing(sym, *args, &block) ==> aString
*  Keyboard.any_undefined_method() ==> aString
*
*Allows you to use shortcuts like <tt>Imitator::X::Keyboard.ctrl_c</tt>. 
*===Return value
*The method name after being subject to some modifications (see _Remarks_). 
*===Raises
*[ArgumentError] Calls with arguments aren't allowed. 
*===Example
*  #Instead of Imitator::X::Keyboard.key("Ctrl+c"): 
*  Imitator::X::Keyboard.ctrl_c
*===Remarks
*The method's name is split up by the underscore _ sign, then each 
*resulting item is capitalized. At last, the items are joined by a plus + sign 
*and send to the Keyboard.key method. 
*This means, you cannot send the plus + or underscore _ signs (and whitespace, of course) 
*this way. 
*/
static VALUE m_method_missing(int argc, VALUE argv[], VALUE self)
{ /*def method_missing(sym, *args, &block)*/
  VALUE rary, rkey;
  int i, length;
  
  if (argc > 1)
    rb_raise(rb_eArgError, "Cannot turn arguments into method calls!");
  
  /*Split the array up by _*/
  rary = rb_str_split(argv[0], "_");
  length = NUM2INT(rb_funcall(rary, rb_intern("length"), 0));
  
  /*Capitalize every key*/
  for(i = 0;i < length;i++)
    rb_funcall(rb_ary_entry(rary, i), rb_intern("capitalize!"), 0);
  /*Join the keys by a plus + sign*/
  rkey = rb_ary_join(rary, RUBY_UTF8_STR("+"));
  
  /*Call Keyboard.key with the joined string*/
  m_key(self, rkey);
  
  return rkey;
}
/*****************Init function***********************/

void Init_keyboard(void)
{
  VALUE hsh;
  VALUE charfile_path;
  
  Keyboard = rb_define_module_under(X, "Keyboard");
  
  /*It would be a very long and unneccassary work to define all the key combinations here. 
  *Instead, to shorten this and because YAML is in the Stdlib, I made a YAML file for this and load 
  *it. This also allows you to modify the alias list at runtime. 
  *Thanks to Ruby 1.9, the Gem module is loaded by default and I can use it to search for my character aliases file. */
  if(RTEST(rb_gv_get("$imitator_x_charfile_path"))) /*This allows to override the default search path - important for debugging where the gem is not installed*/
    charfile_path = rb_str_export_to_enc(rb_gv_get("$imitator_x_charfile_path"), rb_utf8_encoding());
  else
  {
    /*Use Gem.find_files("imitator_x_special_chars.yml") to get the file*/
    charfile_path = rb_funcall(rb_const_get(rb_cObject, rb_intern("Gem")), rb_intern("find_files"), 1, RUBY_UTF8_STR("imitator_x_special_chars.yml"));
    /*Check if we found a file and raise an error if not*/
    if (RTEST(rb_funcall(charfile_path, rb_intern("empty?"), 0)))
      rb_raise(rb_eRuntimeError, "Could not find file 'imitator_x_special_chars.yml'!");
    /*Get the first found file and ensure that we got it in UTF-8*/
    charfile_path = rb_str_export_to_enc(rb_ary_entry(charfile_path, 0), rb_utf8_encoding());
  }
  
  if (RTEST(rb_gv_get("$DEBUG")))
    rb_funcall(rb_mKernel, rb_intern("print"), 3, RUBY_UTF8_STR("Found key combination file at '"), charfile_path, RUBY_UTF8_STR(".'\n"));
  
  /*Actually load the YAML file*/
  hsh = rb_funcall(rb_const_get(rb_cObject, rb_intern("YAML")), rb_intern("load_file"), 1, charfile_path);
  //~ hsh = rb_funcall(rb_const_get(rb_cObject, rb_intern("YAML")), rb_intern("load_file"), 1, rb_ary_entry(charfile_path, 0));
  //~ hsh = rb_funcall(rb_const_get(rb_cObject, rb_intern("YAML")), rb_intern("load_file"), 1, rb_str_new2("/home/marvin/Programmieren/Projekte/Gems/imitator/imitator_x/lib/imitator_x_special_chars.yml"));
  
  
  /*
  *This hash contains key combinations for special characters like the euro sign. 
  *Since they're highly locale dependent, you are encouraged to modify the key sequences 
  *which have been tested on my German keyboard. If you want to change them permanently, 
  *have a look at the "imitator_x_special_chars.yml" file in Imitator for X's gem directory. 
  *See also the description of the Keyboard module for further information. 
  */
  rb_define_const(Keyboard, "SPECIAL_CHARS", hsh);
  
  hsh = rb_hash_new();
  /*Modkey aliases*/
  rb_hash_aset(hsh, RUBY_UTF8_STR("Shift"), RUBY_UTF8_STR("Shift_L"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Control"), RUBY_UTF8_STR("Control_L"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Alt"), RUBY_UTF8_STR("Alt_L"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Ctrl"), RUBY_UTF8_STR("Control_L"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("AltGr"), RUBY_UTF8_STR("ISO_Level3_Shift"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Win"), RUBY_UTF8_STR("Super_L"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Super"), RUBY_UTF8_STR("Super_L"));
  /*Special key aliases*/
  rb_hash_aset(hsh, RUBY_UTF8_STR("Del"), RUBY_UTF8_STR("Delete"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("DEL"), RUBY_UTF8_STR("Delete"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Ins"), RUBY_UTF8_STR("Insert"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("INS"), RUBY_UTF8_STR("Insert"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("BS"), RUBY_UTF8_STR("BackSpace"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Enter"), RUBY_UTF8_STR("Return"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("TabStop"), RUBY_UTF8_STR("Tab"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("PUp"), RUBY_UTF8_STR("Prior"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("PDown"), RUBY_UTF8_STR("Next"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Pos1"), RUBY_UTF8_STR("Home"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("ESC"), RUBY_UTF8_STR("Escape"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("Esc"), RUBY_UTF8_STR("Escape"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("CapsLock"), RUBY_UTF8_STR("Caps_Lock"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("ScrollLock"), RUBY_UTF8_STR("Scroll_Lock"));
  rb_hash_aset(hsh, RUBY_UTF8_STR("NumLock"), RUBY_UTF8_STR("Num_Lock"));
  
  /*
  *This hash defines the aliases for keys. For example, the string "Ctrl" 
  *is mapped to "Control_L" what is the key name in X for the left Ctrl 
  *key. It's just for making life easier. 
  */
  rb_define_const(Keyboard, "ALIASES", hsh);
  
  rb_define_module_function(Keyboard, "key", m_key, 1);
  rb_define_module_function(Keyboard, "simulate", m_simulate, -1);
  rb_define_module_function(Keyboard, "delete", m_delete, -1);
  rb_define_module_function(Keyboard, "down", m_down, 1);
  rb_define_module_function(Keyboard, "up", m_up, 1);
  rb_define_module_function(Keyboard, "method_missing", m_method_missing, -1);
}