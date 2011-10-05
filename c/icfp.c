// icfp.cpp : Defines the entry point for the console application.
//

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "um.h"
#include "debugger/debugger.h"


um_t u_machine;

int run_debug_mode (um_t * machine, byte * data, size_t size)
{
  // big GCC / C99 extension
  int next ()
  {
    um_run_one_step (machine, data, size);
  }
  int where ()
  {
    printf ("IP: 0x%08X\n", machine->ip);
  }
  
  debugger_t debugger = {
    .next = next,
    .where = where
  };
  
  return run_debugger (&debugger);
}

int run_normal (um_t * machine, byte * data, size_t size)
{
  return um_run (&machine, data, size);
}

int main (int argc, char ** argv)
{
  FILE * f = fopen ("../data/sandmark.umz", "rb");
  if ( ! f)
    {
      printf ("Could not open the codex file: %d\n", errno);
      return 1;
    }
  
  {
    size_t fs = 0;
    byte * content = NULL;

    fseek (f, 0, SEEK_END);
    fs = ftell (f);
    fseek (f, 0, SEEK_SET);
    
    content = (byte *) malloc (fs);
    if (NULL != content)
      {
	if (fs == fread (content, 1, fs, f))
	  {
	    if (argc > 1
		&& 0 == strncmp (argv[1], "-d", strlen(argv[1])))
	      {
		run_debug_mode (&u_machine, content, fs);
	      }
	    else
	      {
		run_normal (&u_machine, content, fs);
	      }
	  }
      }
        
    free (content);
  }
    
  return 0;
}

