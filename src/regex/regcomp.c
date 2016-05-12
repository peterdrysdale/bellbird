/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*     This contains regcomp() and its helper functions which compile    */
/*     regular expressions and is used in compile_regexes.c.             */
/*************************************************************************/

// This code is descended from code written by Alan W Black and David
// Huggins-Daines which is in turn descended from Edinburgh Speech Tools
// written by Richard Caley and others.

/*
 * regcomp
 *
 *     Copyright (c) 1986 by University of Toronto.
 *     Written by Henry Spencer.  Not derived from licensed software.
 *
 *     Permission is granted to anyone to use this software for any
 *     purpose on any computer system, and to redistribute it freely,
 *     subject to the following restrictions:
 *
 *     1. The author is not responsible for the consequences of use of
 *             this software, no matter how awful, even if they arise
 *             from defects in it.
 *
 *     2. The origin of this software must not be misrepresented, either
 *             by explicit claim or by omission.
 *
 *     3. Altered versions must be plainly marked as such, and must not
 *             be misrepresented as being the original software.
 *** THIS IS AN ALTERED VERSION.  It was altered by John Gilmore,
 *** hoptoad!gnu, on 27 Dec 1986, to add \n as an alternative to |
 *** to assist in implementing egrep.
 *** THIS IS AN ALTERED VERSION.  It was altered by John Gilmore,
 *** hoptoad!gnu, on 27 Dec 1986, to add \< and \> for word-matching
 *** as in BSD grep and ex.
 *** THIS IS AN ALTERED VERSION.  It was altered by John Gilmore,
 *** hoptoad!gnu, on 28 Dec 1986, to optimize characters quoted with \.
 *** THIS IS AN ALTERED VERSION.  It was altered by James A. Woods,
 *** ames!jaw, on 19 June 1987, to quash a regcomp() redundancy.
 *
 * Beware that some of this code is subtly aware of the way operator
 * precedence is structured in regular expressions.  Serious changes in
 * regular-expression syntax might require a total rethink.  */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "hs_reg.h"
#include "cst_regex.h"

// compile_regexes.c provides a wrapped memory allocator
// which we use to ensure no dependencies on the main bellbird library
void *wrapped_calloc(int size);

#define    FAIL(m)      { printf("regexp failure: %s\n", m); exit(1); }
#define    ISMULT(c)    ((c) == '*' || (c) == '+' || (c) == '?')

/*
 * Flags to be passed up and down.
 */
#define    HASWIDTH    01    /* Known never to match null string. */
#define    SIMPLE      02    /* Simple enough to be STAR/PLUS operand. */
#define    SPSTART     04    /* Starts with * or +. */
#define    WORST        0    /* Worst case. */

/*
 * Global work variables for regcomp().
 */
static const char *regparse;        /* Input-scan pointer. */
static int regnpar;                 /* () count. */
static char regdummy;
static char *regcode;       /* Code-emit pointer; &regdummy = don't. */
static long regsize;        /* Code size. */

// Forward declaration of reg() as recomp() helper
// functions contain circular dependencies
static char *reg(int paren, int *flagp);

void hs_regdelete(cst_regex *r)
{
    free(r->program);
    free(r);
}

/*
 - regnode - emit a node
 */
static char * regnode(char op) /* Location. */
{
    char *ret;
    char *ptr;

    ret = regcode;
    if (ret == &regdummy) {
        regsize += 3;
        return(ret);
    }

    ptr = ret;
    *ptr++ = op;
    *ptr++ = '\0';        /* Null "next" pointer. */
    *ptr++ = '\0';
    regcode = ptr;

    return(ret);
}

/*
 - regnext - dig the "next" pointer out of a node
 */
static char * regnext(char *p)
{
    int offset;

    if (p == &regdummy)
        return(NULL);

    offset = NEXT(p);
    if (offset == 0)
        return(NULL);

    if (OP(p) == BACK)
        return(p-offset);
    else
        return(p+offset);
}

/*
 - regtail - set the next-pointer at the end of a node chain
 */
static void regtail(char *p, char *val)
{
    char *scan;
    char *temp;
    int offset;

    if (p == &regdummy)
        return;

    /* Find last node. */
    scan = p;
    for (;;) {
        temp = regnext(scan);
        if (temp == NULL)
            break;
        scan = temp;
    }

    if (OP(scan) == BACK)
        offset = scan - val;
    else
        offset = val - scan;
    *(scan+1) = (offset>>8)&0377;
    *(scan+2) = offset&0377;
}

/*
 - regoptail - regtail on operand of first argument; nop if operandless
 */
static void regoptail(char *p, char *val)
{
    /* "Operandless" and "op != BRANCH" are synonymous in practice. */
    if (p == NULL || p == &regdummy || OP(p) != BRANCH)
        return;
    regtail(OPERAND(p), val);
}

