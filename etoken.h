#ifndef ETOKEN_H
#define ETOKEN_H

#include <stdint.h>

/* enable ASCII-only case-folding for lookups */
#define CASEFOLD

/* maximum length of a single token in UC codepoints including 0 terminator */
#define TOKENMAX 8

/* size of the root hash and leaf hashes */
#define INITSIZE 32
#define SUBSIZE 4

/* hash macro */
/* Simple hash function for uint32_t Unicode code points. */
#define UCHASH(uc, size) ((uc + 3) % size)
/* Extremely simple hash function for uint32_t code points */
/* horrible distribution for size = 64 ! */

/* Token atom, an entry in the token hashtable */
typedef struct tEntry {
    uint32_t val;
    struct tEntry *nextEntry; /* next in linked list */
    int endpoint; /* boolean, possible endpoint? */
    struct tEntry *tEntries[SUBSIZE];
} tEntry;

/* Public interface */
int tokenize(uint32_t **tokens, tEntry **tokenht, const uint32_t *uc_str);

tEntry *insertEntry(tEntry **ht, uint32_t val, unsigned int size);
void destroyEntry(tEntry *te);
tEntry* addToken(tEntry **ht, const uint32_t *toks);
void printDict(tEntry **at, int size, int level, int verbose);
tEntry *find(tEntry **ht, uint32_t val, unsigned int size);

tEntry **createTokenHash(uint32_t *tokenDef[], unsigned int size);
void destroyTokenHash(tEntry **te);

/* UTF-8 to Unicode conversion */
int utf8_to_uc(uint32_t *uc_str, const char *utf8_str, unsigned int size);
int uc_to_utf8(char *utf8_str, const uint32_t *uc_str, unsigned int size);

#endif
