/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*     This contains regexec() and its helper functions which match      */
/*     regular expressions.                                              */
/*************************************************************************/

// This code is descended from code written by Alan W Black and David
// Huggins-Daines which is in turn descended from Edinburgh Speech Tools
// written by Richard Caley and others.

/*
 * regexec
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
#include "cst_alloc.h"
#include "cst_string.h"
#include "cst_file.h"
#include "cst_error.h"
#include "cst_regex.h"
#include "hs_reg.h"

#ifdef DEBUG
#include "bell_file.h"
#endif

#define    FAIL(m)      { cst_errmsg("regexp failure: %s\n", m); cst_error(); }

/*
 * regexec and friends
 */

/* dhd@cepstral.com changed all this stuff to use a state structure
   (and thus, be re-entrant) 2001-10-18 */

// Global work variable 
static char regdummy;

#ifdef DEBUG
#define regnarrate stdout
void regdump();
static char *regprop(char *scan);
#endif

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
 - regrepeat - repeatedly match something simple, report how many
 */
static int regrepeat(cst_regstate *state, char *p)
{
    int count = 0;
    const char *scan;
    char *opnd;

    scan = state->input;
    opnd = OPERAND(p);
    switch (OP(p)) {
    case ANY:
        count = cst_strlen(scan);
        scan += count;
        break;
    case EXACTLY:
        while (*opnd == *scan) {
            count++;
            scan++;
        }
        break;
    case ANYOF:
        while (*scan != '\0' && strchr(opnd, *scan) != NULL) {
            count++;
            scan++;
        }
        break;
    case ANYBUT:
        while (*scan != '\0' && strchr(opnd, *scan) == NULL) {
            count++;
            scan++;
        }
        break;
    default:        /* Oh dear.  Called inappropriately. */
        FAIL("internal foulup");
        count = 0;    /* Best compromise. */
        break;
    }
    state->input = scan;

    return(count);
}

/*
 - regmatch - main matching routine
 *
 * Conceptually the strategy is simple:  check to see whether the current
 * node matches, call self recursively to see whether the rest matches,
 * and then act accordingly.  In practice we make some effort to avoid
 * recursion, in particular by going through "ordinary" nodes (that don't
 * need to know whether the rest of the match failed) by a loop instead of
 * by recursion.
 */
