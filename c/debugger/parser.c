#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "parser.h"


#define EOK 0


#define gc_malloc malloc
#define gc_free free

typedef enum operator_symbol_t
  {
    OPERATOR_NONE = -1,
    OPERATOR_EQUAL,
    OPERATOR_IS_GREATER_THAN,
    OPERATOR_INC,
 
  } operator_symbol_t;

typedef struct operator_t
{
  operator_symbol_t value;
  
  // original string representation
  char * repr;
  
} operator_t;

static operator_t g_operators [] = {
  { .value = OPERATOR_EQUAL, .repr = "=" }
  , { .value = OPERATOR_IS_GREATER_THAN, .repr = ">" }
  , { .value = OPERATOR_INC, .repr = "++" }
};


typedef enum token_type_t
  {
    NONE = -1,
    SYMBOL,
    IMMEDIATE,
    OPERATOR,
    
  } token_type_t;

typedef union token_value_t
{
  unsigned int numeric;
  char * symbol;
  operator_symbol_t op;
  
} token_value_t;

typedef struct token_t
{
  token_type_t type;
  
  const char * repr_start;
  const char * repr_end;
  
  token_value_t value;
  
} token_t;


const char * eatwhitespace (const char * const s)
{
  if (NULL == s || 0 == strlen (s))
    {
      return s;
    }
  
  {
    const char * r = s;
    size_t i = 0;
    
    while (i < strlen(s) && isspace (s[i])) { ++r; ++i; }
    
    return r;
  }
}

 
/**
 *
 * @return index of operator in table or -1 if not found
 */
static int find_operator_by_repr (const char * const r
				  , const operator_t * operators
				  , size_t operatorcnt)
{
  size_t i = 0;
  
  for (i = 0
	 ; i < operatorcnt
	 ; ++i)
    {
      assert (i < operatorcnt);
      
      if (0 == strncmp (operators[i].repr
			, r
			, strlen(operators[i].repr))
	  )
        {
	  break;
        }
    }
  
  return i == operatorcnt ? -1 : i;
}

const char * next (const char * const s)
{
  const char * current = s;
  if (NULL == current)
    {
      return NULL;
    }
  
  current = eatwhitespace (current);
  
  if (NULL == current || *current == '\0')
    {
      return current;
    }
  
  while ( ! isspace (*current)) ++current;
  
  return current;
}

const char peek (const char * const s)
{
  assert (NULL != s);
  if (NULL == s)
    {
      // should abort! & kill all
      return -1;
    }
  
  return 'a';
}

#define VALIDATE_INPUT(s)			\
  if (NULL == (s))				\
    {						\
      return t;					\
    }						\
  if (0 == strlen ((s)))			\
    {						\
      return t;					\
    }

token_t next_token_expect_symbol (const char * s)
{
  token_t t = { .type = NONE };
  
  VALIDATE_INPUT (s);
  
  s = eatwhitespace (s);
  
  t.repr_start = s;
  
  // search for end of input
  while ('\0' != *s
	 && ! isspace (*s)
	 && isalnum (*s)
	 )
    {
      ++s;
    }
  
  if (s == t.repr_start)
    {
      return t;
    }
  
  t.repr_end = s;
  
  assert (t.repr_end > t.repr_start);
  
  {
    const size_t SYMBOL_SIZE =  t.repr_end - t.repr_start;
    
    t.value.symbol = gc_malloc (SYMBOL_SIZE+1);
    
    if (t.value.symbol == NULL)
      {
	return t;
      }
    
    memset (t.value.symbol, 0, SYMBOL_SIZE+1);
    
    memcpy (t.value.symbol, t.repr_start, SYMBOL_SIZE);
  }
  
  t.type = SYMBOL;
  
  return t;
}

token_t next_token_expect_operator (const char * s)
{
  token_t t = { .type = NONE };
  
  size_t inputsize = 0;
  
  VALIDATE_INPUT (s);
  
  s = eatwhitespace (s);
  
  t.repr_start = s;
  
  // search for end of input
  while ('\0' != *s
	 && ! isalnum (*s)
	 && ! isspace (*s)
	 )
    {
      ++inputsize;
      ++s;
    }
  
  if (s == t.repr_start)
    {
      return t;
    }
  
  // C99 VLA
  {
    char symbolrepr [inputsize + 1];
    int idx = -1;
    
    strncpy (symbolrepr, t.repr_start, inputsize);
    symbolrepr[inputsize] = '\0';
    
    idx = find_operator_by_repr (symbolrepr
				 , g_operators
				 , sizeof(g_operators) / sizeof(g_operators[0])
				 );
    if (-1 == idx)
      {
	// oops
	return t;
      }
    
    t.repr_end = s;
    
    t.value.op = g_operators[idx].value;
    
    t.type = OPERATOR;
  }
  
  return t;
}

