The Forf Language
=================

Welcome to Forf!  *PUNCH*

Forf is a simple, postfix, stack-based programming language.  It was
designed to be used as an extension language for games.  Forf programs
run in fixed size memory and can be time-constrained.

Forf was heavily influenced by the PostScript programming language.


About Stacks
============

Central to the operation of Forf is the notion of a stack.  A stack is a
last-in-first-out (LIFO) list, like a stack of dishes.  Items can be
"pushed" onto the top of the stack, or "popped" off the top of the stack. 
Items may only be pushed or popped, meaning the top stack element is the
only element that can be accessed.  To get to the third item down in a
stack, it is necessary to first pop two items off.


Data Types
==========

There are three data types in Forf: Integers, Substacks, and Procedures.


Integers
--------

Integers are stored as the "long" type and have whatever boundaries the
host CPU and C compiler enforce.  They may be entered in decimal (12),
octal (014), or hex (0xC), and may be positive or negative.

The following are valid integers:

* 58
* -58
* 0x3A (hex)
* 072  (octal)


Procedures
----------

Data is read one by one from your program and either pushed onto the
data stack (integers and substacks) or evaluated (procedures).  When a
procedures is evaluated, it pops zero or more elements off the data
stack, does something with them, and then pushes zero or more elements
back onto the stack.

The "+" procedure, for instance, pops two values, adds them, and pushes
the result.  The following data stack:

    [bottom] 58 88 5 [top]

When given to the "+" procedure, would yield:

    [bottom] 58 93 [top]


Substacks
---------

Substacks are groups of data on the data stack.  They are used only by
the "if" and "ifelse" procedures, and are denoted by "{" (start
substack) and "}" (end substack).  Substacks may be nested.

The following will result in 58 on the top of the stack:

    5 8 < { 50 8 + } { 50 8 - } ifelse


Comments
--------

Anything inside parenthesis is ignored by the forf interpreter. 



Built-in Procedures
===================

The following procedures are built in to Forf.  Since the language was
designed to be extended, your game provides additional procedures.


Unary Operations
----------------

These procedures pop one value and push one.

* `x ~`   (bitwise invert)
* `x !`   (logical not)
* `x abs` (absolute value)


Binary Operations
-----------------

The following procedures pop two values and push one.  They work as in
an RPN calculator, meaning that `8 4 /` yields `2`.  

* `y x +`  (y + x)
* `y x -`  (y - x)
* `y x *`  (y * x)
* `y x /`  (y / x)
* `y x %`  (y modulo x)
* `y x &`  (y and x)
* `y x |`  (y or x)
* `y x ^`  (x xor x)
* `y x <<` (y shifted left by x)
* `y x >>` (y shifted right by x)


Comparison
----------

These procedures pop two numbers and compare them, pushing `1` if the
comparison is true, `0` if false.  For instance, `5 3 >` yields `1`.

* `y x >`  (y greater than x)
* `y x >=` (y greater than or equal to x)
* `y x <`  (y less than x)
* `y x <=` (y less than or equal to x)
* `y x =`  (y equal to x)
* `y x <>` (y not equal to x)


Conditional Procedures
----------------------

* `x { i ... } if`                (if x, evaluate i)
* `x { i ... } { e ... } ifelse`  (if x, evaluate i, otherwise evaluate e)


Stack Manipulation
------------------

* `x pop`    (discard x)
* `x dup`    (duplicate x)
* `y x exch` (exchange x and y on the stack)


Memory
------

Your game may provide you with one or more memory slots.  These are like
variables in other languages, and may persist across invocations of your
program.

* `y x mset` (store y in slot x)
* `x mget`   (retrieve value in slot x)


Examples
========

Compute 58²:

    58 58 *

Compute 58³:

    58 58 58 * *

Another way to compute 58³:

    58 dup dup * *


The ifelse example, which does a comparison and puts 58 on the stack:

    5 8 < { 50 8 + } { 50 8 - } ifelse

Another way to do that:

    50 5 8 < { 8 + } { 8 - } ifelse

Yet another way:

    50 8 5 8 < { + } { - } ifelse

Is memory slot 3 greater than 100?

    3 mget 100 >

Given x, if x² is greater than 100 yield x², otherwise 0:

    dup dup * 100 > { dup * } { pop 0 } ifelse

Another way to do the same thing:

    dup * dup 100 < { pop 0 } if

Given coordinates (x, y) on the stack, is the distance to (x, y) less
than 88?  This compares x²+y² against 88².

    dup * exch dup * + 88 88 * <

Perform different actions given some number between 0 and 3:

    dup 0 = { action0 } if
    dup 1 = { action1 } if
    dup 2 = { action2 } if
    dup 3 = { action3 } if
    pop


