/*
 *   Copyright 2011, University Corporation for Atmospheric Research.
 *   See COPYRIGHT file for copying and redistribution conditions.
 */
/**
 *   @file noaaportIngester.c
 *
 *   This file contains the code for the \c noaaportIngester(1) program. This
 *   program reads NOAAPORT data from a file or multicast packet stream,
 *   creates LDM data-products, and writes the data-products into an LDM
 *   product-queue.
 */
#define _XOPEN_SOURCE 500
#define __EXTENSIONS__

#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <ldm.h>
#include <ulog.h>

#undef PACKAGE
#undef PACKAGE_BUGREPORT
#undef PACKAGE_NAME
#undef PACKAGE_STRING
#undef PACKAGE_TARNAME
#undef PACKAGE_VERSION
#undef VERSION

#include "config.h"

#include "noaaportLog.h"
#include "fifo.h"
#include "fileReader.h"
#include "ldmProductQueue.h"
#include "multicastReader.h"
#include "productMaker.h"
#include "reader.h"

static pthread_t        readerThread;
static ProductMaker*    productMaker;
static pthread_t        productMakerThread;
static const size_t     DEFAULT_NPAGES = 5000;
static unsigned         logFacility = LOG_LDM;  /* default LDM facility */
static const char*      COPYRIGHT_NOTICE = \
    "Copyright (C) 2011 University Corporation for Atmospheric Research";

/**
 * Unconditionally logs a usage message.
 */
static void usage(
    const char* const   progName)       /**< [in] Program name */
{
    int logmask = setulogmask(LOG_UPTO(LOG_NOTICE));

    unotice(
"%s version %s\n"
"%s\n"
"\n"
"Usage: %s [-n|v|x] [-l log] [-u n] [-m addr] [-q queue] [-b npages] [-I iface]\n"
"where:\n"
"   -b npages   Allocate \"npages\" pages of memory for the internal buffer.\n"
"               Default is %lu pages.\n"
"   -I iface    Listen for multicast packets on interface \"iface\".\n"
"               Default is to listen on all available interfaces.\n"
"   -l log      Log to file \"log\".  Default is to use the system logging\n"
"               daemon if the current process is a daemon; otherwise, the\n"
"               standard error stream is used.\n"
"   -m addr     Read data from IPv4 dotted-quad multicast address \"addr\".\n"
"               Default is to read from the standard input stream.\n"
"   -n          Log through level NOTE. Report each data-product.\n"
"   -q queue    Use \"queue\" as LDM product-queue. Default is \"%s\".\n"
"   -u n        Use logging facility local\"n\". Default is to use the\n"
"               default LDM logging facility, %s.\n"
"   -v          Log through level INFO.\n"
"   -x          Log through level DEBUG. Too much information.\n"
"\n"
"If neither \"-n\", \"-v\", nor \"-x\" is specified, then only levels ERROR\n"
"and WARN are logged.\n"
"\n"
"SIGUSR1 causes statistics to be unconditionally logged at level NOTE.\n"
"SIGUSR2 rotates the logging level.\n",
        progName, PACKAGE_VERSION, COPYRIGHT_NOTICE, progName, (unsigned
        long)DEFAULT_NPAGES, lpqGetQueuePath(), getFacilityName(LOG_LDM));

    (void)setulogmask(logmask);
}

/**
 * Initializes logging.
 *
 * @retval 0    Success
 * @retval 1    Usage error.
 */
static int initLogging(
    const char* const   progName,       /**< [in] Name of the program */
    const unsigned      logOptions,     /**< [in] Logging options */
    const unsigned      logFacility,    /**< [in] Logging facility */
    const char* const   logPath)        /**< [in] Pathname of the log file,
                                          *  "-", or NULL */
{
    int status;

    if (openulog(progName, logOptions, logFacility, logPath) == -1) {
        LOG_SERROR0("Couldn't initialize logging");
        log_log(LOG_ERR);
        usage(progName);
        status = 1;
    }
    else {
        status = 0;
    }

    return status;
}

/*
 * Handles a signal.
 */
static void signal_handler(
        const int       sig)    /**< [in] Signal to be handled */
{
#ifdef SVR3SIGNALS
    /*
     * Some systems reset handler to SIG_DFL upon entry to handler.
     * In that case, we reregister our handler.
     */
    (void)signal(sig, signal_handler);
#endif

    switch (sig) {
        case SIGINT:
            exit(1);
        case SIGTERM:
            (void)pthread_cancel(readerThread);
            break;
        case SIGUSR1:
            if (NULL != productMaker)
                pmSetLogStats(productMaker);
            break;
        case SIGUSR2: {
            unsigned logMask = getulogmask();

            if ((logMask & LOG_MASK(LOG_WARNING)) == 0) {
                (void)setulogmask(LOG_UPTO(LOG_WARNING));
            }
            else if ((logMask & LOG_MASK(LOG_NOTICE)) == 0) {
                (void)setulogmask(LOG_UPTO(LOG_NOTICE));
            }
            else if ((logMask & LOG_MASK(LOG_INFO)) == 0) {
                (void)setulogmask(LOG_UPTO(LOG_INFO));
            }
            else if ((logMask & LOG_MASK(LOG_DEBUG)) == 0) {
                (void)setulogmask(LOG_UPTO(LOG_DEBUG));
            }
            else {
                (void)setulogmask(LOG_UPTO(LOG_ERR));
            }
        }
    }

    return;
}

