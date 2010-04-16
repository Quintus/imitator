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
require "test/unit"
require_relative "../lib/imitator/x"

class KeyboardTest < Test::Unit::TestCase
  
  ASCII_STRING = "The quick brown fox jumped over the lazy dog"
  UTF8_STRING = "ÄÖÜä€öüßabc"
  ESCAPE_STRING = "This has\t2 escseqs: {Tab}!"
  INVALID_STRING = "Incorrect escape: {Nosuchkey}"
  
  EDITOR = ["gedit", "kwrite", "mousepad", "kate"].find{|cmd| `which '#{cmd}'`; $?.exitstatus == 0}
  raise("No editor found!") if EDITOR.nil?
  puts "Chosen editor: '#{EDITOR}'." if $VERBOSE
  
  def self.startup
    @win_proc = spawn(EDITOR)
    sleep 2
  end
  
  def self.shutdown
    Process.kill("SIGKILL", @win_proc)
  end
  
  def teardown
    Imitator::X::Keyboard.ctrl_a
    Imitator::X::Keyboard.delete
  end
  
  def setup
    sleep 0.5
  end
  
  def test_delete
    Imitator::X::Keyboard.simulate(ASCII_STRING)
    sleep 1
    Imitator::X::Keyboard.ctrl_a
    Imitator::X::Keyboard.delete
    assert_equal("", get_text)
    Imitator::X::Keyboard.simulate(ASCII_STRING)
    sleep 1
    Imitator::X::Keyboard.ctrl_a
    Imitator::X::Keyboard.delete(true)
    assert_equal("", get_text)
  end
  
  def test_ascii
    Imitator::X::Keyboard.simulate(ASCII_STRING)
    assert_equal(ASCII_STRING, get_text)
    Imitator::X::Keyboard.delete
    Imitator::X::Keyboard.simulate(ASCII_STRING, true)
    assert_equal(ASCII_STRING, get_text)
  end
  
  def test_utf8
    Imitator::X::Keyboard.simulate(UTF8_STRING)
    assert_equal(UTF8_STRING, get_text)
    Imitator::X::Keyboard.delete
    Imitator::X::Keyboard.simulate(UTF8_STRING, true)
    assert_equal(UTF8_STRING, get_text)
  end
  
  def test_escape
    Imitator::X::Keyboard.simulate(ESCAPE_STRING)
    assert_equal(ESCAPE_STRING.gsub("{Tab}", "\t"), get_text)
    Imitator::X::Keyboard.delete
    assert_raise(Imitator::X::XError){Imitator::X::Keyboard.simulate(INVALID_STRING)}
  end
  
  def test_hold
    Imitator::X::Keyboard.down("a")
    sleep 1
    Imitator::X::Keyboard.up("a")
    assert_block{get_text.size > 1}
  end
  
  def test_key
    Imitator::X::Keyboard.key("Shift+a")
    assert_equal("A", get_text)
  end
  
  private
  
  def get_text
    sleep 1
    Imitator::X::Keyboard.ctrl_a
    sleep 1
    Imitator::X::Clipboard.read(:primary)
  end
  
end