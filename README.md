# Qbrt, bytecode

Qbrt is a concurrent bytecode assembly language and runtime interpreter.
It's purpose is to provide flexible, accessible runtime features
and a common runtime environment to high level programming language.
By doing so, qbrt aims to reduce the barrier to entry for language
designers to build more concurrent and interoperable programming
languages that take advantage of multiprocessor systems.

## Hello World Example

The obligatory hello world example, written in qbrt.
It's longer than most other languages, but remember, qbrt is
a bytecode assembly language that's designed to be easy for
computer programs to write, at the expense of some human
usability.

```
dfunc "__main" 0
lfunc $1 "io" "print"
const $1.0 "hello world\n"
call void $1
return
end.
```

### Documentation

* [Instruction Set Reference](DOCS/instruction_set.md)
* [Registers](DOCS/registers.md)
* [Core Modules](DOCS/core_modules.md)

## Features

Why might you find qbrt interesting? Well first, it probably helps
if you're interested in designing a high level programming
language yourself.

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

The compiler takes a .uqb assembly code file as input and writes a
corresponding binary bytecode file (.qb) as output.

```> qbc T/hello.uqb```

### The inspector

The inspector is used primarily as a development tool for the compiler.
It takes a compiled .qb file and displays information about
various resources included in the file and all the code.

```> qbi T/hello```

### The interpreter

The interpreter takes a compiled .qb file and executes it. This is
where all of the runtime

```> qbrt T/hello```

### Build Dependencies

To build the components of qbrt, you'll need a few things:

* gcc
* rake
