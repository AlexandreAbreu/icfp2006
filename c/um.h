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
int um_run (struct um_t *, byte *, size_t);


#endif // UC_H