/*
 * Registers the signal_handler
 */
static void set_sigactions(void)
{
    struct sigaction sigact;

    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    /* Ignore these */
    sigact.sa_handler = SIG_IGN;
    (void)sigaction(SIGHUP, &sigact, NULL);
    (void)sigaction(SIGALRM, &sigact, NULL);
    (void)sigaction(SIGCHLD, &sigact, NULL);
    (void)sigaction(SIGCONT, &sigact, NULL);

    /* Handle these */
#ifdef SA_RESTART		/* SVR4, 4.3+ BSD */
    /* Usually, restart system calls */
    sigact.sa_flags |= SA_RESTART;
#endif
    sigact.sa_handler = signal_handler;
    (void)sigaction(SIGTERM, &sigact, NULL);
    (void)sigaction(SIGUSR1, &sigact, NULL);
    (void)sigaction(SIGUSR2, &sigact, NULL);

    /* Don't restart after interrupt */
    sigact.sa_flags = 0;
#ifdef SA_INTERRUPT		/* SunOS 4.x */
    sigact.sa_flags |= SA_INTERRUPT;
#endif
    (void)sigaction(SIGINT, &sigact, NULL);
}

/**
 * Reads a NOAAPORT data stream, creates LDM data-products from the stream, and
 * inserts the data-products into an LDM product-queue.  The NOAAPORT data
 * stream can take the form of multicast UDP packets from (for example) a
 * Novra S300 DVB-S2 receiver or the standard input stream.
 *
 * Usage:
 *     noaaportIngester [-l <em>log</em>] [-n|-v|-x] [-q <em>queue</em>] [-u <em>n</em>] [-m <em>mcastAddr</em>] [-I <em>iface</em>] [-b <em>npages</em>]\n
 *
 * Where:
 * <dl>
 *      <dt>-b <em>npages</em></dt>
 *      <dd>Allocate \e npages pages of memory for the internal buffer.</dd>
 *
 *      <dt>-I <em>iface</em></dt>
 *      <dd>Listen for multicast packets on interface \e iface.</dd>
 *
 *      <dt>-l <em>log</em></dt>
 *      <dd>Log to file \e log. The default is to use the system logging daemon
 *      if the current process is a daemon; otherwise, the standard error
 *      stream is used.</dd>
 *
 *      <dt>-m <em>mcastAddr</em></dt>
 *      <dd>Use the multicast address \e mcastAddr. The default is to
 *      read from the standard input stream.</dd>
 *
 *      <dt>-n</dt>
 *      <dd>Log messages of level NOTE and higher priority. Each data-product
 *      will generate a log message.</dd>
 *
 *      <dt>-q <em>queue</em></dt>
 *      <dd>Use \e queue as the pathname of the LDM product-queue. The default
 *      is to use the default LDM pathname of the product-queue.</dd>
 *
 *      <dt>-u <em>n</em></dt>
 *      <dd>If logging is to the system logging daemon, then use facility 
 *      <b>local</b><em>n</em>. The default is to use the LDM facility.</dd>
 *
 *      <dt>-v</dt>
 *      <dd>Log messages of level INFO and higher priority.</dd>
 *
 *      <dt>-x</dt>
 *      <dd>Log messages of level DEBUG and higher priority.</dd>
 * </dl>
 *
 * If neither -n, -v, nor -x is specified, then logging will be restricted to
 * levels ERROR and WARN only.
 *
 * @retval 0 if successful.
 * @retval 1 if an error occurred. At least one error-message will be logged.
 */
