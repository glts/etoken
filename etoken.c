/* etoken.c
 *
 * Etoken is a simple existence-table tokenizer. Before the actual
 * tokenization, a "token hash" structure is constructed. This is some kind of
 * hash of hashes, representing a model of all valid tokens, ie. the existence
 * table. A token is a sequence of Unicode codepoints, terminated with 0.
 *
 * The public interface is:
 * - createTokenHash()
 * - tokenize()
 * - destroyTokenHash()
 *
 * The size of the initial hash and the size of the subhashes can be
 * controlled with INITSIZE and SUBSIZE, respectively. The number of elements
 * in a token may not exceed TOKENMAX.
 *
 * The program expects input to be in Unicode codepoints, represented as
 * unsigned 32-bit integers. Conversion functions for UTF-8 are included. They
 * do not check the validity of the input. */

#include <stdio.h>
#include <stdlib.h>

#include "etoken.h"
#ifdef IGNORECASE
# include <ctype.h>
#endif

/* Converts a sequence of UTF-8 (multibyte) characters into a sequence of
 * Unicode codepoints, terminated with 0. Size specifies the maximum number of
 * codepoints to be read minus 1. Returns number of codepoints read, without
 * terminating 0. */
int utf8_to_uc(uint32_t *uc_str, const char *utf8_str, unsigned int size)
{
    int len, bytes;
    unsigned char head; /* making this unsigned saves us a few casts */

    /* Care must be taken to treat the input chars as unsigned since we are
     * going to do quite a bit of bit-twiddling on them. */

    len = 0;

    while (*utf8_str != '\0' && len < size) {
        if ((unsigned char) *utf8_str < 0x80)
            *uc_str = *utf8_str++;
        else {
            /* count 1 bits in prefix */
            for (head = *utf8_str++, bytes = 0; head & 0x80; head <<= 1)
                bytes++;

            *uc_str = head >> bytes;
            while (--bytes > 0)
                *uc_str = (*uc_str << 6) | ((unsigned char) *utf8_str++ & 0x3f);
        }
        len++, uc_str++;
    }

    *uc_str = 0; /* terminator */

    return len;
}

/* Converts a 0-terminated sequence of Unicode codepoints to a UTF-8 character
 * string (terminated with '\0'). Reads a maximum of size minus 1 characters
 * into the buffer. Returns the number of chars read. */
int uc_to_utf8(char *utf8_str, const uint32_t *uc_str, unsigned int size)
{
    int i, len, bytes;

    len = 0;

    while (*uc_str != 0 && len < size) {
        if (*uc_str < 0x80) {
            *utf8_str++ = *uc_str;
            len++;
        } else {
            if (*uc_str < 0x800)        { bytes = 2; }
            else if (*uc_str < 0x10000) { bytes = 3; }
            else                        { bytes = 4; }

            *utf8_str++ = (~0 << (8-bytes)) | (*uc_str >> ((bytes-1) * 6));
            for (i = bytes-2; i >= 0; i--)
                *utf8_str++ = ((*uc_str >> 6*i) & 0x3f) | 0x80;

            len += bytes;
        }
        uc_str++;
    }

    *utf8_str = '\0';

    return len;
}

/* Creates and returns the major "token hash" structure. All the token
 * definitions passed in the tokenDef array of the given size, are
 * consecutively inserted into the token hash, which is essentially a big hash
 * of nested hashes.
 * The token hash must be destroyed with destroyTokenHash() eventually. */
tEntry **createTokenHash(uint32_t *tokenDef[], unsigned int size)
{
    int i;
    uint32_t *tok;

    tEntry **ht = malloc(sizeof(*ht) * INITSIZE); /* memory for root hash */
    for (i = 0; i < INITSIZE; i++)
        ht[i] = NULL;

    for (i = 0; i < size; i++) {
        tok = tokenDef[i];
        addToken(ht, tok);
    }

    return ht;
}

/* Adds a full token (sequence of UC codepoints, terminated with 0) to the
 * root hash given. Returns a pointer to the first tEntry of the resulting
 * hash chain. */
tEntry *addToken(tEntry **ht, const uint32_t *ptok)
{
    tEntry **pht = ht;
    tEntry *head; /* head entry to be returned */
    tEntry *next;

    if (ptok == NULL)
        return NULL;

    head = insertEntry(pht, *ptok++, INITSIZE);
    next = head;
    pht = next->tEntries;

    while (*ptok != 0) {
        next = insertEntry(pht, *ptok++, SUBSIZE);
        pht = next->tEntries;
    }
    next->endpoint = 1; /* the last token element is an endpoint */

    return head;
}

/* Insert a value into a particular hashtable. Size specifies the size of the
 * hashtable and is used by the hash function. */
