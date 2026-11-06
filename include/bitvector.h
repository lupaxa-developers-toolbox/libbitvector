#ifndef BITVECTOR_H
#define BITVECTOR_H

#include <stddef.h>

#define BITVECTOR_VERSION_STRING "0.1.0-rc1"

/* BITVECTOR_OK is success. BITVECTOR_NOMEM means allocation failed.
   bitvector_create and bitvector_from_string set the out-pointer to NULL.
   A failed grow leaves the vector unchanged. BITVECTOR_INVAL means a missing
   vector, a missing out-pointer, a bad string, or an index past the length
   on a call that does not grow. */
#define BITVECTOR_OK 0
#define BITVECTOR_NOMEM (-1)
#define BITVECTOR_INVAL (-2)

#ifdef __cplusplus
extern "C" {
#endif

typedef struct bitvector bitvector;

/* Allocate a vector of length bits, all clear. Length 0 is an empty vector.
   On success *bv receives the vector. On allocation failure *bv is NULL.
   A NULL out-pointer leaves nothing to write and returns BITVECTOR_INVAL. */
int bitvector_create(bitvector **bv, size_t length);

/* Free the vector and set *bv to NULL. Destroying an already NULL vector
   is success. */
int bitvector_destroy(bitvector **bv);

/* Allocate a new vector with the same length and the same bits. */
int bitvector_clone(const bitvector *src, bitvector **out);

/* Set, clear, or flip one bit. An index past the length grows the vector,
   and the new bits are clear. On allocation failure the vector is unchanged. */
int bitvector_set(bitvector *bv, size_t index);
int bitvector_unset(bitvector *bv, size_t index);
int bitvector_toggle(bitvector *bv, size_t index);

/* Write 0 or 1 to *bit. An index past the length returns BITVECTOR_INVAL
   and leaves *bit unchanged. This call does not grow the vector. */
int bitvector_get(const bitvector *bv, size_t index, int *bit);

/* Flip every bit in the length. Bits past the length stay clear. */
int bitvector_invert(bitvector *bv);

/* Clear every bit. The length stays the same. */
int bitvector_clear(bitvector *bv);

/* Write 1 to *empty when every bit is clear, including a length of 0. */
int bitvector_is_empty(const bitvector *bv, int *empty);

/* Write 1 to *full when the length is at least 1 and every bit is set.
   A length of 0 writes 0. */
int bitvector_is_full(const bitvector *bv, int *full);

size_t bitvector_length(const bitvector *bv);

/* Write a string of length characters, '1' for a set bit and '0' for a
   clear bit, including trailing zeros. The caller frees *text. */
int bitvector_to_string(const bitvector *bv, char **text);

/* Build a vector whose length is the string length. Index 0 is the first
   character. Only '0' and '1' are valid. An empty string is a length of 0.
   A bad character leaves *bv unchanged. */
int bitvector_from_string(const char *text, bitvector **bv);

#ifdef __cplusplus
}
#endif

#endif /* BITVECTOR_H */
