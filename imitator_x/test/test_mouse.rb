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

class MouseTest < Test::Unit::TestCase
  
  def test_move
    Imitator::X::Mouse.move(100, 100)
    assert_equal([100, 100], Imitator::X::Mouse.pos)
    assert_equal([100, 100], Imitator::X::Mouse.position)
    
  end
  
end