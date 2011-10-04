#include <stdio.h>
#include <stdlib.h>

#include "uc.h"

struct uc uc_machine;

int main (int argc, char ** argv)
{
  FILE * f = fopen ("./data/codex.umz", "rb");
  if ( ! f)
    {
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
	uc_run (&uc_machine, content, fs);
      }
    
    free (content);
  }
    
  return 0;
}

