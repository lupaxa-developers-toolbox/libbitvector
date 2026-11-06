/* Bit vector.
   The public contract is in include/bitvector.h. This file holds the word
   array and the function bodies. length is the number of addressable bits.
   words holds that length rounded up to 64-bit words. Bits past length in
   the last word stay clear. */

/* Quoted path so a syntax check without -I still finds the public header. */
#include "../include/bitvector.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define WORD_BITS 64u

/* words is NULL when length is 0. nwords is the allocated word count. */
struct bitvector {
    uint64_t *words;
    size_t length;
    size_t nwords;
};

/* Number of words needed for length bits. A length of 0 needs no words.
   Returns 0 when length + 63 would wrap. */
static int words_for(size_t length, size_t *count)
{
    if (length == 0) {
        *count = 0;
        return 1;
    }
    if (length > SIZE_MAX - (WORD_BITS - 1)) {
        return 0;
    }
    *count = (length + (WORD_BITS - 1)) / WORD_BITS;
    return 1;
}

/* Mask of the live bits in the last word. A multiple of 64 uses the whole
   word. A length of 0 is not a last word. */
static uint64_t tail_mask(size_t length)
{
    size_t bits = length % WORD_BITS;

    if (bits == 0) {
        return UINT64_MAX;
    }
    return (UINT64_C(1) << bits) - 1;
}

static uint64_t bit_mask(size_t index)
{
    return UINT64_C(1) << (index % WORD_BITS);
}

/* Grow so index is inside the length. New bits are clear. A failed realloc
   leaves the pointer, the length, and the bits unchanged. */
static int grow_to(bitvector *bv, size_t index)
{
    size_t new_length;
    size_t need;
    uint64_t *grown;

    if (index == SIZE_MAX) {
        return BITVECTOR_INVAL;
    }
    new_length = index + 1;
    if (new_length <= bv->length) {
        return BITVECTOR_OK;
    }
    if (!words_for(new_length, &need)) {
        return BITVECTOR_INVAL;
    }
    if (need > bv->nwords) {
        grown = realloc(bv->words, need * sizeof(*grown));
        if (grown == NULL) {
            return BITVECTOR_NOMEM;
        }
        memset(grown + bv->nwords, 0, (need - bv->nwords) * sizeof(*grown));
        bv->words = grown;
        bv->nwords = need;
    }
    bv->length = new_length;
    return BITVECTOR_OK;
}

/* Create a cleared vector. Length 0 allocates no words. Allocation failure
   sets *bv to NULL. A length that cannot be counted returns BITVECTOR_INVAL
   and leaves *bv unchanged. */
int bitvector_create(bitvector **bv, size_t length)
{
    bitvector *created;
    uint64_t *words;
    size_t nwords;

    if (bv == NULL) {
        return BITVECTOR_INVAL;
    }
    if (!words_for(length, &nwords)) {
        return BITVECTOR_INVAL;
    }

    created = calloc(1, sizeof(*created));
    words = NULL;
    if (nwords > 0) {
        words = calloc(nwords, sizeof(*words));
    }
    if (created == NULL || (nwords > 0 && words == NULL)) {
        free(created);
        free(words);
        *bv = NULL;
        return BITVECTOR_NOMEM;
    }

    created->words = words;
    created->length = length;
    created->nwords = nwords;
    *bv = created;
    return BITVECTOR_OK;
}

/* Free the words and the header. *bv is set to NULL. */
int bitvector_destroy(bitvector **bv)
{
    if (bv == NULL) {
        return BITVECTOR_INVAL;
    }
    if (*bv == NULL) {
        return BITVECTOR_OK;
    }

    free((*bv)->words);
    free(*bv);
    *bv = NULL;
    return BITVECTOR_OK;
}

/* Copy length and words into a new vector. The two vectors do not share
   storage. */
int bitvector_clone(const bitvector *src, bitvector **out)
{
    bitvector *copy = NULL;
    int rc;

    if (src == NULL || out == NULL) {
        return BITVECTOR_INVAL;
    }

    rc = bitvector_create(&copy, src->length);
    if (rc != BITVECTOR_OK) {
        *out = NULL;
        return rc;
    }
    if (src->nwords > 0) {
        memcpy(copy->words, src->words, src->nwords * sizeof(*src->words));
    }
    *out = copy;
    return BITVECTOR_OK;
}

int bitvector_set(bitvector *bv, size_t index)
{
    int rc;

    if (bv == NULL) {
        return BITVECTOR_INVAL;
    }
    rc = grow_to(bv, index);
    if (rc != BITVECTOR_OK) {
        return rc;
    }
    bv->words[index / WORD_BITS] |= bit_mask(index);
    return BITVECTOR_OK;
}

int bitvector_unset(bitvector *bv, size_t index)
{
    int rc;

    if (bv == NULL) {
        return BITVECTOR_INVAL;
    }
    rc = grow_to(bv, index);
    if (rc != BITVECTOR_OK) {
        return rc;
    }
    bv->words[index / WORD_BITS] &= ~bit_mask(index);
    return BITVECTOR_OK;
}

