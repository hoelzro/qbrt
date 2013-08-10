# Qbrt Registers

Qbrt is a hybrid register/stack virtual machine. Each function in
the call stack has the necessary registers allocated when the
function is loaded.

There are 4 types of registers:
* primary
* secondary
* common system constants
* special registers

This stretches the meaning of "register" a bit so requires
some explanation.

## Primary Registers

Primary registers are what most people mean when they say registers.
They are an indexed place in the virtual machine that instructions
can directly access and operate on directly.

In qbrt, primary registers are accessed with a dollar sign followed
by an integer (eg $5). The primary registers will be reindexed during
compilation so do not expect the integers to remain the same.

Function parameters are a subtype of primary register and are accessed
from within the function with a percent sign followed by an integer
(eg %0). Parameters are not reindexed so user languages must use the
correct indexes for function parameters. $ and % registers have a
different index space so that $0 and %0 can both be used without
conflict.

There is currently an upper limit of 128 for
the maximum number of combined function parameters and registers
available in a single function.

```
const $1 3      // Load constant 3 into direct register 1
imult $0 %0 $1  // Multiple parameter 0 and primary register 1,
                // storing the result in primary register 0
```

## Secondary Registers

Secondary registers are used primarily to pass values from a calling
function into the primary register set of the function being called.
They are accessed with a dollar sign followed by a decimal number
(eg. $4.1). The integer before the decimal is the index of the primary
register that stores the loaded function. The integer after the decimal
is the index of the function parameter.

```
lfunc $0 ./foo
const $0.0 3   // Load constant 3 into parameter 0 of function "foo"
const $0.1 8   // Load constant 8 into parameter 1 of function "foo"
call $1 $0
```

## Const Registers

Some values are used frequently enough that having direct access to them
is beneficial. Three examples of these are void, true and false. In these
cases, the user language can simply write the supported constant where
the register would be.

## Special Registers

Special registers are a way for a qbrt program to get direct access
into data in the virtual machine. The most often used example of this
is the "result" register which contains the result of a function.

```
const $0 2
const $1 3
imult result $0 $1  // assigns the product of 2 and 3 to the result register
```
