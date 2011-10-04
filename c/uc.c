#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "uc.h"

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


static ArrayCell * us_priv_new_array_cell (platter_t capacity);
static ArrayCell * us_priv_add_array_cell (struct uc * machine, ArrayCell * p);
static platter_t uc_priv_read_platter_from (address_t a);
static int uc_priv_initialize_machine (struct uc * machine);
static byte decode_register_value_from_platter (platter_t p, Register r);
static void fail (struct uc * machine);
static int uc_priv_do_spin (struct uc * machine);


/////////////////////////
// operators / handlers
/////////////////////////

typedef enum OperatorCodes
  {
    OP_COND_MOVE        = 0x0,
    OP_ARRAY_INDEX      = 0x1,
    OP_ARRAY_AMEND      = 0x2,
    OP_ADDITION         = 0x3,
    OP_MULTIPLICATION   = 0x4,
    OP_DIVISION         = 0x5,
    OP_NOT_AND          = 0x6,

  } OperatorCodes;

static int uc_priv_handler_cond_mov  (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);
static int uc_priv_handler_array_idx (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);
static int uc_priv_handler_array_amend (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);

static int uc_priv_handler_addition (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);
static int uc_priv_handler_multiplication (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);
static int uc_priv_handler_division (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);

static int uc_priv_handler_not_and (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);


/////////////////////////
// operator definitions
/////////////////////////

typedef int (* op_handler) (struct uc * machine, platter_t p, byte rega, byte regb, byte regc);

struct Operator
{
  OperatorCodes   code;
  op_handler      handler;
  
} g_operators [] = {
  [OP_COND_MOVE] = { .code = OP_COND_MOVE, .handler = uc_priv_handler_cond_mov },
  [OP_ARRAY_INDEX] = { .code = OP_ARRAY_INDEX, .handler = uc_priv_handler_array_idx },
  [OP_ARRAY_AMEND] = { .code = OP_ARRAY_AMEND, .handler = uc_priv_handler_array_amend },
  [OP_ADDITION] = { .code = OP_ADDITION, .handler = uc_priv_handler_addition },
  [OP_MULTIPLICATION] = { .code = OP_MULTIPLICATION, .handler = uc_priv_handler_multiplication },
  [OP_DIVISION] = { .code = OP_DIVISION, .handler = uc_priv_handler_division },
  [OP_NOT_AND] = { .code = OP_NOT_AND, .handler = uc_priv_handler_not_and },
};


/////////////////////////
// operator definitions
/////////////////////////

static ArrayCell * us_priv_get_last_array_cell (struct uc * machine)
{
  ArrayCell * p = (ArrayCell *) machine->arrays;
    
  assert (NULL != p);
    
  while (NULL != p->next)
    {
      p = p->next;
    }
    
  return p;
}

static void us_priv_delete_array (ArrayCell * cell)
{
  if (NULL == cell)
    {
      return;
    }
  
  free (cell->data);
  free (cell);
}

static ArrayCellId us_priv_get_next_cellid ()
{
  // 0 is for the program
  static ArrayCellId id = 0;
  return id++;
}

static ArrayCell * us_priv_new_array_cell (platter_t capacity)
{
  ArrayCell * p = (ArrayCell *) malloc (sizeof (ArrayCell));

  p->next = NULL;
  p->data = (platter_t *) malloc (capacity * sizeof(platter_t));
  p->datasize = capacity;
  p->id = us_priv_get_next_cellid ();
  
  return p;
}

static ArrayCell * us_priv_new_array_cell_from_cell (ArrayCell * cell)
{
  ArrayCell * p = (ArrayCell *) malloc (sizeof (ArrayCell));

  p->next = NULL;
  p->data = (platter_t *) malloc (cell->datasize * sizeof(platter_t));
  memcpy ((char *) p->data, (char *) cell->data, cell->datasize * sizeof(platter_t));
  
  p->datasize = cell->datasize;
  p->id = us_priv_get_next_cellid ();
  
  return p;
}

static ArrayCell * us_priv_search_for_cell_id (struct uc * machine, ArrayCellId id)
{
  ArrayCell * p = (ArrayCell *) machine->arrays;
  while (p != NULL)
    {
      if (p->id = id)
	{
	  break;
	}
      
      p = p->next;
    }
  
  return p;
}

static ArrayCell * us_priv_add_array_cell (struct uc * machine, ArrayCell * p)
{
  assert (NULL != p);
  
  {
    ArrayCell * last = us_priv_get_last_array_cell (machine->arrays);
    assert (NULL != last);
    last->next = p;
  }
  
  return p;
}