int main(
    const int           argc,           /**< [in] Number of arguments */
    char* const         argv[])         /**< [in] Arguments */
{
    int                 status = 0;     /* default success */
    extern int          optind;
    extern int          opterr;
    int                 ch;
    const char* const   progName = ubasename(argv[0]);
    const char*         interface = NULL;
    int                 logmask = LOG_UPTO(LOG_WARNING);
    const unsigned      logOptions = LOG_CONS | LOG_PID;
    const char*         mcastSpec = NULL;
    const char*         prodQueuePath = NULL;
    size_t              npages = DEFAULT_NPAGES;
    Fifo*               fifo;
    int                 ttyFd = open("/dev/tty", O_RDONLY);
    const char*         logPath = (-1 == ttyFd)
        ? NULL                          /* log to system logging daemon */
        : "-";                          /* log to standard error stream */

    (void)close(ttyFd);
    (void)setulogmask(logmask);

    status = initLogging(progName, logOptions, logFacility, logPath);
    opterr = 0;                         /* no error messages from getopt(3) */

    while (0 == status && (ch = getopt(argc, argv, "b:I:l:m:nq:u:vx")) != -1) {
        switch (ch) {
            extern char*    optarg;
            extern int      optopt;

            case 'b': {
                unsigned long   n;

                if (sscanf(optarg, "%lu", &n) != 1) {
                    LOG_SERROR1("Couldn't decode FIFO size in pages: \"%s\"",
                            optarg);
                    status = 1;
                }
                else {
                    npages = n;
                }
            }
            case 'I':
                interface = optarg;
                break;
            case 'l':
                logPath = optarg;
                status = initLogging(progName, logOptions, logFacility,
                        logPath);
                break;
            case 'm':
                mcastSpec = optarg;
                break;
            case 'n':
                logmask |= LOG_MASK(LOG_NOTICE);
                (void)setulogmask(logmask);
                break;
            case 'q':
                prodQueuePath = optarg;
                break;
            case 'u': {
                int         i = atoi(optarg);

                if (0 > i || 7 < i) {
                    LOG_START1("Invalid logging facility number: %d", i);
                    status = 1;
                }
                else {
                    static int  logFacilities[] = {LOG_LOCAL0, LOG_LOCAL1,
                        LOG_LOCAL2, LOG_LOCAL3, LOG_LOCAL4, LOG_LOCAL5,
                        LOG_LOCAL6, LOG_LOCAL7};

                    logFacility = logFacilities[i];

                    status = initLogging(progName, logOptions, logFacility,
                            logPath);
                }

                break;
            }
            case 'v':
                logmask |= LOG_MASK(LOG_INFO);
                (void)setulogmask(logmask);
                break;
            case 'x':
                logmask |= LOG_MASK(LOG_DEBUG);
                (void)setulogmask(logmask);
                break;
            default:
                optopt = ch;
                /*FALLTHROUGH*/
            case '?': {
                uerror("Unknown option: \"%c\"", optopt);
                status = 1;
            }
        }                               /* option character switch */
    }                                   /* getopt() loop */

    if (0 == status) {
        if (optind < argc) {
            uerror("Extraneous command-line argument: \"%s\"",
                    argv[optind]);
            status = 1;
        }
    }

    if (0 != status) {
        uerror("Error decoding command-line");
        usage(progName);
    }
    else {
        unotice("Starting Up %s", PACKAGE_VERSION);
        unotice("%s", COPYRIGHT_NOTICE);

        if ((status = fifoNew(npages, &fifo)) != 0) {
            LOG_ADD0("Couldn't create FIFO");
            log_log(LOG_ERR);
        }
        else {
            Reader* reader;

            if (NULL == mcastSpec) {
                if ((status = fileReaderNew(NULL, fifo, &reader)) != 0) {
                    LOG_ADD0("Couldn't create file-reader");
                    log_log(LOG_ERR);
                }
            }
            else {
                if ((status = multicastReaderNew(mcastSpec, interface, fifo,
                                &reader)) != 0) {
                    LOG_ADD0("Couldn't create multicast-reader");
                    log_log(LOG_ERR);
                }
            }

            if (0 == status) {
                LdmProductQueue* prodQueue;

                if ((status = lpqGet(prodQueuePath, &prodQueue)) != 0) {
                    LOG_ADD0("Couldn't open LDM product-queue");
                    log_log(LOG_ERR);
                }
                else {
                    if ((status = pmNew(fifo, prodQueue, &productMaker)) != 0) {
                        LOG_ADD0("Couldn't create new LDM product-maker");
                        log_log(LOG_ERR);
                    }
                    else {
                        if (pthread_create(&productMakerThread, NULL, pmStart,
                                    productMaker) != 0) {
                            LOG_SERROR0("Couldn't start product-maker thread");
                            log_log(LOG_ERR);
                            status = 1;
                        }
                        else {
                            if (pthread_create(&readerThread, NULL, readerStart,
                                        reader) != 0) {
                                LOG_SERROR0(
                                        "Couldn't start data-reader thread");
                                log_log(LOG_ERR);
                                status = 1;
                            }
                            else {
                                set_sigactions();
                                (void)pthread_join(readerThread, NULL);

                                status = readerStatus(reader);

                                fifoCloseWhenEmpty(fifo);
                            }           /* "readerThread" running */

                            (void)pthread_join(productMakerThread, NULL);

                            if (0 != status)
                                status = pmStatus(productMaker);

                            pmLogStats(productMaker);
                        }               /* "productMakerThread" running */
                    }                   /* "productMaker" created */

                    (void)lpqClose(prodQueue);
                }                       /* "prodQueue" open */

                readerFree(reader);
            }                           /* "reader" created */
        }                               /* "fifo" created */
    }                                   /* command-line correctly decoded */

    return status;
}
