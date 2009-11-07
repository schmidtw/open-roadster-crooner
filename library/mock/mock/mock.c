
#include <stdlib.h>
#include "mock.h"

#include "CUnit/Basic.h"

void internal_init( mock_obj_t *obj );

void internal_init( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return;
    }
    if( 0 == obj->is_initialized ) {
        obj->init_fct = NULL;
        obj->do_stuff_fct = NULL;
        obj->is_expecting_call = true;
        obj->is_initialized = 1;
        obj->call_std_fct = true;
        obj->rv = 0;
    }
}

/**
 * External Functions
 */

void mock_reset( mock_obj_t *obj, size_t num )
{
    int ii;
    initializer fct;
    mock_obj_t *cur_obj;
    for( ii = 0; ii < num; ii++ ) {
        cur_obj = &obj[ii];
        if( NULL == cur_obj ) {
            return;
        }
        fct = cur_obj->init_fct;
        cur_obj->is_initialized = 0;
        set_initialize_function( cur_obj, fct );
    }
}

void mock_test_assert( bool check )
{
    CU_ASSERT( check );
}

void set_use_standard_fct( mock_obj_t *obj, bool val )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->call_std_fct = val;
}

void set_initialize_function( mock_obj_t *obj, initializer initial_fct )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->init_fct = initial_fct;
}

void set_do_stuff_function( mock_obj_t *obj, int32_t *do_fct )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->do_stuff_fct = do_fct;
}

void set_is_expecting( mock_obj_t *obj, bool expecting )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->is_expecting_call = expecting;
}

bool get_is_expecting( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return false;
    }
    internal_init( obj );
    return obj->is_expecting_call;
}

void set_return_pvoid( mock_obj_t *obj, void *rv)
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv = ((int32_t) rv);
}

void set_return_int16_t( mock_obj_t *obj, int16_t rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv = ((int32_t) rv);
}

void set_return_uint16_t( mock_obj_t *obj, uint16_t rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv = ((int32_t) rv);
}

void set_return_int32_t( mock_obj_t *obj, int32_t rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv = rv;
}

void set_return_uint32_t( mock_obj_t *obj, uint32_t rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv = ((int32_t) rv);
}

void set_return_bool( mock_obj_t *obj, bool rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv  = ((int32_t) rv);
}

void set_return_size_t( mock_obj_t *obj, size_t rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv = ((int32_t) rv);
}

void set_return_pchar( mock_obj_t *obj, char *rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->rv = ((int32_t) rv);
}

void *get_return_pvoid( mock_obj_t *obj)
{
    if( NULL == obj ) {
        return 0;
    }
    internal_init( obj );
    return ((void *) obj->rv);
}

int16_t get_return_int16_t( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return 0;
    }
    internal_init( obj );
    return ((int16_t) obj->rv);
}

uint16_t get_return_uint16_t( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return 0;
    }
    internal_init( obj );
    return ((uint16_t) obj->rv);
}

int32_t get_return_int32_t( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return 0;
    }
    internal_init( obj );
    return ((int32_t) obj->rv);
}

uint32_t get_return_uint32_t( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return 0;
    }
    internal_init( obj );
    return ((uint32_t) obj->rv);
}

bool get_return_bool( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return false;
    }
    internal_init( obj );
    return ((bool) obj->rv);
}

size_t get_return_size_t( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return 0;
    }
    internal_init( obj );
    return ((size_t) obj->rv);
}

char *get_return_pchar( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return NULL;
    }
    internal_init( obj );
    return ((char *) obj->rv);
}
