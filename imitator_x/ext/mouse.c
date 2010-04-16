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
#include "mouse.h"

/*
*Document-module: Imitator::X::Mouse
*This module allows you to interact with the mouse pointer. You can do funny things 
*with this, but make sure, it's *useful* for your users. 
*
*Every method besides Mouse.move that claims to move the cursor to a specified 
*position uses Mouse.move internally; so make sure you've read Mouse.move's documentation. 
*/

/*
*Retrieves the current mouse cursor position. 
*===Return value
*The cursor position as a 2-element array of form <tt>[x, y]</tt>. 
*===Example
*  p Imitator::X::Mouse.position #=> [464, 620]
*/
static VALUE m_pos(VALUE self)
{
  Display * p_display;
  Window root, child_win;
  int rx, ry, wx, wy;
  unsigned int mask;
  VALUE pos = rb_ary_new();
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  root = XDefaultRootWindow(p_display);
  if (XQueryPointer(p_display, root, &root, &child_win, &rx, &ry, &wx, &wy, &mask) == False)
    rb_raise(XError, "Could not query the pointer's position!");
  rb_ary_push(pos, INT2NUM(rx));
  rb_ary_push(pos, INT2NUM(ry));
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  
  return pos;
}

/*
*call-seq: 
*  Mouse.move(x, y [, step = 1 [, set = false ] ] ) ==> anArray
*
*Moves the mouse cursor to the specified position. 
*===Parameters
*[+x+] The goal X coordinate. 
*[+y+] The goal Y coordinate. 
*[+step+] (1) This specifies the move amount per computation. Higher values make the movement faster, but more inaccurate. Anyway, the cursor will be exactly at your specified position. 
*[+set+] (+false+) If this is +true+, the cursor isn't moved to the goal position, but directly set to it. Ignores +step+ if set to +true+. 
*===Return value
*The new position. 
*===Raises
*[ArgumentError] Besides the normal meanings of this error, it may also indicate that the +step+ parameter has to be greater than 0. 
*===Example
*  #Move to (100|100)
*  Imitator::X::Mouse.move(100, 100) #| [100, 100]
*  #Move to (0|0), only executing every 3rd movement; 
*  #Altough this would never trigger the origin, the cursor 
*  #will be there. 
*  Imitator::X::Mouse.move(0, 0, 3) #| [0, 0]
*  #Don't move, directly set the cursor. The step parameter (1 here) is ignored. 
*  Imitator::X::Mouse.move(100, 100, 1, true) #| [100, 100]
*  #If you move off the screen, the cursor will stop at the edge. 
*  Imitator::X::Mouse.move(-100, 100) #| [0, 100]
*===Remarks
*If you specify a +step+ parameter that is greater than the difference of the actual cursor position to 
*the goal position, you'll get an infinite loop here, so be careful. If you're in doubt on how high 
*you should set +step+, just stick to the directly setting variant. 
*/
static VALUE m_move(int argc, VALUE argv[], VALUE self)
{
  VALUE rx, ry, rstep, rset, temp;
  VALUE args[4];
  Display * p_display;
  int goal_x, goal_y, step, ix, iy, dir;
  
  rb_scan_args(argc, argv, "22", &rx, &ry, &rstep, &rset);
  
  if (NIL_P(rstep))
    step = 1;
  else
    step = NUM2INT(rstep);
  goal_x = NUM2INT(rx);
  goal_y = NUM2INT(ry);
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  /*Ignore the rest of the function if we only want to set the cursor*/
  if (RTEST(rset))
  {
    XTestFakeMotionEvent(p_display, 0, goal_x, goal_y, CurrentTime);
    XSetErrorHandler(NULL);
    XCloseDisplay(p_display);
    return m_pos(self);
  }
  
  temp = m_pos(self); /*Get actual position, since we don't want to move off (0|0).*/
  ix = NUM2INT(rb_ary_entry(temp, 0));
  iy = NUM2INT(rb_ary_entry(temp, 1));
  
  if (step <= 0)
    rb_raise(rb_eArgError, "The step parameter has to be greater than 0!");
  
  /*Get the move direction, which indicates wheather we will increment or decrement the X and Y coordinates. */
  if (ix < goal_x)
  {
    if (iy < goal_y)
      dir = 0; /*Right-down*/
    else
      dir = 1; /*Right-up*/
  }
  else
  {
    if (iy < goal_y)
      dir = 2; /*Left-down*/
    else
      dir = 3; /*Left-up*/
  }
  
  /*Perform the Y movement*/
  while ((iy < goal_y - step) || (iy > goal_y + step)) /*Got as near as possible to the goal Y*/
  {
    args[0] = INT2NUM(ix);
    args[1] = INT2NUM(iy);
    args[2] = rstep;
    args[3] = Qtrue;
    m_move(4, args, self); /*Needed, since even a sleep() call here doesn't cause X to update the cursor position when trying to use XTestFakeMotionEvent here*/
    if (dir == 0 || dir == 2)
      iy += step;
    else
      iy -= step;
  }
  /*Perform the X movement*/
  while ((ix < goal_x - step) || (ix > goal_x + step)) /*Got as near as possible to the goal X*/
  {
    args[0] = INT2NUM(ix);
    args[1] = INT2NUM(iy);
    args[2] = rstep;
    args[3] = Qtrue;
    m_move(4, args, self); /*Needed, since even a sleep() call here doesn't cause X to update the cursor position when trying to use XTestFakeMotionEvent here*/
    if (dir == 0 || dir == 1)
      ix += step;
    else
      ix -= step;
  }
  
  /*Ensure that the cursor is really at the correct position*/
  XTestFakeMotionEvent(p_display, 0, goal_x, goal_y, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return m_pos(self);
}

/*
*call-seq: 
*  Mouse.click(hsh = {:button => :left}) ==> anArray
*
*Executes a mouse click at the current or a given position. 
*===Parameters
*This method takes a hash as it's only argument. You may pass the following keys and values: 
*[:x] (Current X) The X coordinate where you want to click. 
*[:y] (Current Y) The Y coordinate where you want to click. 
*[:button] (:left) The mouse button you want to click with. One of :left, :right and :middle. 
*[:step] (1) +step+ parameter for Mouse.move. 
*[:set] (false) +set+ parameter for Mouse.move. 
*===Return value
*The position where the click was executed. 
*===Example
*  #Click at current position
*  Imitator::X::Mouse.click #| [376, 509]
*  #Click at (100|100)
*  Imitator::X::Mouse.click(:x => 100, :y => 100) #| [100, 100]
*  #Click at current position with right mouse button
*  Imitator::X::Mouse.click(:button => :right)
*/
static VALUE m_click(int argc, VALUE argv[], VALUE self)
{
  VALUE hsh, rbutton, rbutton2;
  VALUE args[4];
  int button;
  Display * p_display;
  
  rb_scan_args(argc, argv, "01", &hsh);
  
  if (NIL_P(hsh))
    hsh = rb_hash_new();
  
  rbutton = rb_hash_lookup(hsh, ID2SYM(rb_intern("button")));
  
  if (NIL_P(rbutton))
    rbutton = ID2SYM(rb_intern("left")); /*No button specified, defaulting to left*/
  rbutton2 = rb_hash_lookup(rb_const_get(Mouse, rb_intern("BUTTONS")), rbutton);
  if (NIL_P(rbutton2))
    rb_raise(rb_eArgError, "Invalid button specified!");
  button = FIX2INT(rbutton2);
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  /*Move the cursor if wanted before clicking*/
  args[0] = rb_hash_lookup(hsh, ID2SYM(rb_intern("x")));
  args[1] = rb_hash_lookup(hsh, ID2SYM(rb_intern("y")));
  args[2] = rb_hash_lookup(hsh, ID2SYM(rb_intern("step")));
  args[3] = rb_hash_lookup(hsh, ID2SYM(rb_intern("set")));
  if (!NIL_P(args[0]) && !NIL_P(args[1]))
    m_move(4, args, self);
  
  XTestFakeButtonEvent(p_display, (unsigned int)button, True, CurrentTime);
  XTestFakeButtonEvent(p_display, (unsigned int)button, False, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return m_pos(self);
}

/*
*call-seq: 
*  Mouse.down( [ button = :left ] ) ==> nil
*
*Holds the given mouse button down. Don't forget to release it sometime. 
*===Parameters
*[+button+] (:left) The button to hold down. 
*===Return value
*nil. 
*===Example
*  Imitator::X::Mouse.down
*  Imitator::X::Mouse.up #Never forget to...
*  Imitator::X::Mouse.down(:right)
*  Imitator::X::Mouse.up(:right) #...Release a pressed button. 
*/
static VALUE m_down(int argc, VALUE argv[], VALUE self)
{
  VALUE rbutton;
  int button;
  Display * p_display;
  
  rb_scan_args(argc, argv, "01", &rbutton);
  if (NIL_P(rbutton))
    rbutton = ID2SYM(rb_intern("left"));
  rbutton = rb_hash_lookup(rb_const_get(Mouse,  rb_intern("BUTTONS")), rbutton);
  if (NIL_P(rbutton))
    rb_raise(rb_eArgError, "Invalid button specified!");
  button = FIX2INT(rbutton);
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  XTestFakeButtonEvent(p_display, (unsigned int)button, True, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*call-seq: 
*  Mouse.up( [ button = :left ] ) ==> nil
*
*Releases the given mouse button. This doesn't have any effect if 
*you didn't call Mouse.down before. 
*===Parameters
*[+button+] (:left) The button to release. 
*===Return value
*nil. 
*===Example
*  Imitator::X::Mouse.down
*  Imitator::X::Mouse.up
*  Imitator::X::Mouse.down(:right)
*  Imitator::X::Mouse.up(:right)
*/
static VALUE m_up(int argc, VALUE argv[], VALUE self)
{
  VALUE rbutton;
  int button;
  Display * p_display;
  
  rb_scan_args(argc, argv, "01", &rbutton);
  if (NIL_P(rbutton))
    rbutton = ID2SYM(rb_intern("left"));
  rbutton = rb_hash_lookup(rb_const_get(Mouse,  rb_intern("BUTTONS")), rbutton);
  if (NIL_P(rbutton))
    rb_raise(rb_eArgError, "Invalid button specified!");
  button = FIX2INT(rbutton);
  
  p_display = XOpenDisplay(NULL);
  XSetErrorHandler(handle_x_errors);
  
  XTestFakeButtonEvent(p_display, (unsigned int)button, False, CurrentTime);
  
  XSetErrorHandler(NULL);
  XCloseDisplay(p_display);
  return Qnil;
}

/*
*call-seq: 
*  Mouse.drag(hsh) ==> anArray
*
*Executes a drag&drop operation. 
*===Parameters
*This method takes a hash as it's only parameter. You may pass in one of the following keys: 
*[+x1+] (Current X) Start X coordinate. 
*[+y1+] (Current Y) Start Y coordinate. 
*[+x2+] (*Required*) Goal X coordinate. 
*[+y2+] (*Required*) Goal Y coordinate. 
*[+button+] (<tt>:left</tt>) The button to hold down during the movement. 
*[+step+] +step+ parameter to Mouse.move. 
*[+set+] +set+ parameter to Mouse.move. Use not recommanded here. 
*===Return value
*Returns the new cursor position as a two-element array of form <tt>[x, y]</tt>. 
*===Example
*  #From current to (200|200)
*  Imitator::X::Mouse.drag(:x2 => 200, :y2 => 200)
*  #From current to (200|200), dragging with the right mouse button
*  Imitator::X::Mouse.drag(:x2 => 200, :y2 => 200, :button => :right)
*  #From (300|300) to (200|200)
*  Imitator::X::Mouse.drag(:x1 => 300, :y1 => 300, :x2 => 200, :y2 => 200)
*  #From (CURRENT X|300) to (200|200)
*  Imitator::X::Mouse.drag(:y1 => 300, :x2 => 200, :y2 => 200)
*/
static VALUE m_drag(VALUE self, VALUE hsh)
{
  VALUE rx1, ry1, rx2, ry2, rbutton, rstep, rset, temp;
  VALUE args[4];
  
  rx1 = rb_hash_lookup(hsh, ID2SYM(rb_intern("x1")));
  ry1 = rb_hash_lookup(hsh, ID2SYM(rb_intern("y1")));
  rx2 = rb_hash_lookup(hsh, ID2SYM(rb_intern("x2")));
  ry2 = rb_hash_lookup(hsh, ID2SYM(rb_intern("y2")));
  rbutton = rb_hash_lookup(hsh, ID2SYM(rb_intern("button")));
  rstep = rb_hash_lookup(hsh, ID2SYM(rb_intern("step")));
  rset = rb_hash_lookup(hsh, ID2SYM(rb_intern("set")));
  
  temp = m_pos(self);
  if (NIL_P(rx1))
    rx1 = rb_ary_entry(temp, 0);
  if (NIL_P(ry1))
    ry1 = rb_ary_entry(temp, 1);
  if (NIL_P(rx2))
    rb_raise(rb_eArgError, "No goal X coordinate specified!");
  if (NIL_P(ry2))
    rb_raise(rb_eArgError, "No goal Y coordinate specified!");
  if (NIL_P(rbutton))
    rbutton = ID2SYM(rb_intern("left"));
  /*If rstep or rset is ommited, it's nil, and this is OK*/
  
  args[0] = rx1;
  args[1] = ry1;
  args[2] = rstep;
  args[3] = rset;
  m_move(4, args, self);
  args[0] = rbutton;
  args[1] = 0;
  args[2] = 0;
  args[3] = 0;
  m_down(1, args, self);
  args[0] = rx2;
  args[1] = ry2;
  args[2] = rstep;
  args[3] = rset;
  m_move(4, args, self);
  args[0] = rbutton;
  args[1] = 0;
  args[2] = 0;
  args[3] = 0;
  m_up(1, args, self);
  
  return m_pos(self);
}

/*
*call-seq: 
*  Mouse.wheel(direction [, amount = 1 ] ) ==> nil
*
*Rolls the mouse wheel. 
*===Parameters
*[+direction+] The direction in which to roll the mouse wheel, either <tt>:up</tt> or <tt>:down</tt>. 
*[+amount+] (1) The amount of roll steps. This <b>does not</b> mean full turns!
*===Return value
*nil. 
*===Example
*  #1 roll step up. 
*  Imitator::X::Mouse.wheel(:up)
*  #2 roll steps down. 
*  Imitator::X::Mouse.wheel(:down, 2)
*/
static VALUE m_wheel(int argc, VALUE argv[], VALUE self)
{
  VALUE rdir, ramount;
  VALUE args[1];
  int amount, i;
  
  rb_scan_args(argc, argv, "11", &rdir, &ramount);
  if (NIL_P(ramount))
    ramount = INT2FIX(1);
  if ( (SYM2ID(rdir) != rb_intern("up")) && (SYM2ID(rdir) != rb_intern("down")) )
    rb_raise(rb_eArgError, "Invalid wheel direction specified!");
  amount = NUM2INT(ramount);
  
  args[0] = rb_hash_new();
  rb_hash_aset(args[0], ID2SYM(rb_intern("button")), rdir);
  
  for(i = 0; i < amount; i++)
    m_click(1, args, self);
  
  return Qnil;
}

/******************************Init-Function*********************************/

void Init_mouse(void)
{
  VALUE hsh;
  Mouse = rb_define_module_under(X, "Mouse");
  hsh = rb_hash_new();
  rb_hash_aset(hsh, ID2SYM(rb_intern("left")), INT2FIX(1));
  rb_hash_aset(hsh, ID2SYM(rb_intern("middle")), INT2FIX(2));
  rb_hash_aset(hsh, ID2SYM(rb_intern("right")), INT2FIX(3));
  rb_hash_aset(hsh, ID2SYM(rb_intern("up")), INT2FIX(4)); /*Yeah, the two wheel directions... */
  rb_hash_aset(hsh, ID2SYM(rb_intern("down")), INT2FIX(5)); /*...are handled as buttons by X. */
  /*A hash mapping button names to internal numbers. */
  rb_define_const(Mouse, "BUTTONS", hsh);
  
  rb_define_module_function(Mouse, "position", m_pos, 0);
  rb_define_module_function(Mouse, "pos", m_pos, 0);
  rb_define_module_function(Mouse, "move", m_move, -1);
  rb_define_module_function(Mouse, "click", m_click, -1);
  rb_define_module_function(Mouse, "down", m_down, -1);
  rb_define_module_function(Mouse, "up", m_up, -1);
  rb_define_module_function(Mouse, "drag", m_drag, 1);
  rb_define_module_function(Mouse, "wheel", m_wheel, -1);
}