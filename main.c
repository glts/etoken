#include <stdio.h>
#include <stdlib.h>

#include "etoken.h"

/* initial size of dynamic array of tokens */
#define DYNTOKENS 256

int readTokenDefinitions(uint32_t **tokdefs, FILE *pf);
void printTokens(uint32_t **tokens, unsigned int size);

int main(int argc, char *argv[])
{
    int i;
    uint32_t *toksdef[64]; /* space for 64 token definitions */
    int ndefs; /* number of token definitions */
    tEntry **tokenht;

    /* 1. Construct token hash */

    /* Read tokens definitions from file */

    FILE *toksfile;
    if ((toksfile = fopen("toksdef.txt", "r")) == NULL)
        return 1;
    ndefs = readTokenDefinitions(toksdef, toksfile);
    if (fclose(toksfile) != 0)
        return 1;

    printf("Tokcount is %d\n", ndefs);

    /* Create token hash structure */

    tokenht = createTokenHash(toksdef, ndefs);

    for (i = 0; i < ndefs; i++) /* free memory of token definitions */
        free(toksdef[i]);

    printDict(tokenht, INITSIZE, 0, 0); /* print for debugging */


    /* Tokenize a file */

    FILE *fin;
    if ((fin = fopen("test.txt", "r")) == NULL)
        return 1;

    int numtoks = 0;
    int nalloc = DYNTOKENS;
    char filebuf[DYNTOKENS];
    uint32_t tokbuf[DYNTOKENS];
    uint32_t **dyntokens; /* dynamic storage for arbitrary num of tokens */
    dyntokens = malloc(sizeof(*dyntokens)*nalloc); /* start out with space for 256 toks */
    uint32_t **curdyntok = dyntokens;
    while (fgets(filebuf, DYNTOKENS, fin)) {
        utf8_to_uc(tokbuf, filebuf, DYNTOKENS);
        if (numtoks >= (nalloc-DYNTOKENS)) {
            nalloc *= 2;
            dyntokens = realloc(dyntokens, (sizeof(*dyntokens)*nalloc));
        }
        curdyntok = &dyntokens[numtoks];
        numtoks += tokenize(curdyntok, tokenht, tokbuf);
    }

    if (fclose(fin) != 0)
        return 1;

    printf("Found %d tokens (memory: %d)!\n", numtoks, nalloc);

    //printTokens(dyntokens, numtoks);

    /* deallocate everything */
    for (i = 0; i < numtoks; i++)
        free(dyntokens[i]);
    free(dyntokens);

    /* destroy token hash */
    destroyTokenHash(tokenht); // check return value

    return 0;
}

int readTokenDefinitions(uint32_t **tokdefs, FILE *pf)
{
    int i;
    int numtok = 0;
    char utf8buf[TOKENMAX*4]; /* buf for char sequences of length 16 */
    uint32_t ucbuf[TOKENMAX]; /* buffer length for single tokens in UC codepoints */
    int tokcount = 0;
    uint32_t *ptoksdef;

    while (fgets(utf8buf, sizeof(utf8buf), pf)) {
        numtok = utf8_to_uc(ucbuf, utf8buf, TOKENMAX); /* includes newline */
        ptoksdef = malloc(sizeof(uint32_t)*numtok); /* cut off newline */

        for (i = 0; i < numtok-1; i++)
            ptoksdef[i] = ucbuf[i];
        ptoksdef[numtok-1] = 0; /* overwrite newline with terminating null */
        tokdefs[tokcount++] = ptoksdef;
    }

    return tokcount;
}

/* Prints tokens in an array of pointers to tokens. The size of the array
 * of pointers needs to be given explicitly. */
void printTokens(uint32_t **tokens, unsigned int size)
{
    int i;
    char utf8tok[TOKENMAX*4];

    for (i = 0; i < size; i++) {
        uc_to_utf8(utf8tok, tokens[i], TOKENMAX*4);
        printf("Token: %s\n", utf8tok);
    }
}