/*
 - regc - emit (if appropriate) a byte of code
 */
static void regc(char b)
{
    if (regcode != &regdummy)
        *regcode++ = b;
    else
        regsize++;
}

/*
 - reginsert - insert an operator in front of already-emitted operand
 *
 * Means relocating the operand.
 */
static void reginsert(char op, char *opnd)
{
    char *src;
    char *dst;
    char *place;

    if (regcode == &regdummy) {
        regsize += 3;
        return;
    }

    src = regcode;
    regcode += 3;
    dst = regcode;
    while (src > opnd)
        *--dst = *--src;

    place = opnd;        /* Op node, where operand used to be. */
    *place++ = op;
    *place++ = '\0';
    *place++ = '\0';
}

/*
 - regatom - the lowest level
 *
 * Optimization:  gobbles an entire sequence of ordinary characters so that
 * it can turn them into a single node, which is smaller to store and
 * faster to run.  Backslashed characters are exceptions, each becoming a
 * separate node; the code is simpler that way and it's not worth fixing.
 */
static char * regatom(int *flagp)
{
    char *ret = NULL;
    int flags;

    *flagp = WORST;        /* Tentatively. */

    switch (*regparse++) {
    /* FIXME: these chars only have meaning at beg/end of pat? */
    case '^':
        ret = regnode(BOL);
        break;
    case '$':
        ret = regnode(EOL);
        break;
    case '.':
        ret = regnode(ANY);
        *flagp |= HASWIDTH|SIMPLE;
        break;
    case '[': {
            int class1;
            int classend;

            if (*regparse == '^') {    /* Complement of range. */
                ret = regnode(ANYBUT);
                regparse++;
            } else
                ret = regnode(ANYOF);
            if (*regparse == ']' || *regparse == '-')
                regc(*regparse++);
            while (*regparse != '\0' && *regparse != ']') {
                if (*regparse == '-') {
                    regparse++;
                    if (*regparse == ']' || *regparse == '\0')
                        regc('-');
                    else {
                        class1 = UCHARAT(regparse-2)+1;
                        classend = UCHARAT(regparse);
                        if (class1 > classend+1)
                            FAIL("invalid [] range");
                        for (; class1 <= classend; class1++)
                            regc(class1);
                        regparse++;
                    }
                } else
                    regc(*regparse++);
            }
            regc('\0');
            if (*regparse != ']')
                FAIL("unmatched []");
            regparse++;
            *flagp |= HASWIDTH|SIMPLE;
        }
        break;
    case '(':
        ret = reg(1, &flags);
        if (ret == NULL)
            return(NULL);
        *flagp |= flags&(HASWIDTH|SPSTART);
        break;
    case '\0':
    case '|':
    case '\n':
    case ')':
        FAIL("internal urp");    /* Supposed to be caught earlier. */
        break;
    case '?':
    case '+':
    case '*':
        FAIL("?+* follows nothing");
        break;
    case '\\':
        switch (*regparse++) {
        case '\0':
            FAIL("trailing \\");
            break;
        case '<':
            ret = regnode(WORDA);
            break;
        case '>':
            ret = regnode(WORDZ);
            break;
        /* FIXME: Someday handle \1, \2, ... */
        default:
            /* Handle general quoted chars in exact-match routine */
            goto de_fault;
        }
        break;
    de_fault:
    default:
        /*
         * Encode a string of characters to be matched exactly.
         *
         * This is a bit tricky due to quoted chars and due to
         * '*', '+', and '?' taking the SINGLE char previous
         * as their operand.
         *
         * On entry, the char at regparse[-1] is going to go
         * into the string, no matter what it is.  (It could be
         * following a \ if we are entered from the '\' case.)
         *
         * Basic idea is to pick up a good char in  ch  and
         * examine the next char.  If it's *+? then we twiddle.
         * If it's \ then we frozzle.  If it's other magic char
         * we push  ch  and terminate the string.  If none of the
         * above, we push  ch  on the string and go around again.
         *
         *  regprev  is used to remember where "the current char"
         * starts in the string, if due to a *+? we need to back
         * up and put the current char in a separate, 1-char, string.
         * When  regprev  is NULL,  ch  is the only char in the
         * string; this is used in *+? handling, and in setting
         * flags |= SIMPLE at the end.
         */
        {
            const char *regprev;
            char ch = 0;

            regparse--;                 /* Look at cur char */
            ret = regnode(EXACTLY);
            for ( regprev = 0 ; ; ) {
                ch = *regparse++;       /* Get current char */
                switch (*regparse) {    /* look at next one */

                default:
                    regc(ch);           /* Add cur to string */
                    break;

                case '.': case '[': case '(':
                case ')': case '|': case '\n':
                case '$': case '^':
                case '\0':
                /* FIXME, $ and ^ should not always be magic */
                magic:
                    regc(ch);           /* dump cur char */
                    goto done;          /* and we are done */

                case '?': case '+': case '*':
                    if (!regprev)       /* If just ch in str, */
                        goto magic;     /* use it */
                    /* End mult-char string one early */
                    regparse = regprev; /* Back up parse */
                    goto done;

                case '\\':
                    regc(ch);    /* Cur char OK */
                    switch (regparse[1]){ /* Look after \ */
                    case '\0':
                    case '<':
                    case '>':
                    /* FIXME: Someday handle \1, \2, ... */
                        goto done;        /* Not quoted */
                    default:
                        /* Backup point is \, scan * point is after it. */
                        regprev = regparse;
                        regparse++;
                        continue;         /* NOT break; */
                    }
                }
                regprev = regparse;    /* Set backup point */
            }
        done:
            regc('\0');
            *flagp |= HASWIDTH;
            if (!regprev)        /* One char? */
                *flagp |= SIMPLE;
        }
        break;
    }

    return(ret);
}

