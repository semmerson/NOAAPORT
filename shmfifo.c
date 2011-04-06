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

void shmfifo_print (struct shmhandle *shm);

/* todo:
 * - implement zavorachivanie
 * - ... s uchetom hvosta (write = limit-1);
 * - locking
 */


void
shmfifo_printmemstatus (struct shmhandle *shm)
{
  struct shmprefix *p;

  if (ulogIsDebug ())
    {
      shmfifo_lock (shm);
      p = (struct shmprefix *) shm->mem;

      udebug
	("<%d> c: %d sz: %d, r: %d, w: %d, used: %d, free: %d, maxblock: %d",
	 getpid (), p->counter, shm->sz, p->read, p->write,
	 shmfifo_ll_memused (shm), shmfifo_ll_memfree (shm),
	 shmfifo_ll_memfree (shm) - sizeof (struct shmbh));
      shmfifo_unlock (shm);
    }
  return;
}

int
shmfifo_ll_memfree (struct shmhandle *shm)
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

int
shmfifo_ll_memused (struct shmhandle *shm)
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

void
shmfifo_setpriv (struct shmhandle *shm, void *priv)
{
  shmfifo_lock (shm);
  memcpy ((char *) shm->mem + sizeof (struct shmprefix), priv, shm->privsz);
  shmfifo_unlock (shm);
}

void
shmfifo_getpriv (struct shmhandle *shm, void *priv)
{
  shmfifo_lock (shm);
  memcpy (priv, (char *) shm->mem + sizeof (struct shmprefix), shm->privsz);
  shmfifo_unlock (shm);
}


void
shmfifo_ll_hrewind (struct shmhandle *shm)
{
  struct shmprefix *p = (struct shmprefix *) shm->mem;
  p->read -= sizeof (struct shmbh);

  if (p->read < (int) (sizeof (struct shmprefix) + shm->privsz))
    {
      p->read = shm->sz + p->read - sizeof (struct shmprefix) - shm->privsz;
    };
}

int
shmfifo_ll_put (struct shmhandle *shm, void *data, int sz)
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

