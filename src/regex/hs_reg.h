/*************************************************************************/
/*                This code has been modified for Bellbird.              */
/*                See COPYING for more copyright details.                */
/*                The unmodified source code copyright notice            */
/*                is included below.                                     */
/*************************************************************************/
/*     This contains common macros for regcomp() and regexec()           */
/*************************************************************************/

/*     Copyright (c) 1986 by University of Toronto.
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

/*
 * The first byte of the regexp internal "program" is actually this magic
 * number; the start node begins in the second byte.
 */

#define CST_REGMAGIC    0234

/*
 * Structure for regexp "program".  This is essentially a linear encoding
 * of a nondeterministic finite-state machine (aka syntax charts or
 * "railroad normal form" in parsing technology).  Each node is an opcode
 * plus a "next" pointer, possibly plus an operand.  "Next" pointers of
 * all nodes except BRANCH implement concatenation; a "next" pointer with
 * a BRANCH on both ends of it is connecting two alternatives.  (Here we
 * have one of the subtle syntax dependencies:  an individual BRANCH (as
 * opposed to a collection of them) is never concatenated with anything
 * because of operator precedence.)  The operand of some types of node is
 * a literal string; for others, it is a node leading into a sub-FSM.  In
 * particular, the operand of a BRANCH node is the first node of the branch.
 * (NB this is *not* a tree structure:  the tail of the branch connects
 * to the thing following the set of BRANCHes.)  The opcodes are:
 */

/* definition    number    opnd?    meaning */
#define    END      0    /* no    End of program. */
#define    BOL      1    /* no    Match "" at beginning of line. */
#define    EOL      2    /* no    Match "" at end of line. */
#define    ANY      3    /* no    Match any one character. */
#define    ANYOF    4    /* str   Match any character in this string. */
#define    ANYBUT   5    /* str   Match any character not in this string. */
#define    BRANCH   6    /* node  Match this alternative, or the next... */
#define    BACK     7    /* no    Match "", "next" ptr points backward. */
#define    EXACTLY  8    /* str   Match this string. */
#define    NOTHING  9    /* no    Match empty string. */
#define    STAR    10    /* node  Match this (simple) thing 0 or more times. */
#define    PLUS    11    /* node  Match this (simple) thing 1 or more times. */
#define    WORDA   12    /* no    Match "" at wordchar, where prev is nonword */
#define    WORDZ   13    /* no    Match "" at nonwordchar, where prev is word */
#define    OPEN    20    /* no    Mark this point in input as start of #n. */
            /*    OPEN+1 is number 1, etc. */
#define    CLOSE   30    /* no    Analogous to OPEN. */

/*
 * Opcode notes:
 *
 * BRANCH    The set of branches constituting a single choice are hooked
 *        together with their "next" pointers, since precedence prevents
 *        anything being concatenated to any individual branch.  The
 *        "next" pointer of the last BRANCH in a choice points to the
 *        thing following the whole choice.  This is also where the
 *        final "next" pointer of each individual branch points; each
 *        branch starts with the operand node of a BRANCH node.
 *
 * BACK        Normal "next" pointers all implicitly point forward; BACK
 *        exists to make loop structures possible.
 *
 * STAR,PLUS    '?', and complex '*' and '+', are implemented as circular
 *        BRANCH structures using BACK.  Simple cases (one character
 *        per match) are implemented with STAR and PLUS for speed
 *        and to minimize recursive plunges.
 *
 * OPEN,CLOSE    ...are numbered at compile time.
 */

/*
 * A node is one char of opcode followed by two chars of "next" pointer.
 * "Next" pointers are stored as two 8-bit pieces, high order first.  The
 * value is a positive offset from the opcode of the node containing it.
 * An operand, if any, simply follows the node.  (Note that much of the
 * code generation knows about this implicit relationship.)
 *
 * Using two bytes for the "next" pointer is vast overkill for most things,
 * but allows patterns to get big without disasters.
 */
#define    OP(p)      (*(p))
#define    NEXT(p)    (((*((p)+1)&0377)<<8) + (*((p)+2)&0377))
#define    OPERAND(p) ((p) + 3)

/*
 * Utility definitions.
 */
#ifndef CHARBITS
#define    UCHARAT(p)   ((int)*(unsigned char *)(p))
#else
#define    UCHARAT(p)   ((int)*(p)&CHARBITS)
#endif

/*
 * The "internal use only" fields in cst_regex.h are present to pass info from
 * compile to execute that permits the execute phase to run lots faster on
 * simple cases.  They are:
 *
 * regstart    char that must begin a match; '\0' if none obvious
 * reganch    is the match anchored (at beginning-of-line only)?
 * regmust    string (pointer into program) that match must include, or NULL
 * regmlen    length of regmust string
 *
 * Regstart and reganch permit very fast decisions on suitable starting points
 * for a match, cutting down the work a lot.  Regmust permits fast rejection
 * of lines that cannot possibly match.  The regmust tests are costly enough
 * that regcomp() supplies a regmust only if the r.e. contains something
 * potentially expensive (at present, the only such thing detected is * or +
 * at the start of the r.e., which can involve a lot of backup).  Regmlen is
 * supplied because the test in regexec() needs it and regcomp() is computing
 * it anyway.
 */
