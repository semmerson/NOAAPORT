/*
 *   Copyright 2011, University Corporation for Atmospheric Research.
 *   See file COPYRIGHT for copying and redistribution conditions.
 */
#define _XOPEN_SOURCE 500

#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>

#include "noaaportLog.h"
#include "fifo.h"
#include "reader.h"     /* Eat own dog food */

struct reader {
    Fifo*           fifo;           /**< Pointer to FIFO into which to put data
                                      */
    size_t          maxSize;        /**< Maximum amount to read in a single
                                      *  call in bytes */
    int             fd;             /**< File-descriptor to read from */
    volatile int    status;         /**< Termination status */
};

/**
 * Returns a new reader. The client should call \link \c readerFree() \endlink
 * when the reader is no longer needed.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success.
 * @retval 1    Precondition failure. \c nplStart() called.
 * @retval 2    O/S failure. \c nplStart() called.
 */
int readerNew(
    const int           fd,         /**< [in] File-descriptor to read from */
    Fifo* const         fifo,       /**< [in] Pointer to FIFO into which to put
                                     *   data */
    const size_t        maxSize,    /**< [in] Maximum amount to read in a single
                                     *   call in bytes */
    Reader** const      reader)     /**< [out] Pointer to pointer to address of
                                     *   reader */
{
    int       status = 2;       /* default failure */
    Reader*   r = (Reader*)malloc(sizeof(Reader));

    if (NULL == r) {
        NPL_SERROR0("Couldn't allocate new reader");
    }
    else {
        r->fifo = fifo;
        r->fd = fd;
        r->maxSize = maxSize;
        r->status = 0;
        *reader = r;
        status = 0;
    }

    return status;
}

/**
 * Frees a reader.
 */
void readerFree(
    Reader* const   reader)     /**< Pointer to the reader to be freed */
{
    free(reader);
}

/**
 * Executes a reader. Returns when end-of-file is encountered or an error
 * occurs.
 *
 * This function is thread-compatible but not thread-safe.
 *
 * @return NULL
 * @see \link readerStatus() \endlink
 */
void* readerStart(
    void* const     arg)      /**< Pointer to the reader to be executed */
{
    Reader* const   reader = (Reader*)arg;
    int             status;

    for (;;) {
        unsigned char*  data;

        if ((status = fifoWriteReserve(reader->fifo, reader->maxSize, &data))
                != 0) {
            NPL_ADD1("Couldn't reserve %lu bytes in FIFO", reader->maxSize);
            nplLog(LOG_ERR);
            break;
        }
        else {
            const ssize_t   nbytes = read(reader->fd, data, reader->maxSize);

            if (fifoWriteUpdate(reader->fifo, nbytes) != 0) {
                NPL_ADD0("Couldn't update FIFO");
                nplLog(LOG_ERR);
                break;
            }
            if (0 == nbytes) {
                status = 0;
                break;
            }
            if (-1 == nbytes) {
                NPL_SERROR0("read() failure");
                nplLog(LOG_ERR);
                status = 2;
                break;
            }
        }                                   /* FIFO space reserved */
    }                                       /* I/O loop */

    reader->status = status;

    return NULL;
}

/**
 * Returns the termination status of a data-reader.
 *
 * @retval 0    Success. End-of-file encountered.
 * @retval 1    Precondition failure. \c nplStart() called.
 * @retval 2    O/S failure. \c nplStart() called.
 */
int readerStatus(
    Reader* const   reader) /**< [in] Pointer to the reader */
{
    return reader->status;
}
