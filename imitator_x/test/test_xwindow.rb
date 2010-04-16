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

#Ensure we use the correct key combinations file
$imitator_x_charfile_path = File.join(File.expand_path(File.dirname(__FILE__)), "..", "lib", "imitator_x_special_chars.yml")

require "test/unit"
require_relative "../lib/imitator/x"

class XWindowTest < Test::Unit::TestCase
  
  EDITOR = ["gedit", "kwrite", "mousepad", "kate"].find{|cmd| `which '#{cmd}'`; $?.exitstatus == 0}
  raise("No editor found!") if EDITOR.nil?
  puts "Chosen editor: '#{EDITOR}'." if $VERBOSE
  #~ p  Imitator::X::XWindow.from_title(Regexp.new(Regexp.escape(EDITOR)))
  
  def self.startup
    @@editor_pid = spawn(EDITOR)
    @@xwin = Imitator::X::XWindow.wait_for_window(Regexp.new(Regexp.escape(EDITOR)))
  end
  
  def self.shutdown
    Process.kill("SIGKILL", @@editor_pid)
  end
  
  def setup
    sleep 0.5
  end
  
  def test_activate_unfocus
    @@xwin.activate
    sleep 2
    assert_equal(Imitator::X::XWindow.from_active, @@xwin)
    @@xwin.unfocus
    sleep 1
    assert_not_equal(@@xwin, Imitator::X::XWindow.from_focused)
  end
  
  def test_exists
    assert(@@xwin.exists?)
  end
  
  def test_map
    assert(@@xwin.mapped?)
    @@xwin.unmap
    sleep 1
    assert(!@@xwin.mapped?)
    @@xwin.map
    sleep 1
    assert(@@xwin.mapped?)
  end
  
  def test_parent
    assert_equal(@@xwin.root_win, @@xwin.parent)
  end
  
  def test_pid
    begin
      assert_equal(@@editor_pid, @@xwin.pid)
    rescue NotImplementedError
      notify("Your window manager or your editor doesn't support EWMH _NET_WM_PID.")
    end
  end
  
  def test_resize
    @@xwin.resize(500, 400)
    sleep 1
    assert_equal([500, 400], @@xwin.size)
  end
  
  def test_is_root_win
    assert(Imitator::X::XWindow.default_root_window.root_win?)
  end
  
  def test_is_visible
    assert(@@xwin.visible?)
    @@xwin.unmap
    sleep 1
    assert(!@@xwin.visible?)
    @@xwin.map
    sleep 1
    assert(@@xwin.visible?)
  end
  
end