static int regmatch(cst_regstate *state, char *scan)
/* returns: 0 failure, 1 success */
{
    char *next;        /* Next node. */

#ifdef DEBUG
    if (scan != NULL && regnarrate)
        fprintf(regnarrate, "%s(\n", regprop(scan));
#endif
    while (scan != NULL) {
#ifdef DEBUG
        if (regnarrate)
            fprintf(regnarrate, "%s...\n", regprop(scan));
#endif
        next = regnext(scan);

        switch (OP(scan)) {
        case BOL:
            if (state->input != state->bol)
                return(0);
            break;
        case EOL:
            if (*state->input != '\0')
                return(0);
            break;
        case WORDA:
            /* Must be looking at a letter, digit, or _ */
            if ((!isalnum((int)*state->input)) && *state->input != '_')
                return(0);
            /* Prev must be BOL or nonword */
            if (state->input > state->bol &&
                (isalnum((int)state->input[-1]) || state->input[-1] == '_'))
                return(0);
            break;
        case WORDZ:
            /* Must be looking at non letter, digit, or _ */
            if (isalnum((int)*state->input) || *state->input == '_')
                return(0);
            /* We don't care what the previous char was */
            break;
        case ANY:
            if (*state->input == '\0')
                return(0);
            state->input++;
            break;
        case EXACTLY: {
                int len;
                char *opnd;

                opnd = OPERAND(scan);
                /* Inline the first character, for speed. */
                if (*opnd != *state->input)
                    return(0);
                len = cst_strlen(opnd);
                if (len > 1 && strncmp(opnd, state->input, len) != 0)
                    return(0);
                state->input += len;
            }
            break;
        case ANYOF:
             if (*state->input == '\0' || strchr(OPERAND(scan), *state->input) == NULL)
                return(0);
            state->input++;
            break;
        case ANYBUT:
             if (*state->input == '\0' || strchr(OPERAND(scan), *state->input) != NULL)
                return(0);
            state->input++;
            break;
        case NOTHING:
            break;
        case BACK:
            break;
        case OPEN+1:
        case OPEN+2:
        case OPEN+3:
        case OPEN+4:
        case OPEN+5:
        case OPEN+6:
        case OPEN+7:
        case OPEN+8:
        case OPEN+9: {
                int no;
                const char *save;

                no = OP(scan) - OPEN;
                save = state->input;

                if (regmatch(state, next)) {
                    /*
                     * Don't set startp if some later
                     * invocation of the same parentheses
                     * already has.
                     */
                    if (state->startp[no] == NULL)
                      state->startp[no] = save;
                    return(1);
                } else
                    return(0);
            }
            break;
        case CLOSE+1:
        case CLOSE+2:
        case CLOSE+3:
        case CLOSE+4:
        case CLOSE+5:
        case CLOSE+6:
        case CLOSE+7:
        case CLOSE+8:
        case CLOSE+9: {
                int no;
                const char *save;

                no = OP(scan) - CLOSE;
                save = state->input;

                if (regmatch(state, next)) {
                    /*
                     * Don't set endp if some later
                     * invocation of the same parentheses
                     * already has.
                     */
                    if (state->endp[no] == NULL)
                      state->endp[no] = save;
                    return(1);
                } else
                    return(0);
            }
            break;
        case BRANCH: {
                const char *save;

                if (OP(next) != BRANCH)        /* No choice. */
                    next = OPERAND(scan);    /* Avoid recursion. */
                else {
                    do {
                        save = state->input;
                        if (regmatch(state, OPERAND(scan)))
                            return(1);
                        state->input = save;
                        scan = regnext(scan);
                    } while (scan != NULL && OP(scan) == BRANCH);
                    return(0);
                    /* NOTREACHED */
                }
            }
            break;
        case STAR:
        case PLUS: {
                char nextch;
                int no;
                const char *save;
                int min;

                /*
                 * Lookahead to avoid useless match attempts
                 * when we know what character comes next.
                 */
                nextch = '\0';
                if (OP(next) == EXACTLY)
                    nextch = *OPERAND(next);
                min = (OP(scan) == STAR) ? 0 : 1;
                save = state->input;

                no = regrepeat(state, OPERAND(scan));
                while (no >= min) {
                    /* If it could work, try it. */
                    if (nextch == '\0' || *state->input == nextch)
                      {
                        if (regmatch(state, next))
                          return(1);
                      }
                    /* Couldn't or didn't -- back up. */
                    no--;
                    state->input = save + no;
                }
                return(0);
            }
            break;
        case END:
            return(1);    /* Success! */
            break;
        default:
            FAIL("memory corruption");
            return(0);
            break;
        }

        scan = next;
    }

    /*
     * We get here only if there's trouble -- normally "case END" is
     * the terminating point.
     */
    FAIL("corrupted pointers");
    return(0);
}

/*
 - regtry - try match at specific point
 */
static int regtry(cst_regstate *state, const char *string, char *prog)
/* returns: 0 failure, 1 success */
{
    int i;
    const char **sp;
    const char **ep;

    state->input = string;

    sp = state->startp;
    ep = state->endp;
    for (i = CST_NSUBEXP; i > 0; i--) {
        *sp++ = NULL;
        *ep++ = NULL;
    }
    if (regmatch(state, prog)) {
      state->startp[0] = (char *)string;
      state->endp[0] = (char *)state->input;
        return(1);
    } else
        return(0);
}

/*
 - regexec - match a regexp against a string
 */
