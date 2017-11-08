/*
** popen.h -- prototypes for pipe functions
*/
#if !defined(FILE)
#include <stdio.h>
#endif

extern FILE *os_popen( char *, char * );
extern int  os_pclose( FILE * );
