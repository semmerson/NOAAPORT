/*
 *   Copyright 2011, University Corporation for Atmospheric Research.
 *   See file COPYRIGHT for copying and redistribution conditions.
 */
#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "noaaportLog.h"
#include "fifo.h"                   /* Eat own dog food */

struct fifo {
    unsigned char*      buf;        /**< Pointer to start of buffer */
    size_t              nextWrite;  /**< Offset to next byte to write */
    size_t              nbytes;     /**< Number of bytes in the buffer */
    size_t              size;       /**< Size of buffer in bytes */
    pthread_mutex_t     mutex;      /**< Concurrent access lock */
    pthread_mutex_t     writeMutex; /**< Concurrent write access lock */
    pthread_mutex_t     readMutex;  /**< Concurrent read access lock */
    pthread_cond_t      readCond;   /**< Reading condition variable */
    pthread_cond_t      writeCond;  /**< Writing condition variable */
    int                 closeIfEmpty;   /**< Close the FIFO if empty? */
};

/**
 * Initializes a FIFO
 *
 * @retval 0    Success
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S failure. \c nplStart() called.
 */
static int initializeFifo(
    Fifo* const             fifo,   /**< [in/out] Pointer to the FIFO */
    unsigned char* const    buf,    /**< [in] The buffer */
    const size_t            size)   /**< [in] Size of the buffer in bytes */
{
    int                 status = 2; /* default failure */
    pthread_mutexattr_t mutexAttr;

    if (pthread_mutexattr_init(&mutexAttr) != 0) {
        NPL_SERROR0("Couldn't initialize mutex attributes");
    }
    else {
        (void)pthread_mutexattr_settype(&mutexAttr, PTHREAD_MUTEX_ERRORCHECK);

        if (pthread_mutex_init(&fifo->writeMutex, &mutexAttr) != 0) {
            NPL_SERROR0("Couldn't initialize write-mutex");
        }
        else {
            if (pthread_mutex_init(&fifo->readMutex, &mutexAttr) != 0) {
                NPL_SERROR0("Couldn't initialize read-mutex");
            }
            else {
                if (pthread_mutex_init(&fifo->mutex, NULL) != 0) {
                    NPL_SERROR0("Couldn't initialize FIFO mutex");
                }
                else {
                    if (pthread_cond_init(&fifo->readCond, NULL) != 0) {
                        NPL_SERROR0(
                            "Couldn't initialize reading condition variable");
                    }
                    else {
                        if (pthread_cond_init(&fifo->writeCond, NULL) != 0) {
                            NPL_SERROR0(
                            "Couldn't initialize writing condition variable");
                        }
                        else {
                            fifo->buf = buf;
                            fifo->nextWrite = 0;
                            fifo->nbytes = 0;  /* indicates startup */
                            fifo->size = size;
                            fifo->closeIfEmpty = 0;
                            status = 0; /* success */
                        }               /* "fifo->writeCond" initialized */

                        if (0 != status)
                            (void)pthread_cond_destroy(&fifo->readCond);
                    }                   /* "fifo->readCond" initialized */

                    if (0 != status)
                        (void)pthread_mutex_destroy(&fifo->mutex);
                }                       /* "fifo->mutex" initialized */
            }

            if (0 != status)
                (void)pthread_mutex_destroy(&fifo->writeMutex);
        }                               /* "fifo->writeMutex" initialized */

        (void)pthread_mutexattr_destroy(&mutexAttr);
    }                                   /* "mutexAttr" initialized */

    return status;
}

/**
 * Returns a FIFO.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success.
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S failure. \c nplStart() called.
 */
int fifoNew(
    const size_t        npages,         /**< [in] FIFO size in pages */
    Fifo** const        fifo)           /**< [out] Pointer to pointer to be set
                                         *   to address of FIFO */
{
    int                 status = 2;     /* default failure */
    Fifo*               f = (Fifo*)malloc(sizeof(Fifo));

    if (NULL == f) {
        NPL_SERROR0("Couldn't allocate FIFO");
    }
    else {
        const long              pagesize = sysconf(_SC_PAGESIZE);
        const size_t            size = npages*pagesize;
        unsigned char* const    buf = (unsigned char*)malloc(size);

        if (NULL == buf) {
            NPL_SERROR1("Couldn't allocate %lu bytes for FIFO buffer",
                    (unsigned long)size);
        }
        else {
            if ((status = initializeFifo(f, buf, size)) == 0)
                *fifo = f;

            if (0 != status)
                free(buf);
        }                               /* "buf" allocated */

        if (0 != status)
            free(f);
    }                                   /* "f" allocated */

    return status;
}

