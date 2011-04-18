/*
 * ShmFIFO.c
 *
 * Shared Memory FIFO Pipe implementation library
 *
 * Copyright 2004 Yaroslav Polyakov.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */
#define _XOPEN_SOURCE 500

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/wait.h>

#include "shmfifo.h"

#include "ulog.h"

#define DVBS_ID 43210000

typedef enum {
    SI_LOCK = 0,
    SI_WRITER,
    SI_READER,
    SI_SEM_COUNT
} SemIndex;

static int
shmfifo_ll_memfree (const struct shmhandle* const shm)
{
  struct shmprefix *p;
  int count;

  p = (struct shmprefix *) shm->mem;

  if (p->write >= p->read)
    {
      count = shm->sz - p->write;
      count += p->read - sizeof (struct shmprefix) - shm->privsz;
    }
  else
    {
      count = p->read - p->write;
    }
  return count;
}

static int
shmfifo_ll_memused (const struct shmhandle* const shm)
{
  struct shmprefix *p = (struct shmprefix *) shm->mem;
  int count;
  if (p->write >= p->read)
    {
      return p->write - p->read;
    }

  /* here we have wrapping */
  count = shm->sz - sizeof (struct shmprefix) - shm->privsz - p->read;
  count += p->write;

  return count;
}

/*
 * Check to make sure that the current process doesn't have the lock on a
 * shared-memory FIFO.
 *
 * Precondition:
 *      The FIFO is not locked by this process.
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 *      havLck          Pointer to indicator to be set.
 * Returns:
 *      0               Success. The current process doesn't have the FIFO
 *                      locked.
 *      EINVAL          The current process has the FIFO locked.  Error-message
 *                      logged.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 *      ECANCELED       Operating-system failure. Error-message logged.
 */
static int
checkUnlocked(
    const struct shmhandle* const       shm)
{
    int status;

    if (0 > shm->semid) {
        uerror("haveLock(): Invalid semaphore ID: %d", shm->semid);
        status = EINVAL;
    }
    else {
        int     semval = semctl(shm->semid, SI_LOCK, GETVAL);
        int     pid = semctl(shm->semid, SI_LOCK, GETPID);

        if (-1 == semval || -1 == pid) {
            serror("haveLock(): semctl() failure");
            status = ECANCELED;
        }
        else {
            if ((0 == semval) && (getpid() == pid)) {
                uerror("haveLock(): FIFO already locked by this process: %d",
                    pid);
                status = EINVAL;
            }
            else {
                status = 0;
            }
        }
    }

    return status;
}

/*
 * Locks a shared-memory FIFO.
 *
 * Preconditions:
 *      The shared-memory FIFO is unlocked.
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 * Returns:
 *      0               Success
 *      ECANCELED       Operating-system failure. Error-message logged.
 *      EINVAL          "shm" is uninitialized. Error-message logged.
 *      EINVAL          Semaphore  is uninitialized. Error-message logged.
 *      EINVAL          FIFO is already locked by this process. Error-message
 *                      logged.
 */
static int
shmfifo_lock(
    const struct shmhandle* const       shm)
{
    int status = checkUnlocked(shm);

    if (0 == status) {
        struct sembuf op[1];

        /*  printf("called shmfifo_lock semid: %d in process %d\n",shm->semid,
         *  getpid());
         *  printf("<%d>locking %d\n",getpid(),shm->semid); */

        op[0].sem_num = SI_LOCK;
        op[0].sem_op = -1;
        op[0].sem_flg = 0;

        /* dvbs_multicast(1) hangs here */
        if (semop (shm->semid, op, 1) == -1) {
            serror("shmfifo(): semop(2) failure");
            status = ECANCELED;
        }
        else {
            status = 0;                 /* success */
        }

        /*   printf("<%d> locked\n",getpid()); */
    }

    return status;
}

