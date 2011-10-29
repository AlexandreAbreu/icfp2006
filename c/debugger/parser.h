#ifndef PARSER_H
#define PARSER_H

typedef struct environment_t
{
  unsigned int (*get_symbol_value) (const char * const symbol);
  
} environment_t;


/**
 *
 * @param command the command that needs to be parsed / executed
 * @param env the environment sink that is to be used to retrieved data "about the outside"
 * @return result of the execution
 */
int execute_command (const char * const command
		     , environment_t env);

#endif // #ifndef PARSER_H

