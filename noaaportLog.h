/*
 *   Copyright © 2011, University Corporation for Atmospheric Research.
 *   See file COPYRIGHT for copying and redistribution conditions.
 */
#ifndef NOAAPORT_LOG_H
#define NOAAPORT_LOG_H

#include <stdarg.h>

#include <ulog.h>

#define NPL_FMT(fmt)                fmt " [%s:%d]"
#define NPL_ERRNO()                 nplStart(NPL_FMT("%s"),strerror(errno),__FILE__,__LINE__)
#define NPL_START0(fmt)             nplStart(NPL_FMT(fmt),__FILE__,__LINE__);
#define NPL_START1(fmt,a)           nplStart(NPL_FMT(fmt),a,__FILE__,__LINE__);
#define NPL_START2(fmt,a,b)         nplStart(NPL_FMT(fmt),a,b,__FILE__,__LINE__);
#define NPL_START3(fmt,a,b,c)       nplStart(NPL_FMT(fmt),a,b,c,__FILE__,__LINE__);
#define NPL_START4(fmt,a,b,c,d)     nplStart(NPL_FMT(fmt),a,b,c,d,__FILE__,__LINE__);
#define NPL_START5(fmt,a,b,c,d,e)   nplStart(NPL_FMT(fmt),a,b,c,d,e,__FILE__,__LINE__);
#define NPL_ADD0(fmt)               nplAdd(NPL_FMT(fmt),__FILE__,__LINE__)
#define NPL_ADD1(fmt,a)             nplAdd(NPL_FMT(fmt),a,__FILE__,__LINE__)
#define NPL_ADD2(fmt,a,b)           nplAdd(NPL_FMT(fmt),a,b,__FILE__,__LINE__)
#define NPL_ADD3(fmt,a,b,c)         nplAdd(NPL_FMT(fmt),a,b,c,__FILE__,__LINE__)
#define NPL_ADD4(fmt,a,b,c,d)       nplAdd(NPL_FMT(fmt),a,b,c,d,__FILE__,__LINE__)
#define NPL_ADD5(fmt,a,b,c,d,e)     nplAdd(NPL_FMT(fmt),a,b,c,d,e,__FILE__,__LINE__)
#define NPL_SERROR0(fmt)            nplErrno(NPL_FMT(fmt),__FILE__,__LINE__)
#define NPL_SERROR1(fmt,a)          nplErrno(NPL_FMT(fmt),a,__FILE__,__LINE__)
#define NPL_SERROR2(fmt,a,b)        nplErrno(NPL_FMT(fmt),a,b,__FILE__,__LINE__)
#define NPL_SERROR3(fmt,a,b,c)      nplErrno(NPL_FMT(fmt),a,b,c,__FILE__,__LINE__)
#define NPL_SERROR4(fmt,a,b,c,d)    nplErrno(NPL_FMT(fmt),a,b,c,d,__FILE__,__LINE__)
#define NPL_SERROR5(fmt,a,b,c,d,e)  nplErrno(NPL_FMT(fmt),a,b,c,d,e,__FILE__,__LINE__)

/**
 * Logs a system error.
 *
 * This function is thread-safe.
 */
void nplSerror(
    const char* fmt,    /**< The message format */
    ...)                /**< Arguments referenced by the format */;

/*
 * Logs a program error.
 *
 * This function is thread-safe.
 */
void nplError(
    const char* const fmt,  /**< The message format */
    ...)                    /**< Arguments referenced by the format */;

/*
 * Logs a warning.
 *
 * This function is thread-safe.
 */
void nplWarn(
    const char* const fmt,  /**< The message format */
    ...)                    /**< Arguments referenced by the format */;

/*
 * Logs a notice.
 *
 * This function is thread-safe.
 */
void nplNotice(
    const char* const fmt,  /**< The message format */
    ...)                    /**< Arguments referenced by the format */;

/*
 * Logs an informational message.
 *
 * This function is thread-safe.
 */
void nplInfo(
    const char* const fmt,  /**< The message format */
    ...)                    /**< Arguments referenced by the format */;

/*
 * Logs a debuging message.
 *
 * This function is thread-safe.
 */
void nplDebug(
    const char* const fmt,  /**< The message format */
    ...)                    /**< Arguments referenced by the format */;

/**
 * Adds a variadic log-message to the message-list for the current thread.
 *
 * This function is thread-safe.
 *
 * @retval 0            Success
 * @retval EAGAIN       Failure due to the buffer being too small for the
 *                      message.  The buffer has been expanded and the client
 *                      should call this function again.
 * @retval EINVAL       \a fmt or \a args is \c NULL. Error message logged.
 * @retval EINVAL       There are insufficient arguments. Error message logged.
 * @retval EILSEQ       A wide-character code that doesn't correspond to a
 *                      valid character has been detected. Error message logged.
 * @retval ENOMEM       Out-of-memory. Error message logged.
 * @retval EOVERFLOW    The length of the message is greater than {INT_MAX}.
 *                      Error message logged.
 */
int nplVadd(
    const char* const   fmt,  /**< The message format */
    va_list             args) /**< The arguments referenced by the format. */;

/*
 * Sets the first log-message.
 *
 * This function is thread-safe.
 */
void nplStart(
    const char* const fmt,  /**< The message format */
    ...)                    /**< Arguments referenced by the format */;

/*
 * Adds a log-message.
 *
 * This function is thread-safe.
 */
void nplAdd(
    const char* const fmt,  /**< The message format */
    ...)                    /**< Arguments referenced by the format */;

/*
 * Adds a system error-message and a higher-level error-message.
 *
 * This function is thread-safe.
 */
void nplErrno(
    const char* const fmt,  /**< The higher-level message format */
    ...)                    /**< Arguments referenced by the format */;

/*
 * Logs the currently-accumulated log-messages and resets the message-list for
 * the current thread.
 *
 * This function is thread-safe.
 */
void nplLog(
    const int   level)  /**< The level at which to log the messages.  One of
                          *  LOG_ERR, LOG_WARNING, LOG_NOTICE, LOG_INFO, or
                          *  LOG_DEBUG; otherwise, the behavior is undefined. */;

#endif
