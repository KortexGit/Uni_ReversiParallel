/* Copyright (C) 2007-2021
 * Free Software Foundation, Inc.
 *
 *  Gaius Mulley <gaius@glam.ac.uk> wrote Tiny Othello.
 */

/*
This file is part of GNU Tiny Othello

GNU Tiny Othello is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2, or (at your option)
any later version.

GNU Tiny Othello is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tiny Othello; see the file COPYING.  If not, write to the
Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA
02110-1301, USA.
*/


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if !defined(SEQUENTIAL)
#  include "multiprocessor.h"
#  include "mailbox.h"
#endif

#if !defined(TRUE)
#  define TRUE (1==1)
#endif

#if !defined(FALSE)
#  define FALSE (1==0)
#endif


#define MAXX                         8
#define MAXY                         8
#define MAXPOS                       (MAXX*MAXY)
#define IS_OUR_COLOUR(COLOUR,USED,BIT,COL)  \
                                     (IN((USED),(BIT)) && (IN((COLOUR),(BIT)) == (COL)))
#define GET_COLOUR(COLOUR,BIT)       (IN((COLOUR),(BIT)))
#define UN_USED(SET,BIT)             (IN((SET),(BIT)) == 0)
#define IS_USED(SET,BIT)             (IN((SET),(BIT)) == 1)
#define LEFT_EDGE(BIT)               ((BIT) & (~7))
#define RIGHT_EDGE(BIT)              ((BIT) | 7)
#define TOP_EDGE(BIT)                ((BIT) | (32+16+8))
#define BOT_EDGE(BIT)                ((BIT) & (~(32+16+8)))

#define ASSERT(X)                    do { if (!(X)) { fprintf(stderr, "%s:%d: assert failed\n", __FILE__, __LINE__); exit(1); } } while (0);

#define INITIALPLY          6
#define MAXPLY             14
#define MAXMOVES           60

#undef USE_CORNER_SCORES

#if defined(USE_CORNER_SCORES)
#  define CORNERVAL       (64*4)
#  define PIECEVAL           64

#  define MAXSCORE           (64*64*64)
#  define MINSCORE          -(64*64*64)
#else
#  define CORNERVAL           0
#  define PIECEVAL            1

#  define MAXSCORE           64
#  define MINSCORE          -64
#endif


#define WINSCORE       (64*PIECEVAL)
#define LOOSESCORE    (-64*PIECEVAL)

#define AmountOfTime        1      /* seconds */
#define WHITE               1
#define BLACK               0

typedef unsigned long long BITSET64;

static BITSET64 Colours;
static BITSET64 Used;
static int noPlies = INITIALPLY;
static int timePerMove = 10;
static int positionsExplored;  /* no of positions evaluated in the current move.  */

#if 0
static int bestMove[MAXPLY+1];
static int bestMoveDepth;
static int currentMove[MAXPLY+1];
static int currentDepth;
#endif


/*
 *  IN - return 1 or 0 depending whether, bit, is in, set.
 */

static __inline__ int IN (BITSET64 set, int bit)
{
  if (sizeof(BITSET64) > sizeof(unsigned int)) {
    unsigned int *p = (unsigned int *)&set;

    if (bit >= sizeof(unsigned int)*8)
      /* high unsigned int */
      return (p[1] >> (bit-(sizeof(unsigned int)*8))) & 1;
    else
      /* low */
      return (p[0] >> bit) & 1;
  }
  else
    return (set >> bit) & 1;
}

/*
 *  INCL - set, bit, in, set.
 */

static __inline__ void INCL (BITSET64 *set, int bit)
{
  if (sizeof(BITSET64) > sizeof(unsigned int)) {
    unsigned int *p = (unsigned int *)set;

    if (bit >= sizeof(unsigned int)*8)
      /* high unsigned int */
      p[1] |= 1 << (bit-(sizeof(unsigned int)*8));
    else
      /* low */
      p[0] |= 1 << bit;
  }
  else
    (*set) |= 1 << bit;
}

/*
 *  EXCL - unset, bit, in, set.
 */

