
#ifndef __MOCK_H__
#define __MOCK_H__

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdarg.h>

/* We must do a declaration of struct mock_obj_struct to avoid
 * compiler warnings
 */
struct mock_obj_struct;

typedef void (*initializer)( struct mock_obj_struct *obj );

typedef struct mock_obj_struct {
    int32_t is_initialized;
    bool is_expecting_call;
    bool call_std_fct;     // when true, if there is a std function (like memcpy) this function will be called and the return will be used
    int32_t rv;
    int32_t *do_stuff_fct; // Will be the pointer to a specified function
    initializer init_fct;
} mock_obj_t;

void mock_test_assert( bool check );

/* Will re-initialize the object.  If there is an initializer this will
 * not be cleared.  The specified initializer will be called after the
 * obj is reset.
 * 
 * @param obj pointer to the first mock_obj_t in the structure
 * @param num number of object in the structure to reset.  Not the
 *            the total size of the object
 */
void mock_reset( mock_obj_t *obj, size_t num );

void set_use_standard_fct( mock_obj_t *obj, bool val );

void set_initialize_function( mock_obj_t *obj, initializer initial_fct );
void set_do_stuff_function( mock_obj_t *obj, int32_t *do_fct );

void set_is_expecting( mock_obj_t *obj, bool expecting );
bool get_is_expecting( mock_obj_t *obj );

void set_return_pvoid( mock_obj_t *obj, void *rv);
void set_return_int16_t( mock_obj_t *obj, int16_t rv );
void set_return_uint16_t( mock_obj_t *obj, uint16_t rv );
void set_return_int32_t( mock_obj_t *obj, int32_t rv );
void set_return_uint32_t( mock_obj_t *obj, uint32_t rv );
void set_return_bool( mock_obj_t *obj, bool rv );
void set_return_size_t( mock_obj_t *obj, size_t rv );
void set_return_pchar( mock_obj_t *obj, char *rv );

void *get_return_pvoid( mock_obj_t *obj);
int16_t get_return_int16_t( mock_obj_t *obj );
uint16_t get_return_uint16_t( mock_obj_t *obj );
int32_t get_return_int32_t( mock_obj_t *obj );
uint32_t get_return_uint32_t( mock_obj_t *obj );
bool get_return_bool( mock_obj_t *obj );
size_t get_return_size_t( mock_obj_t *obj );
char *get_return_pchar( mock_obj_t *obj );


#endif /* __MOCK_H__ */
