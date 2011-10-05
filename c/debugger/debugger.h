#if ! defined (DEBUGGER_H)
#define DEBUGGER_H

typedef struct debugger_t
{
  int (* next) (void);
  int (* where) (void);
  
} debugger_t;

int run_debugger (debugger_t *);

#endif
