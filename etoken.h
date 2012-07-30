#ifndef ETOKEN_H
#define ETOKEN_H

#include <stdint.h>

/* enable ASCII case-folding for lookups */
#define IGNORECASE

/* maximum length of a single token in UC codepoints including 0 terminator */
#define TOKENMAX 8

/* size of the root hash and leaf hashes */
#define INITSIZE 32
#define SUBSIZE 4

/* Simple hash function for uint32_t Unicode code points. */
/* Extremely simple. Horrible distribution for size = 64 ... */
#define UCHASH(uc, size) ((uc + 3) % size)

/* UTF-8 to Unicode conversion */
int utf8_to_uc(uint32_t *uc_str, const char *utf8_str, unsigned int size);
int uc_to_utf8(char *utf8_str, const uint32_t *uc_str, unsigned int size);

/* Token element, an entry in the token hashtable */
typedef struct tEntry {
    uint32_t val;
    struct tEntry *nextEntry; /* next in linked list */
    int endpoint; /* boolean, possible endpoint */
    struct tEntry *tEntries[SUBSIZE];
} tEntry;

tEntry **createTokenHash(uint32_t *tokenDef[], unsigned int size);
tEntry *addToken(tEntry **ht, const uint32_t *toks);
tEntry *insertEntry(tEntry **ht, uint32_t val, unsigned int size);
int tokenize(uint32_t **tokens, tEntry **tokenht, const uint32_t *uc_str);
tEntry *find(tEntry **ht, uint32_t val, unsigned int size);
void destroyTokenHash(tEntry **te);
void destroyEntry(tEntry *te);
void printDict(tEntry **at, int size, int level);

#endif