/*
 * Check to make sure that the current process has the lock on a shared-memory
 * FIFO.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 *      havLck          Pointer to indicator to be set.
 * Returns:
 *      0               Success. The current process has the FIFO locked.
 *      EINVAL          The current process doesn't have the FIFO locked.
 *                      Error-message logged.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 *      ECANCELED       Operating-system failure. Error-message logged.
 */
static int
checkLocked(
    const struct shmhandle* const       shm)
{
    int status;

    if (0 > shm->semid) {
        uerror("haveLock(): Invalid semaphore ID: %d", shm->semid);
        status = EINVAL;
    }
    else {
        int     semval = semctl(shm->semid, SI_LOCK, GETVAL);
        int     pid = semctl(shm->semid, SI_LOCK, GETPID);

        if (-1 == semval || -1 == pid) {
            serror("haveLock(): semctl() failure");
            status = ECANCELED;
        }
        else {
            if (0 != semval) {
                uerror("haveLock(): FIFO not locked: %d", semval);
                status = EINVAL;
            }
            else if (getpid() != pid) {
                uerror("haveLock(): FIFO locked by another process: %d", pid);
                status = EINVAL;
            }
            else {
                status = 0;
            }
        }
    }

    return status;
}

/*
 * Unlocks a shared-memory FIFO.
 *
 * Precondition:
 *      The FIFO is locked by this process.
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 * Returns:
 *      0               Success.
 *      ECANCELED       Operating-system failure. Error-message logged.
 *      EINVAL          "shm" is uninitialized.
 *      EINVAL          FIFO is locked by this process. Error-message logged.
 */
static int
shmfifo_unlock(
    const struct shmhandle* const       shm)
{
    int status = checkLocked(shm);

    if (0 == status) {
        struct sembuf   op[1];

        /*   printf("<%d> unlocking %d\n",getpid(),shm->semid); */

        op[0].sem_num = SI_LOCK;
        op[0].sem_op = 1;
        /*op[0].sem_flg = SEM_UNDO; */
        op[0].sem_flg = 0;

        if (semop(shm->semid, op, 1) == -1) {
            serror("shmfifo_unlock(): semop(2) failure");
            status = ECANCELED;
        }
        else {
            status = 0;                 /* success */
        }

        /*   printf("unlocked. done\n");  */
    }

    return status;
}

static void
shmfifo_printmemstatus (const struct shmhandle* const shm)
{
  struct shmprefix *p;

  if (ulogIsDebug ())
    {
      (void)shmfifo_lock (shm);
      p = (struct shmprefix *) shm->mem;

      udebug
	("<%d> c: %d sz: %d, r: %d, w: %d, used: %d, free: %d, maxblock: %d",
	 getpid (), p->counter, shm->sz, p->read, p->write,
	 shmfifo_ll_memused (shm), shmfifo_ll_memfree (shm),
	 shmfifo_ll_memfree (shm) - sizeof (struct shmbh));
      (void)shmfifo_unlock (shm);
    }
  return;
}

static void
shmfifo_ll_hrewind (const struct shmhandle* const shm)
{
  struct shmprefix *p = (struct shmprefix *) shm->mem;
  p->read -= sizeof (struct shmbh);

  if (p->read < (int) (sizeof (struct shmprefix) + shm->privsz))
    {
      p->read = shm->sz + p->read - sizeof (struct shmprefix) - shm->privsz;
    };
}

static int
shmfifo_ll_put (const struct shmhandle* const shm, void *data, int sz)
{
  struct shmprefix *p = (struct shmprefix *) shm->mem;
  int copysz;


  if (shmfifo_ll_memfree (shm) < sz)
    return -1;

  p->counter++;

  copysz = shm->sz - p->write;
  if (copysz > sz)
    {
      copysz = sz;
    };

  memcpy ((char *) shm->mem + p->write, data, copysz);


  p->write += copysz;
  if (p->write == shm->sz)
    {
      p->write = shm->privsz + sizeof (struct shmprefix);

    }

  if (copysz < sz)
    {
      memcpy ((char *) shm->mem + p->write, &((char *) data)[copysz],
	      sz - copysz);

      p->write += sz - copysz;
    }

  return sz;

}

