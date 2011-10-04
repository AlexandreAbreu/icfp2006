#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "um.h"

#if ! defined (EOK)
#   define EOK 0
#endif

#if defined (OPCODE_FROM_PLATTER)
#   error "Oops macro redefinition: OPCODE_FROM_PLATTER"
#endif

#define OPCODE_FROM_PLATTER(platter) (((platter) >> 28) & 0xF)


typedef enum Register
  {
    REGISTER_A = 2,
    REGISTER_B = 1,
    REGISTER_C = 0,

  } Register;


typedef unsigned int ArrayCellId;
typedef struct ArrayCell
{
  ArrayCellId id;
  platter_t datasize; // in platter_t count
  platter_t * data;
  
  struct ArrayCell * next;

} ArrayCell;


static ArrayCell * um_priv_new_array_cell (platter_t capacity);
static ArrayCell * um_priv_add_array_cell (struct um_t * machine, ArrayCell * p);
static platter_t um_priv_read_platter_from (struct um_t * machine, address_t a);
static int um_priv_initialize_machine (struct um_t * machine);
static int um_priv_initialize_program_array_with (struct um_t * machine
						                          , byte * data
						                          , size_t size);
static byte decode_register_value_from_platter (platter_t p, Register r);
static void fail (struct um_t * machine);
static int um_priv_do_spin (struct um_t * machine);


/////////////////////////
// operators / handlers
/////////////////////////

typedef enum OperatorCodes
  {
    OP_COND_MOVE,
    OP_ARRAY_INDEX,
    OP_ARRAY_AMEND,
    OP_ADDITION,
    OP_MULTIPLICATION,
    OP_DIVISION,
    OP_NOT_AND,
    OP_HALT,
    OP_ALLOCATION,
    OP_ABANDONMENT,
    OP_OUTPUT,
    OP_INPUT,
    OP_LOAD_PROGRAM,
    OP_ORTHOGRAPHY,

  } OperatorCodes;

