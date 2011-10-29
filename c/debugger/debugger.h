#if ! defined (DEBUGGER_H)
#define DEBUGGER_H

typedef struct instruction_t
{
  unsigned int v;
  
} instruction_t;

typedef struct debugger_t
{
  int (* next) (void);
  int (* where) (void);
  int (* registers) (void);
  int (* run_until) (const char * const arguments);
  
  instruction_t * instructions;
  
} debugger_t;

int run_debugger (debugger_t *);

#endif