static int
shmfifo_ll_get (const struct shmhandle* const shm, void *data, int sz)
{
  struct shmprefix *p;
  int copysz;


  p = (struct shmprefix *) shm->mem;
  p->counter++;

  if (sz <= 0)
    {
      uerror ("sanity check failed in ll_get. sz is %d", sz);
      (void)shmfifo_unlock (shm);
      abort ();
    }

  if (shmfifo_ll_memused (shm) < sz)
    return -1;

  if (p->write > p->read)
    {
      /* normal */
      copysz = p->write - p->read;
      if (copysz > sz)
	copysz = sz;
    }
  else
    {
      copysz = shm->sz - p->read;
      if (copysz > sz)
	copysz = sz;
    }

  memcpy (data, (char *) shm->mem + p->read, copysz);


  p->read += copysz;

  if (p->read == shm->sz)
    p->read = shm->privsz + sizeof (struct shmprefix);

  if (copysz < sz)
    {
      memcpy (&((char *) data)[copysz], (char *) shm->mem + p->read,
	      sz - copysz);
      p->read += sz - copysz;
    }

  return sz;
}

/*
 * Ensures that a semaphore index is either SI_READER or SI_WRITER.
 *
 * Arguments:
 *      semIndex        Index to be vetted.
 * Returns:
 *      0               Success.
 *      EINVAL          "semIndex" isn't SI_READER or SI_WRITER. Error-message
 *                      logged.
 */
static int
vetSemIndex(
    const SemIndex      semIndex)
{
    if (SI_READER == semIndex || SI_WRITER == semIndex) {
        return 0;
    }

    uerror("vetSemIndex(): Invalid semaphore index: %d", semIndex);

    return EINVAL;
}

/*
 * Waits for a shared-memory FIFO to be notified. The shared-memory FIFO must
 * be locked.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO.
 *      semIndex        The index of the semaphore to wait upon. One of
 *                      SI_READER or SI_WRITER.
 * Returns:
 *      0               Success. Another thread of control has locked and
 *                      released the FIFO. Upon return, the shared-memory FIFO
 *                      shall be locked.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 *      EINVAL          The FIFO isn't locked by the current process.
 *                      Error-message logged.
 *      EINVAL          "semIndex" isn't SI_READER or SI_WRITER.
 *      ECANCELED       Operating-system failure. Error-message logged.
 * Raises:
 *      SIGSEGV if "shm" is NULL.
 */
static int
shmfifo_wait(
    const struct shmhandle* const       shm,
    const SemIndex                      semIndex)
{
    int status = vetSemIndex(semIndex);

    if (0 == status) {
        /* Release the lock */
        if ((status = shmfifo_unlock(shm)) == 0) {
            struct sembuf   op[1];

            /* Wait for a notification from the other process */
            op[0].sem_num = semIndex;
            op[0].sem_op = -1;
            op[0].sem_flg = 0;

            if (semop(shm->semid, op, 1) == -1) {
                serror("shmfifo_wait(): semop() failure");
                status = ECANCELED;
            }
            else {
                /* Reacquire the lock */
                if ((status = shmfifo_lock(shm)) == 0) {
                    status = 0;     /* success */
                }                   /* lock reacquired */
            }                       /* notification occurred */
        }                           /* lock released */
    }                               /* valid "semIndex" */

    return status;
}

/*
 * Waits for the reader process to be notified. The shared-memory FIFO must
 * be locked.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO.
 * Returns:
 *      0               Success.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 *      EINVAL          The FIFO isn't locked by the current process.
 *                      Error-message logged.
 *      ECANCELED       Operating-system failure. Error-message logged.
 * Raises:
 *      SIGSEGV if "shm" is NULL.
 */
