#if ! defined (UC_H)
#define UC_H

// should be more precise ... 
typedef unsigned char byte;
typedef unsigned int platter_t;
typedef platter_t address_t;


/**
 * 
 * 
 */
typedef enum UM_CONSTANTS
  {
    UM_REGISTER_COUNT   = 8,
    UM_PROGRAM_ARRAY_ID = 0,

  } UM_CONSTANTS;


/**
 * 
 * 
 */
typedef struct um_t
{
  // registers
  platter_t registers[UM_REGISTER_COUNT];

  // basic runtime structures
  address_t ip;
    
  void * arrays;

} um_t;


/**
 *
 *
 */
int um_run (struct um_t *
	    , byte *
	    , size_t);

/**
 * 
 * @param on_one_step
 */

typedef struct pp_opcode_data_t
{
  platter_t p;
  byte rega;
  byte regb;
  byte regc;
  
}  pp_opcode_data_t;

typedef void (* pp_opcode_func) (char * out
				 , size_t outsize
				 , struct um_t * machine
				 , pp_opcode_data_t d);

typedef void (* on_run_one_step_func) (struct um_t * machine
				       , pp_opcode_func pp_opcode
				       , pp_opcode_data_t d);

int um_run_one_step (struct um_t * machine
		     , byte * codex
		     , size_t codex_size
		     , on_run_one_step_func f
		     );

typedef int (* should_be_stopped_func) (struct um_t * machine
					, platter_t instruction);

int um_run_until (struct um_t * machine
		  , byte * codex
		  , size_t codex_size
		  , on_run_one_step_func onestep
		  , should_be_stopped_func should_be_stopped);

#endif // UC_H

