<p align="center">
  <a href="https://github.com/lupaxa-developers-toolbox">
    <img src="https://raw.githubusercontent.com/the-lupaxa-project/brand-assets/master/logos/organisations/developers-toolbox/readme-logo.png" alt="Developers Toolbox" />
  </a>
</p>

<h1 align="center">Libbitvector</h1>

C library for a growable vector of bits.

Each vector has a length and a private array of 64-bit words. The length is
the number of addressable bits. Storage rounds up to a whole word, and the
spare bits in the last word stay clear. Set, unset, and toggle grow the
length when the index is past the end. Get does not grow. The vector type is
opaque. The public header is `include/bitvector.h`. It uses C linkage, so a
C++ translation unit can include it.

## Requirements

- GNU make.
- gcc, unless `CC` is set. The C skill uses gcc for syntax checks.
- cppcheck, for `make c-check`.

## Build

`make lib` writes the static archive `build/libbitvector.a`. The archive is
built as C11 with `-Wall -Wextra -Wpedantic -Werror`. The compiler is gcc
unless `CC` is set. `make clean` removes `build/`.

```bash
make lib
cc -std=c11 -Iinclude -o demo demo.c build/libbitvector.a
```

`make lib CC=clang` selects the compiler. `make lib CFLAGS=-g` adds flags to
that C11 build. `make` with no target prints the makefile help.

`make update` refreshes the makefile skills. `make lib` and `make test` do
not need that checkout. `make c-check` and `make version` do.

## Quick Start

Create an 8-bit vector, set bit 3, read it back, and destroy the vector.

```c
#include "bitvector.h"

#include <stdio.h>

int main(void)
{
    bitvector *bv = NULL;
    int bit = 0;

    if (bitvector_create(&bv, 8) != BITVECTOR_OK) {
        return 1;
    }

    if (bitvector_set(bv, 3) != BITVECTOR_OK) {
        bitvector_destroy(&bv);
        return 1;
    }

    bitvector_get(bv, 3, &bit);
    printf("%d\n", bit);

    bitvector_destroy(&bv);
    return 0;
}
```

## Length

The length is the count of bits the caller can address. `bitvector_create`
starts at the length it is given. A length of 0 is a vector with no bits.

`bitvector_set`, `bitvector_unset`, and `bitvector_toggle` grow the length
to `index + 1` when the index is past the end. The new bits are clear, then
the requested bit is updated. A failed growth leaves the length and the bits
unchanged.

`bitvector_get` does not grow. An index past the length returns
`BITVECTOR_INVAL` and leaves the out-pointer unchanged.

`bitvector_clear` and `bitvector_invert` keep the length. Invert flips only
the live bits.

> **Note:** Callers serialise use of a given vector.

## Return Codes

Every function that can fail returns an `int`.

| Code              | Value | Meaning                                                                                               |
| ----------------- | ----- | ----------------------------------------------------------------------------------------------------- |
| `BITVECTOR_OK`    | `0`   | The call succeeded.                                                                                   |
| `BITVECTOR_NOMEM` | `-1`  | Allocation failed. Create and from-string set the out-pointer to NULL.                                |
| `BITVECTOR_INVAL` | `-2`  | The vector or out-pointer is missing, the string is not `0` and `1`, or the index is past the length. |

`bitvector_destroy(NULL)` returns `BITVECTOR_INVAL`. `bitvector_destroy` on a
pointer that is already NULL returns `BITVECTOR_OK` and leaves it NULL.

A bad `bitvector_from_string` leaves `*bv` unchanged. Allocation failure sets
it to NULL.

## Functions

```c
typedef struct bitvector bitvector;

int bitvector_create(bitvector **bv, size_t length);
int bitvector_destroy(bitvector **bv);
int bitvector_clone(const bitvector *src, bitvector **out);
int bitvector_set(bitvector *bv, size_t index);
int bitvector_unset(bitvector *bv, size_t index);
int bitvector_toggle(bitvector *bv, size_t index);
int bitvector_get(const bitvector *bv, size_t index, int *bit);
int bitvector_invert(bitvector *bv);
int bitvector_clear(bitvector *bv);
int bitvector_is_empty(const bitvector *bv, int *empty);
int bitvector_is_full(const bitvector *bv, int *full);
size_t bitvector_length(const bitvector *bv);
int bitvector_to_string(const bitvector *bv, char **text);
int bitvector_from_string(const char *text, bitvector **bv);
```

| Name                    | Purpose                                                     |
| ----------------------- | ----------------------------------------------------------- |
| `bitvector_create`      | Allocate a cleared vector of the given length.              |
| `bitvector_destroy`     | Free the vector and set `*bv` to NULL.                      |
| `bitvector_clone`       | Allocate a copy with the same length and the same bits.     |
| `bitvector_set`         | Set one bit. Grow when the index is past the length.        |
| `bitvector_unset`       | Clear one bit. Grow when the index is past the length.      |
| `bitvector_toggle`      | Flip one bit. Grow when the index is past the length.       |
| `bitvector_get`         | Write 0 or 1 to `*bit`. Do not grow.                        |
| `bitvector_invert`      | Flip every live bit.                                        |
| `bitvector_clear`       | Clear every live bit. The length stays the same.            |
| `bitvector_is_empty`    | Write 1 when every live bit is clear.                       |
| `bitvector_is_full`     | Write 1 when the length is at least 1 and every bit is set. |
| `bitvector_length`      | Return the number of live bits. A NULL vector returns 0.    |
| `bitvector_to_string`   | Write `0` and `1` for every live bit. The caller frees it.  |
| `bitvector_from_string` | Build a vector from a string of `0` and `1`.                |

`bitvector_is_empty` writes 1 for a length of 0. `bitvector_is_full` writes 0
for a length of 0.

## Strings

Index 0 is the first character. A clear bit is `0`, including a trailing
clear bit. The string length is the vector length.

```c
bitvector *bv = NULL;
char *text = NULL;

bitvector_from_string("1000", &bv);
bitvector_to_string(bv, &text);
```

`text` is `1000`. The caller frees `text` with `free`. An empty string builds
a vector of length 0. Any other character returns `BITVECTOR_INVAL`.

`bitvector_clone` copies the words. Changing the copy does not change the
source.

## Empty Vector

A new vector of length 0 is empty and not full. Its string is empty.
Destroying it returns `BITVECTOR_OK` and sets the caller's pointer to NULL.
Destroying that NULL pointer again returns `BITVECTOR_OK`.

`bitvector_length(NULL)` returns 0.

## Version

The version text is the macro `BITVECTOR_VERSION_STRING` in
`include/bitvector.h`. It is a dotted semantic version. `.bumpversion.toml`
is the version source, and a bump updates that macro.

After `make init`, `make version` prints the version from
`.bumpversion.toml`.

## Development

```bash
make update
make lib
make check
make clean
```

`make c-check` syntax-checks every `.c` and `.h` file, then runs cppcheck.
`make check` runs `c-check` and then the behaviour tests. `make test` runs
only the behaviour tests.

<a href="https://github.com/the-lupaxa-project">
    <img src="https://raw.githubusercontent.com/the-lupaxa-project/brand-assets/master/logos/components/footer-for-child-orgs.svg" alt="The Lupaxa Project Footer" width="100%" />
</a>
