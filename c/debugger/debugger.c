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
      return EOK;
    }
  
  if (0 == strncmp (command, "where", strlen(command)))
    {
      debugger->where ();
      return EOK;
    }
  
  if (0 == strncmp (command, "next", strlen(command)))
    {
      debugger->next ();
      return EOK;
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