static platter_t uc_priv_read_platter_from (address_t a)
{
  // TODO validate address
  platter_t p = 0;
  return p;
}

static int uc_priv_initialize_machine (struct uc * machine)
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

static int uc_priv_initialize_program_array_with (struct uc * machine
						  , byte * data
						  , size_t size)
{
  // check if already exists
  ArrayCell * cell = uc_priv_search_for_cellid (UC_PROGRAM_ARRAY_ID);
  if (NULL != cell)
    {
      fail (machine);
    }
  
  if (0 != (size % sizeof(platter_t)))
    {
      fail (machine);
    }
  
  cell = uc_priv_new_array_cell (size);
  
  // should be the first allocation
  if (NULL == cell || cell->id != UC_PROGRAM_ARRAY_ID)
    {
      fail (machine);
    }
  
  machine->arrays = cell;
}

static byte decode_register_value_from_platter (platter_t p, Register r)
{
  static const byte REG_MASK = 0x3U;
  const byte offset   = r * REG_MASK;
    
  assert (r <= 2 & r >= 0);
    
  return (p & (REG_MASK << offset)) >> offset;
}

static void fail (struct uc * machine)
{
  printf ("fail: invalid operation\n");
  exit (1);
}

#include <setjmp.h>
static jmp_buf jmp_env;

static int uc_priv_do_spin (struct uc * machine)
{
#define VALIDATE_OPCODE(opcode) if (opcode >= (sizeof(g_operators) / sizeof(g_operators[0]))) fail (machine)
  
  while (1)
    {
      if ( ! setjmp (jmp_env))
	{
	  platter_t op = uc_priv_read_platter_from (machine->ip);
	  
	  VALIDATE_OPCODE(op);
	  
	  assert (g_operators [OPCODE_FROM_PLATTER (op)].code == op);
	  
	  g_operators [OPCODE_FROM_PLATTER (op)].handler (machine
							  , op
							  , decode_register_value_from_platter (op, REGISTER_A)
							  , decode_register_value_from_platter (op, REGISTER_B)
							  , decode_register_value_from_platter (op, REGISTER_C)
							  );
	  
	  machine->ip += sizeof(platter_t);
	}
      else
	{
	  printf ("Processor halted\n");
	}
    }
  
#undef VALIDATE_OPCODE

  return EOK;
}


//////////////////////////////////////////////////
// operator handler definitions
//////////////////////////////////////////////////

#define VALIDATE_OFFSET(offset,array) if ((offset) >= (array)->datasize) fail(machine)
#define VALIDATE_REGISTER_INDEX(r) if ((r) >= UC_REGISTER_COUNT) fail(machine)
#define VALIDATE_REGISTERS()         VALIDATE_REGISTER_INDEX(rega); VALIDATE_REGISTER_INDEX(regb); VALIDATE_REGISTER_INDEX(regc)
    

static int uc_priv_handler_cond_mov (struct uc * machine
                                     , platter_t p
                                     , byte rega
                                     , byte regb
                                     , byte regc
                                     )
{
  VALIDATE_REGISTERS ();
    
  {
    if (0 != machine->registers[regc])
      {
	machine->registers[rega] = machine->registers[regb];
      }
  }
    
  return EOK;
}