int bitvector_toggle(bitvector *bv, size_t index)
{
    int rc;

    if (bv == NULL) {
        return BITVECTOR_INVAL;
    }
    rc = grow_to(bv, index);
    if (rc != BITVECTOR_OK) {
        return rc;
    }
    bv->words[index / WORD_BITS] ^= bit_mask(index);
    return BITVECTOR_OK;
}

/* Read one bit. Past the length, *bit is left alone. */
int bitvector_get(const bitvector *bv, size_t index, int *bit)
{
    if (bv == NULL || bit == NULL || index >= bv->length) {
        return BITVECTOR_INVAL;
    }
    *bit = (bv->words[index / WORD_BITS] & bit_mask(index)) != 0;
    return BITVECTOR_OK;
}

/* Flip live bits only. The spare bits in the last word are masked back
   to zero so later empty and full checks ignore them. */
int bitvector_invert(bitvector *bv)
{
    size_t full;
    size_t rem;
    size_t i;
    uint64_t mask;

    if (bv == NULL) {
        return BITVECTOR_INVAL;
    }
    if (bv->length == 0) {
        return BITVECTOR_OK;
    }

    full = bv->length / WORD_BITS;
    rem = bv->length % WORD_BITS;
    for (i = 0; i < full; i++) {
        bv->words[i] = ~bv->words[i];
    }
    if (rem != 0) {
        mask = tail_mask(bv->length);
        bv->words[full] ^= mask;
        bv->words[full] &= mask;
    }
    return BITVECTOR_OK;
}

int bitvector_clear(bitvector *bv)
{
    if (bv == NULL) {
        return BITVECTOR_INVAL;
    }
    if (bv->nwords > 0) {
        memset(bv->words, 0, bv->nwords * sizeof(*bv->words));
    }
    return BITVECTOR_OK;
}

int bitvector_is_empty(const bitvector *bv, int *empty)
{
    size_t full;
    size_t rem;
    size_t i;

    if (bv == NULL || empty == NULL) {
        return BITVECTOR_INVAL;
    }

    full = bv->length / WORD_BITS;
    rem = bv->length % WORD_BITS;
    for (i = 0; i < full; i++) {
        if (bv->words[i] != 0) {
            *empty = 0;
            return BITVECTOR_OK;
        }
    }
    if (rem != 0 && (bv->words[full] & tail_mask(bv->length)) != 0) {
        *empty = 0;
        return BITVECTOR_OK;
    }
    *empty = 1;
    return BITVECTOR_OK;
}

int bitvector_is_full(const bitvector *bv, int *full)
{
    size_t whole;
    size_t rem;
    size_t i;
    uint64_t mask;

    if (bv == NULL || full == NULL) {
        return BITVECTOR_INVAL;
    }
    if (bv->length == 0) {
        *full = 0;
        return BITVECTOR_OK;
    }

    whole = bv->length / WORD_BITS;
    rem = bv->length % WORD_BITS;
    for (i = 0; i < whole; i++) {
        if (bv->words[i] != UINT64_MAX) {
            *full = 0;
            return BITVECTOR_OK;
        }
    }
    if (rem != 0) {
        mask = tail_mask(bv->length);
        if ((bv->words[whole] & mask) != mask) {
            *full = 0;
            return BITVECTOR_OK;
        }
    }
    *full = 1;
    return BITVECTOR_OK;
}

size_t bitvector_length(const bitvector *bv)
{
    if (bv == NULL) {
        return 0;
    }
    return bv->length;
}

/* One character per live bit. Trailing clear bits stay in the string as
   '0'. The caller frees the buffer. */
int bitvector_to_string(const bitvector *bv, char **text)
{
    char *buffer;
    size_t i;

    if (bv == NULL || text == NULL || bv->length == SIZE_MAX) {
        return BITVECTOR_INVAL;
    }

    buffer = malloc(bv->length + 1);
    if (buffer == NULL) {
        return BITVECTOR_NOMEM;
    }
    for (i = 0; i < bv->length; i++) {
        uint64_t word = bv->words[i / WORD_BITS];

        buffer[i] = (word & bit_mask(i)) != 0 ? '1' : '0';
    }
    buffer[bv->length] = '\0';
    *text = buffer;
    return BITVECTOR_OK;
}

/* Each character is one bit. A character other than '0' or '1' returns
   BITVECTOR_INVAL and does not write *bv. */
int bitvector_from_string(const char *text, bitvector **bv)
{
    bitvector *created = NULL;
    size_t length;
    size_t i;
    int rc;

    if (text == NULL || bv == NULL) {
        return BITVECTOR_INVAL;
    }

    length = strlen(text);
    for (i = 0; i < length; i++) {
        if (text[i] != '0' && text[i] != '1') {
            return BITVECTOR_INVAL;
        }
    }

    rc = bitvector_create(&created, length);
    if (rc != BITVECTOR_OK) {
        *bv = NULL;
        return rc;
    }
    for (i = 0; i < length; i++) {
        if (text[i] == '1') {
            rc = bitvector_set(created, i);
            if (rc != BITVECTOR_OK) {
                bitvector_destroy(&created);
                *bv = NULL;
                return rc;
            }
        }
    }
    *bv = created;
    return BITVECTOR_OK;
}
