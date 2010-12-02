/*
 * ShmFIFO.h
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

struct shmhandle
{
   int sid;
   void *mem;
   int privsz; /* size of single priv structure of upper program */
   int semid; /* semaphore id */
   int sz;    /* size of shared mem in bytes */
};

struct shmprefix
{  int counter;
   int read;  /* offsets from shm->mem where we should read and write */
   int write; /* min value for both is shm->privsz+sizeof(struct shmprefix) */
   int sz;
   int privsz;
};

struct shmbh /* shmem block header */
{
   int sz;
   unsigned canary;
};

#if ! ( _NO_XOPEN4 && _NO_XOPEN5 )
union semun {
int val;
struct semid_ds *buf;
unsigned short int *array;
struct seminfo *__buf;
};
#endif

/* Public functions */
struct shmhandle *shmfifo_create(int npages, int privsz, int nkey);
int shmfifo_attach(struct shmhandle *shm);
void shmfifo_dealloc(struct shmhandle *shm);
void shmfifo_detach(struct shmhandle *shm);

void shmfifo_setpriv(struct shmhandle *shm, void *priv);
void shmfifo_getpriv(struct shmhandle *shm, void *priv);

int shmfifo_get(struct shmhandle *shm, void *data,int sz);
int shmfifo_put(struct shmhandle *shm, void *data, int sz);

/* Unidata created function */
int shmfifo_shm_from_key ( struct shmhandle *shm, int nkey );

int shmfifo_empty(struct shmhandle *shm);
/* Internal and debugging functions */
void shmfifo_print(struct shmhandle *shm);
void shmfifo_printmemstatus(struct shmhandle *shm);
int shmfifo_ll_memfree(struct shmhandle *shm);
int shmfifo_ll_memused(struct shmhandle *shm);
int shmfifo_ll_put(struct shmhandle *shm, void *data, int sz);
int shmfifo_ll_get(struct shmhandle *shm, void *data, int sz);
void shmfifo_print(struct shmhandle *shm);
void shmfifo_lock(struct shmhandle *shm);
void shmfifo_unlock(struct shmhandle *shm);
