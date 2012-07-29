#include <stdio.h>
#include <stdlib.h>

#include "etoken.h"
#ifdef CASEFOLD
# include <ctype.h>
#endif

/*
 * Terminology -- TODO clean up
 *
 * tokenht -- the tokens hash of hashes data structure
 * tEntry  -- one entry somewhere in the token ht
 *   tEntries -- entries inside one hash entry
 * tokens -- the final output an array of actual tokens (uc codepoint sequences)
 */

/*
 * Some thoughts about the interfaces:
 * 1. Construct the tokenht structure by reading token definitions
 *    -> createTokenHash()
 * 2. Tokenize some input
 *    -> read from some stream
 *    -> make it into codepoints
 *    -> tokenize
 *    -> output it as a UTF-8 byte stream
 * 3. Pull everything down again
 *    -> destroyTokenHash()
 * */

/* TODO There is no error checking in the UTF-8 conversion functions! */

/* Size specifies the maximum number of codepoints to be read minus 1.
 * Returns number of codepoints read, without terminating 0. */
int utf8_to_uc(uint32_t *uc_str, const char *utf8_str, unsigned int size)
{
    int len, bytes;
    unsigned char head; /* making this unsigned saves us a few casts */

    /* Care must be taken to treat the input chars as unsigned since we
     * are going to do quite a bit of bit-twiddling on them. */

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

/* Converts a '0' terminated sequence of Unicode codepoints to UTF-8
 * characters sequences. Reads a maximum of size-1 characters into the
 * buffer, terminator is '\0'. Returns the number of chars read. */
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

/* Step through the root token hash and call destroyEntry() for every element
 * in the hash, then free yourself. Relies on the INITSIZE constant. */
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
 * entries contained in the hashes. Normally the argument entry would be the
 * head of a linked list. */
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

/* Adds a full token (sequence of UC codepoints, terminated with 0) to the
 * root hash given. Returns a pointer to the first tEntry of the resulting
 * hash chain. */
/* TODO Add token should use INITSIZE for the root hash then use insert
 * entry only with SUBSIZE. */
tEntry* addToken(tEntry **ht, const uint32_t *toks)
{
    tEntry **te = ht;
    tEntry *next;
    tEntry *head; /* store token head for return value */

    if (toks == NULL)
        return NULL;

    next = insertEntry(te, *toks++, INITSIZE);
    head = next;
    te = next->tEntries;

    while (*toks != 0) {
        next = insertEntry(te, *toks++, SUBSIZE);
        te = next->tEntries;
    }
    next->endpoint = 1; /* the last token element is an endpoint */

    return head;
}

/* Insert a value in a particular hashtable ht */
tEntry *insertEntry(tEntry **ht, uint32_t val, unsigned int size)
{
    int i;
    tEntry *pte;
    unsigned int hi; /* hash index */

    hi = UCHASH(val, size);

    pte = ht[hi]; /* head of the linked list we're hashing to */

    if (pte == NULL) { /* empty bucket */
        pte = malloc(sizeof(*pte));
        pte->nextEntry = NULL;
    } else {
        for ( ; pte != NULL; pte = pte->nextEntry)
            if (pte->val == val) /* value found, return it untouched */
                return pte;

        pte = malloc(sizeof(*pte));
        pte->nextEntry = ht[hi]; /* insert as head, easier */
    }

    /* Initialize new entry */
    pte->val = val;
    pte->endpoint = 0; /* not endpoint by default, must be set by caller */
    for (i = 0; i < SUBSIZE; i++)
        pte->tEntries[i] = NULL;

    ht[hi] = pte; /* insert as head of the linked list */

    return pte;
}

/* Look up a value in the hash table given. Returns a pointer to an entry or
 * NULL. With the CASEFOLD feature enabled it will lowercase ASCII lookup keys
 * first. */
tEntry *find(tEntry **ht, uint32_t val, unsigned int size) // size needs to go
{
    tEntry *pte;
    unsigned int hi;

#ifdef CASEFOLD
    if (val < 0x80)
        val = tolower(val);
#endif

    hi = UCHASH(val, size);

    for (pte = ht[hi]; pte != NULL; pte = pte->nextEntry)
        if (pte->val == val)
            return pte;

    return NULL;
}

/* Tokenize some input string (0-terminated sequence of UC codepoints) with
 * respect to the existence table modeled in the token hash structure, and
 * copy the tokens into **tokens. It is the responsibility of the caller not to
 * overflow the **tokens array.
 * Returns the number of tokens written into the tokens array.
 * Depends on TOKENMAX the maximum allowed size for a single token. */
int tokenize(uint32_t **tokens, tEntry **tokenht, const uint32_t *uc_str)
{
    int i;
    const uint32_t *resumepucs; /* points to resume savepoint */
    uint32_t *ptok;
    tEntry **pht;
    tEntry *pEntry;

    int numtokens = 0;

    pht = tokenht;
    resumepucs = uc_str;

    uint32_t tokbuf[TOKENMAX]; /* buffer for tokens under construction */
    int bufidx = 0;
    int endidx = 0; /* points one past last element in tokbuf */

    /* Scan throug the input: keep building a token while the elements can be
     * found in the hash; else stop, create a new token, push it to the tokens
     * array and restart at the savepoint. */
    while (*uc_str != 0) {
        if ((pEntry = find(pht, *uc_str, endidx > 0 ? SUBSIZE : INITSIZE)) != NULL) {
            tokbuf[bufidx++] = *uc_str++;
            if (pEntry->endpoint) { /* is a possible endpoint */
                endidx = bufidx;
                resumepucs = uc_str;
            }
            pht = pEntry->tEntries;
        } else {
            if (endidx == 0) { /* singleton unknown tok */
                endidx = 1;
                tokbuf[bufidx] = *uc_str;
                resumepucs++; /* resume one uc codepoint later */
            }

            /* copy token buf into new token memory */
            ptok = malloc(sizeof(uint32_t)*(endidx+1)); /* need room for terminator */
            for (i = 0; i < endidx; i++)
                ptok[i] = tokbuf[i];
            ptok[endidx] = 0; /* terminator */
            *tokens++ = ptok;
            numtokens++;

            /* realign leftover tokens */
            bufidx = 0;
            endidx = 0;
            uc_str = resumepucs;

            pht = tokenht; /* reset hashtable pointer */
        }
    }

    return numtokens;
}

/* Prints the token hash tables. Possible endpoints are marked with an
 * asterisk. */
void printDict(tEntry **ht, int size, int level, int verbose)
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
                printDict(pte->tEntries, SUBSIZE, level+1, verbose);
            }
        }
        else if (verbose) {
            for (j = 0; j < level; j++)
                printf("\t");
            printf("[%d]: NULL\n", i);
        }
    }
}
