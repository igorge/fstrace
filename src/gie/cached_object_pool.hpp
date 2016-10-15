//================================================================================================================================================
// FILE: cached_object_pool.h
// (c) GIE 2016-10-15  04:58
//
//================================================================================================================================================
#ifndef H_GUARD_CACHED_OBJECT_POOL_2016_10_15_04_58
#define H_GUARD_CACHED_OBJECT_POOL_2016_10_15_04_58
//================================================================================================================================================
#pragma once
//================================================================================================================================================
#include "gie/simple_caching_allocator.hpp"
#include "gie/simple_caching_allocator_mt_wrapper.hpp"
#include "gie/simple_to_std_allocator.hpp"
#include "gie/exceptions.hpp"

#include <boost/shared_ptr.hpp>
#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>

#include <queue>
//================================================================================================================================================
namespace gie {


    template <class T, class MutexT = boost::mutex >
    struct cached_pool_t {

        typedef boost::shared_ptr<T> shared_ptr_t;

        template <class ...Args>
        shared_ptr_t alloc(Args&&...  args){
            typename MutexT::scoped_lock lock(m_mutex);

            if (m_cache_objects_queue_.empty()){
                //GIE_DEBUG_LOG("POOL EMPTY: size: "<<m_cache_objects_queue_.size() << ", ALIVE: "<< m_alive_objects_count);

                ++m_alive_objects_count;
                lock.unlock();

                shared_ptr_t tmp{alloc_<T>(std::forward<Args>(args)...), [this](T *const v) { this->deleter_(v); }, m_std_caching_allocator};
                return tmp;
            } else {
                //GIE_DEBUG_LOG("POOL: size: "<<m_cache_objects_queue_.size() << ", ALIVE: "<< m_alive_objects_count);
                T* const cached_v = m_cache_objects_queue_.back();
                m_cache_objects_queue_.resize( m_cache_objects_queue_.size() - 1);
                ++m_alive_objects_count;

                shared_ptr_t tmp{cached_v, [this](T *const v) { this->deleter_(v); }, m_std_caching_allocator};
                return tmp;
            }
        }


        template <class T1>
        explicit cached_pool_t(T1&& resetter): m_resetter( std::forward<T1>(resetter) ){}


        ~cached_pool_t(){
            assert(m_alive_objects_count==0);

            for(auto &&v: m_cache_objects_queue_){
                dealloc_(v);
            }
        }

    private:

        template <class TT, class... Args>
        TT* alloc_(Args&& ...args){
            return m_std_caching_allocator.alloc_<TT>(std::forward<Args>(args)...);
        }

        template <class TT>
        void dealloc_(TT* const p){
            m_std_caching_allocator.dealloc_(p);
        }

        void clear_(T* const v){
            assert(v);
            m_resetter(*v);
        }

        void deleter_(T* const v)noexcept {
            assert(v);

            try {
                typename MutexT::scoped_lock lock(m_mutex);
                --m_alive_objects_count;

                try {
                    clear_(v);
                    m_cache_objects_queue_.push_back(v);
                }catch (...){
                    dealloc_(v);
                    throw;
                }

            }catch (...){
                GIE_UNEXPECTED_IN_DTOR();
            }

        }

        std::function<void(T&)> const m_resetter;

        typedef simple_to_std_allocator_t<T, simple_mt_allocator_t<simple_caching_allocator>> std_caching_allocator_t;

        MutexT m_mutex;
        std::vector<T* > m_cache_objects_queue_;
        simple_mt_allocator_t<simple_caching_allocator> m_caching_allocator{};
        std_caching_allocator_t m_std_caching_allocator{m_caching_allocator};
        size_t m_alive_objects_count = 0;

    };

}
//================================================================================================================================================
#endif
//================================================================================================================================================
