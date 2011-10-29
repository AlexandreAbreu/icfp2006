#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "debugger.h"

#if ! defined(EOK)
#    define EOK 0
#endif


static int debugger_console (debugger_t * debugger);
static int handle_command (debugger_t * debugger, const char * command);


static int trim_end (char * const s, const char * const seps)
{
  if (NULL == s || NULL == seps)
    {
      return EINVAL;
    }
  
  {
    size_t len = strlen (s);
    char buf[2] = {0};
    size_t idx = 0;
    
    for (idx = len; idx > 0; idx--)
      {
	buf[0] = s[idx - 1];
	if (strstr (seps, buf))
	  {
	    s[idx - 1] = '\0';
	  }
      }
  }
  
  return EOK;
}

static int debugger_console (debugger_t * debugger)
{
  static const char * const prompt = "> ";
  
  printf ("Debugger console (h to help)\n");
  
  while (1)
    {
      char buf[64] = {0};
      
      printf (prompt);
      
      if (fgets (buf, sizeof(buf), stdin))
	{
	  trim_end (buf, "\r\n");
	  handle_command (debugger, buf);
	}
    }
  
  return EOK;
}

static int handle_command (debugger_t * debugger, const char * command)
{
  if (NULL == command || NULL == debugger)
    {
      return EINVAL;
    }
  
  if (0 == strncmp (command, "h", strlen(command)))
    {
      // TODO should be handled better
      printf ("where: printf the current IP location\n");
      printf ("next: executes the next instruction\n");
      printf ("registers: dumps the current state of registers\n");
      printf ("run-until [symbol] [=|>] [value]: runs the program until the command\n"
	      "\tevaluates to true.\n"
	      "\tThe command is a symbol ('IP') followed by an operator ('=' or '>')\n"
	      "\tfollowed by a value.\n");
      printf ("q: quit\n");
      return EOK;
    }
  
  if (0 == strncmp (command, "q", strlen(command)))
    {
      return 1;
    }
  
  if (0 == strncmp (command, "where", strlen(command))
      && NULL != debugger->where)
    {
      debugger->where ();
      return EOK;
    }
  
  if (
      (0 == strncmp (command, "next", strlen(command))
       || 0 == strncmp (command, "n", strlen(command)))
      && NULL != debugger->next)
    {
      debugger->next ();
      return EOK;
    }
  
  if (0 == strncmp (command, "registers", strlen(command))
      && NULL != debugger->registers)
    {
      debugger->registers ();
      return EOK;
    }
  
  {
    // bad usage of macro (multiple eval of a and b)
#if defined(MIN)
#error MIN already defined
#endif
    
#define MIN(a,b) ((a) > (b) ? (b) : (a))
    
    const char * COMMAND_NAME = "run-until";
    
    if (0 == strncmp (command
		      , COMMAND_NAME
		      , MIN (strlen(command)
			     , strlen(COMMAND_NAME)
			     )
		      )
	&& NULL != debugger->run_until)
      {
	debugger->run_until (command + strlen(COMMAND_NAME));
	return EOK;
      }
    
#undef MIN
  }
  
  printf ("'%s' is an unknown command (type 'h' for help)\n", command);
  
  return EOK;
}


int run_debugger (debugger_t * debugger)
{
  if (NULL == debugger)
    {
      return EINVAL;
    }
  
  return debugger_console (debugger);
}

