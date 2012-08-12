
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

void set_use_standard_fct( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );

    obj->do_stuff_fct = NULL;
    obj->call_std_fct = true;
}

void set_initialize_function( mock_obj_t *obj, initializer initial_fct )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->init_fct = initial_fct;
}

void set_do_stuff_function( mock_obj_t *obj, void *do_fct )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->do_stuff_fct = do_fct;
    obj->call_std_fct = false;
}

void set_is_expecting( mock_obj_t *obj, const bool expecting )
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

void set_return_value( mock_obj_t *obj, const uint64_t rv )
{
    if( NULL == obj ) {
        return;
    }
    internal_init( obj );
    obj->do_stuff_fct = NULL;
    obj->call_std_fct = false;
    obj->rv = (uint64_t) rv;
}

uint64_t get_return_value( mock_obj_t *obj )
{
    if( NULL == obj ) {
        return 0;
    }
    internal_init( obj );
    return obj->rv;
}
