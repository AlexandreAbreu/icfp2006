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
typedef enum UC_CONSTANTS
{
    UC_REGISTER_COUNT      = 8,
    UC_PROGRAM_ARRAY_ID    = 0,
    
} UC_CONSTANTS;


/**
 * 
 * 
 */
typedef struct uc
{
    // registers
    platter_t registers[UC_REGISTER_COUNT];

    // basic runtime structures
    address_t ip;
    
    void * arrays;
  
} uc;


/**
 *
 *
 */
int uc_run (struct uc *, byte *, size_t);


#endif // UC_H