static __inline__ void EXCL (BITSET64 *set, int bit)
{
  if (sizeof(BITSET64) > sizeof(unsigned int)) {
    unsigned int *p = (unsigned int *)set;

    if (bit >= sizeof(unsigned int)*8)
      /* high unsigned int */
      p[1] &= ~(1 << (bit-(sizeof(unsigned int)*8)));
    else
      /* low */
      p[0] &= ~(1 << bit);
  }
  else
    (*set) &= ~(1 << bit);
}

static __inline__ int scan (BITSET64 c, BITSET64 u, int p, int our_colour, int o,
			    int increment, int final,
			    BITSET64 *newc, BITSET64 *newu)
{
  int n = 0;
  int i;

  if (p != final) {
    for (i = p+increment; (i != final) && (IS_USED(u, i) && (GET_COLOUR(c, i) == o)); i += increment)
      n++;

    if ((IS_USED(u, i) && (GET_COLOUR(c, i) == our_colour))) {
      INCL(newu, p);
      if (our_colour == WHITE)
	INCL(newc, p);
      else
	EXCL(newc, p);
      for (i = p+increment; (i != final) && (IS_USED(u, i) && (GET_COLOUR(c, i) == o)); i += increment) {
	INCL(newu, i);
	if (our_colour == WHITE)
	  INCL(newc, i);
	else
	  EXCL(newc, i);
      }

      return n;
    }
  }
  return 0;
}

static __inline__ int makeMove (BITSET64 c, BITSET64 u, int p, int our_colour,
				BITSET64 *m, BITSET64 *newc, BITSET64 *newu)
{
  int o = 1-our_colour;

  *newc = c;
  *newu = u;
  if (UN_USED(u, p)) {
    int n;
    int x = p % MAXX;
    int y = p / MAXY;

    n = scan (c, u, p, our_colour, o, 1, RIGHT_EDGE(p), newc, newu);  /* right */
    n += scan (c, u, p, our_colour, o, -1, LEFT_EDGE(p), newc, newu);  /* left */
    n += scan (c, u, p, our_colour, o, MAXX, TOP_EDGE(p), newc, newu);  /* up */
    n += scan (c, u, p, our_colour, o, -MAXX, BOT_EDGE(p), newc, newu);  /* down */

    if (x>0 && y>0) { /* diag left down */
      int l;
      if (x > y)
	l = x-y;
      else
	l = (y-x) * MAXY;
      n += scan (c, u, p, our_colour, o, -(MAXX+1), l, newc, newu);
    }

    if (x<MAXX && y<MAXY) {  /* diag right up */
      int l;
      if (x > y)
	l = (MAXX-x+y)*MAXY-1;
      else
	l = ((MAXY-1)-y+x) + (MAXY-1)*MAXY;
      n += scan(c, u, p, our_colour, o, +(MAXX+1), l, newc, newu);
    }

    if (x>0 && y<MAXY) {  /* diag left up */
      int l;
      if (x >= MAXY-y)
	l = (MAXY-1)*8+(x-(MAXY-1-y));
      else
	l = (y+x)*MAXY;
      n += scan(c, u, p, our_colour, o, +(MAXX-1), l, newc, newu);
    }

    if (x<MAXX && y>0) {  /* diag right down */
      int l;
      if (y >= MAXX-x)
	l = (y-(MAXX-x-1))*MAXY+MAXX-1;
      else
	l = x+y;
      n += scan(c, u, p, our_colour, o, -(MAXX-1), l, newc, newu);
    }

    if (n > 0)
      INCL(m, p);

    return n;
  }
  return 0;
}


static void setup (void)
{
  INCL(&Used, 35);
  INCL(&Colours, 35);
  INCL(&Used, 36);
  INCL(&Used, 27);
  INCL(&Used, 28);
  INCL(&Colours, 28);
}


