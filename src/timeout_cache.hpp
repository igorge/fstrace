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
#include "gie/exceptions.hpp"
#include "gie/debug.hpp"

#include <boost/optional.hpp>
#include <boost/multi_index_container.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/member.hpp>

#include <iostream>
#include <boost/chrono.hpp>
#include <boost/range/iterator_range.hpp>
//================================================================================================================================================
namespace gie {

    template <class T, class KeyExtractor>
    struct timeout_cache_t{

        typedef typename KeyExtractor::result_type key_type;

        typedef boost::chrono::steady_clock::time_point time_point_t;

        struct cache_value_t{
            T data;
            time_point_t key2;
        };


        struct key_extractor_t{
            typedef key_type result_type;
            result_type operator()(cache_value_t const& v) const { return m_key_extractor(v.data); }
            result_type& operator()(cache_value_t& v) const { return m_key_extractor(v.data); }

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
        T const& insert(U&& new_value){
            cache_value_t v={std::forward<U>(new_value), boost::chrono::steady_clock::now()};
            auto const insert_r = m_index.template get<0>().insert(v);

            GIE_CHECK(insert_r.second);

            return insert_r.first->data;
        }

        boost::optional<T const&> get(key_type const& key){
            auto const& index = m_index.template get<0>();
            auto r = index.find(key);

            if(r==index.end()){
                return boost::none;
            } else {
                auto modify_r = m_index.template get<0>().modify(r, [](cache_value_t& v){v.key2=boost::chrono::steady_clock::now();});
                GIE_CHECK(modify_r);
                return r->data;
            }
        }

        template <class ValueGenerator, class Mutator>
        T const& get_insert_modify(key_type const& key, ValueGenerator const& vg, Mutator const& mut){
            auto const& index = m_index.template get<0>();
            auto r = index.find(key);

            if(r==index.end()){
                return insert(vg());
            } else {
                auto modify_r = m_index.template get<0>().modify(r, [&](cache_value_t& v){
                    mut(v.data);
                    v.key2=boost::chrono::steady_clock::now();
                });
                GIE_CHECK(modify_r);
                return r->data;
            }
        }

        void gc(boost::chrono::seconds const& ttl){
            auto& index = m_index.template get<1>();

            auto const& now = boost::chrono::steady_clock::now();
            auto const& dead_line = now - ttl;;

            auto const& r = index.lower_bound(dead_line);

//            for( auto&& v: boost::make_iterator_range(index.begin(), r) ){
//                GIE_DEBUG_LOG("REMOVING : "<< boost::chrono::duration_cast<boost::chrono::seconds> (now - v.key2 ));
//            }

            index.erase(index.begin(), r);
        }

    private:
        index_t m_index;

    };

}
//================================================================================================================================================
#endif
//================================================================================================================================================
