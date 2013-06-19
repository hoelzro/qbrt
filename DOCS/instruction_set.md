# Qbrt Instruction Set Reference

Here are the instructions that qbrt currently supports. You will
notice some inconsistency among instructions. This is temporary
and will be made consistent once the instruction set becomes more
stable.

## Arithmetic Instructions

Instructions for doing arithmetic operations.

### Binary Operations

Binary operations all have the same arguments: 3 registers, a result
register and 2 operand registers.

Arguments: <result> <opa> <opb>

result - the register in which to store the result
opa - the register with the first operand
opb - the register with the second operand

### addi

Add 2 integers

Example:
```
const $1 5
const $2 7
addi $0 $1 $2
```
After execution, register $0 will contain integer 12.

### isub

Subtract the b register from the a register

Example:
```
const $1 9
const $2 4
addi $0 $1 $2
```
After execution, register $0 will contain integer 5.

### imult

Multiply the a register by the b register

Example:
```
const $1 3
const $2 6
imult $0 $1 $2
```
After execution, register $0 will contain integer 18.

### idiv

Divide the a register by the b register

Example:
```
const $1 12
const $2 3
idiv $0 $1 $2
```
After execution, register $0 will contain integer 4.
