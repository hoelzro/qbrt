require './crake'
require 'rubygems'
require 'popen4'
include Rake::DSL

QBC = CTarget.new()
QBC.name = 'qbc'
QBC.include 'lib'
QBC.compile_files("lib/asm.cpp", \
		  "lib/core.cpp", \
		  "lib/function.cpp", \
		  "lib/instruction.cpp", \
		  "lib/module.cpp", \
		  "lib/qbparse.c", \
		  "lib/qblex.c", \
		  "lib/stmt.cpp", \
		  "lib/type.cpp", \
		 )
QBC.obj_dir = 'o/qbc'
QBC.debug!
QBCDEPS = ["o","o/qbc","o/qbc/lib","lib/qbparse.h","lib/qbparse.c"]

QBI = CTarget.new()
QBI.name = 'qbi'
QBI.include 'lib'
QBI.compile_files("lib/info.cpp", \
		  "lib/core.cpp", \
		  "lib/function.cpp", \
		  "lib/module.cpp", \
		  "lib/type.cpp", \
		 )
QBI.obj_dir = 'o/qbi'
QBI.debug!

QBRT = CTarget.new()
QBRT.name = 'qbrt'
QBRT.include 'lib'
QBRT.compile_files("lib/main.cpp", \
		  "lib/core.cpp", \
		  "lib/function.cpp", \
		  "lib/io.cpp", \
		  "lib/module.cpp", \
		  "lib/schedule.cpp", \
		  "lib/type.cpp", \
		  )
QBRT.obj_dir = 'o/qbrt'
QBRT.link 'pthread'
QBRT.debug!
QBRT_DIRS = ["o","o/qbrt","o/qbrt/lib"]
LIBQB = ['libqb/core.qb', 'libqb/io.qb']

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
task :compile_qbrt => QBRT_DIRS + QBRT.objects + LIBQB
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

task :libqb => ["libqb/core.qb", "libqb/io.qb"]
rule '.qb' => [ proc { |qb| qb.sub(".qb", ".uqb") } ] do |t|
	src = t.name.sub('libqb/', '').sub('.qb', '.uqb')
	Dir.chdir "libqb/"
	sh "../qbc #{src}"
	Dir.chdir "../"
end

file 'lemon' => ['lempar/lemon.c'] do
	sh "gcc -o lemon lempar/lemon.c"
end

task :clean => [] do
	sh "rm -rf o/"
	sh "rm -f lib/qbparse.h lib/qbparse.c lib/qblex.c"
end


TestFiles = ['hello.uqb',
	'argc.uqb',
	'arithmetic.uqb',
	'badmath.uqb',
	'bool.uqb',
	'echo.uqb',
	'fact.uqb',
	'fork_hello.uqb',
	'listprint.uqb',
	'matchargs.uqb',
	'maybe.uqb',
	'missingmodule.uqb',
	'multimethod.uqb',
	'newproc.uqb',
	'param_types.uqb',
	'polymorph.uqb',
	'struct.uqb',
]

def test_uqb(file)
	passed = false
	Dir.chdir "T/"
	mod = file.chomp(File.extname(file))
	sh "../qbc #{file}"
	Dir.chdir "../"
	input_file = "T/DATA/#{mod}.input"
	args_file = "T/DATA/#{mod}.args"
	if File.exist? args_file
		args = File.read(args_file)
	else
		args = ""
	end
	cmd = "QBPATH=libqb:T ./qbrt #{mod} #{args} 2>&1"
	if File.exist? input_file
		output = `cat #{input_file} | #{cmd}`
	else
		output = `#{cmd}`
	end
	expected = File.read("T/DATA/#{mod}.output")
	if (output != expected)
		puts "Expected output:\n#{expected}..."
		puts "Actual output:\n#{output}..."
		passed = false
	else
		puts "Correct output:\n#{output}..."
		passed = true
	end
	return passed
end

task :T => ['qbc', 'qbrt'] do
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
