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

To do so, we define some macros which you should use through the project

- `STATIC_ASSERT(condition, __error_msg_as_variable_literal)` is a construct that produces a compile-time error if condition is not met. 
Use `STATIC_ASSERT` to assert certain base conditions which your code assumes.
An example is `STATIC_ASSERT(sizeof(uint8_t) == sizeof(char), __bad_size)` or
`STATIC_ASSERT(KEY_SIZE=64, __bad_key_size)`.

- `SIZEOF(x)`
- `ARRAY_LEN(x)`


## Conventions

**Strings/byte arrays**
We use `uint8_t* buf` for binary data and `char* str` for string data (e.g zero-terminated).

Rationale: While `uint8_t` is the same size as `char` (we actully `STATIC_ASSERT` that),
`char` is signed and thus it should produce warnings if these two types are interchanged.
Note that you should *not* use `unsigned char` for this reason.

**Lengths**
Speaking of buffers and strings, strings always carry an implicit length of `strlen(str)` (which conviniently returns `size_t`).
If you ever need to *output* a string, it should be taken into that `strlen(str)` is ONE less than the space this takes in memory.

On the other hand, byte buffers do not have associated length in C. 
As such, any byte buffer *must* be associated with an explicit length (of type `size_t`) which confirms its length.

**Sizes vs lengths**
If something is size in bytes, use `size_t`. Note that `size_t` is `unsigned` type.
This hels to guard against underflow errors such as passing `inputLen=-1` to a function as the argument would end up very large.
As a consequence, every *outputLen* parameter should be checked by `ASSERT(outputLen < MAX_SIZE)`.

** Rationale: checking signed version of lengths is tricky. Using unsigned values simplifies the logic.
Unfortunately, signed-unsigned comparison is tricky in C. As a consequence, always use `SIZEOF()` macro for testing available buffer space.

- Input parameters first
- Output parameters second

Always place input size (in case of buffers) and max output size
