// icfp.cpp : Defines the entry point for the console application.
//

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "um.h"
#include "debugger/debugger.h"

#if ! defined(EOK)
#define EOK 0
#endif


um_t u_machine;


int run_debug_mode (um_t * machine, byte * data, size_t size)
{
  int should_be_stopped (struct um_t * machine, platter_t instruction)
  {
    return 0;
  }
  
  void onestep (struct um_t * machine, pp_opcode_func pp_opcode, pp_opcode_data_t d)
  {
    if (pp_opcode)
      {
        char out [128] = {0};
	
	pp_opcode (out, sizeof(out) / sizeof(out[0]), machine, d);
	
	printf ("0x%08X : %s\n", machine->ip, out);
      }
  }
  
  // big GCC / C99 extension
  int next ()
  {
    um_run_one_step (machine, data, size, onestep);
    return EOK;
  }
  
  int peek_next ()
  {
    um_run_one_step (machine, data, size, onestep);
    return EOK;
  }
  
  int run_until ()
  {
    um_run_until (machine, data, size, onestep, should_be_stopped);
  }
  
  int where ()
  {
    printf ("IP: 0x%08X\n", machine->ip);
    return EOK;
  }
  
  int registers ()
  {
    size_t i = 0;
    for (i = 0; i < UM_REGISTER_COUNT; ++i)
      {
	assert (i < (sizeof(machine->registers) / sizeof(machine->registers[0])));
	
	printf ("reg[0] = 0x%08X\n", machine->registers[i]);
      }
    return EOK;
  }
  
  debugger_t debugger = {
    .next = next,
    .where = where,
    .registers = registers,
    .run_until = run_until
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

