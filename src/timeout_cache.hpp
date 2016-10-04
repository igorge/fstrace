//================================================================================================================================================
// FILE: timeout_cache.h
// (c) GIE 2016-10-04  20:13
//
//================================================================================================================================================
#ifndef H_GUARD_TIMEOUT_CACHE_2016_10_04_20_13
#define H_GUARD_TIMEOUT_CACHE_2016_10_04_20_13
//================================================================================================================================================
#pragma once
//================================================================================================================================================
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>
#include <chrono>
//================================================================================================================================================
namespace gie {

    template <class T, class KeyExtractor>
    struct timeout_cache_t{

        typedef std::chrono::steady_clock::time_point time_point_t;

        struct cache_value_t{
            T data;
            time_point_t key2;
        };


        struct key_extractor_t{
            typedef typename KeyExtractor::result_type result_type;
            result_type operator()(cache_value_t const& v) const { return m_key_extractor(v.data); }

        private:
            KeyExtractor const m_key_extractor;
        };

        typedef boost::multi_index_container<
            cache_value_t,
            boost::multi_index::indexed_by<
                boost::multi_index::ordered_unique<key_extractor_t>,
                boost::multi_index::ordered_non_unique<boost::multi_index::member<cache_value_t, time_point_t, &cache_value_t::key2> >
            >
        > index_t;


        template <class U>
        bool insert(U&& new_value){
            cache_value_t v={std::forward<U>(new_value), std::chrono::steady_clock::now()};
            auto const insert_r = m_index.template get<0>().insert(v).second;
        }

    private:
        index_t m_index;

    };

}
//================================================================================================================================================
#endif
//================================================================================================================================================
