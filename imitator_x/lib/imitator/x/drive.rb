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

module Imitator
  
  module X
    
    #This class wraps CD/DVD devices. After creating an object of this class 
    #you can open or close your drives. Also, you're able to lock and unlock them. 
    class XDrive
      include Open3
      
      #The +eject+ command. 
      EJECT = "eject"
      
      #Returns the mount point of the default drive. 
      #===Return value
      #A string describing the default drive. 
      #===Raises
      #[XError] +eject+ command failed. 
      #===Example
      #  p Imitator::X::Drive.default #=> "cdrom"
      def self.default
        err = ""
        out = ""
        Open3.popen3("#{EJECT} -d") do |stdin, stdout, stderr|
          stdin.close_write
          out << stdout.read.chomp
          err << stderr.read.chomp
        end
        raise(XError, err) unless err.empty?
        out.match(/`(.*)'/)[1]
      end
      
      #Creates a new XDrive object. 
      #===Parameters
      #[+drive+] (<tt>XDrive.default()</tt>) The drive to wrap. Either a device file like <tt>/dev/scd1</tt> or a mount point like <tt>cdrom1</tt>. 
      #===Return value
      #The newly created object. 
      #===Raises
      #[ArgumentError] Invalid drive. 
      #===Example
      #  #Get a handle to the default drive
      #  drive = Imitator::X::XDrive.new
      #  #By device file
      #  drive = Imitator::X::XDrive.new("/dev/scd1")
      #  #By mount point
      #  drive = Imitator::X::Drive.new("cdrom1")
      def initialize(drive = XDrive.default)
        @drive = drive
        @locked = false
        raise(ArgumentError, "No such drive: '#{@drive}'!") unless system("#{EJECT} -n #{@drive} > /dev/null 2>&1")
      end
      
      #call-seq: 
      #  eject() ==> nil
      #  open() ==> nil
      #
      #Opens a drive. 
      #===Return value
      #nil. 
      #===Raises
      #[XError] +eject+ command failed. 
      #===Example
      #  drive.eject
      def eject
        err = ""
        popen3(EJECT + " " + @drive) do |stdin, stdout, stderr|
          #stdin.close_write
          err << stderr.read.chomp
        end
        raise(XError, err) unless err.empty?
        nil
      end
      alias open eject
      
      #Closes a drive. 
      #===Return value
      #nil. 
      #===Raises
      #[XError] +eject+ command failed. 
      #===Example
      #  #The drive should be opened first, otherwise this has no effect
      #  drive.eject
      #  drive.close
      #===Remarks
      #Laptop devices usually can't be closed. If you try this, you'll 
      #get an XError saying that an Input/Output error occured. 
      def close
        err = ""
        popen3("eject -t #{@drive}") do |stdin, stdout, stderr|
          stdin.close_write
          err << stderr.read.chomp
        end
        raise(XError, err) unless err.empty?
        nil
      end
      
      #Locks a drive. That means, it can't be opened by 
      #using the eject button. 
      #===Return value
      #nil. 
      #===Example
      #  Imitator::X::Drive.lock!
      def lock!
        err = ""
        popen3("#{EJECT} -i on #{@drive}") do |stdin, stdout, stderr|
          stdin.close_write
          err << stderr.read.chomp
        end
        raise(XError, err) unless err.empty?
        @locked = true
        nil
      end
      
      #Unlocks a drive. That means, it can be opened by the eject 
      #button. 
      #===Return value
      #nil. 
      #===Raises
      #[XError] +eject+ command failed. 
      #===Example
      #  #First, we have to lock a drive. 
      #  Imitator::X::Drive.lock!
      #  Imitator::X::Drive.release!
      def release!
        err = ""
        popen3("#{EJECT} -i off #{@drive}") do |stdin, stdout, stderr|
          stdin.close_write
          err << stderr.read.chomp
        end
        raise(XError, err) unless err.empty?
        @locked = false
        nil
      end
      
      #Checks wheather or not we locked a drive. 
      #===Return value
      #true or false
      #===Example
      #  puts drive.locked?#=> false
      #  drive.lock!
      #  puts drive.locked? #=> true
      #  drive.release!
      #  puts drive.locked? #=> false
      #===Remarks
      #This method works only within one Ruby application - if the drive is locked 
      #from outside, for example by the user typing "eject -i on cdrom1" that won't be 
      #recognized by this method. 
      def locked?
        @locked
      end
      
    end
    
  end
  
end