static int
shmfifo_wait_reader(
    const struct shmhandle* const       shm)
{
    return shmfifo_wait(shm, SI_READER);
}

/*
 * Waits for the writer process to be notified. The shared-memory FIFO must
 * be locked.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO.
 * Returns:
 *      0               Success.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 *      EINVAL          The FIFO isn't locked by the current process.
 *                      Error-message logged.
 *      ECANCELED       Operating-system failure. Error-message logged.
 * Raises:
 *      SIGSEGV if "shm" is NULL.
 */
static int
shmfifo_wait_writer(
    const struct shmhandle* const       shm)
{
    return shmfifo_wait(shm, SI_WRITER);
}

/*
 * Notifies the reader or writer process.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 *      semIndex        Which process to notify. One of SI_WRITER or SI_READER.
 * Returns:
 *      0               Success
 *      ECANCELED       Operating-system failure. Error-message logged.
 *      EINVAL          The FIFO isn't locked by the current process.
 *                      Error-message logged.
 *      EINVAL          "which" isn't SI_WRITER or SI_READER. Error-message
 *                      logged.
 * Precondition:
 *      The FIFO is locked by the current process.
 */
static int
shmfifo_notify(
    const struct shmhandle*     shm,
    const SemIndex              which)
{
    int status = checkLocked(shm);

    if (0 == status) {
        if ((status = vetSemIndex(which)) == 0) {
            if (semctl(shm->semid, which, SETVAL, 1)) {
                serror("shmfifo_notify(): semctl() failure");
                status = ECANCELED;
            }
            else {
                status = 0;
            }
        }
    }

    return status;
}

/*
 * Notifies the writer process.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 * Returns:
 *      0               Success
 *      ECANCELED       Operating-system failure. Error-message logged.
 *      EINVAL          The FIFO isn't locked by the current process.
 *                      Error-message logged.
 * Precondition:
 *      The FIFO is locked by the current process.
 */
static int
shmfifo_notify_writer(
    const struct shmhandle*     shm)
{
    return shmfifo_notify(shm, SI_WRITER);
}

/*
 * Notifies the reader process.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 * Returns:
 *      0               Success
 *      ECANCELED       Operating-system failure. Error-message logged.
 *      EINVAL          The FIFO isn't locked by the current process.
 *                      Error-message logged.
 * Precondition:
 *      The FIFO is locked by the current process.
 */
static int
shmfifo_notify_reader(
    const struct shmhandle*     shm)
{
    return shmfifo_notify(shm, SI_READER);
}

static void
shmfifo_print (const struct shmhandle* const shm)
{
  struct shmprefix *p;

  uerror ("My Shared Memory information:\n");
  if (shm == NULL)
    {
      uerror ("Handle is NULL!\n");
      return;
    }

  if (shm->mem == NULL)
    {
      uerror ("isn't attached to shared mem\n");
      return;
    }
  p = (struct shmprefix *) shm->mem;


  uerror ("Segment id: %d\nMem: %p\nRead pos: %d\nWrite pos: %d\n",
	  shm->sid, shm->mem, p->read, p->write);

  if (p->read == p->write)
    uerror ("No blocks in shared memory\n");
  else
    {
      void *ptr = (char *) shm->mem + p->read;
      int count = 0;
      while (ptr != (char *) shm->mem + p->write)
	{
	  struct shmbh *h = (struct shmbh *) ptr;
	  count++;
	  udebug ("block: %d ", count);
	  udebug ("size: %d ", h->sz);
/*           printf("data: \"%s\" ",(char*)ptr + sizeof(struct shmbh)); */
/*           printf("\n"); */
	  /*(char*)ptr += h->sz + sizeof(struct shmbh); */
	  ptr = (char *) ptr + h->sz + sizeof (struct shmbh);
	}
/*      printf("---\n"); */

    }
}

