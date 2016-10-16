//================================================================================================================================================
// FILE: allocator.h
// (c) GIE 2016-10-16  23:12
//
//================================================================================================================================================
#ifndef H_GUARD_ALLOCATOR_2016_10_16_23_12
#define H_GUARD_ALLOCATOR_2016_10_16_23_12
//================================================================================================================================================
#pragma once
//================================================================================================================================================
#include "gie/simple_caching_allocator_mt_wrapper.hpp"
#include "gie/simple_caching_allocator.hpp"
#include "gie/simple_to_std_allocator.hpp"

#include <boost/shared_ptr.hpp>
//================================================================================================================================================
namespace gie {

    typedef gie::simple_mt_allocator_t<gie::simple_caching_allocator> caching_simple_allocator_t;
    typedef gie::simple_to_std_allocator_t<char, caching_simple_allocator_t> std_caching_allocator_t;

    typedef std::vector<char, std_caching_allocator_t> buffer_t;
    typedef boost::shared_ptr<buffer_t> shared_buffer_t;
    typedef std::function<shared_buffer_t()> buffer_allocator_t;

    inline
    shared_buffer_t allocate_buffer(std_caching_allocator_t const& alloc){
        buffer_t * const buffer = alloc.alloc_<buffer_t>(alloc);

        shared_buffer_t tmp{ buffer, [alloc](buffer_t * v){  alloc.dealloc_(v);}, alloc};

        return tmp;
    }

    inline
    shared_buffer_t allocate_buffer(caching_simple_allocator_t& simple_alloc){

        std_caching_allocator_t alloc{simple_alloc};

        return allocate_buffer(alloc);
    }

}
//================================================================================================================================================
#endif
//================================================================================================================================================