static void setupTest (void)
{
  int i;

  for (i=49; i<=54; i++) {
    INCL(&Used, i);
    if (i % 2 == 0)
      INCL(&Colours, i);
  }
  INCL(&Used, 48);
  INCL(&Used, 40);
  INCL(&Used, 57);
  INCL(&Used, 58);
  INCL(&Colours, 49);
  INCL(&Colours, 21);

  for (i=41; i<=46; i++) {
    INCL(&Used, i);
    if (i % 2 == 1)
      INCL(&Colours, i);
  }
  for (i=33; i<=38; i++) {
    INCL(&Used, i);
    if (i % 2 == 0)
      INCL(&Colours, i);
  }
  for (i=0; i<=55; i++) {
    INCL(&Used, i);
    if (i % 2 == 1)
      INCL(&Colours, i);
  }
}


static void displayBoard (BITSET64 c, BITSET64 u, BITSET64 h, int hint)
{
  int i, j;
  int pos;

  printf("\n================================\n") ;
  j=0;
  for (j=MAXY-1; j>=0; j--) {
    printf(" %d ", j+1);
    for (i=0; i<MAXX; i++) {
      pos = j*MAXY+i;
      if (IS_USED(u, pos)) {
	if (IS_OUR_COLOUR(c, u, pos, WHITE))
	  printf(" W ");
	else
	  printf(" B ");
      }
      else if (hint && (IN(h, pos)))
	printf(" @ ");
      else
	printf(" . ");
    }
    printf("\n");
  }
  printf("\n");
  printf("    a  b  c  d  e  f  g  h\n");
}


static int enterMove (void)
{
  char ch;
  int pos;

  printf("enter your move: ");
  pos = 0;
  ch = getchar();
  if (ch == '?') {
    ch = getchar();
    return -1;
  }
  do {
    if ((ch>='a') && (ch<='h')) {
      pos = pos + (int) (ch - 'a');
    }
    ch = getchar();
    if ((ch>='1') && (ch<='8')) {
      pos = (pos + ((int) (ch-'1'))*MAXY) % (MAXY*MAXX);
    }
    ch = getchar();
  } while ((pos < 0) && (pos > (MAXX*MAXY-1))) ;
  printf("entered %d\n", pos);
  return pos;
}


#if 0
  /*       R   L   U   D  LD RU  LU  RD */
  makeMove(Colours, Used, 21, 1, NULL, NULL,
	   23, 16, 61, 5, 3, 39, 56, 7);

  makeMove(Colours, Used, 30, 1, NULL, NULL,
	   31, 24, 62, 6, 3, 39, 58, 23);

  makeMove(Colours, Used, 43, 1, NULL, NULL,
	   47, 40, 59, 3, 16, 61, 57, 15);

  makeMove(Colours, Used, 18, 1, NULL, NULL,
	   23, 16, 58, 2, 0, 63, 32, 4);

  makeMove(Colours, Used, 45, 1, NULL, NULL,
	   47, 40, 61, 5, 0, 63, 59, 31);

  makeMove(Colours, Used, 50, 1, NULL, NULL,
	   55, 48, 58, 2, 32, 59, 57, 15);

  makeMove(Colours, Used, 24, 1, NULL, NULL,
	   31, 24, 56, 0, 24, 60, 24, 3);
#endif


/*
 *  max - returns the greatest value from, x or y.
 */

static int max (int x, int y)
{
  if (x > y)
    return x;
  else
    return y;
}

/*
 *  min - returns the smallest value from, x or y.
 */

static int min (int x, int y)
{
  if (x < y)
    return x;
  else
    return y;
}

/*
 *  evaluate - returns a measure of goodness for the current board
 *             position. A positive value indicates a good move for
 *             white and a negative value means a good move for black.
 */

