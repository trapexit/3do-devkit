/*
        File:		GetOpt.c

        Contains:	getopt function

        Written by:	Bart Besseling

        Copyright © 1993 The 3DO Company. All Rights Reserved.

        Change History (most recent first):

                 <1>	  6/28/93	BJB		first checked in

        To Do:
*/

#include <stdio.h>
#include <string.h>

/* ------------------------------------------------------------
 *  My_GetOpt -- parse command line options
 * ------------------------------------------------------------
 */

char *optarg = ""; /* points to current option argument */
int optind;        /* current index in argv */
int opterr = 1;    /* flag to disable error message */
int optopt;        /* incorrect option */

int
getopt (int argc, char *const *argv, const char *optstring)
{
  static char *argp = "";
  char *p;

  /* are we beginning a new argument? */
  if (*argp == '\0')
    {
      optind++;
      argp = &argv[optind][0];
      /* process only contiguous arguments with single '-'s */
      if (optind >= argc || *argp++ != '-' || *argp == '-')
        return EOF;
    }

  /* check if argument is legal */
  optarg = argp;
  if ((p = strchr (optstring, *argp)) == 0)
    {
      if (opterr)
        fprintf (stderr, "error: bad option '-%c'\n", *argp);
      optopt = *argp++;
      optarg = "";
      return '?';
    }

  /* if option has argument return and skip to next */
  if (p[1] == ':')
    {
      argp = "";
      return *optarg++;
    }
  else
    {
      optarg = "";
      return *argp++;
    }
}
