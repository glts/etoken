#include <stdio.h>
#include <stdlib.h>

#include "etoken.h"

/* initial size of dynamic array of tokens */
#define DYNTOKENS 256

int readTokenDefinitions(uint32_t **tokdefs, FILE *pf);
void printTokens(uint32_t **tokens, unsigned int size);

/* Example etoken client. */
int main(int argc, char *argv[])
{
    int i;
    uint32_t *toksdef[64]; /* space for 64 token definitions */
    int ndefs; /* number of token definitions */
    tEntry **tokenht;
    FILE *fp;

    /* Read token definitions from file and create the hash */

    if ((fp = fopen("exampledef.txt", "r")) == NULL)
        return 1;
    ndefs = readTokenDefinitions(toksdef, fp);
    if (fclose(fp) != 0)
        return 1;

    tokenht = createTokenHash(toksdef, ndefs);

    for (i = 0; i < ndefs; i++) /* free memory of token definitions */
        free(toksdef[i]);

    /* Print the token hash */
    printf("Created token hash for %d token definitions\n", ndefs);
    printDict(tokenht, INITSIZE, 0);


    /* Tokenize a file */

    if ((fp = fopen("example.txt", "r")) == NULL)
        return 1;

    int ntokens = 0;
    int nalloc = DYNTOKENS;
    char filebuf[DYNTOKENS*4];
    uint32_t tokbuf[DYNTOKENS];
    uint32_t **dyntokens; /* dynamic storage for arbitrary num of tokens */
    dyntokens = malloc(sizeof(*dyntokens)*nalloc); /* start out with space for 256 toks */
    uint32_t **pdyntok = dyntokens;
    while (fgets(filebuf, DYNTOKENS*4, fp)) {
        utf8_to_uc(tokbuf, filebuf, DYNTOKENS);
        if (ntokens >= (nalloc-DYNTOKENS)) {
            nalloc *= 2;
            dyntokens = realloc(dyntokens, (sizeof(*dyntokens)*nalloc));
        }
        pdyntok = &dyntokens[ntokens];
        ntokens += tokenize(pdyntok, tokenht, tokbuf);
    }

    if (fclose(fp) != 0)
        return 1;

    printf("\nFound %d tokens (memory: %d)!\n", ntokens, nalloc);
    printTokens(dyntokens, ntokens);

    /* deallocate everything */
    for (i = 0; i < ntokens; i++)
        free(dyntokens[i]);
    free(dyntokens);

    destroyTokenHash(tokenht); /* destroy token hash */

    return 0;
}

/* Reads token definitions from a file into an array. The caller must free the
 * memory after use. */
int readTokenDefinitions(uint32_t **tokdefs, FILE *fp)
{
    int i;
    int numtok = 0;
    char utf8buf[TOKENMAX*4]; /* buf for char sequences of length 16 */
    uint32_t ucbuf[TOKENMAX]; /* buffer length for single tokens in UC codepoints */
    int tokcount = 0;
    uint32_t *ptoksdef;

    while (fgets(utf8buf, sizeof(utf8buf), fp)) {
        numtok = utf8_to_uc(ucbuf, utf8buf, TOKENMAX); /* includes newline */
        ptoksdef = malloc(sizeof(uint32_t)*numtok); /* cut off newline */

        for (i = 0; i < numtok-1; i++)
            ptoksdef[i] = ucbuf[i];
        ptoksdef[numtok-1] = 0; /* overwrite newline with terminating null */
        tokdefs[tokcount++] = ptoksdef;
    }

    return tokcount;
}

/* Prints tokens in an array of pointers to tokens. The size of the array of
 * pointers needs to be given explicitly. */
void printTokens(uint32_t **tokens, unsigned int size)
{
    int i;
    char utf8tok[TOKENMAX*4];

    for (i = 0; i < size; i++) {
        uc_to_utf8(utf8tok, tokens[i], TOKENMAX*4);
        printf("Token: %s\n", utf8tok);
    }
}