static int evaluate (BITSET64 c, BITSET64 u, int final)
{
  int score = 0;
  int used = 0;
  int i;

  positionsExplored++;
  for (i=0; i<MAXPOS; i++) {
    if (IS_USED(u, i)) {
      used++;
      if (GET_COLOUR(c, i) == WHITE)
	score += PIECEVAL;
      else
	score -= PIECEVAL;
    }
  }

  if (used == MAXPOS || final) {
    if (score > 0)
      return MAXSCORE;
    if (score < 0)
      return MINSCORE;
  }

  if (IS_USED(u, 0)) {
    /* bottom left corner */
    if (GET_COLOUR(c, 0) == WHITE)
      score += CORNERVAL;
    else
      score -= CORNERVAL;
  }

  if (IS_USED(u, 7)) {
    /* bottom right corner */
    if (GET_COLOUR(c, 7) == WHITE)
      score += CORNERVAL;
    else
      score -= CORNERVAL;
  }

  if (IS_USED(u, 56)) {
    /* top left corner */
    if (GET_COLOUR(c, 56) == WHITE)
      score += CORNERVAL;
    else
      score -= CORNERVAL;
  }

  if (IS_USED(u, 63)) {
    /* top right corner */
    if (GET_COLOUR(c, 63) == WHITE)
      score += CORNERVAL;
    else
      score -= CORNERVAL;
  }

  return score;
}

/*
 *  countCounters - returns the number of used positions.
 */

static int countCounters (BITSET64 u)
{
  int count=0;
  int i;

  for (i=0; i<MAXPOS; i++)
    if (IN(u, i))
      count++;
  return count;
}

/*
 *  findPossible - returns the number of legal moves found.
 *                 These are also assigned to the bitset, m.
 */

static int findPossible (BITSET64 Colours, BITSET64 Used, int o, BITSET64 *m, int l[])
{
  int n, p, i;
  BITSET64 nc, nu;

  n = 0;
  for (p = 0; p<MAXPOS; p++) {
    i = makeMove(Colours, Used, p, o, m, &nc, &nu);
    if (i > 0) {
      /* printf("legal position %d\n", p); */
      INCL(m, p);
      if (l != NULL)
	l[n] = p;
      n++;
    }
  }
  return n;
}

/*
 *  doMove - keep requesting user for a legal move.
 */

static int doMove (BITSET64 c, BITSET64 u, BITSET64 m, int o)
{
  int p;

  do {
    do {
      if (o == WHITE)
	printf("white ");
      else
	printf("black ");
      p = enterMove();
      if (p == -1)
	displayBoard(c, u, m, TRUE);
    } while (p == -1);
    if (! IN(m, p))
      printf("illegal move\n");
  } while (! IN(m, p));
  return p;
}

/*
 *  humanMove - asks player to enter a legal move and updates the
 *              board.
 */

static int humanMove (BITSET64 c, BITSET64 u, int o)
{
  BITSET64 m = 0;
  BITSET64 nc, nu;
  int l[MAXMOVES];
  int n = findPossible(c, u, o, &m, l);
  int p;

  displayBoard(c, u, m, FALSE);

  if (countCounters(u) == MAXPOS)
    return FALSE;

  if (n == 0) {
    printf("you cannot move...\n");
    return FALSE;
  }
  if (n == 1) {
    p = l[0];
    printf("you are forced to play the only move available which is %c%d\n",
	   ((char)(p % MAXX))+'a', p / MAXY+1);
  } else
    p = doMove(c, u, m, o);

  n = makeMove(c, u, p, o, &m, &nc, &nu);
  Colours = nc;
  Used = nu;
  return TRUE;
}

/*
 *  alphaBeta - returns the score estimated should move, p, be chosen.
 *              The board, c, u, is in the state _before_ move p is made.
 *              o is the colour who is attempting to play move, p.
 */

