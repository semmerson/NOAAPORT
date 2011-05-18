/*
 *   Copyright 2011, University Corporation for Atmospheric Research.
 *   See file COPYRIGHT for copying and redistribution conditions.
 */
#ifndef NOAAPORT_LOG_H
#define NOAAPORT_LOG_H

#include <string.h>
#include <errno.h>
#include <log.h>

#if !defined(LOG_FMT)
#   define LOG_SERROR0(fmt)             {log_errno(); log_add(fmt);}
#   define LOG_SERROR1(fmt,a)           {log_errno(); log_add(fmt,a);}
#   define LOG_SERROR2(fmt,a,b)         {log_errno(); log_add(fmt,a,b);}
#   define LOG_SERROR3(fmt,a,b,c)       {log_errno(); log_add(fmt,a,b,c);}
#   define LOG_SERROR4(fmt,a,b,c,d)     {log_errno(); log_add(fmt,a,b,c,d);}
#   define LOG_SERROR5(fmt,a,b,c,d,e)   {log_errno(); log_add(fmt,a,b,c,d,e);}

#   define LOG_START0(fmt)              log_start(fmt)
#   define LOG_START1(fmt,a)            log_start(fmt,a)
#   define LOG_START2(fmt,a,b)          log_start(fmt,a,b)
#   define LOG_START3(fmt,a,b,c)        log_start(fmt,a,b,c)
#   define LOG_START4(fmt,a,b,c,d)      log_start(fmt,a,b,c,d)
#   define LOG_START5(fmt,a,b,c,d,e)    log_start(fmt,a,b,c,d,e)

#   define LOG_ADD0(fmt)                log_add(fmt)
#   define LOG_ADD1(fmt,a)              log_add(fmt,a)
#   define LOG_ADD2(fmt,a,b)            log_add(fmt,a,b)
#   define LOG_ADD3(fmt,a,b,c)          log_add(fmt,a,b,c)
#   define LOG_ADD4(fmt,a,b,c,d)        log_add(fmt,a,b,c,d)
#   define LOG_ADD5(fmt,a,b,c,d,e)      log_add(fmt,a,b,c,d,e)
#endif

#endif