/*
 - regpiece - something followed by possible [*+?]
 *
 * Note that the branching code sequences used for ? and the general cases
 * of * and + are somewhat optimized:  they use the same NOTHING node as
 * both the endmarker for their branch list and the body of the last branch.
 * It might seem that this node could be dispensed with entirely, but the
 * endmarker role is not redundant.
 */
static char * regpiece(int *flagp)
{
    char *ret;
    char op;
    char *next;
    int flags;

    ret = regatom(&flags);
    if (ret == NULL)
        return(NULL);

    op = *regparse;
    if (!ISMULT(op)) {
        *flagp = flags;
        return(ret);
    }

    if (!(flags&HASWIDTH) && op != '?')
        FAIL("*+ operand could be empty");
    *flagp = (op != '+') ? (WORST|SPSTART) : (WORST|HASWIDTH);

    if (op == '*' && (flags&SIMPLE))
        reginsert(STAR, ret);
    else if (op == '*') {
        /* Emit x* as (x&|), where & means "self". */
        reginsert(BRANCH, ret);            /* Either x */
        regoptail(ret, regnode(BACK));     /* and loop */
        regoptail(ret, ret);               /* back */
        regtail(ret, regnode(BRANCH));     /* or */
        regtail(ret, regnode(NOTHING));    /* null. */
    } else if (op == '+' && (flags&SIMPLE))
        reginsert(PLUS, ret);
    else if (op == '+') {
        /* Emit x+ as x(&|), where & means "self". */
        next = regnode(BRANCH);            /* Either */
        regtail(ret, next);
        regtail(regnode(BACK), ret);       /* loop back */
        regtail(next, regnode(BRANCH));    /* or */
        regtail(ret, regnode(NOTHING));    /* null. */
    } else if (op == '?') {
        /* Emit x? as (x|) */
        reginsert(BRANCH, ret);            /* Either x */
        regtail(ret, regnode(BRANCH));     /* or */
        next = regnode(NOTHING);           /* null. */
        regtail(ret, next);
        regoptail(ret, next);
    }
    regparse++;
    if (ISMULT(*regparse))
        FAIL("nested *?+");

    return(ret);
}

/*
 - regbranch - one alternative of an | operator
 *
 * Implements the concatenation operator.
 */
static char * regbranch(int *flagp)
{
    char *ret;
    char *chain;
    char *latest;
    int flags;

    *flagp = WORST;        /* Tentatively. */

    ret = regnode(BRANCH);
    chain = NULL;
    while (*regparse != '\0' && *regparse != ')' &&
           *regparse != '\n' && *regparse != '|') {
        latest = regpiece(&flags);
        if (latest == NULL)
            return(NULL);
        *flagp |= flags&HASWIDTH;
        if (chain == NULL)    /* First piece. */
            *flagp |= flags&SPSTART;
        else
            regtail(chain, latest);
        chain = latest;
    }
    if (chain == NULL)    /* Loop ran zero times. */
        (void) regnode(NOTHING);

    return(ret);
}

/*
 - reg - regular expression, i.e. main body or parenthesized thing
 *
 * Caller must absorb opening parenthesis.
 *
 * Combining parenthesis handling with the base level of regular expression
 * is a trifle forced, but the need to tie the tails of the branches to what
 * follows makes it hard to avoid.
 */