tEntry *insertEntry(tEntry **ht, uint32_t val, unsigned int size)
{
    int i;
    tEntry *te;
    unsigned int hi; /* hash index */

    hi = UCHASH(val, size);

    te = ht[hi]; /* head of the linked list we're hashing to */

    if (te == NULL) { /* empty bucket */
        te = malloc(sizeof(*te));
        te->nextEntry = NULL;
    } else {
        for ( ; te != NULL; te = te->nextEntry)
            if (te->val == val) /* value found, return it untouched */
                return te;

        te = malloc(sizeof(*te));
        te->nextEntry = ht[hi]; /* insert as head, easier */
    }

    /* Initialize new entry */
    te->val = val;
    te->endpoint = 0;
    for (i = 0; i < SUBSIZE; i++)
        te->tEntries[i] = NULL;

    ht[hi] = te; /* insert as head of the linked list */

    return te;
}

/* Look up a value in the hashtable. Returns a pointer to an entry or NULL.
 * Size specifies the size of the hashtable. */
tEntry *find(tEntry **ht, uint32_t val, unsigned int size)
{
    tEntry *te;
    unsigned int hi;

#ifdef IGNORECASE
    if (val < 0x80)
        val = tolower(val);
#endif

    hi = UCHASH(val, size);

    for (te = ht[hi]; te != NULL; te = te->nextEntry)
        if (te->val == val)
            return te;

    return NULL;
}

/* Tokenizes some input string (0-terminated sequence of UC codepoints) with
 * reference to the existence table modeled in the token hash structure. The
 * tokens are copied into **tokens. It is the responsibility of the caller not
 * to overflow the **tokens array.
 * Returns the number of tokens written into the tokens array. */
int tokenize(uint32_t **tokens, tEntry **ht, const uint32_t *uc_str)
{
    int i, cursor, end, ntokens;
    const uint32_t *puc;
    uint32_t *ptok;
    tEntry **pht; /* current hashtable */
    tEntry *te;

    puc = uc_str;
    cursor = 0;
    end = 0; /* points one past last element in tokbuf */
    ntokens = 0;
    pht = ht;

    /* Scan throug the input: keep building a token while the elements can be
     * found in the hash; else stop, create a new token, push it to the tokens
     * array and restart one element further on in the stream. */
    while (puc[cursor] != 0) {
        if ((te = find(pht, puc[cursor], end > 0 ? SUBSIZE : INITSIZE)) != NULL) {
            cursor++;
            if (te->endpoint) /* is a possible endpoint */
                end = cursor;
            pht = te->tEntries;
        } else {
            if (end == 0) /* singleton unknown tok */
                end = 1;

            /* copy token buf into new token memory */
            ptok = malloc(sizeof(*ptok)*(end+1)); /* need room for terminator */
            for (i = 0; i < end; i++)
                ptok[i] = puc[i];
            ptok[end] = 0; /* terminator */

            tokens[ntokens++] = ptok;

            /* reset cursor variables */
            puc = &puc[end];
            cursor = 0;
            end = 0;

            pht = ht; /* reset hashtable pointer */
        }
    }

    return ntokens;
}

/* Step through the token hash and call destroyEntry() for every element in
 * the hash, then free token hash. */
void destroyTokenHash(tEntry **ht)
{
    int i;

    if (ht == NULL)
        return;

    for (i = 0; i < INITSIZE; i++)
        if (ht[i] != NULL)
            destroyEntry(ht[i]);

    free(ht);
}

/* Destroy an entry and all links following it, recursively destroying all
 * entries contained in the hashes. Normally the argument would be the head of
 * a linked list. */
void destroyEntry(tEntry *te)
{
    int i;
    tEntry *next;

    for ( ; te != NULL; te = next) {
        next = te->nextEntry;
        for (i = 0; i < SUBSIZE; i++)
            if (te->tEntries[i] != NULL)
                destroyEntry(te->tEntries[i]);

        free(te);
    }
}

/* Prints a graphical representation of the token hash. Possible endpoints are
 * marked with an asterisk. */
void printDict(tEntry **ht, int size, int level)
{
    int i, j, k;
    tEntry *pte;

    /* step through all buckets in this hash */
    for (i = 0; i < size; i++) {
        if (ht[i] != NULL) {
            for (j = 0; j < level; j++)
                printf("\t");
            printf("[%d]\n", i);
            for (pte = ht[i], k = 0; pte != NULL; pte = pte->nextEntry, k++) {
                for (j = 0; j < level; j++)
                    printf("\t");
                printf("{%d}: value '%x'%c\n", k, pte->val, pte->endpoint ? '*' : ' ');
                printDict(pte->tEntries, SUBSIZE, level+1);
            }
        }
    }
}
