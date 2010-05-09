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

require "rake/gempackagetask"
require "rake/clean"
require "rake/testtask"
begin
  require "hanna/rdoctask"
rescue LoadError
  require "rake/rdoctask"
end

$stdout.sync = true

CLEAN.include("ext/*.o")
CLEAN.include("ext/Makefile")
CLEAN.include("ext/mkmf.log")
CLOBBER.include("ext/*.so")

Rake::RDocTask.new do |rt|
  rt.options = ["--charset=ISO-8859-1"] #Needed for correct displaying of chars like ä. 
  rt.rdoc_files.include("README.rdoc", "TODO.rdoc", "COPYING.rdoc", "COPYING.LESSER.rdoc")
  rt.rdoc_files.include("ext/x.c")
  rt.rdoc_files.include("ext/xwindow.c")
  rt.rdoc_files.include("ext/mouse.c")
  rt.rdoc_files.include("ext/clipboard.c")
  rt.rdoc_files.include("ext/keyboard.c")
  rt.rdoc_files.include("lib/imitator/x.rb")
  rt.rdoc_files.include("lib/imitator/x/drive.rb")
  rt.title = "Imitator for X: RDocs"
  rt.main = "README.rdoc"
  rt.rdoc_dir = "doc"
end

spec = Gem::Specification.new do |s|
  s.name = "imitator_x"
  s.summary = "Simulate keyboard and mouse input directly via Ruby to a Linux X server system"
  s.description =<<DESC
The Imitator for X library allows you to simulate key and 
mouse input to any Linux system that uses the X server version 11. 
Features also include CD/DVD drive control and direct interaction 
with Windows, wrapped as Rubyish objects. 
It's a Ruby 1.9-only library. Please also note that this is 
a C extension that will be compiled during installing. 
DESC
  s.add_development_dependency("test-unit", ">= 2.0")
  s.requirements = ["Linux with X11", "eject command (usually installed)"]
  s.version = "0.0.1"
  s.author = "Marvin Gülker"
  s.email = "sutniuq<>gmx<>net"
  s.platform = Gem::Platform::RUBY #BUT it's Linux-only
  s.required_ruby_version = ">=1.9"
  s.files = [Dir["lib/**/*.rb"], Dir["ext/**/**.c"], Dir["ext/**/*.h"], Dir["test/*.rb"], "ext/extconf.rb", "lib/imitator_x_special_chars.yml", "Rakefile.rb", "README.rdoc", "TODO.rdoc", "COPYING.rdoc", "COPYING.LESSER.rdoc"].flatten
  s.extensions << "ext/extconf.rb"
  s.has_rdoc = true
  s.extra_rdoc_files = %w[README.rdoc TODO.rdoc COPYING.rdoc COPYING.LESSER.rdoc ext/x.c ext/xwindow.c ext/mouse.c ext/clipboard.c ext/keyboard.c] #Why doesn't RDoc document the C files automatically?
  s.rdoc_options << "-t" << "Imitator for X: RDocs" << "-m" << "README.rdoc" << "-c" << "ISO-8859-1"
  s.test_files = Dir["test/test_*.rb"]
  #s.rubyforge_project = 
  s.homepage = "http://github.com/Quintus/imitator"
end
Rake::GemPackageTask.new(spec).define

Rake::TestTask.new("test") do |t|
  t.pattern = "test/test_*.rb"
  t.warning = true
end

desc "Compile Imitator for X"
task :compile do 
  Dir.chdir("ext") do
    ruby "extconf.rb"
    sh "make"
  end
end

desc "Compiles, tests and then creates the gem file."
task :default => [:clobber, :compile, :test, :gem]