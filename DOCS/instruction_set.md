# Qbrt Instruction Set Reference

Here are the instructions that qbrt currently supports. You will
notice some inconsistency among instructions. This is temporary
and will be made consistent once the instruction set becomes more
stable.

## Concurrency Instructions

### fork

## Jump Instructions

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