static int alphaBeta (int p, BITSET64 c, BITSET64 u, int depth, int o,
		      int alpha, int beta)
{
  BITSET64 m, nc, nu;
  int n, try;

  if (p == -1) {
    /* no move was possible */
    nc = c;
    nu = u;
  }
  else
    n = makeMove(c, u, p, o, &m, &nc, &nu);

  o = 1-o;
  if (depth == 0)
    return evaluate(c, u, FALSE);
  else {
    int l[MAXMOVES];
    int n = findPossible(nc, nu, o, &m, l);
    int i;

    if (n == 0) {
      if (p == -1)
	return evaluate(nc, nu, TRUE);
      else
	/* o, forfits a go and 1-o plays a move instead */
	return alphaBeta(-1, nc, nu, depth, 1-o, alpha, beta);
    }
    else if (o == WHITE) {
      /* white to move, move is possible, continue searching */
      for (i=0; i<n; i++) {
	try = alphaBeta(l[i], nc, nu, depth-1, WHITE, alpha, beta);
	if (try > alpha)
	  /* found a better move */
	  alpha = try;
	if (alpha >= beta)
	  return alpha;
      }
      return alpha;  /* the best score for a move WHITE has found */
    }
    else {
      /* black to move, move is possible, continue searching */
      for (i=0; i<n; i++) {
	try = alphaBeta(l[i], nc, nu, depth-1, BLACK, alpha, beta);
	if (try < beta)
	  /* found a better move */
	  beta = try;
	if (alpha >= beta)
	  return beta;  /* no point searching further as WHITE would choose
			   a different previous move */
      }
      return beta;  /* the best score for a move BLACK has found */
    }
  }
}

/*
 *  finalScore - returns the final score.
 */

static int finalScore (BITSET64 c, BITSET64 u)
{
  int score=0;
  int i;

  for (i=0; i<MAXPOS; i++) {
    if (IS_USED(u, i)) {
      if (GET_COLOUR(c, i) == WHITE)
	score++;
      else
	score--;
    }
  }
  return score;
}

#if 0
/*
 *  displayBestMoves - dumps the anticipated move sequence
 */

static void displayBestMoves (int depth, int o)
{
  int i;

  printf("I'm anticipating the move sequence:\n");
  for (i=depth; i>=0; i--) {
    if (o == WHITE)
      printf("white ");
    else
      printf("black ");
    o = 1-o;
    if (bestMove[i] != -1)
      printf("%c%d\n",
	     (char)(bestMove[i] % MAXX)+'a', bestMove[i] / MAXY+1);
  }
}
#endif

static mailbox *barrier;
static sem_t *processorAvailable;

void setupIPC (void)
{
  barrier = mailbox_init ();
  processorAvailable = multiprocessor_initSem (multiprocessor_maxProcessors ());
}


#if !defined(SEQUENTIAL)
int parallelSearch (int *totalExplored, int *move,
		    int best, int *l, int noOfMoves,
		    BITSET64 c, BITSET64 u, int noPlies, int o, int minscore, int maxscore)
{
    // My code
    int pid = fork();
    if (pid == 0)
    {
        /* Child is the source which spawns each move on a separate core */
        int i, currentMove;
        for (i = 0; i < noOfMoves; i++) /* i in no of moves */
        {
            multiprocessor_wait(processorAvailable); /* wait for a processor to become available */
            /* spawn a child process */
            if (fork() == 0)
            {
                /* child must search move i */
                currentMove = alphaBeta(l[i], c, u, noPlies, o, minscore, maxscore); /* search best move using alphabeta could take many minutes hence parallel */

                mailbox_send(barrier, currentMove, i, positionsExplored); /* need to send move back to parent using mailbox_send */

                multiprocessor_signal(processorAvailable); /* signal that a processor is available */
                exit();
            }
        }
        exit();
    }
    else
    {
        /* parent is the sink, which waits for any move to be returned and remembers the best move score */
        int i, move_score, move_index, positionsExplored;
        for (i = 0; i < noOfMoves; i++)
        {
            printf("parent waiting for a result\n");
            mailbox_rec(barrier, &move_score, &move_index, &positionsExplored);
            printf("... parent has received a result: move %d has a score of %d after exploring %d positions\n", move_index, move_score, positionsExplored);
            *totalExplored += positionsExplored; /* add count to the running total */
            if (move_score > best)
            {
                best = move_score;
                *move = l[move_index];
            }
        }
    }
    return best;
}
#endif


int sequentialSearch (int *totalExplored, int *move,
		      int best, int *l, int noOfMoves,
		      BITSET64 c, BITSET64 u, int noPlies, int o, int minscore, int maxscore)
{
  int i, try;

  for (i=0; i < noOfMoves; i++)
    {
      try = alphaBeta (l[i], c, u, noPlies, o, minscore, maxscore);
      if (try > best)
	{
	  best = try;
	  *move = l[i];
	}
    }
  *totalExplored = positionsExplored;
  return best;
}


