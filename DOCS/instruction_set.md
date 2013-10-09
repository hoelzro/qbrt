# Qbrt Instruction Set Reference

Here are the instructions that qbrt currently supports. You will
notice some inconsistency among instructions. This is temporary
and will be made consistent once the instruction set becomes more
stable.

There is almost (but not quite) a one to one match between
the qbrt instructions that you write and the compiled instructions that
the virtual machine executes. An example of this is the ```const```
instruction which compiles to type specific instructions
(eg. int or string).

## String Instructions

String operations are very common so get special support as instructions
for things that might otherwise be implemented as functions.

### stracc

Append one string to another, to easily build up longer strings,
particularly interpolated strings.

Arguments: &lt;base&gt; &lt;postfix&gt;

* **base** the initial string to be appended to
* **postfix** the string to be appended to the base

Example:
```
const $0 "base string"
const $1 " postfixed"
stracc $0 $1
## $0 now contains "base string postfixed"
```

### const

The const instruction puts a constant string into a given register.

The const instruction can also load an integer into a register. In
the compiled code, const string and const int are actually implemented
with two different instructions.

Arguments: &lt;dst&gt; &lt;value&gt;

* **dst** the register where the constant will be stored
* **value** the value to store, can be a string or an int

Example:
```
const $0 "this is a string"
const $1 5
```

## Function Instructions

Functions are a necessary construct for abstracting and reusing
functionality. Here is an example and the related functions.

Example:
```
func decrement
dparam x core/Int
const $y 1
isub \result $x $y
end.

func __main
lfunc $d ./decrement ## $d contains the "decrement" function
const $d.0 5
call $x $d           ## $x should now contain 4
end.
```

### func

Declare a function with a given name and number of parameters.
Function declarations begin a function block and must be ended
with an end statement.

Arguments: &lt;name&gt; &lt;return type&gt;

* **name** a string for the function name
* **return type** the full type name of the type that this function returns

### lfunc

Load a function from the given module with the given function name and
store it in the destination register.

Arguments: &lt;dst&gt; &lt;full function name&gt;

* **dst** the register where the function should be loaded
* **full function name** the module & function to be loaded eg. module/function

### call

Call a function that has been loaded into a register and store its
result in a different register.

Arguments: &lt;result&gt; &lt;function&gt;

* **result** the register where the function result should be stored
* **function** the function to call, with parameters initialized

## Concurrency Instructions

### fork

Fork is a fun instruction. It breaks execution into two
paths so that a value can be determined asynchronously. In the main
path, the value being determined is set as a promise and the main
path should wait on this register before using it. The second path
executes asynchronously, and needs to set the promised register to
a real value so that the main path can resume execution.
This explanation could use a graphic.

Fork is the simple version of a class of concurrent and asynchronous
instructions that will eventually be implemented. This version is
asynchronous only and will not result in the second path executing
on a different CPU.

Arguments: &lt;reg&gt;

* **reg** the register to store the promised value

Example:
```
fork $0            ## put a promise in $0
  const $1 2
  const $2 3
  imult $0 $1 $2   ## overwrite the promise in $0 with an actual value
  end.             ## the fork instruction starts a block that stops here
                   ## with the end. statement
const $3 4
imult $4 $0 $3     ## VM waits here until $0 is ready
                   ## $4 now contains 2 * 3 * 4, or 24
```

### newproc

Launch a new process that executes the given function. Communication to
and from the new process should happen via message passing using
the send function and recv instruction.

Arguments:

* **pid** the register where the pid of the new process should be stored
* **function** the function to

Example:
```
lfunc $0 ./foo
newproc $2 $0	## create a new process executing foo()

lfunc $4 core/send
copy $4.0 $2
const $4.1 "hello"
call void $4

lfunc $3 io/print
recv $3.0	## wait here until a message arrives, then store it in $3.0
call void $3	## print the value received from function foo()
```

### recv

Look for a message on the process's inbound message queue.

Arguments:

* **dest** the register to store the incoming value

Example:
```
lfunc $3 io/print
lfunc $0 ./foo
newproc $2 $0	## create a new process executing foo()
recv $3.0	## wait here until a message arrives, then store it in $3.0
call void $3	## print the value received from function foo()
```

## Jump Instructions

### goto

Jump to a particular location

Arguments: &lt;label&gt;

* **label** the location to jump to

Example:
```
const $1 "initialized"
goto @label
const $1 "never gets here"
@label
## $1 will always contain "initialized"
```

### if/ifnot

if: continue if the operand is true, else jump to the label
ifnot: continue if the operand is *not* true, else jump to the label

Arguments: &lt;op&gt; &lt;label&gt;

* **op** the operand register to test
* **label** the location to jump to based on the result of the test

Example:
```
const $s "initialized"
if $c @label
const $s "condition is true"
@label
## if $c is true, register $s will now contain "condition is true"
## if $c is false, register $s will still contain "initialized"
```

### iffail/ifnotfail

iffail: continue if the operand is a failure, else jump to the label
ifnotfail: continue if the operand is *not* a failure, else jump to the label

Arguments: &lt;op&gt; &lt;label&gt;

* **op** the operand register to test
* **label** the location to jump to based on the result of the test

Example:
```
const $1 "initialized"
iffail $1 @label
const $1 "no branch"
@label
## $1 is not a failure type, register $1 will always contain "no branch"
```

### Comparison Branch Instructions

Compare two operands and branch if the comparison is true

Arguments: &lt;op1&gt; &lt;op2&gt; &lt;label&gt;

* **op1** the first operand in the comparison
* **op2** the second operand in the comparison
* **label** the location to jump to if the test is successful

## Arithmetic Instructions

Instructions for doing arithmetic operations.

### Binary Operations

Binary operations all have the same arguments: 3 registers, a result
register and 2 operand registers.

Arguments: &lt;result&gt; &lt;op1&gt; &lt;op2&gt;

* **result** the register in which to store the result
* **op1** the register with the first operand
* **op2** the register with the second operand

### iadd

Add 2 integers

Example:
```
const $1 5
const $2 7
iadd $0 $1 $2   ## register $0 will contain integer 12
```

### isub

Subtract the b register from the a register

Example:
```
const $1 9
const $2 4
isub $0 $1 $2   ## register $0 will contain integer 5.
```

### imult

Multiply the a register by the b register

Example:
```
const $1 3
const $2 6
imult $0 $1 $2  ## register $0 will contain integer 18.
```

### idiv

Divide the a register by the b register

Example:
```
const $1 12
const $2 3
idiv $0 $1 $2   ## register $0 will contain integer 4.
```