token_t
next_token_expect_immediate_value (const char * s)
{
  token_t t = { .type = NONE, .repr_start = s };
  
  assert (NULL != s);
  
  VALIDATE_INPUT (s);
  
  s = eatwhitespace (s);
  
  assert (NULL != s);
  if (NULL == s)
    {
      // should abort
      return t;
    }
  
  {
    size_t i = 0;
    
    t.repr_start = s;
    
    while (*s != '\0'
	   && isdigit (*s)
	   )
      {
	++s;
      }
    
    t.repr_end = s;
    
    if (s != t.repr_start)
      {
	const size_t VALUE_SIZE = t.repr_end - t.repr_start;
	char v[VALUE_SIZE + 1];
	
	assert (s > t.repr_start);
	
	memcpy (&v[0], t.repr_start, VALUE_SIZE);
	v[VALUE_SIZE] = '\0';
	
	t.type = IMMEDIATE;
	t.value.numeric = strtoul (&v[0], NULL, 0);
      }
  }
  
  return t;
}

token_t next_token (const char * s)
{
  token_t t = { .type = NONE };
  if (NULL == s)
    {
      return t;
    }
  
  s = eatwhitespace (s);
  
  char next_char = *s;
  if (isdigit (next_char))
    {
      t = next_token_expect_immediate_value (s);
      
      if (t.type != IMMEDIATE)
	{
	  // fail!
	  return t;
	}
      
      return t;
    }
  else if (isalpha (next_char))
    {
      t = next_token_expect_symbol (s);
      
      if (t.type != SYMBOL)
	{
	  // fail!
	  return t;
	}
      
      return t;
    }
  
  return next_token_expect_operator (s);
}


typedef struct Node
{
  token_type_t type;
  
  token_value_t value;
  
  struct Node * left;
  struct Node * right;
  
} Node;


void parse_error (const char * s)
{
  printf ("Parse error: %s\n", s);
  //  exit (0);
}


Node * parse_symbol (const char ** s)
{
  token_t t = next_token (*s);
  if (t.type != SYMBOL)
    {
      parse_error (*s);
      return NULL;
    }
  
  assert (t.repr_start < t.repr_end);
  
  *s = t.repr_end;
  
  {
    const size_t SYMBOL_SIZE = t.repr_end - t.repr_start;

    Node * node = gc_malloc (sizeof(Node));
    if (NULL == node) { return NULL; }
    
    node->type = SYMBOL;
    
    node->value.symbol = t.value.symbol;
    
    return node;
  }
  
  assert (0 && "not reachable");
  return NULL;
}

Node * parse_immediate (const char ** s)
{
  token_t t = next_token (*s);
  if (t.type != IMMEDIATE)
    {
      parse_error (*s);
      return NULL;
    }
  
  assert (t.repr_start < t.repr_end);
    
  *s = t.repr_end;
  
  {
    Node * node = gc_malloc (sizeof(Node));
    if (NULL == node) { return NULL; }
        
    node->type = IMMEDIATE;
    node->value.numeric = t.value.numeric;
    
    return node;
  }
    
  assert (0 && "not reachable");
  return NULL;
}

Node * parse_operator (const char ** s)
{
  token_t t = next_token (*s);
  if (t.type != OPERATOR)
    {
      parse_error (*s);
      return NULL;
    }
    
  assert (t.repr_start < t.repr_end);
  
  *s = t.repr_end;
  
  {
    const size_t SYMBOL_SIZE = t.repr_end - t.repr_start;
    
    Node * node = gc_malloc (sizeof(Node));
    if (NULL == node) { return NULL; }
        
    node->type = OPERATOR;
    node->value.op = t.value.op;
    
    return node;
  }
  
  assert (0 && "not reachable");
  return NULL;
}

Node * parse_expression (const char ** s)
{
  Node * op = NULL;
  Node * immediate = NULL;
  Node * symbol = parse_symbol (&s);
  
  if (NULL == symbol)
    {
      return NULL;
    }
  
  op = parse_operator (&s);
  if (NULL == op)
    {
      return NULL;
    }
  
  immediate = parse_immediate (&s);
  if (NULL == immediate)
    {
      return NULL;
    }
  
  op->left = symbol;
  op->right = immediate;
  
  return op;
}

Node * parse (const char * s)
{
  return parse_expression (s);
}

void post_order_traverse (Node * node, void (*do_me) (Node *))
{
  if (NULL == node)
    {
      return;
    }
  
  if (node->left) post_order_traverse (node->left, do_me);
  if (node->right) post_order_traverse (node->right, do_me);
  
  do_me (node);
}

// TBD my schorr-waite version
void post_order_traverse_schorr_waite (Node * node, void (*do_me) (Node *))
{
  if (NULL == node)
    {
      return;
    }
  
  if (node->left) post_order_traverse (node->left, do_me);
  if (node->right) post_order_traverse (node->right, do_me);
  
  do_me (node);
}


// small stack based VM
// 32bits stack

