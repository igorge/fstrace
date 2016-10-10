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
#include "gie/asio/simple_common.hpp"

#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>

#include <queue>
//================================================================================================================================================
namespace gie {




    struct async_writer_t { // use only with 1 async worker thread

        typedef boost::shared_ptr<std::vector<char> > shared_buffer_t;


        template <class T>
        explicit async_writer_t(T&& io, boost::asio::posix::stream_descriptor && out)
                : m_io( std::forward<T>(io) )
                , m_out( std::move(out) )
        {

        }

        ~async_writer_t(){
            try{
                m_out.cancel();
            }catch(...){
                GIE_UNEXPECTED_IN_DTOR();
            }
        }


        shared_buffer_t get_shared_buffer(){
            return boost::make_shared<shared_buffer_t::element_type>();
        }

        void release_vector(shared_buffer_t&& buffer){
            if( !buffer.unique() ) {
                GIE_DEBUG_LOG("shared_buffer_t is not unique");
            } else {
                //GIE_DEBUG_LOG("buffer can be reused");
                buffer.reset();
            }


        }


        void async_write(shared_buffer_t const& data){
            m_io->post([this,data](){
                if(busy_()) {
                    //GIE_DEBUG_LOG("WRITER: Busy, enqueuing.");
                    m_queue.push( data );
                } else {
                    //GIE_DEBUG_LOG("WRITER: Idle, direct async write.");
                    m_queue.push( data );
                    async_write_(m_queue.front());
                }
            });
        }

        template <class T>
        auto async_wrap_(T&& fun) {
            return [this,fun=std::forward<T>(fun)](boost::system::error_code const & ec, long unsigned int const& size)mutable{

                if(ec){
                    if(ec==boost::system::errc::operation_canceled){
                        GIE_DEBUG_LOG("CANCELED: " << ec.message());
                        return;
                    } else if(ec==boost::system::errc::broken_pipe) {
                        GIE_DEBUG_LOG("BROKEN PIPE: " << ec.message()<<". Clearing "<<m_queue.size()<<" queued writes."); // clear queue till pipe become unbroken again
                        while(!m_queue.empty()){ m_queue.pop(); }
                        return;
                    } else {
                        GIE_THROW(gie::exception::unexpected() << gie::exception::error_code_einfo(ec));
                    }
                }

                fun(ec, size);
            };
        }

        void async_write_(shared_buffer_t & data){

            boost::asio::async_write(m_out, boost::asio::buffer(*data),  async_wrap_([this,data](boost::system::error_code const & ec, long unsigned int const& size) mutable {

                GIE_CHECK(size==data->size());


                m_queue.pop(); //remove completed buffer
                release_vector( std::move(data));

                if(m_queue.empty()){
                    // GIE_DEBUG_LOG("WRITER: No more work.");
                } else {
                    GIE_DEBUG_IF_LOG(m_queue.size()>2048, "WRITER: Warning, queue length: "<<m_queue.size()<<".");
                    auto & current = m_queue.front();
                    auto header = get_shared_buffer();
                    async_write_( current );
                }

            }));

        }

        bool busy_()const{
            return !m_queue.empty();
        }


    private:
        shared_io_service_t m_io;
        boost::asio::posix::stream_descriptor m_out;
        std::queue<shared_buffer_t> m_queue;

    };




}
//================================================================================================================================================
#endif
//================================================================================================================================================
