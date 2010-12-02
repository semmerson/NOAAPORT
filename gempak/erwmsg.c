#include "ulog.h"

void	er_wmsg ( char *errgrp, int *numerr, char *errstr, int *iret)
{
*iret = 0;

if ( *numerr != 0 )
   uerror("[%s %d] %s",errgrp,*numerr,errstr);
else
   if(ulogIsVerbose()) uinfo("[%s %d] %s",errgrp,*numerr,errstr);
}