/******************************************************************************
 * Public API:
 ******************************************************************************/

void
shmfifo_setpriv (struct shmhandle *shm, void *priv)
{
  (void)shmfifo_lock (shm);
  memcpy ((char *) shm->mem + sizeof (struct shmprefix), priv, shm->privsz);
  (void)shmfifo_unlock (shm);
}

void
shmfifo_getpriv (struct shmhandle *shm, void *priv)
{
  (void)shmfifo_lock (shm);
  memcpy (priv, (char *) shm->mem + sizeof (struct shmprefix), shm->privsz);
  (void)shmfifo_unlock (shm);
}

int
shmfifo_shm_from_key (struct shmhandle *shm, int nkey)
{
  int sid, semid;
  key_t key;
  struct shmprefix *p;

  if (shm == NULL)
    {
      uerror ("shm_from_key: shm is NULL");
      exit (-1);
    }

  if (nkey != -1)
    {
      key = (key_t) (DVBS_ID + nkey);
      semid = semget (key, SI_SEM_COUNT, 0660);
      sid = shmget (key, 0, 0);

      if ((semid == -1) || (sid == -1))
	return (-1);
      else
	{
	  shm->semid = semid;
	  shm->sid = sid;
	  shmfifo_attach (shm);
	  p = (struct shmprefix *) (shm->mem);
	  shm->privsz = p->privsz;
	  shm->sz = p->sz;
	  udebug ("look sizes %d %d\n", shm->privsz, shm->sz);
	  return (0);
	}
    }
  return (-1);
}

