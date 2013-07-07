require './crake'
require 'rubygems'
require 'popen4'
include Rake::DSL

QBC = CTarget.new()
QBC.name = 'qbc'
QBC.include 'lib'
QBC.compile_files("lib/asm.cpp", \
		  "lib/core.cpp", \
		  "lib/qbparse.c", \
		  "lib/qblex.c")
QBC.obj_dir = 'o/qbc'
QBC.debug!
QBCDEPS = ["o","o/qbc","o/qbc/lib","lib/qbparse.h","lib/qbparse.c"]

QBI = CTarget.new()
QBI.name = 'qbi'
QBI.include 'lib'
QBI.compile_files("lib/info.cpp", \
		  "lib/core.cpp", \
		  "lib/module.cpp")
QBI.obj_dir = 'o/qbi'
QBI.debug!

QBRT = CTarget.new()
QBRT.name = 'qbrt'
QBRT.include 'lib'
QBRT.compile_files("lib/main.cpp", \
		  "lib/core.cpp", \
		  "lib/module.cpp", \
		  "lib/schedule.cpp")
QBRT.obj_dir = 'o/qbrt'
QBRT.debug!

PROJECT = CProject.new()
PROJECT.cc = CCompiler.new()
PROJECT.targets = [QBC,QBI,QBRT]


directory 'o'
directory 'o/qbc'
directory 'o/qbc/lib'
directory 'o/qbrt'
directory 'o/qbrt/lib'
directory 'o/qbi'
directory 'o/qbi/lib'

task :default => :all

task :all => ["qbc","qbi","qbrt"]

file "qbc" => :compile_qbc do
	PROJECT.link(QBC)
end
task :compile_qbc => QBCDEPS + QBC.objects

file "qbi" => :compile_qbi do
	PROJECT.link(QBI)
end
task :compile_qbi => ["o","o/qbi","o/qbi/lib"] + QBI.objects
task :qbi_deps do
	QBI.all_dependencies.each { |d| puts d }
end

file "qbrt" => :compile_qbrt do
	PROJECT.link(QBRT)
end
task :compile_qbrt => ["o","o/qbrt","o/qbrt/lib"] + QBRT.objects
task :qbrt_deps do
	QBRT.all_dependencies.each { |d| puts d }
end


rule '.o' => [ proc { |o| PROJECT.dependencies( o ) } ] do |t|
	PROJECT.compile( t.name )
end

file 'lib/qblex.c' => ['lexparse/qb.l','lexparse/qb.y'] do
	sh 'flex -t lexparse/qb.l | sed -r "s/static int yy_start/int yy_start/" > lib/qblex.c'
end

file 'lib/qbparse.h' => ['lib/qbparse.c','lexparse/qb.y']
file 'lib/qbparse.c' => ['lempar/lempar.c', 'lexparse/qb.y'] do
	sh "./lemon -g lexparse/qb.y"
	sh "./lemon -s -Tlempar/lempar.c lexparse/qb.y"
	File.rename('lexparse/qb.h','lib/qbparse.h')
	File.rename('lexparse/qb.c','lib/qbparse.c')
end

file 'lemon' => ['lempar/lemon.c'] do
	sh "gcc -o lemon lempar/lemon.c"
end

task :clean => [] do
	sh "rm -rf o/"
	sh "rm -f lib/qbparse.h lib/qbparse.c lib/qblex.c"
end


TestFiles = ['T/hello.uqb',
	'T/fork_hello.uqb',
	'T/newproc.uqb',
]

def test_uqb(file)
	passed = false
	mod = file.chomp(File.extname(file))
	sh "./qbc #{file}"
	modbase = mod.sub('T/', '')
	output = `cat T/INPUT/#{modbase} | ./qbrt #{mod} 2>&1`
	expected = File.read("T/OUTPUT/#{modbase}")
	if (output != expected)
		puts "Expected output:\n#{expected}..."
		puts "Actual output:\n#{output}..."
		passed = false
	else
		passed = true
	end
	return passed
end

task :T do
	failures = []
	TestFiles.each do |t|
		if not test_uqb t
			failures << t
		end
	end

	if failures.empty?
		puts "No failures"
	else
		puts "Failures:"
		puts failures
	end
end
