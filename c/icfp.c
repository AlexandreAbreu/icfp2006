// icfp.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>

#include "um.h"

um_t u_machine;

int main (int argc, char ** argv)
{
	FILE * f = fopen ("E:\\Tools\\misc\\icfp\\icfp\\data\\sandmark.umz", "rb");
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
            if (fs == fread (content, 1, fs, f))
            {
                um_run (&u_machine, content, fs);
            }
        }
        
        free (content);
	}
    
    return 0;
}