struct shmhandle *
shmfifo_create (int npages, int privsz, int nkey)
{

  int segment_id;
  struct shmhandle *shm;
  struct shmprefix *p;
  union semun arg;
  unsigned short values[SI_SEM_COUNT];
  key_t key;

  if (nkey == -1)
    segment_id = shmget (IPC_PRIVATE, npages * getpagesize (),
			 IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
  else
    {
      key = (key_t) (DVBS_ID + nkey);
      /* EXCL creates an error condition if the memory already exists...
         we can use the existing memory if the program has not changed the
         size of the segment or the private structure size
      segment_id = shmget (key, npages * getpagesize (),
			   IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);*/
      segment_id = shmget (key, npages * getpagesize (),
			   IPC_CREAT | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    }


  if (segment_id == -1)
    {
      serror ("shmfifo cannot shmget");
      return NULL;
    }

  shm = (struct shmhandle *) malloc (sizeof (struct shmhandle));
  shm->sid = segment_id;
  shm->mem = NULL;
  shm->privsz = privsz;
  shm->sz = npages * getpagesize ();

  /* tmp attach to init control structure */
  p = (struct shmprefix *) shmat (segment_id, 0, 0);
  if ( p == (void *)-1 )
     {
     serror("shmat");
     return NULL;
     }

  p->read = p->write = sizeof (struct shmprefix) + privsz;
  /*bzero ((char *) p + sizeof (struct shmprefix), privsz);*/
  memset ((char *) p + sizeof (struct shmprefix), 0, privsz);
  p->sz = shm->sz;
  p->privsz = privsz;

  shmdt ((const void *)p);


  /* get semaphore */
  if (nkey == -1)
    shm->semid = semget (IPC_PRIVATE, SI_SEM_COUNT,
            IPC_CREAT | IPC_EXCL + 0600);
  else
    shm->semid = semget (key, SI_SEM_COUNT, IPC_CREAT + 0660);
    /* Removed EXCL to use existing semaphore if possible
    shm->semid = semget (key, SI_SEM_COUNT, IPC_CREAT | IPC_EXCL + 0600);*/
  if (shm->semid == -1)
    {
      serror ("failed to create semaphore");
      exit (1);
    }
  else
    {
      udebug ("%d : got semid %d", getpid (), shm->semid);
    }

  values[SI_LOCK] = 1;
  values[SI_WRITER] = 0;
  values[SI_READER] = 0;
  arg.array = values;

  while (semctl (shm->semid, 0, SETALL, arg) == -1)
    {
      int icnt = 0;
      serror ("failed to init semaphore");
      sleep (2);
      if (icnt > 20)
	abort ();
      icnt++;
    };

  return shm;
}


int
shmfifo_attach (struct shmhandle *shm)
{
  if (shm->mem)
    {
      uerror ("attempt to attach already attached mem?\n");
      return -1;
    }

  shm->mem = shmat (shm->sid, 0, 0);
  return 1;
}

int
shmfifo_empty (struct shmhandle *shm)
{
  struct shmprefix *p;

  if (shm == NULL)
    return 1;
  p = (struct shmprefix *) shm->mem;
  if (p == NULL)
    return 1;
  if (p->read == p->write)
    return 1;
  return 0;
}

void
shmfifo_detach (struct shmhandle *shm)
{

  if (!shm->mem)
    {
      uerror ("attempt to detach already detached mem?\n");
      return;
    }
/*   printf("detaching %p\n",shm->mem); */
  shmdt (shm->mem);
  shm->mem = NULL;
}

/*
 * Reads one record's worth of data from the FIFO and writes it to a
 * client-supplied buffer. Blocks until data is available.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure. The
 *                      FIFO must be unlocked.
 *      data            Pointer to the buffer into which to put data from the
 *                      FIFO.
 *      sz              The size of the buffer in bytes.
 *      nbytes          Pointer to the memory location to be set to the number
 *                      of bytes read.
 * Returns:
 *      0               Success. "*nbytes" is set to the number of bytes read.
 *      ECANCELED       Operating-system failure. Error-message logged.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 *      EINVAL          "sz" is non-positive. Error-message logged.
 *      EINVAL          The buffer is too small for the record's data.  No data
 *                      is read.  Error-message logged.
 *      EIO             FIFO is corrupt. Error-message logged.
 * Raises:
 *      SIGSEGV if "shm" is NULL
 *      SIGSEGV if "data" is NULL
 *      SIGSEGV if "nbytes" is NULL
 *      SIGABRT if "shm" is uninitialized
 */
int
shmfifo_get(
    const struct shmhandle* const       shm,
    void* const                         data,
    const int                           sz,
    int* const                          nbytes)
{
    int status;

    if (sz <= 0) {
        uerror("shmfifo_get(): Non-positive number of bytes to read: %d", sz);
        status = EINVAL;
    }
    else {
        int     loggedEmptyFifo = 0;

        shmfifo_printmemstatus(shm);

        if ((status = shmfifo_lock(shm)) == 0) {
            for (status = 0; shmfifo_ll_memused(shm) == 0; ) {
                if (!loggedEmptyFifo) {
                    uinfo("shmfifo_get(): FIFO is empty");
                    loggedEmptyFifo = 1;
                }
                if ((status = shmfifo_wait_reader(shm)) != 0) {
                    break;
                }
            }

            if (0 == status) {
                struct shmbh        header;

                if (shmfifo_ll_memused(shm) < (int)sizeof(header)) {
                    uerror("shmfifo_get(): Insufficient data for a record: "
                            "should be at least %d bytes; was %d bytes",
                            sizeof(header), shmfifo_ll_memused(shm));
                    shmfifo_print(shm);

                    status = EINVAL;
                }
                else {
                    shmfifo_ll_get(shm, &header, sizeof(header));

                    if (header.canary != 0xDEADBEEF) {
                        uerror("shmfifo_get(): Invalid header sentinel: 0x%X",
                                header.canary);

                        status = EIO;
                    }
                    else if (shmfifo_ll_memused(shm) < header.sz) {
                        uerror("shmfifo_get(): "
                                "Inconsistent data-length of record: "
                                "expected %d bytes; encountered %d bytes",
                                header.sz, shmfifo_ll_memused(shm));
                        shmfifo_print(shm);

                        status = EIO;
                    }
                    else if (header.sz > sz) {
                        uerror("shmfifo_get(): "
                                "Client-supplied buffer too small: "
                                "need %d bytes; %d bytes supplied",
                                header.sz, sz);
                        shmfifo_ll_hrewind(shm);

                        status = EINVAL;
                    }
                    else {
                        shmfifo_ll_get(shm, data, header.sz);

                        if (loggedEmptyFifo) {
                            uinfo("shmfifo_get(): "
                                    "Got %d bytes of data from FIFO",
                                    header.sz);
                        }

                        shmfifo_printmemstatus(shm);

                        if ((status = shmfifo_notify_writer(shm)) == 0) {
                            *nbytes = header.sz;
                        }
                    }
                }
            }                           /* FIFO has data */

            status = shmfifo_unlock(shm);
        }                               /* shared-memory FIFO locked */
    }

    return status;
}

/*
 * Writes data to the shared-memory FIFO.
 *
 * Arguments:
 *      shm             Pointer to the shared-memory FIFO data-structure.
 *      data            Pointer to the data to be written.
 *      sz              The amount of data to be written in bytes.
 * Returns:
 *      0               Success.
 *      E2BIG           "sz" is larger than the FIFO can handle.  Error-message
 *                      logged.
 *      ECANCELED       Operating-system failure. Error-message logged.
 *      EIO             I/O error. Error-message logged.
 *      EINVAL          "sz" is negative. Error-message logged.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 */
int
shmfifo_put(
    const struct shmhandle* const       shm,
    void* const                         data,
    const int                           sz)
{
    int status;

    if (0 > sz) {
        uerror("shmfifo_put(): Invalid size argument: %d", sz);
        status = EINVAL;
    }
    else {
        shmfifo_printmemstatus(shm);

        if ((status = shmfifo_lock(shm)) == 0) {
            struct shmbh    header;
            const size_t    totalBytesToWrite = sz + sizeof(header);
            size_t          maxSize;

            maxSize = shmfifo_ll_memused(shm) + shmfifo_ll_memfree(shm);

            if (maxSize < totalBytesToWrite) {
                uerror("shmfifo_put(): Record bigger than entire FIFO: "
                        "record is %lu bytes; FIFO capacity is %lu bytes",
                        totalBytesToWrite, maxSize);
                status = E2BIG;
            }
            else {
                int loggedNoRoom = 0;
                int freeSpace;

                status = 0;

                /*
                 * Wait for the FIFO to have room for the data.
                 */
                while ((freeSpace = shmfifo_ll_memfree(shm)) <=
                        totalBytesToWrite) {
                    if (!loggedNoRoom) {
                        uerror("shmfifo_put(): No room in FIFO: "
                                "need %d bytes; only %d bytes available. "
                                "Waiting...", totalBytesToWrite, freeSpace);
                        loggedNoRoom = 1;
                    }
                    if ((status = shmfifo_wait_writer(shm)) != 0) {
                        break;
                    }
                }

                if (0 == status) {
                    header.sz = sz;
                    header.canary = 0xDEADBEEF;
                    shmfifo_ll_put(shm, &header, sizeof(header));
                    shmfifo_ll_put(shm, data, sz);

                    if (loggedNoRoom) {
                        uerror("shmfifo_put(): Wrote %d bytes to FIFO",
                                totalBytesToWrite);
                    }

                    status = shmfifo_notify_reader(shm);
                }
            }

            status = shmfifo_unlock(shm);
        }                               /* shared-memory FIFO locked */
    }

    return status;
}

void
shmfifo_dealloc (struct shmhandle *shm)
{
  union semun ignored;

  semctl (shm->semid, 0, IPC_RMID, ignored);    /* 2nd arg is ignored */
  shmctl (shm->sid, IPC_RMID, 0);
}
