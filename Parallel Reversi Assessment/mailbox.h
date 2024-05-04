/*  mailbox.h provides a very simple mailbox datatype.
 *  Gaius Mulley <gaius.southwales@gmail.com>.
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/sysinfo.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>

#include "multiprocessor.h"

typedef struct triple_t {
  int result;
  int move_no;
  int positions_explored;
} triple;


typedef struct mailbox_t {
  triple data[MAX_MAILBOX_DATA]; // My line of code
  int in; // My line of code
  int out; // My line of code
  sem_t *item_available;    /* are there data in the mailbox?  */
  sem_t *space_available;   /* space for more data in the mailbox.  */
  sem_t *mutex;             /* access to the mailbox.  */
  struct mailbox_t *prev;  /* previous mailbox.  */
} mailbox;


#if !defined(mailbox_h)
#  define mailbox_h
#  if defined(mailbox_c)
#     if defined(__GNUG__)
#        define EXTERN extern "C"
#     else /* !__GNUG__.  */
#        define EXTERN
#     endif /* !__GNUG__.  */
#  else /* !mailbox_c.  */
#     if defined(__GNUG__)
#        define EXTERN extern "C"
#     else /* !__GNUG__.  */
#        define EXTERN extern
#     endif /* !__GNUG__.  */
#  endif /* !mailbox_c.  */


/*
 *  init - create a single mailbox which can contain a single triple.
 */

EXTERN mailbox *mailbox_init (void);


/*
 *  send - send (result, move_no, positions_explored) to the mailbox mbox.
 */

EXTERN void mailbox_send (mailbox *mbox,
			  int result, int move_no, int positions_explored);


/*
 *  rec - receive (result, move_no, positions_explored) from the
 *        mailbox mbox.
 */

EXTERN void mailbox_rec (mailbox *mbox,
			 int *result, int *move_no,
			 int *positions_explored);

#  undef EXTERN
#endif /* !mailbox_h.  */