static char * reg(int paren, int *flagp)
                      /* Parenthesized? */
{
    char *ret;
    char *br;
    char *ender;
    int parno=0;
    int flags;

    *flagp = HASWIDTH;    /* Tentatively. */

    /* Make an OPEN node, if parenthesized. */
    if (paren) {
        if (regnpar >= CST_NSUBEXP)
            FAIL("too many ()");
        parno = regnpar;
        regnpar++;
        ret = regnode(OPEN+parno);
    } else
        ret = NULL;

    /* Pick up the branches, linking them together. */
    br = regbranch(&flags);
    if (br == NULL)
        return(NULL);
    if (ret != NULL)
        regtail(ret, br);    /* OPEN -> first. */
    else
        ret = br;
    if (!(flags&HASWIDTH))
        *flagp &= ~HASWIDTH;
    *flagp |= flags&SPSTART;
    while (*regparse == '|' || *regparse == '\n') {
        regparse++;
        br = regbranch(&flags);
        if (br == NULL)
            return(NULL);
        regtail(ret, br);    /* BRANCH -> BRANCH. */
        if (!(flags&HASWIDTH))
            *flagp &= ~HASWIDTH;
        *flagp |= flags&SPSTART;
    }

    /* Make a closing node, and hook it on the end. */
    ender = regnode((paren) ? CLOSE+parno : END);
    regtail(ret, ender);

    /* Hook the tails of the branches to the closing node. */
    for (br = ret; br != NULL; br = regnext(br))
        regoptail(br, ender);

    /* Check for proper termination. */
    if (paren && *regparse++ != ')') {
        FAIL("unmatched ()");
    } else if (!paren && *regparse != '\0') {
        if (*regparse == ')') {
            FAIL("unmatched ()");
        } else
            FAIL("junk on end");    /* "Can't happen". */
        /* NOTREACHED */
    }

    return(ret);
}

/*
 - regcomp - compile a regular expression into internal code
 *
 * We can't allocate space until we know how big the compiled form will be,
 * but we can't compile it (and thus know how big it is) until we've got a
 * place to put the code.  So we cheat:  we compile it twice, once with code
 * generation turned off and size counting turned on, and once "for real".
 * This also means that we don't allocate space until we are sure that the
 * thing really will compile successfully, and we never have to move the
 * code and thus invalidate pointers into it.  (Note that it has to be in
 * one piece because free() must be able to free it all.)
 *
 * Beware that the optimization-preparation code in here knows about some
 * of the structure of the compiled regexp.
 */
cst_regex * hs_regcomp(const char *exp)
{
        cst_regex *r;
    char *scan;
    char *longest;
    unsigned int len;
    int flags;
    if (exp == NULL)
        FAIL("NULL argument");

    /* First pass: determine size, legality. */
#ifdef notdef
    if (exp[0] == '.' && exp[1] == '*') exp += 2;  /* aid grep */
#endif
    regparse = exp;
    regnpar = 1;
    regsize = 0L;
    regcode = &regdummy;
    regc(CST_REGMAGIC);
    if (reg(0, &flags) == NULL)
        return(NULL);

    /* Small enough for pointer-storage convention? */
    if (regsize >= 32767L)        /* Probably could be 65535L. */
        FAIL("regexp too big");

    /* Allocate space. */
    r = (cst_regex *)wrapped_calloc(sizeof(cst_regex));
    r->regsize = regsize;
    r->program = (char *)wrapped_calloc(sizeof(char)*regsize);
    if (r == NULL)
        FAIL("out of space");

    /* Second pass: emit code. */
    regparse = exp;
    regnpar = 1;
    regcode = r->program;
    regc(CST_REGMAGIC);
    if (reg(0, &flags) == NULL)
        return(NULL);

    /* Dig out information for optimizations. */
    r->regstart = '\0';    /* Worst-case defaults. */
    r->reganch = 0;
    r->regmust = NULL;
    r->regmlen = 0;
    scan = r->program+1;            /* First BRANCH. */

    if (OP(regnext(scan)) == END) {        /* Only one top-level choice. */
        scan = OPERAND(scan);

        /* Starting-point info. */
        if (OP(scan) == EXACTLY)
            r->regstart = *OPERAND(scan);
        else if (OP(scan) == BOL)
            r->reganch++;

        /*
         * If there's something expensive in the r.e., find the
         * longest literal string that must appear and make it the
         * regmust.  Resolve ties in favor of later strings, since
         * the regstart check works with the beginning of the r.e.
         * and avoiding duplication strengthens checking.  Not a
         * strong reason, but sufficient in the absence of others.
         */
        if (flags&SPSTART) {
            longest = NULL;
            len = 0;
            for (; scan != NULL; scan = regnext(scan))
                if ((OP(scan) == EXACTLY) && 
                    (strlen(OPERAND(scan)) >= len)) {
                    longest = OPERAND(scan);
                    len = strlen(OPERAND(scan));
                }
            r->regmust = longest;
            r->regmlen = len;
        }
    }
    return(r);
}