cst_regstate * hs_regexec(const cst_regex *prog, const char *string)
{
    cst_regstate *state;
    char *s;

    /* Be paranoid... */
    if (prog == NULL || string == NULL) {
        FAIL("NULL parameter");
        return(0);
    }

    /* Check validity of program. */
    if (UCHARAT(prog->program) != CST_REGMAGIC) {
        FAIL("corrupted program");
        return(0);
    }

    /* If there is a "must appear" string, look for it. */
    if (prog->regmust != NULL) {
        s = (char *)string;
        while ((s = strchr(s, prog->regmust[0])) != NULL) {
            if (strncmp(s, prog->regmust, prog->regmlen) == 0)
                break;    /* Found it. */
            s++;
        }
        if (s == NULL)    /* Not present. */
            return(0);
    }

    state = cst_alloc(cst_regstate, 1);
    /* Mark beginning of line for ^ . */
    state->bol = string;

    /* Simplest case:  anchored match need be tried only once. */
    if (prog->reganch) {
        if (regtry(state, string, prog->program+1))
            return state;
        else {
            cst_free(state);
            return NULL;
        }
    }

    /* Messy cases:  unanchored match. */
    s = (char *)string;
    if (prog->regstart != '\0')
        /* We know what char it must start with. */
        while ((s = strchr(s, prog->regstart)) != NULL) {
            if (regtry(state, s, prog->program+1))
                return state;
            s++;
        }
    else
        /* We don't -- general case. */
        do {
            if (regtry(state, s, prog->program+1))
                return state;
        } while (*s++ != '\0');

    cst_free(state);
    return NULL;
}

#ifdef DEBUG

static char *regprop(char *scan);

/*
 - regdump - dump a regexp onto stdout in vaguely comprehensible form
 */
void regdump(cst_regex *r)
{
    char *s;
    char op = EXACTLY;    /* Arbitrary non-END op. */
    char *next;


    s = r->program + 1;
    while (op != END) {    /* While that wasn't END last time... */
        op = OP(s);
        printf("%2ld%s", s-r->program, regprop(s));    /* Where, what. */
        next = regnext(s);
        if (next == NULL)        /* Next ptr. */
            printf("(0)");
        else
            printf("(%ld)", (s-r->program)+(next-s));
        s += 3;
        if (op == ANYOF || op == ANYBUT || op == EXACTLY) {
            /* Literal string, where present. */
            while (*s != '\0') {
                putchar(*s);
                s++;
            }
            s++;
        }
        putchar('\n');
    }

    /* Header fields of interest. */
    if (r->regstart != '\0')
        printf("start `%c' ", r->regstart);
    if (r->reganch)
        printf("anchored ");
    if (r->regmust != NULL)
        printf("must have \"%s\"", r->regmust);
    printf("\n");
}

/*
 - regprop - printable representation of opcode
 */
static char * regprop(char *op)
{
    char *p=NULL;
    static char buf[50];

    (void) strcpy(buf, ":");

    switch (OP(op)) {
    case BOL:
        p = "BOL";
        break;
    case EOL:
        p = "EOL";
        break;
    case ANY:
        p = "ANY";
        break;
    case ANYOF:
        p = "ANYOF";
        break;
    case ANYBUT:
        p = "ANYBUT";
        break;
    case BRANCH:
        p = "BRANCH";
        break;
    case EXACTLY:
        p = "EXACTLY";
        break;
    case NOTHING:
        p = "NOTHING";
        break;
    case BACK:
        p = "BACK";
        break;
    case END:
        p = "END";
        break;
    case OPEN+1:
    case OPEN+2:
    case OPEN+3:
    case OPEN+4:
    case OPEN+5:
    case OPEN+6:
    case OPEN+7:
    case OPEN+8:
    case OPEN+9:
        bell_snprintf(buf+cst_strlen(buf),50-cst_strlen(buf), "OPEN%d", OP(op)-OPEN);
        p = NULL;
        break;
    case CLOSE+1:
    case CLOSE+2:
    case CLOSE+3:
    case CLOSE+4:
    case CLOSE+5:
    case CLOSE+6:
    case CLOSE+7:
    case CLOSE+8:
    case CLOSE+9:
        bell_snprintf(buf+cst_strlen(buf),50-cst_strlen(buf), "CLOSE%d", OP(op)-CLOSE);
        p = NULL;
        break;
    case STAR:
        p = "STAR";
        break;
    case PLUS:
        p = "PLUS";
        break;
    case WORDA:
        p = "WORDA";
        break;
    case WORDZ:
        p = "WORDZ";
        break;
    default:
        FAIL("corrupted opcode");
        break;
    }
    if (p != NULL)
        (void) strcat(buf, p);
    return(buf);
}
#endif