typedef enum OpCode
  {
    DONE = -1,
    ISEQUAL,
    IS_GREATER_THAN,
    INC,
    PUSH_IMMEDIATE_VALUE,
    PUSH_SYMBOL_VALUE,
    
  } OpCode;

typedef unsigned long long opcode_t;
typedef unsigned long long stack_element_t;

typedef struct VM
{
  enum
    {
      STACK_SIZE = 1024,
      CODE_SIZE = 1024,
    };
  
  stack_element_t stack[STACK_SIZE];
  unsigned short sp;
  
  opcode_t opcodes[CODE_SIZE];
  unsigned short ip;

} VM;


int vm_push_stack (VM * vm, stack_element_t e)
{
  assert (NULL != vm);
  
  if (vm->sp >= STACK_SIZE)
    {
      // overflow
      return 1;
    }
  
  vm->stack[vm->sp++] = e;
  
  return EOK;
}

stack_element_t vm_pop_stack (VM * vm)
{
  assert (NULL != vm);
  
  if (vm->sp == 0)
    {
      // mmmh better way to report underflow
      return (stack_element_t) -1;
    }
    
  return vm->stack[--vm->sp];
}

unsigned int
vm_execute (VM * vm, environment_t env)
{
  if (NULL == vm)
    {
      // need a proper way to report
      return;
    }
  
  vm->ip = 0;
  
  for (;;)
    {
      switch (vm->opcodes[vm->ip++])
        {
        case OPERATOR_EQUAL:
	  {
	    stack_element_t s1 = vm_pop_stack(vm);
	    stack_element_t s2 = vm_pop_stack(vm);
	    
	    vm_push_stack (vm, s1 == s2);
	  }
	  break;
	  
	case OPERATOR_IS_GREATER_THAN:
	  {
	    stack_element_t top = vm_pop_stack(vm);
	    stack_element_t bottom = vm_pop_stack(vm);
	    
	    vm_push_stack (vm, bottom > top);
	  }
	  break;
	  
        case PUSH_IMMEDIATE_VALUE:
	  vm_push_stack (vm, vm->opcodes[vm->ip++]);
	  break;
	  
        case PUSH_SYMBOL_VALUE:
	  vm_push_stack (vm, env.get_symbol_value (vm->opcodes[vm->ip++]));
	  break;
	  
	default:
	  goto done;
	  break;
        }
    }
  
 done:
  return vm_pop_stack (vm);
}

void generate_opcodes (Node * node, VM * vm)
{
  void build_stack_from_node (Node * node)
  {
#define VALIDATE_VM_CODE_ARRAY(vm)
    
    if (NULL == node)
      {
	return;
      }
    
    // pretty lamely use the ip as a "sp"
    
    switch (node->type)
      {
      case IMMEDIATE:
	
	vm->opcodes[vm->ip++] = PUSH_IMMEDIATE_VALUE;
	vm->opcodes[vm->ip++] = node->value.numeric;
	break;
        
      case SYMBOL:
        
	vm->opcodes[vm->ip++] = PUSH_SYMBOL_VALUE;
	vm->opcodes[vm->ip++] = node->value.symbol;
	break;
	
      case OPERATOR:
        
	vm->opcodes[vm->ip++] = node->value.op;
	break;
      }
  }
  
  vm->ip = 0;
  vm->opcodes[vm->ip] = DONE;
  
  post_order_traverse (node, build_stack_from_node);
  
  // pretty hacky
  vm->opcodes[vm->ip++] = DONE;
}



////////////////////////////////////////
//
//     public
// 
//////////////////////////////////////// 


int execute_command (const char * const command
		     , environment_t env)
{
  VM vm = {0};
  generate_opcodes (parse (command), &vm);
  return vm_execute (&vm, env);
}


// #define WITH_MAIN

#if defined (WITH_MAIN)

#include <stdio.h>

int main ()
{
  token_t t;
  const char * s = "";
  
  VM vm;
  
  printf ("0 - testing '%s'\n", s);
  generate_opcodes (parse (""), &vm);
  printf ("0 - %d\n", vm_execute (&vm));
  
  s = "d";
  printf ("1 - testing '%s'\n", s);
  generate_opcodes (parse (s), &vm);
  printf ("1 - %d\n", vm_execute (&vm));
  
  s = "IP = 12";
  printf ("2 - testing '%s'\n", s);
  generate_opcodes (parse (s), &vm);
  printf ("2 - %d\n", vm_execute (&vm));
  printf ("IP = 0x%08X\n", vm.ip);
  
  s = "IP > 0";
  printf ("2.5 - testing '%s'\n", s);
  generate_opcodes (parse (s), &vm);
  printf ("2.5 - %d\n", vm_execute (&vm));
  printf ("IP = 0x%08X\n", vm.ip);
  
  s = " 2 33 4 =";
  printf ("3 - testing '%s'\n", s);
  generate_opcodes (parse (s), &vm);
  printf ("3 - %d\n", vm_execute (&vm));
  
  return 0;
}

#endif

