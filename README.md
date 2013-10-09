# Qbrt, bytecode

Qbrt is a concurrent bytecode assembly language and runtime interpreter.
It's aim is to reduce the barrier to entry for language
designers to build more concurrent and interoperable programming
languages that take advantage of multiprocessor systems.
It does so by making it easy to access and use advanced features
and a common runtime environment.

## Hello World Example

The obligatory hello world example, written in qbrt, and Swedish.
It's longer than most other languages, but remember, qbrt is
a bytecode assembly language that's designed to be easy for
computer programs to write, at the expense of some human
usability.

```
func __main
lfunc $p io/print
const $p.0 "hallå världen\n"
call \void $p
end.
```

### Documentation

* [Instruction Set Reference](DOCS/instruction_set.md)
* [Registers](DOCS/registers.md)
* [Core Modules](DOCS/core_modules.md)

Run unit tests in testlib/ with ```rake unit```.
Run integration tests in T/ with ```rake T```.

*NOTE*: If you want to run the unit tests, make sure to run `git submodule init`
and `git submodule update` to grab the accertion submodule, upon which the
unit tests depend.

## Features

Why might you find qbrt interesting?

1. You're implementing a language and don't want to implement
   qbrt features yourself, like concurrency, asynchronous I/O and
   memory management.
2. You're looking for a runtime environment that takes advantage
   of multiple core processors.

### Dual Band Variables

Expecting and recovering from failure is a necessary of good software.
Qbrt pays extra attention to failured operations so developers can
worry about them only when necessary.

### Concurrency

Now that the free lunch is over, programs must improve their
performance by taking advantage of more processors: concurrency.
Qbrt provides high level concurrency primitives and runtime scheduling
so that you as the language designer can focus on human interfaces to
your language.

As an interpreter, straight line, single threaded speed is not
currently a strong suit of qbrt. JIT compiled execution
is a potential improvement here, but for now qbrt aims to get
its performance by taking advantage of multiple CPUs.

### Inline Asynchronous I/O

As a language designer, qbrt makes I/O asynchronous for you.
There's no need to convert inline code into callbacks, or even worse,
force your users to write their code using callbacks.

### Multiple Dispatch Polymorphism

Traditional object oriented languages bind their polymorphic behavior
to individual types. This restricts the ability of the programmer
to create and use polymorphic behavior based on the distinct types of 
two or more parameters.

### Pattern Matching Primitives

Declarative style pattern matching is a tremendously useful feature
for languages that aligns well with the inate human ability to see
and identify visual patterns. Even C supports limited pattern
matching via the switch statement. Pattern matching is made easy
in qbrt with special byte code instructions to compare function
parameters or local tuples to user defined patterns.

### Common Runtime Environment

Seamless interaction between programming languages is a great benefit. By
implementing your language in a common runtime environment, you get ready
access to the variety of libraries available from other languages that
already run in that environment.

## Usage

qbrt is currently composed of 3 executables:

* The compiler: qbc
* The inspector: qbi
* The interpreter: qbrt

These tools will eventually be built as embedable libraries
but at this stage of development, they exist only as standalone
executables.

### The compiler

The compiler takes a module name as input, converts it to the
uncompiled source file <module>.uqb and compiles a
corresponding binary bytecode file (.qb) as output.

```> QBPATH=libqb:T ./qbc hello```

### The inspector

The inspector is used primarily as a development tool for the compiler.
It takes a compiled .qb file and displays information about
various resources included in the file and all the code.

```> QBPATH=libqb:T ./qbi hello```

### The interpreter

The interpreter takes a compiled .qb file and executes it. This is
where all of the runtime

```> QBPATH=libqb:T ./qbrt hello```

### Build Dependencies

To build the components of qbrt, you'll need a few things:

* gcc
* rake
* rubygem popen4

## Client Languages

Looking for a client language that compiles to qbrt bytecode?

* [Jaz](http://github.com/mdg/jaz)
