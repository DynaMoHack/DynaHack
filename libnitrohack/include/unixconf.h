/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NitroHack may be freely redistributed.  See license for details. */

#ifdef UNIX
#ifndef UNIXCONF_H
#define UNIXCONF_H

#include <time.h>

#include "system.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

/* Cygwin already defines this. */
#if !defined(__CYGWIN__)
#define strncmpi(a,b,c) strncasecmp(a,b,c)
#endif

#endif /* UNIXCONF_H */
#endif /* UNIX */