/**
 * Reserves space in a FIFO and returns a pointer to it. Blocks until the
 * requested amount of space is available. The client should call \link
 * fifoUpdate() \endlink when the space has been written into. Only one thread
 * can execute this function and the subsequent \link fifoUpdate() \endlink
 * at a time.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success. \c *bytes points to at least \c nbytes of memory.
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S error. \c nplStart() called.
 * @retval 3    FIFO is closed.
 */
int fifoWriteReserve(
    Fifo* const             fifo,       /**< [in/out] Pointer to the FIFO */
    const size_t            nbytes,     /**< [in] The amount of space to
                                         *   reserve */
    unsigned char** const   bytes)      /**< [out] Pointer to the pointer to be
                                         *   set to the address of the reserved
                                         *   space */
{
    int             status = 0; /* default success */

    if (nbytes > fifo->size) {
        NPL_START2("Requested space larger than FIFO: %lu > %lu", nbytes,
                fifo->size);
        status = 1;
    }
    else {
        if (pthread_mutex_lock(&fifo->writeMutex) != 0) {
            NPL_SERROR0("Couldn't lock write-mutex");
            status = 2;
        }
        else {
            if (pthread_mutex_lock(&fifo->mutex) != 0) {
                NPL_SERROR0("Couldn't lock FIFO mutex");
                status = 2;
            }
            else {
                for (;;) {
                    if (fifo->closeIfEmpty && 0 == fifo->nbytes) {
                        status = 3;
                        break;
                    }

                    if ((fifo->size - fifo->nbytes) >= nbytes)
                        break;

                    if (pthread_cond_wait(&fifo->writeCond, &fifo->mutex) 
                            != 0) {
                        NPL_SERROR0(
                                "Couldn't wait on writing condition variable");
                        status = 2;
                        break;
                    }
                }

                if (0 == status)
                    *bytes = fifo->buf + fifo->nextWrite;

                (void)pthread_mutex_unlock(&fifo->mutex);
            }                           /* "fifo->mutex" locked */

            if (0 != status)
                (void)pthread_mutex_unlock(&fifo->writeMutex);
        }                               /* "fifo->writeMutex" locked */
    }                                   /* "nbytes" OK */

    return status;
}

/**
 * Updates the FIFO based on a successful write to the space obtained from
 * \link fifoWriteReserve() \endlink. This function must be called by the
 * thread that returned from the previous call to \link fifoWriteReserve()
 * \endlink. 
 *
 * This function in thread-safe.
 *
 * @retval 0    Success
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S error. \c nplStart() called.
 */
int fifoWriteUpdate(
    Fifo* const     fifo,   /**< [in/out] Pointer to FIFO */
    const size_t    nbytes) /**< [in] The number of bytes actually written */
{
    int             status;

    if (pthread_mutex_lock(&fifo->mutex) != 0) {
        NPL_SERROR0("Couldn't lock FIFO mutex");
        status = 2;
    }
    else {
        if (nbytes > (fifo->size - fifo->nbytes)) {
            NPL_START4("Amount written > amount possible: %lu > %l = (%lu-%lu)",
                    (unsigned long)nbytes, 
                    (long)(fifo->size - fifo->nbytes),
                    (unsigned long)fifo->size,
                    (unsigned long)fifo->nbytes);
            status = 1;                 /* usage error */
        }
        else {
            if (pthread_cond_signal(&fifo->readCond) != 0) {
                NPL_SERROR0("Couldn't signal reading condition variable");
                status = 2;             /* Usage error */
            }
            else {
                fifo->nextWrite = (fifo->nextWrite + nbytes) % fifo->size;
                fifo->nbytes += nbytes;

                if (pthread_mutex_unlock(&fifo->writeMutex) != 0) {
                    NPL_SERROR0("Couldn't unlock write-mutex");
                    status = 1;         /* Usage error */
                }
                else {
                    status = 0;         /* success */
                }
            }
        }

        (void)pthread_mutex_unlock(&fifo->mutex);
    }                                   /* "fifo->mutex" locked */

    return status;
}

/**
 * Returns a pointer to the next FIFO region to be read from. Blocks until
 * sufficient data exists. Only one thread can execute this function and the
 * subsequent \link fifoReadUpdate() \endlink at a time.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S error. \c nplStart() called.
 * @retval 3    FIFO is closed.
 */