int
shmfifo_ll_get (struct shmhandle *shm, void *data, int sz)
{
  struct shmprefix *p;
  int copysz;


  p = (struct shmprefix *) shm->mem;
  p->counter++;

  if (sz <= 0)
    {
      uerror ("sanity check failed in ll_get. sz is %d", sz);
      shmfifo_unlock (shm);
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
      semid = semget (key, 1, 0660);
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
  unsigned short values[1];
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
    shm->semid = semget (IPC_PRIVATE, 1, IPC_CREAT | IPC_EXCL + 0600);
  else
    shm->semid = semget (key, 1, IPC_CREAT + 0660);
    /* Removed EXCL to use existing semaphore if possible
    shm->semid = semget (key, 1, IPC_CREAT | IPC_EXCL + 0600);*/
  if (shm->semid == -1)
    {
      serror ("failed to create semaphore");
      exit (1);
    }
  else
    {
      udebug ("%d : got semid %d", getpid (), shm->semid);
    }

  values[0] = 1;
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



void
shmfifo_lock (struct shmhandle *shm)
{
  int DONE;
  struct sembuf op[1];
  if (!shm)
    {
      uerror ("shm uninitialized\n");
      abort ();
      return;
    }
  if (shm->semid < 0)
    {
      uerror ("semid uninitialized\n");
      abort ();
      return;
    }


/*  printf("called shmfifo_lock semid: %d in process %d\n",shm->semid,getpid()); */

/*  printf("<%d>locking %d\n",getpid(),shm->semid); */

  DONE = 0;
  while (!DONE)
    {
      op[0].sem_num = 0;
      op[0].sem_op = -1;
      op[0].sem_flg = 0;

      if (semop (shm->semid, op, 1) == -1)
	{
	  serror ("semop lock failed");
	  usleep (100);
	}
      else
	DONE = 1;
    }
/*   printf("<%d> locked\n",getpid()); */
}

/*
 * Waits for a shared-memory FIFO to be notified. The shared-memory FIFO must
 * be locked.
 *
 * Arguments:
 *      shm     Pointer to the shared-memory FIFO.
 * Returns:
 *      0               Success. Another thread of control has locked and
 *                      released the FIFO. Upon return, the shared-memory FIFO
 *                      shall be locked.
 *      EINVAL          "shm" uninitialized. Error-message logged.
 *      EINVAL          The FIFO isn't locked by the current process.
 *                      Error-message logged.
 *      ECANCELED       Operating-system failure. Error-message logged.
 * Raises:
 *      SIGSEGV if "shm" is NULL.
 */
int
shmfifo_wait(
    const struct shmhandle* const       shm)
{
    int status;
    int pid;
    int semval;

    if (0 > shm->semid) {
        uerror("shmfifo_wait(): Invalid semaphore ID: %d", shm->semid);
        status = EINVAL;
    }
    else if ((semval = semctl(shm->semid, 0, GETVAL)) != 0) {
        uerror("shmfifo_wait(): FIFO not locked: %d", semval);
        status = EINVAL;
    }
    else if ((pid = semctl(shm->semid, 0, GETPID)) != getpid()) {
        uerror("shmfifo_wait(): FIFO locked by another process: %d", pid);
        status = EINVAL;
    }
    else {
        /*
         * Use the FIFO's semaphore-based lock to implement a brain-damaged
         * condition variable: notification or signaling occurs when another
         * thread of control acquires and releases the FIFO's semaphore-based
         * lock (i.e., another thread of control does something with the FIFO).
         *
         * THIS SOLUTION TO THE PROBLEM IMPLEMENTING AN EVENT-DRIVEN FIFO IS
         * VIABLE IF AND ONLY IF THERE ARE ONLY TWO THREADS OF CONTROL.
         */
        struct sembuf   op[3];

        /* Release lock */
        op[0].sem_num = 0;
        op[0].sem_op = 1;
        op[0].sem_flg = 0;

        /* Wait for another process to acquire lock */
        op[1].sem_num = 0;
        op[1].sem_op = 0;
        op[1].sem_flg = 0;

        /* Reacquire lock */
        op[2].sem_num = 0;
        op[2].sem_op = -1;
        op[2].sem_flg = 0;

        if (semop(shm->semid, op, sizeof(op)/sizeof(op[0])) != -1) {
            status = 0;                 /* success */
        }
        else {
            serror("shmfifo_wait()");
            status = ECANCELED;
        }
    }

    return status;
}

void
shmfifo_unlock (struct shmhandle *shm)
{
  int DONE;
  struct sembuf op[1];
  if (!shm)
    {
      uerror ("shm uninitialized\n");
      return;
    }
  if (shm->semid < 0)
    {
      uerror ("semid uninitialized\n");
      return;
    }
/*   printf("<%d> unlocking %d\n",getpid(),shm->semid); */
  DONE = 0;
  while (!DONE)
    {
      op[0].sem_num = 0;
      op[0].sem_op = 1;
      /*op[0].sem_flg = SEM_UNDO; */
      op[0].sem_flg = 0;
      if (semop (shm->semid, op, 1) == -1)
	{
	  serror ("semop unlock failed");
	  usleep (100);
	}
      else
	DONE = 1;
    }
/*   printf("unlocked. done\n");  */
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
 * client-supplied buffer.
 *
 * Arguments:
 *      shm     Pointer to the shared-memory FIFO data-structure. The FIFO
 *              must be unlocked.
 *      data    Pointer to the buffer into which to put data from the FIFO.
 *      sz      The size of the buffer in bytes.
 * Returns:
 *      >0      The size of the record's data in bytes. Copied to the buffer.
 *      -1      FIFO is empty.
 *      -2      The buffer is too small for the record's data. No data is read.
 *              Error message logged.
 * Raises:
 *      SIGABRT if the FIFO has data but not an entire header-record.
 *      SIGABRT if "sz" is not positive.
 *      SIGABRT if the signal value (canary) in the header-record is invalid.
 */
int
shmfifo_get (struct shmhandle *shm, void *data, int sz)
{
  struct shmbh h;

  if (sz <= 0)
    {
      uerror ("Insane sz: %d\n", sz);
      abort ();
    }

  shmfifo_printmemstatus (shm);
  shmfifo_lock (shm);
  if (shmfifo_ll_memused (shm) == 0)
    {
/*    printf("shmem has no data\n"); */
      shmfifo_unlock (shm);
      return -1;
    }
  if (shmfifo_ll_memused (shm) < (int) sizeof (h))
    {
      shmfifo_unlock (shm);
      uerror ("used mem less than header sz!\n");
      shmfifo_print (shm);
      abort ();
    }
  shmfifo_ll_get (shm, &h, sizeof (h));
  if (h.canary != 0xDEADBEEF)
    {
      uerror ("canary died!\n");
      abort ();
    }
  if (shmfifo_ll_memused (shm) < h.sz)
    {
      uerror ("Shared mem is corrupted!? h.sz: %d, used mem: %d\n",
	      h.sz, shmfifo_ll_memused (shm));
      shmfifo_print (shm);
      abort ();
    }

  if (h.sz > sz)
    {
      uerror ("Attempt to fetch block of size %d into mem of size %d!",
	      h.sz, sz);

      shmfifo_ll_hrewind (shm);
      shmfifo_unlock (shm);
      return -2;

    }
  shmfifo_ll_get (shm, data, h.sz);
  shmfifo_unlock (shm);

  shmfifo_printmemstatus (shm);
  return h.sz;
}


/*
 * Writes data to the shared-memory FIFO.
 *
 * Arguments:
 *      shm     Pointer to the shared-memory FIFO data-structure.
 *      data    Pointer to the data to be written.
 *      sz      The amount of data to be written in bytes.
 * Returns:
 *      sz      Success.
 *      -1      Error. "errno" set to
 *                  E2BIG       "sz" is larger than the FIFO can handle.
 *                              Error-message logged.
 *                  EIO         I/O error. Error-message logged.
 *                  EINVAL      "sz" is negative. Error-message logged.
 */
int
shmfifo_put(
    struct shmhandle*   shm,
    void*               data,
    int                 sz)
{
    int                 status;

    if (0 > sz) {
        uerror("shmfifo_put(): Invalid size argument: %d", sz);
        errno = EINVAL;
        status = -1;
    }
    else {
        struct shmbh    h;
        const size_t    totalBytesToWrite = sz + sizeof(h);
        size_t          maxSize;

        shmfifo_printmemstatus(shm);
        shmfifo_lock(shm);

        maxSize = shmfifo_ll_memused(shm) + shmfifo_ll_memfree(shm);

        if (maxSize < totalBytesToWrite) {
            uerror("shmfifo_put(): Can't write %lu bytes to %lu-byte FIFO", 
                    totalBytesToWrite, maxSize);
            errno = E2BIG;
            status = -1;
        }
        else {
            int freeSpace = shmfifo_ll_memfree(shm);
            int insufficientSpaceLogged = 0;

            status = sz;

            while (freeSpace <= totalBytesToWrite) {
                if (!insufficientSpaceLogged) {
                    uerror("Can't write %d bytes to FIFO with %d bytes free. "
                            "Waiting...", sz, freeSpace);
                    insufficientSpaceLogged = 1;
                }
                if (shmfifo_wait(shm) != 0) {
                    errno = EIO;
                    status = -1;
                }
            }

            if (sz == status) {
                h.sz = sz;
                h.canary = 0xDEADBEEF;
                shmfifo_ll_put(shm, &h, sizeof(h));
                shmfifo_ll_put(shm, data, sz);

                if (insufficientSpaceLogged) {
                    uerror("Wrote %d bytes to FIFO", sz);
                }
            }
        }

        shmfifo_unlock(shm);
    }

    return status;
}

void
shmfifo_dealloc (struct shmhandle *shm)
{
  union semun ignored;

  semctl (shm->semid, 1, IPC_RMID, ignored);
  shmctl (shm->sid, IPC_RMID, 0);
}


void
shmfifo_print (struct shmhandle *shm)
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
