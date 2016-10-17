//================================================================================================================================================
// FILE: async_writer.h
// (c) GIE 2016-10-10  01:42
//
//================================================================================================================================================
#ifndef H_GUARD_ASYNC_WRITER_2016_10_10_01_42
#define H_GUARD_ASYNC_WRITER_2016_10_10_01_42
//================================================================================================================================================
#pragma once
//================================================================================================================================================
#include "allocator.hpp"
#include "gie/sio2/sio2_push_back_stream.hpp"
#include "gie/asio/custom_alloc_handler.hpp"
#include "gie/asio/simple_common.hpp"
#include "gie/asio/simple_service.hpp"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <queue>
//================================================================================================================================================
namespace gie {


    namespace exception {

        struct error_code : virtual root {} ;
        struct cancelled : virtual error_code {} ;
        struct broken_pipe : virtual error_code {} ;
    }


    struct eh_noop_t{
        void operator()()const{  // executed in catch context
            throw;
        }

    };

    template <class ParentT, class ChildT>
    struct eh_chain_t{
        void operator()()const{  // executed in catch context
            try {
                m_eh_child();
            } catch (...) {
                m_eh_parent();
            }
        }

        template <class T, class U>
        eh_chain_t(T&& eh_parent, U&& eh_child)
                : m_eh_parent(std::forward<T>(eh_parent))
                , m_eh_child(std::forward<U>(eh_child))
        {
        }

        ParentT m_eh_parent;
        ChildT  m_eh_child;
    };


    template <class ParentT, class ChildT>
    eh_chain_t<ParentT, ChildT> chain_eh(ParentT&& p, ChildT&& c){
        return eh_chain_t<ParentT, ChildT>(std::forward<ParentT>(p), std::forward<ChildT>(c));
    };




    struct async_writer_t { // use only with 1 async worker thread

        typedef gie::shared_buffer_t shared_buffer_t;


        template <class T>
        explicit async_writer_t(std_caching_allocator_t const& allocator, T&& io, boost::asio::posix::stream_descriptor && out)
                : m_allocator (allocator)
                , m_io( std::forward<T>(io) )
                , m_out( std::move(out) )
                , m_strand( m_io.service() )
        {

        }

        ~async_writer_t(){
            try{
                m_io.post([this]{
                    m_aborted = true;
                    m_out.cancel();
                });

                m_out.cancel();

                m_io.stop_sync();
            }catch(...){
                GIE_UNEXPECTED_IN_DTOR();
            }

        }



        void async_write(shared_buffer_t const& data){
            m_io.post(m_strand.wrap([this,data](){
                if(busy_()) {
                    //GIE_DEBUG_LOG("WRITER: Busy, enqueuing.");
                    m_queue.push( data );
                } else {
                    //GIE_DEBUG_LOG("WRITER: Idle, direct async write.");
                    m_queue.push( data );
                    async_write_one_(m_queue.front());
                }
            }));
        }

    private:

        shared_buffer_t get_shared_buffer_(){
            return allocate_buffer(m_allocator);
        }

        void async_write_one_(shared_buffer_t const & data) {
            async_write_size_and_buffer_(data, [this](std::exception_ptr const& e) {
                try {
                    if(e) std::rethrow_exception(e);

                    m_queue.pop(); //remove completed buffer

                    if (m_queue.empty()) {
                        // GIE_DEBUG_LOG("WRITER: No more work.");
                    } else {
                        GIE_DEBUG_IF_LOG(m_queue.size() > 1024,
                                         "WRITER: Warning, queue length: " << m_queue.size() << ".");
                        auto &current = m_queue.front();

                        async_write_one_(current);
                    }

                } catch (exception::broken_pipe const& e){
                    auto const ec = boost::get_error_info<gie::exception::error_code_einfo>(e);
                    GIE_DEBUG_LOG("BROKEN PIPE: " << ec->message()<<". Clearing "<<m_queue.size()<<" queued writes."); // clear queue till pipe become unbroken again
                    while(!m_queue.empty()){ m_queue.pop(); }

                } catch (exception::cancelled const& e){
                    auto const ec = boost::get_error_info<gie::exception::error_code_einfo>(e);
                    GIE_DEBUG_LOG("CANCELED: " << ec->message());
                }
            });
        }


        template <class ContT>
        void async_write_size_and_buffer_(shared_buffer_t const & data, ContT&& continuation){

            GIE_CHECK(data->size()<=std::numeric_limits<std::uint32_t>::max());
            GIE_DEBUG_IF_LOG(m_queue.size()==0, "WARNING: zero length write.");

            auto size_buffer = get_shared_buffer_();

            std::uint32_t size = static_cast<std::uint32_t>(data->size());

            sio2::push_back_writer_t<shared_buffer_t::element_type>{*size_buffer}(sio2::as<sio2::tag::uint32_le>(size));
            assert(size_buffer->size()==4);

            async_write_(size_buffer, [this, data, continuation=std::forward<ContT>(continuation)](std::exception_ptr const& e){
                try {
                    if(e) std::rethrow_exception(e);
                    async_write_(data, continuation);
                }catch (...){
                    continuation(std::current_exception());
                }
            });


        };


        template <class ContT>
        void async_write_(shared_buffer_t const & data, ContT&& continuation){

            boost::asio::async_write(
                    m_out,
                    boost::asio::buffer(*data),
                    make_custom_alloc_handler(m_strand.wrap([this, data, continuation=std::forward<ContT>(continuation)](boost::system::error_code const & ec, long unsigned int const& size) {
                        try{

                            if(ec){
                                if(ec==boost::system::errc::operation_canceled){
                                    GIE_THROW(exception::cancelled() << gie::exception::error_code_einfo(ec));
                                } else if(ec==boost::system::errc::broken_pipe) {
                                    GIE_THROW(exception::broken_pipe() << gie::exception::error_code_einfo(ec));
                                } else {
                                    GIE_THROW(exception::error_code() << gie::exception::error_code_einfo(ec));
                                }
                            }

                            GIE_CHECK(size==data->size());

                            continuation(std::exception_ptr());
                        } catch (...){
                            continuation(std::current_exception());
                        }

                    }), std_allocator_to_simple(m_allocator)));

        }

        bool busy_()const{
            return !m_queue.empty();
        }


    private:
        std_caching_allocator_t m_allocator;
        gie::simple_asio_service_t<> m_io;
        boost::asio::posix::stream_descriptor m_out;
        std::queue<shared_buffer_t> m_queue;

        boost::asio::strand m_strand;

        bool m_aborted = false;


    };




}
//================================================================================================================================================
#endif
//================================================================================================================================================
