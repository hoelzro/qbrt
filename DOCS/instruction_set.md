# Qbrt Instruction Set Reference

Here are the instructions that qbrt currently supports. You will
notice some inconsistency among instructions. This is temporary
and will be made consistent once the instruction set becomes more
stable.

## String Instructions

String operations are very common so get special support as instructions
for things that might otherwise be implemented as functions.

### stracc

Append one string to another, to easily build up longer strings,
particularly interpolated strings.

Arguments: <base> <postfix>

* **base** the initial string to be appended to
* **postfix** the string to be appended to the base

Example:
```
const $0 "base string"
const $1 " postfixed"
stracc $0 $1
// $0 now contains "base string postfixed"
```

## Concurrency Instructions

### fork

Fork is an interesting instruction. It breaks execution into two
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

Arguments: <reg>

* **reg** the register to store the promised value

Example:
```
fork $0            // put a promise in $0
  const $1 2
  const $2 3
  imult $0 $1 $2   // overwrite the promise in $0 with an actual value
  end.             // the forked execution stops here
const $3 4
wait $0            // wait here until $0 is set
imult $4 $0 $3     // $4 now contains 2 * 3 * 4, or 24
```

### wait

Block the current execution path and wait for another execution
path to set an actual value in the register.

Arguments: <reg>

* **reg** the register that currently contains a promise

See **fork** above for an example.

## Jump Instructions

### goto

Jump to a particular location

Arguments: <label>

* **label** the location to jump to

Example:
```
const $1 "initialized"
goto @label
const $1 "never gets here"
@label
// $1 will always contain "initialized"
```

### brt/brf

Branch if the operand is true or false

Arguments: <op> <label>

* **op** the operand register to test
* **label** the location to jump to if the test is successful

Example:
```
const $1 "initialized"
brt $0 @label
const $1 "no branch"
@label
// if $0 is true, register $1 will now contain "initialized"
// if $0 is false, register $1 will now contain "no branch"
```

### brfail/brnfail

Branch if the operand is or is not a failure

Arguments: <op> <label>

* **op** the operand register to test
* **label** the location to jump to if the test is successful

Example:
```
const $1 "initialized"
brfail $1 @label
const $1 "no branch"
@label
// $1 is not a failure type, register $1 will always contain "no branch"
```

### Comparison Branch Instructions

Compare two operands and branch if the comparison is true

Arguments: <op1> <op2> <label>

* **op1** the first operand in the comparison
* **op2** the second operand in the comparison
* **label** the location to jump to if the test is successful

### breq

*NOTE: not implemented yet*

Jump if the two operands are equal

### brne

Jump if the two operands are equal

## Arithmetic Instructions

Instructions for doing arithmetic operations.

### Binary Operations

Binary operations all have the same arguments: 3 registers, a result
register and 2 operand registers.

Arguments: <result> <op1> <op2>

* **result** the register in which to store the result
* **op1** the register with the first operand
* **op2** the register with the second operand

### addi

Add 2 integers

Example:
```
const $1 5
const $2 7
addi $0 $1 $2   // register $0 will contain integer 12
```

### isub

Subtract the b register from the a register

Example:
```
const $1 9
const $2 4
addi $0 $1 $2   // register $0 will contain integer 5.
```

### imult

Multiply the a register by the b register

Example:
```
const $1 3
const $2 6
imult $0 $1 $2  // register $0 will contain integer 18.
```

### idiv

Divide the a register by the b register

Example:
```
const $1 12
const $2 3
idiv $0 $1 $2   // register $0 will contain integer 4.
```