int fifoReadPeek(
    Fifo* const                 fifo,   /**< [in/out] Pointer to FIFO */
    const size_t                nbytes, /**< [in] The number of bytes to be
                                         *   read */
    const unsigned char** const data)   /**< [out] Pointer to pointer to FIFO
                                         *   region containing the data to be
                                         *   read */
{
    int status = 0;             /* default success */

    if (nbytes > fifo->size) {
        NPL_START2("Requested read amount is larger than FIFO: %lu > %lu",
                nbytes, fifo->size);
        status = 1;
    }
    else {
        if (pthread_mutex_lock(&fifo->readMutex) != 0) {
            NPL_SERROR0("Couldn't lock read-mutex");
            status = 2;
        }
        else {
            if (pthread_mutex_lock(&fifo->mutex) != 0) {
                NPL_SERROR0("Couldn't lock FIFO mutex");
                status = 2;
            }
            else {
                for (;;) {
                    if (fifo->closeIfEmpty && 0 == fifo->nbytes) {
                        status = 3;
                        break;
                    }

                    if (fifo->nbytes >= nbytes)
                        break;

                    if (pthread_cond_wait(&fifo->readCond, &fifo->mutex) != 0) {
                        NPL_SERROR0(
                                "Couldn't wait on reading condition variable");
                        status = 2;
                        break;
                    }
                }

                if (0 == status) {
                    if (pthread_cond_signal(&fifo->writeCond) != 0) {
                        NPL_SERROR0(
                                "Couldn't signal writing condition variable");
                        status = 2;
                    }
                    else {
                        ssize_t nextRead = fifo->nextWrite - fifo->nbytes;

                        if (0 > nextRead)
                            nextRead += fifo->size;

                        *data = fifo->buf + nextRead;
                    }
                }

                (void)pthread_mutex_unlock(&fifo->mutex);
            }                           /* "fifo->mutex" locked */

            if (0 != status)
                (void)pthread_mutex_unlock(&fifo->readMutex);
        }                               /* "fifo->readMutex" locked */
    }                                   /* "nbytes" OK */

    return status;
}

/**
 * Updates the FIFO based on a successful read of the data obtained from
 * \link fifoReadPeek() \endlink. This function must be called by the thread
 * that successfully called \link fifoReadPeek() \endlink.
 *
 * This function in thread-safe.
 *
 * @retval 0    Success
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S error. \c nplStart() called.
 */
int fifoReadUpdate(
    Fifo* const     fifo,   /**< [in/out] Pointer to FIFO */
    const size_t    nbytes) /**< [in] The number of bytes read */
{
    int             status = 2; /* default failure */

    if (pthread_mutex_lock(&fifo->mutex) != 0) {
        NPL_SERROR0("Couldn't lock mutex");
    }
    else {
        if (nbytes > fifo->nbytes) {
            NPL_START2("Amount read > amount possible: %lu > %lu", 
                    (unsigned long)nbytes, (unsigned long)fifo->nbytes);
            status = 1;
        }
        else {
            if (pthread_cond_signal(&fifo->readCond) != 0) {
                NPL_SERROR0("Couldn't signal reading condition variable");
            }
            else {
                fifo->nbytes -= nbytes;

                if (pthread_mutex_unlock(&fifo->readMutex) != 0) {
                    NPL_SERROR0("Couldn't unlock read-mutex");
                    status = 1;             /* Usage error */
                }
                else {
                    status = 0;             /* success */
                }
            }
        }

        (void)pthread_mutex_unlock(&fifo->mutex);
    }                                   /* "fifo->mutex" locked */

    return status;
}

/**
 * Returns the next bytes from a FIFO. Blocks until sufficient data exists.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S error. \c nplStart() called.
 * @retval 3    FIFO is closed.
 */
int fifoRead(
    Fifo* const             fifo,   /**< [in/out] Pointer to FIFO */
    unsigned char* const    data,   /**< [out] Pointer to buffer to copy bytes
                                     *   into */
    const size_t            nbytes) /**< [in] The number of bytes to copy */
{
    const unsigned char*    bytes;
    int                     status = fifoReadPeek(fifo, nbytes, &bytes);

    if (0 == status) {
        (void)memcpy(data, bytes, nbytes);

        status = fifoReadUpdate(fifo, nbytes);
    }

    return status;
}

/**
 * Closes a FIFO when it becomes empty. Attempting to write to or read from a
 * closed FIFO will result in an error. Blocked \link fifoWriteReserve() 
 * \endlink and \link fifoRead() \endlink operations will error-return.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success
 * @retval 1    Usage error. \c nplStart() called.
 * @retval 2    O/S error. \c nplStart() called.
 */
int fifoCloseWhenEmpty(
    Fifo* const fifo)       /**< [in/out] Pointer to FIFO */
{
    int         status = 0; /* default success */

    if (pthread_mutex_lock(&fifo->mutex) != 0) {
        NPL_SERROR0("Couldn't lock mutex");
        status = 2;
    }
    else {
        fifo->closeIfEmpty = 1;

        if (pthread_cond_signal(&fifo->writeCond) != 0) {
            NPL_SERROR0("Couldn't signal writing condition variable");
            status = 2;
        }
        if (pthread_cond_signal(&fifo->readCond) != 0) {
            NPL_SERROR0("Couldn't signal reading condition variable");
            status = 2;
        }

        (void)pthread_mutex_unlock(&fifo->mutex);
    }                                   /* "fifo->mutex" locked */

    return status;
}
