#include "popen.h"
#include <stdlib.h>
#include <io.h>
#include <string.h>
#include <process.h>

#ifdef OS2
#ifdef _MSC_VER
#define popen(c,m)   _popen(c,m)
#define pclose(f)    _pclose(f)
#endif
#endif

#ifndef _NFILE
#define _NFILE 40
#endif

static char template[] = "piXXXXXX";
typedef enum { unopened = 0, reading, writing } pipemode;
static
struct {
    char *command;
    char *name;
    pipemode pmode;
} pipes[_NFILE];

FILE *
os_popen( char *command, char *mode ) {
    FILE *current;
    char *name;
    int cur;
    pipemode curmode;
    
#if defined(OS2) && (_MSC_VER != 510)
    if (_osmode == OS2_MODE)
      return(popen(command, mode));
#endif

    /*
    ** decide on mode.
    */
    if(strcmp(mode,"r") == 0)
        curmode = reading;
    else if(strcmp(mode,"w") == 0)
        curmode = writing;
    else
        return NULL;
    /*
    ** get a name to use.
    */
    if((name = tempnam(".","pip"))==NULL)
        return NULL;
    /*
    ** If we're reading, just call system to get a file filled with
    ** output.
    */
    if(curmode == reading) {
        if ((cur = dup(fileno(stdout))) == -1)
	    return NULL;
        if ((current = freopen(name, "w", stdout)) == NULL)
	    return NULL;
        system(command);
        if (dup2(cur, fileno(stdout)) == -1)
	    return NULL;
	close(cur);
        if((current = fopen(name,"r")) == NULL)
            return NULL;
    } else {
        if((current = fopen(name,"w")) == NULL)
            return NULL;
    }
    cur = fileno(current);
    pipes[cur].name = name;
    pipes[cur].pmode = curmode;
    pipes[cur].command = strdup(command);
    return current;
}

int
os_pclose( FILE * current) {
    int cur = fileno(current),rval;

#if defined(OS2) && (_MSC_VER != 510)
    if (_osmode == OS2_MODE)
      return(pclose(current));
#endif

    /*
    ** check for an open file.
    */
    if(pipes[cur].pmode == unopened)
        return -1;
    if(pipes[cur].pmode == reading) {
        /*
        ** input pipes are just files we're done with.
        */
        rval = fclose(current);
        unlink(pipes[cur].name);
    } else {
        /*
        ** output pipes are temporary files we have
        ** to cram down the throats of programs.
        */
	int fd;
        fclose(current);
	rval = -1;
	if ((fd = dup(fileno(stdin))) != -1) {
	  if (current = freopen(pipes[cur].name, "r", stdin)) {
	    rval = system(pipes[cur].command);
	    fclose(current);
	    if (dup2(fd, fileno(stdin)) == -1) rval = -1;
	    close(fd);
	  }
	}
        unlink(pipes[cur].name);
    }
    /*
    ** clean up current pipe.
    */
    pipes[cur].pmode = unopened;
    free(pipes[cur].name);
    free(pipes[cur].command);
    return rval;
}