/*
 *  decideMove - returns the computer choice of move.
 */

static int decideMove (BITSET64 c, BITSET64 u, int o, int n, int *l)
{
  time_t start, end;
  int best, move, try, i;
  int g = countCounters(u);
  int totalExplored = 0;  /* use a local copy as this function can be run with the parallel and sequential solution.  */

  if (n == 1) {
    printf("My move is forced, so I'm not going to delay by considering it..\n");
    return l[0];
  }

  noPlies = min(min (noPlies, MAXPOS-g), MAXPLY);

  printf("I'm going to look %d moves ahead...\n", noPlies);
  if (noPlies + g>=MAXPOS)
    printf("I should be able to see the end position...\n");
  positionsExplored = 0;  /* global count reset.  */
  start = time(NULL);
  best = MINSCORE-1;  /* ensures that no matter what we will initially set best
                         to the first move available. */
#if defined(SEQUENTIAL)
  best = sequentialSearch (&totalExplored, &move, best, l, n, c, u, noPlies, o, MINSCORE, MAXSCORE);
#else
  best = parallelSearch (&totalExplored, &move, best, l, n, c, u, noPlies, o, MINSCORE, MAXSCORE);
#endif
  end = time(NULL) ;

#if 0
  displayBestMoves(noPlies, 1-o);
#endif

  if (best >= WINSCORE)
    printf("I think I can force a win\n");
  if (best <= LOOSESCORE)
    printf("You should be able to force a win\n");

  if (g+noPlies>=60) {
    printf("I can see the end of the game and by playing %c%d\n",
	   (char)(move % MAXX)+'a', move / MAXY+1);
    printf("will give me a final score of at least %d\n", best);
  }
  else
    printf("I'm playing %c%d which will give me a score of %d\n",
	   (char)(move % MAXX)+'a', move / MAXY+1, best);

  if (end-start > timePerMove) {
    printf("I took %d seconds and evaluated %d positions,\nsorry about the wait, I took too long so\nI will reduce my search next go..\n",
	   (int)(end-start), totalExplored);
    if (noPlies > 1)
      noPlies -= 2;
  }
  else {
    printf("time took %d seconds and evaluated %d positions\n",
	   (int)(end-start), totalExplored);
    if (end-start < timePerMove / 10)
      noPlies += 2;
  }

  return move;
}


/*
 *  computerMove - computer looks ahead and decides best move to play.
 */

static int computerMove (BITSET64 c, BITSET64 u, int o)
{
  BITSET64 m = 0;
  int l[MAXMOVES];
  int n = findPossible(c, u, o, &m, l);
  int p;

  displayBoard(c, u, m, FALSE);

  if (countCounters(u) == MAXPOS)
    return FALSE;

  if (n == 0) {
    printf("I cannot move...\n");
    return FALSE;
  }
  p = decideMove(c, u, o, n, l);
  n = makeMove(c, u, p, o, &m, &Colours, &Used);
  return TRUE;
}

int main()
{
  int s, f;

  if (sizeof(BITSET64) != 8) {
    printf("BITSET64 must be 64 bits in length\n");
	exit(1);
  }

#if !defined(SEQUENTIAL)
  setupIPC ();
#endif

  // setupTest();
  setup();

  f = 0;
  while (f != 2) {
# if 1
    if (humanMove(Colours, Used, 0))
      f = 0;
    else
      f++;
# else
    if (computerMove(Colours, Used, 0))
      f = 0;
    else
      f++;
# endif
    if (f == 2)
      break;
    if (computerMove(Colours, Used, 1))
      f = 0;
    else
      f++;
  }
  printf("end of the game and ");
  s = finalScore(Colours, Used);
  if (s < 0)
    printf("you beat me by %d tiles\n", -s);
  if (s > 0)
    printf("I won by %d tiles\n", s);
  if (s == 0)
    printf("the result is a draw\n");
  return 0;
}