static int um_priv_handler_cond_mov  (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_array_idx (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_array_amend (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);

static int um_priv_handler_addition (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_multiplication (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_division (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_not_and (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);

static int um_priv_handler_halt (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_allocation (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_abandonment (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_output (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_input (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);
static int um_priv_handler_load_program (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);

static int um_priv_handler_orthography (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);


/////////////////////////
// operator definitions
/////////////////////////

typedef int (* op_handler) (struct um_t * machine, platter_t p, byte rega, byte regb, byte regc);

struct Operator
{
  OperatorCodes   code;
  op_handler      handler;
  
} g_operators [] = {
#if 0
    [OP_COND_MOVE] = { .code = OP_COND_MOVE, .handler = um_priv_handler_cond_mov },
  [OP_ARRAY_INDEX] = { .code = OP_ARRAY_INDEX, .handler = um_priv_handler_array_idx },
  [OP_ARRAY_AMEND] = { .code = OP_ARRAY_AMEND, .handler = um_priv_handler_array_amend },
  [OP_ADDITION] = { .code = OP_ADDITION, .handler = um_priv_handler_addition },
  [OP_MULTIPLICATION] = { .code = OP_MULTIPLICATION, .handler = um_priv_handler_multiplication },
  [OP_DIVISION] = { .code = OP_DIVISION, .handler = um_priv_handler_division },
  [OP_NOT_AND] = { .code = OP_NOT_AND, .handler = um_priv_handler_not_and },

  [OP_HALT] = { .code = OP_HALT, .handler = um_priv_handler_halt },
  [OP_ALLOCATION] = { .code = OP_ALLOCATION, .handler = um_priv_handler_allocation },
  [OP_ABANDONMENT] = { .code = OP_ABANDONMENT, .handler = um_priv_handler_abandonment },
  [OP_OUTPUT] = { .code = OP_OUTPUT, .handler = um_priv_handler_output },
  [OP_INPUT] = { .code = OP_INPUT, .handler = um_priv_handler_input },
  [OP_LOAD_PROGRAM] = { .code = OP_LOAD_PROGRAM, .handler = um_priv_handler_load_program },

  [OP_ORTHOGRAPHY] = { .code = OP_ORTHOGRAPHY, .handler = um_priv_handler_orthography },
#endif

  { OP_COND_MOVE, um_priv_handler_cond_mov },
  { OP_ARRAY_INDEX, um_priv_handler_array_idx },
  { OP_ARRAY_AMEND, um_priv_handler_array_amend },
  { OP_ADDITION, um_priv_handler_addition },
  { OP_MULTIPLICATION, um_priv_handler_multiplication },
  { OP_DIVISION, um_priv_handler_division },
  { OP_NOT_AND, um_priv_handler_not_and },

  { OP_HALT, um_priv_handler_halt },
  { OP_ALLOCATION, um_priv_handler_allocation },
  { OP_ABANDONMENT, um_priv_handler_abandonment },
  { OP_OUTPUT, um_priv_handler_output },
  { OP_INPUT, um_priv_handler_input },
  { OP_LOAD_PROGRAM, um_priv_handler_load_program },

  { OP_ORTHOGRAPHY, um_priv_handler_orthography },
};


/////////////////////////
// operator definitions
/////////////////////////

static ArrayCell * um_priv_get_last_array_cell (struct um_t * machine)
{
  ArrayCell * p = (ArrayCell *) machine->arrays;
    
  assert (NULL != p);
    
  while (NULL != p->next)
    {
      p = p->next;
    }
    
  return p;
}

static void um_priv_delete_array (ArrayCell * cell)
{
  if (NULL == cell)
    {
      return;
    }
  
  free (cell->data);
  free (cell);
}

static ArrayCellId um_priv_get_next_cellid ()
{
  // 0 is for the program
  static ArrayCellId id = 0;
  return id++;
}

static ArrayCell * um_priv_new_array_cell (platter_t capacity)
{
  ArrayCell * p = (ArrayCell *) malloc (sizeof (ArrayCell));

  p->next = NULL;
  p->data = (platter_t *) malloc (capacity * sizeof(platter_t));
  p->datasize = capacity;
  p->id = um_priv_get_next_cellid ();
  
  return p;
}

static ArrayCell * um_priv_new_array_cell_from_cell (ArrayCell * cell)
{
  ArrayCell * p = (ArrayCell *) malloc (sizeof (ArrayCell));

  p->next = NULL;
  p->data = (platter_t *) malloc (cell->datasize * sizeof(platter_t));
  memcpy ((char *) p->data, (char *) cell->data, cell->datasize * sizeof(platter_t));
  
  p->datasize = cell->datasize;
  p->id = um_priv_get_next_cellid ();
  
  return p;
}

static ArrayCell * um_priv_search_for_cell_id (struct um_t * machine, ArrayCellId id)
{
  ArrayCell * p = (ArrayCell *) machine->arrays;
  while (p != NULL)
    {
      if (p->id == id)
	{
	  break;
	}
      
      p = p->next;
    }
  
  return p;
}

static ArrayCell * um_priv_add_array_cell (struct um_t * machine, ArrayCell * p)
{
  assert (NULL != p);
  
  {
    ArrayCell * last = um_priv_get_last_array_cell (machine->arrays);
    assert (NULL != last);
    last->next = p;
  }
  
  return p;
}

/**
 * 
 * @param a is a platter address
 */
static platter_t um_priv_read_platter_from (struct um_t * machine
                                            , address_t a)
{
#define VALIDATE_ADDRESS(address,array) if ((address) >= (array)->datasize) fail(machine)
    
    platter_t valueAt = 0;

    // TODO validate address
    ArrayCell *
        cell = um_priv_search_for_cell_id (machine, UM_PROGRAM_ARRAY_ID);
    if (NULL == cell)
    {
        fail (machine);
    }
    
    VALIDATE_ADDRESS (a, cell);
    
    {
        // assume little endian acrhitecture
        platter_t v = cell->data[a];
        
        valueAt |= (v & 0xFF) << 24;
        valueAt |= (v & 0xFF00) << 8;
        valueAt |= (v & 0xFF0000) >> 8;
        valueAt |= (v & 0xFF000000) >> 24;
    }
    
    return valueAt;
}

static int um_priv_initialize_machine (struct um_t * machine)
{
  if (NULL == machine)
    {
      return EINVAL;
    }
  
  // assume memset eq value 0
  memset ((void *) &machine->registers[0], 0, sizeof(machine->registers));
  
  machine->ip = 0;
  
  // allocate one for the special case of the program array
  machine->arrays = NULL;
  
  return EOK;
}

static int um_priv_initialize_program_array_with (struct um_t * machine
						  , byte * data
						  , size_t size)
{
  // check if already exists
  ArrayCell * cell = um_priv_search_for_cell_id (machine, UM_PROGRAM_ARRAY_ID);
  if (NULL != cell)
    {
      fail (machine);
    }
  
  if (0 != (size % sizeof(platter_t)))
    {
      fail (machine);
    }
  
  cell = um_priv_new_array_cell (size);
  
  // should be the first allocation
  if (NULL == cell || cell->id != UM_PROGRAM_ARRAY_ID)
    {
      fail (machine);
    }
  
  assert (cell->datasize == size);

  memcpy (cell->data, data, cell->datasize);

  machine->arrays = cell;
}

static byte decode_register_value_from_platter (platter_t p, Register r)
{
  static const byte REG_MASK = 0x7U;
  const byte offset = r * 3;
    
  assert (r <= 2 & r >= 0);
    
  return (p & (REG_MASK << offset)) >> offset;
}

static void fail (struct um_t * machine)
{
    assert(0);
  fprintf (stderr, "fail: invalid operation\n");
  exit (1);
}

#include <setjmp.h>
static jmp_buf jmp_env;

static int um_priv_do_spin (struct um_t * machine)
{
#define VALIDATE_OPCODE(opcode) if (opcode >= (sizeof(g_operators) / sizeof(g_operators[0]))) fail (machine)
  
      if ( ! setjmp (jmp_env))
	{
      while (1)
        {
	      platter_t op = um_priv_read_platter_from (machine, machine->ip++);
    	  
	      VALIDATE_OPCODE ( OPCODE_FROM_PLATTER(op));
    	  
	      assert (g_operators [OPCODE_FROM_PLATTER (op)].code == OPCODE_FROM_PLATTER(op));
    	  
          {
              byte rega = decode_register_value_from_platter (op, REGISTER_A);
              byte regb = decode_register_value_from_platter (op, REGISTER_B);
              byte regc = decode_register_value_from_platter (op, REGISTER_C);
            
	          g_operators [OPCODE_FROM_PLATTER (op)].handler (machine
							          , op
							          , rega
							          , regb
							          , regc
							          );
          }
      }
      }
      else
	{
	  printf ("Processor halted\n");
	}
  
#undef VALIDATE_OPCODE

  return EOK;
}


//////////////////////////////////////////////////
// operator handler definitions
//////////////////////////////////////////////////

#define VALIDATE_OFFSET(offset,array) if ((offset) >= (array)->datasize) fail(machine)
#define VALIDATE_REGISTER_INDEX(r) if ((r) >= UM_REGISTER_COUNT) fail(machine)
#define VALIDATE_REGISTERS(func)        /* printf (#func " ip: %d, rega: %d, regb, %d, regc %d\n", machine->ip, rega, regb, regc);*/ VALIDATE_REGISTER_INDEX(rega); VALIDATE_REGISTER_INDEX(regb); VALIDATE_REGISTER_INDEX(regc)
    

static int um_priv_handler_cond_mov (struct um_t * machine
                                     , platter_t p
                                     , byte rega
                                     , byte regb
                                     , byte regc
                                     )
{
  VALIDATE_REGISTERS (um_priv_handler_cond_mov);
    
  {
    if (0 != machine->registers[regc])
      {
	machine->registers[rega] = machine->registers[regb];
      }
  }
    
  return EOK;
}

static int um_priv_handler_array_idx (struct um_t * machine
                                      , platter_t p
                                      , byte rega
                                      , byte regb
                                      , byte regc
                                      )
{
  VALIDATE_REGISTERS (um_priv_handler_array_idx);
  
  {
    platter_t array_idx = machine->registers[regb];
    platter_t array_offset = machine->registers[regc];
    
    ArrayCell * cell = um_priv_search_for_cell_id (machine, array_idx);
    if (NULL == cell)
      {
	fail (machine);
      }
    
    VALIDATE_OFFSET(array_offset,cell);
    
    // TODO validate data offset
    machine->registers[rega] = cell->data[array_offset];
  }
    
  return EOK;
}

static int um_priv_handler_array_amend (struct um_t * machine
                                        , platter_t p
                                        , byte rega
                                        , byte regb
                                        , byte regc
                                        )
{
  VALIDATE_REGISTERS (um_priv_handler_array_amend);
    
  {
    platter_t array_idx = machine->registers[rega];
    platter_t array_offset = machine->registers[regb];
        
    ArrayCell * cell = um_priv_search_for_cell_id (machine, array_idx);
    if (NULL == cell)
      {
	fail (machine);
      }
    
    VALIDATE_OFFSET(array_offset,cell);
    
    cell->data[array_offset] = machine->registers[regc];
  }
    
  return EOK;
}

static int um_priv_handler_addition (struct um_t * machine
                                     , platter_t p
                                     , byte rega
                                     , byte regb
                                     , byte regc
                                     )
{
  VALIDATE_REGISTERS (um_priv_handler_addition);
  
  // check for overflow in intermediate computation
  machine->registers[rega] = machine->registers[regb] + machine->registers[regc];
  
  return EOK;
}

static int um_priv_handler_multiplication (struct um_t * machine
                                           , platter_t p
                                           , byte rega
                                           , byte regb
                                           , byte regc
                                           )
{
  VALIDATE_REGISTERS (um_priv_handler_multiplication);
    
  // check for overflow in intermediate computation
  machine->registers[rega] = machine->registers[regb] * machine->registers[regc];
    
  return EOK;
}

static int um_priv_handler_division (struct um_t * machine
                                     , platter_t p
                                     , byte rega
                                     , byte regb
                                     , byte regc
                                     )
{
  VALIDATE_REGISTERS (um_priv_handler_division);
  
  // check for overflow in intermediate computation
  if (0 == machine->registers[regc])
    {
      fail (machine);
    }
  
  machine->registers[rega] = (machine->registers[regb] / machine->registers[regc]);
  
  return EOK;
}

static int um_priv_handler_not_and (struct um_t * machine
                                    , platter_t p
                                    , byte rega
                                    , byte regb
                                    , byte regc
                                    )
{
  VALIDATE_REGISTERS (um_priv_handler_not_and);
  
  // check for overflow in intermediate computation
  machine->registers[rega] = ~machine->registers[regb] | ~machine->registers[regc];
  
  return EOK;
}

static int um_priv_handler_allocation (struct um_t * machine
				       , platter_t p
				       , byte rega
				       , byte regb
				       , byte regc
				       )
{
  VALIDATE_REGISTERS (um_priv_handler_allocation);
    
  {
    ArrayCell * cell = um_priv_new_array_cell (machine->registers[regc]);
    memset (cell->data, 0, cell->datasize * sizeof(platter_t));
    machine->registers[rega] = (platter_t) cell;
  }
    
  return EOK;
}

static int um_priv_handler_abandonment (struct um_t * machine
					, platter_t p
					, byte rega
					, byte regb
					, byte regc
					)
{
  VALIDATE_REGISTERS (um_priv_handler_abandonment);
  
  if (machine->registers[regc] == UM_PROGRAM_ARRAY_ID)
    {
      fail (machine);
    }
  
  {
    ArrayCellId
      id = machine->registers[regc];
    
    ArrayCell *
      cell = um_priv_search_for_cell_id (machine, id);
    if (NULL == cell)
      {
	fail (machine);
      }
  }
  
  return EOK;
}

static int um_priv_handler_output (struct um_t * machine
				    , platter_t p
				    , byte rega
				    , byte regb
				    , byte regc
				    )
{
  VALIDATE_REGISTERS (um_priv_handler_output);
  
  {
    if (machine->registers[regc] > 255)
      {
	fail (machine);
      }
    
    printf ("%c", machine->registers[regc]);
  }
  
  return EOK;
}

static int um_priv_handler_input (struct um_t * machine
				    , platter_t p
				    , byte rega
				    , byte regb
				    , byte regc
				    )
{
  VALIDATE_REGISTERS (um_priv_handler_input);
  
  {
    int c = fgetc (stdin);
    if (EOF == c)
      {
	machine->registers[regc] = 0xFFFFFFFF;
      }
    else
      {
	machine->registers[regc] = c;
      }
  }
  
  return EOK;
}

static int um_priv_handler_load_program (struct um_t * machine
					 , platter_t p
					 , byte rega
					 , byte regb
					 , byte regc
					 )
{
  VALIDATE_REGISTERS (um_priv_handler_load_program);
  
  if (UM_PROGRAM_ARRAY_ID != machine->registers[regb])
  {
    ArrayCell *
      cell = um_priv_search_for_cell_id (machine, machine->registers[regb]);
    
    if (NULL == cell)
      {
	fail (machine);
      }
    
    {
        ArrayCell *
          newcell = um_priv_new_array_cell_from_cell (cell);
        
        ArrayCell * zeroc = um_priv_search_for_cell_id (machine, UM_PROGRAM_ARRAY_ID);
        if (NULL != zeroc)
	    {
	      um_priv_delete_array (zeroc);
	    }
    }
  }
  
    {
      platter_t offset = machine->registers[regc];
      machine->ip = offset;
    }

  return EOK;
}

static int um_priv_handler_halt (struct um_t * machine
				 , platter_t p
				 , byte rega
				 , byte regb
				 , byte regc
				 )
{
  longjmp (jmp_env, 1);
  
  return EOK;
}

static int um_priv_handler_orthography (struct um_t * machine
					, platter_t p
					, byte rega
					, byte regb
					, byte regc
					)
{
  platter_t value = p & 0x1FFFFFF;
  
  // correct register a
  rega = (p >> 25) & 0x07;
  
//  printf ("um_priv_handler_orthography ip: %d, rega: %d, value: %d\n", machine->ip, rega, value);

  machine->registers[rega] = value;
  
  return EOK;
}


#undef VALIDATE_REGISTERS
#undef VALIDATE_REGISTER_INDEX
#undef VALIDATE_OFFSET


//////////////////////////////////////
// public functions
//////////////////////////////////////

int um_run (struct um_t * machine, byte * codex, size_t codex_size)
{
  um_priv_initialize_machine (machine);
  um_priv_initialize_program_array_with (machine, codex, codex_size);
  
  return um_priv_do_spin (machine);
}
