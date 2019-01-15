# Developing Cardano Ledger App

Coding in C is hard and prone to errors.
This is even more escalated by the fact that any coding error on Ledger can potentially lead to an exploit handing off private keys to cryptofunds.
As such, security is of an utmost importance for us.

When feasible, we try to use *syntax* to avoid programming pitfalls.
When using syntax is not feasible, we use *convention* to make it easier to avoid potential errors.
This document tries to collect all of these in a single "source of truth" along with an explanation of
why we do things the way we do.

## Syntax tricks

C has many constructs which are not safe by default. As such, we try to exploit syntax in order to early catch some cases.

For example, when feasible, we use `struct` types instead of pointers to input/output buffers. This makes it easier to catch mismatching sizes. When this is not feasible/desired, we define some macros which you should use through the project:

- `STATIC_ASSERT(condition, __error_msg_as_variable_literal)` is a construct that produces a compile-time error if condition is not met. 
Watch out that the condition **must** be compile-time evaluable.
Use `STATIC_ASSERT` to assert certain base conditions which your code assumes.
An example is `STATIC_ASSERT(sizeof(uint8_t) == sizeof(char), __bad_size)` or
`STATIC_ASSERT(KEY_SIZE=64, __bad_key_size)`.

- `SIZEOF(x)`: In C `sizeof(x)` returns the size of variable `x`. Unfortunately, if `x` is `char*`, `sizeof(x)` would return `4` (i.e. size of pointer type). This is very dangerous, especially because `void myFunction(char x[100])` silently converts `char [100]` to `char *` which would yield a bad result. To avoid this (and potently other problems such us `sizeof(my_struct_ptr)`, we define `SIZEOF` macro which does not compile if the underlying type is pointer.

- `ARRAY_LEN(x)`: Similarly to `sizeof`, getting the number of array elements is tricky in C. A standard approach is `sizeof(arr) / sizeof(arr[0])` but this has again lots of edge cases with pointers. We try to abstract such cases with `ARRAY_LEN` macro that fails to compile if `arr` is not an explicit array.

## Conventions

When we cannot use syntax to avoid mistakes, we rely on programming conventions.

### Strings/byte arrays:

We use `uint8_t* buf` for binary data and `char* str` for string data (e.g zero-terminated).

**Rationale:** While `uint8_t` is the same size as `char` (i.e., 1 byte),
`char` is signed and thus it should produce warnings if these two types are interchanged.
Note that you should *not* use `unsigned char` for this reason.

(Note: as paranoid as we are we actully `STATIC_ASSERT` that `sizeof(uint8_t) == sizeof(char)`)

### Lengths:

Speaking of buffers and strings, strings always carry an implicit length of `strlen(str)` (which conviniently returns `size_t`).
If you ever need to *output* a string, it should be taken into account that `strlen(str)` is ONE less than the space string takes in memory.

On the other hand, byte buffers do not have associated length in C. 
As such, any byte buffer *must* be associated with an explicit length (of type `size_t`) which confirms its length.

In particular, **never** assume implicit lengths of buffers. If a function takes public key of known size 32 bytes, it still *should* accept an additional size argument and ASSERT expected size at the beginning. This way a potential mismatch is found early, especially if underlying assumptions (buffer lengths) change.

### Sizes vs lengths:

If something is size in bytes, use `size_t`. Note that `size_t` is `unsigned` type.
This hels to guard against underflow errors such as passing `inputLen=-1` to a function as the argument would end up very large.
As a consequence, every *outputLen* parameter should be checked by `ASSERT(outputLen < MAX_SIZE)`. This check checks both for underflow as well as some other mistakes (buffers of size >4KB are impossible due to RAM constrains of our device).

**Rationale:** checking signed version of lengths is tricky. Using unsigned values simplifies the logic.
Unfortunately, signed-unsigned comparison is tricky in C. As a consequence, always use `SIZEOF()` macro for testing available buffer space.

## Naming conventions:

### Buffers and arrays:

- Buffers should have postfix `Buf`, (i.e. `uint8_t* inputBuf`)
- Buffer sizes should have postfix `Size` (i.e, `size_t inputSize`)
- Arrays should have postfix `Array`, (i.e., `uint32_t* pathArray`)
- Array lengths should have postfix `Length`, (i.e. `uint32_t pathLength`)

### Input parameters:

- Input parameters should be marked as `const`, (e.g., `const struct_t* input`)
- Always use size/length for pointer arguments, e.g. `const uint8_t* inputBuf` followed by `size_t inputSize`.
- Input parameters should normally be first arguments of the function. A exception is an input-output struct parameter which serves to "emulate" C++ classes, e.g. `stream_appendBuf(stream_t* t, ...other args)`

### Output parameters:

- Output parameters normally go last in the function Input parameters first
- Output parameters second

## Function responsibilities:
- Function should always assert buffer sizes
  - to be smaller than `BUF_SIZE_PARANOIA`
  - to be equal to some constant if needed
- If function is expected to throw, *it should try to avoid modiying output* before throwing.
