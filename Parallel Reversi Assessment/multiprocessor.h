/*  multiprocessor.h provides a simple method for using semaphores.
 *  Gaius Mulley <gaius.southwales@gmail.com>.
 */

#if !defined(multiprocessor_h)
#  define multiprocessor_h
#  if defined(multiprocessor_c)
#     if defined(__GNUG__)
#        define EXTERN extern "C"
#     else /* !__GNUG__.  */
#        define EXTERN
#     endif /* !__GNUG__.  */
#  else /* !multiprocessor_c.  */
#     if defined(__GNUG__)
#        define EXTERN extern "C"
#     else /* !__GNUG__.  */
#        define EXTERN extern
#     endif /* !__GNUG__.  */
#  endif /* !multiprocessor_c.  */

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

/*
 *  maxProcessors - return the total number of cores available.
 */

EXTERN int multiprocessor_maxProcessors (void);


/*
 *  initSem - initialise and return a new semaphore containing value.
 */

EXTERN sem_t *multiprocessor_initSem (int value);


/*
 *  wait - block until a token is available in the semaphore.
 */

EXTERN void multiprocessor_wait (sem_t *base_sem);


/*
 *  signal - gives a single token to the semaphore.
 */

EXTERN void multiprocessor_signal (sem_t *base_sem);


/*
 *  initSharedMemory - initialise the shared memory block of memory.
 *                     mem_size determines the size of the block
 *                     the address of the free block of shared memory
 *                     is returned.  As a by product extra shared
 *                     memory is allocated which is used by the semaphore
 *                     data structures.  It allows the user to perform
 *                     a single shared memory allocation and use it for
 *                     multiple objects which is required as semaphores
 *                     also need to be placed in the shared memory region.
 */

EXTERN void *multiprocessor_initSharedMemory (unsigned int mem_size);


/* constructor for the library.  */

EXTERN void _M2_multiprocessor_init (void);


/* deconstructor for the library.  */

EXTERN void _M2_multiprocessor_finish (void);

#  undef EXTERN
#endif /* !multiprocessor_h.  */
