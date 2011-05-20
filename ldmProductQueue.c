/*
 *   Copyright 2011, University Corporation for Atmospheric Research.
 *   See file COPYRIGHT for copying and redistribution conditions.
 */
#define _XOPEN_SOURCE 500
#define __EXTENSIONS__

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#include <ldm.h>
#include <pq.h>

#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#include "config.h"

#ifdef HAVE_GET_QUEUE_PATH
#   include <globals.h>
#else
#   include <paths.h>           /* pre LDM 6.9 style */
#endif

#include "noaaportLog.h"
#include "ldmProductQueue.h"    /* Eat own dog food */

struct LdmProductQueue {
    char*           path;       /**< Pathname of the LDM product-queue */
    pqueue*         pq;         /**< The actual LDM product-queue */
    pthread_mutex_t mutex;      /**< concurrent-access mutex */
};

/**
 * Returns the pathname of the LDM product-queue.
 *
 * @return Pathname of the LDM product-queue.
 */
const char* lpqGetQueuePath(void)
{
    return
#ifdef HAVE_GET_QUEUE_PATH
        getQueuePath();
#else
        DEFAULT_QUEUE;
#endif
}

/**
 * Returns the LDM product-queue that corresponds to a pathname.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success.
 * @retval 1    Precondition failure. \link nplStart() \endlink called.
 * @retval 2    O/S failure. \link nplStart() \endlink called.
 * @retval 3    Couldn't open product-queue. \link nplStart() \endlink called.
 */
int lpqGet(
    const char*             pathname,   /**< [in] LDM product-queue pathname or
                                          *  NULL to obtain the default queue */
    LdmProductQueue** const lpq)        /**< [out] Pointer to pointer to be set
                                         *  to address of corresponding LDM
                                         *  product-queue. */
{
    int                         status = 0;     /* default success */
    static pthread_mutex_t      mutex = PTHREAD_MUTEX_INITIALIZER;

    if (pthread_mutex_lock(&mutex) != 0) {
        NPL_SERROR0("Couldn't lock mutex");
        status = 2;
    }
    else {
        int                         queueIndex;     /* index of queue */
        static int                  queueCount = 0; /* number of queues */
        static LdmProductQueue**    queues;         /* array of unique queues */

        if (NULL == pathname) {
#ifdef HAVE_GET_QUEUE_PATH
            pathname = getQueuePath();
#else
            pathname = DEFAULT_QUEUE;
#endif
        }

        for (queueIndex = 0; queueIndex < queueCount; queueIndex++) {
            if (strcmp(queues[queueIndex]->path, pathname) == 0)
                break;
        }

        if (queueIndex >= queueCount) {
            LdmProductQueue**    newArray = (LdmProductQueue**)realloc(queues,
                    (queueCount+1)*sizeof(LdmProductQueue*));

            if (NULL == newArray) {
                NPL_SERROR1("Unable to allocate new LdmProductQueue array: "
                        "queueCount=%d", queueCount);
                status = 2;
            }
            else {
                LdmProductQueue*    newLpq =
                    (LdmProductQueue*)malloc(sizeof(LdmProductQueue));

                if (NULL == newLpq) {
                    NPL_SERROR0("Unable to allocate new LdmProductQueue");
                    status = 2;
                }
                else {
                    pqueue* pq;

                    if (pq_open(pathname, PQ_DEFAULT, &pq) != 0) {
                        NPL_SERROR1("Couldn't open product-queue \"%s\"",
                                pathname);
                        status = 3;
                    }
                    else {
                        char* path = strdup(pathname);

                        if (NULL == path) {
                            NPL_SERROR1("Couldn't duplicate string \"%s\"",
                                    pathname);
                            status = 2;
                        }
                        else {
                            pthread_mutex_t mutex;

                            if (pthread_mutex_init(&mutex, NULL) != 0) {
                                NPL_SERROR0("Couldn't initialize mutex");
                                status = 2;
                            }
                            else {
                                newLpq->path = path;
                                newLpq->pq = pq;
                                newLpq->mutex = mutex;
                                newArray[queueCount++] = newLpq;
                                queues = newArray;
                            }
                        }

                        if (0 != status)
                            (void)pq_close(pq);
                    }                       /* "pq" open */

                    if (0 != status)
                        free(newLpq);
                }                           /* "newLpq" allocated */

                if (0 != status)
                    free(newArray);
            }                               /* "newArray" allocated */
        }                                   /* if new product-queue */

        if (0 == status) {
            *lpq = queues[queueIndex];
        }

        (void)pthread_mutex_unlock(&mutex);
    }                                       /* "mutex" locked */

    return status;
}

/**
 * Inserts a data-product into an LDM product-queue.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success. Product inserted into queue.
 * @retval 1    Precondition failure. \link nplStart() \endlink called.
 * @retval 2    O/S error. \link nplStart() \endlink called.
 * @retval 3    Product already in queue.
 * @retval 4    Product-queue error. \link nplStart() \endlink called.
 */
int lpqInsert(
    LdmProductQueue* const  lpq,    /**< LDM product-queue to insert data-
                                     *   product into. */
    const product* const    prod)   /**< LDM data-product to be inserted */
{

    int status = 0;                 /* default success */

    if (pthread_mutex_lock(&lpq->mutex) != 0) {
        NPL_SERROR0("Couldn't lock mutex");
        status = 2;
    }
    else {
        if ((status = pq_insert(lpq->pq, prod)) != 0) {
            if (PQUEUE_DUP == status) {
                status = 3;
            }
            else {
                NPL_START1("Couldn't insert product into queue: status=%d",
                        status);
                status = 4;
            }
        }

        (void)pthread_mutex_unlock(&lpq->mutex);
    }                                   /* "lpq->mutex" locked */

    return status;
}

/**
 * Closes an LDM product-queue.
 *
 * This function is thread-safe.
 *
 * @retval 0    Success. Product inserted into queue.
 * @retval 1    Precondition failure. \link nplStart() \endlink called.
 * @retval 2    O/S error. \link nplStart() \endlink called.
 */
int lpqClose(
    LdmProductQueue* const  lpq)    /**< LDM product-queue */
{
    int status;

    if (pthread_mutex_lock(&lpq->mutex) != 0) {
        NPL_SERROR0("Couldn't lock mutex");
        status = 2;
    }
    else {
        if ((status = pq_close(lpq->pq)) != 0) {
            if (EOVERFLOW == status) {
                NPL_START0("LDM product-queue is corrupt");
                status = 1;             /* precondition error */
            }
            else {
                NPL_SERROR0("Couldn't close LDM product-queue");
                status = 2;
            }
        }

        (void)pthread_mutex_unlock(&lpq->mutex);
    }

    return status;
}