static int uc_priv_handler_array_idx (struct uc * machine
                                      , platter_t p
                                      , byte rega
                                      , byte regb
                                      , byte regc
                                      )
{
  VALIDATE_REGISTERS ();
  
  {
    platter_t array_idx = machine->registers[regb];
    platter_t array_offset = machine->registers[regc];
    
    ArrayCell * cell = us_priv_search_for_cell_id (machine, array_idx);
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

static int uc_priv_handler_array_amend (struct uc * machine
                                        , platter_t p
                                        , byte rega
                                        , byte regb
                                        , byte regc
                                        )
{
  VALIDATE_REGISTERS ();
    
  {
    platter_t array_idx = machine->registers[rega];
    platter_t array_offset = machine->registers[regb];
        
    ArrayCell * cell = us_priv_search_for_cell_id (machine, array_idx);
    if (NULL == cell)
      {
	fail (machine);
      }
    
    VALIDATE_OFFSET(array_offset,cell);
    
    cell->data[array_offset] = machine->registers[regc];
  }
    
  return EOK;
}

static int uc_priv_handler_addition (struct uc * machine
                                     , platter_t p
                                     , byte rega
                                     , byte regb
                                     , byte regc
                                     )
{
  VALIDATE_REGISTERS ();
  
  // check for overflow in intermediate computation
  machine->registers[rega] = (machine->registers[regb] + machine->registers[regc]) % (1 << 32);
  
  return EOK;
}

static int uc_priv_handler_multiplication (struct uc * machine
                                           , platter_t p
                                           , byte rega
                                           , byte regb
                                           , byte regc
                                           )
{
  VALIDATE_REGISTERS ();
    
  // check for overflow in intermediate computation
  machine->registers[rega] = (machine->registers[regb] * machine->registers[regc]) % (1 << 32);
    
  return EOK;
}

static int uc_priv_handler_division (struct uc * machine
                                     , platter_t p
                                     , byte rega
                                     , byte regb
                                     , byte regc
                                     )
{
  VALIDATE_REGISTERS ();
  
  // check for overflow in intermediate computation
  if (0 == machine->registers[regc])
    {
      fail (machine);
    }
  
  machine->registers[rega] = (machine->registers[regb] / machine->registers[regc]);
  
  return EOK;
}

static int uc_priv_handler_not_and (struct uc * machine
                                    , platter_t p
                                    , byte rega
                                    , byte regb
                                    , byte regc
                                    )
{
  VALIDATE_REGISTERS ();
  
  // check for overflow in intermediate computation
  machine->registers[rega] = ~machine->registers[regb] ^ ~machine->registers[regc];
  
  return EOK;
}

static int uc_priv_handler_allocation (struct uc * machine
				       , platter_t p
				       , byte rega
				       , byte regb
				       , byte regc
				       )
{
  VALIDATE_REGISTERS ();
    
  {
    ArrayCell * cell = us_priv_new_array_cell (machine->registers[regc]);
    memset (cell->data, 0, cell->datasize * sizeof(platter_t));
    machine->registers[rega] = (platter_t) cell;
  }
    
  return EOK;
}

static int uc_priv_handler_abandonment (struct uc * machine
					, platter_t p
					, byte rega
					, byte regb
					, byte regc
					)
{
  VALIDATE_REGISTERS ();
  
  if (machine->registers[regc] == UC_PROGRAM_ARRAY_ID)
    {
      fail (machine);
    }
  
  {
    ArrayCellId
      id = machine->registers[regc];
    
    ArrayCell *
      cell = us_priv_search_for_cell_id (machine, id);
    if (NULL == cell)
      {
	fail (machine);
      }
  }
  
  return EOK;
}

static int uc_priv_handler_output (struct uc * machine
				    , platter_t p
				    , byte rega
				    , byte regb
				    , byte regc
				    )
{
  VALIDATE_REGISTERS ();
  
  {
    if (machine->registers[regc] > 255)
      {
	fail (machine);
      }
    
    printf ("%c", machine->registers[regc]);
  }
  
  return EOK;
}

static int uc_priv_handler_input (struct uc * machine
				    , platter_t p
				    , byte rega
				    , byte regb
				    , byte regc
				    )
{
  VALIDATE_REGISTERS ();
  
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

static int uc_priv_handler_load_program (struct uc * machine
					 , platter_t p
					 , byte rega
					 , byte regb
					 , byte regc
					 )
{
  VALIDATE_REGISTERS ();
  
  {
    ArrayCell *
      cell = us_priv_search_for_cell_id (machine, machine->registers[regb]);
    
    if (NULL == cell)
      {
	fail (machine);
      }
    
    ArrayCell *
      newcell = us_priv_new_array_cell_from_cell (cell);
    
    {
      ArrayCell * zeroc = us_priv_search_for_cell_id (machine, 0);
      if (NULL != zeroc)
	{
	  us_priv_delete_array (zeroc);
	}
    }
    {
      platter_t offset = machine->registers[regc];
      machine->ip = offset;
    }
  }
  
  return EOK;
}

static int uc_priv_handler_halt (struct uc * machine
				 , platter_t p
				 , byte rega
				 , byte regb
				 , byte regc
				 )
{
  longjmp (jmp_env, 1);
  
  return EOK;
}

static int uc_priv_handler_orthography (struct uc * machine
					, platter_t p
					, byte rega
					, byte regb
					, byte regc
					)
{
  platter_t value = p & 0x3FFFFFF;
  
  // correct register a
  rega = (p >> 26) & 0x03;
  
  machine->registers[rega] = value;
  
  return EOK;
}


#undef VALIDATE_REGISTERS
#undef VALIDATE_REGISTER_INDEX
#undef VALIDATE_OFFSET


//////////////////////////////////////
// public functions
//////////////////////////////////////

int uc_run (struct uc * machine, byte * codex, size_t codex_size)
{
  uc_priv_initialize_machine (machine);
  uc_pric_initialize_program_array_with (codex, codex_size);
  
  return uc_priv_do_spin (machine);
}

