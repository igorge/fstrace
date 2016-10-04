//================================================================================================================================================
// FILE: mount_change_monitor.cpp
// (c) GIE 2016-10-03  17:48
//
//================================================================================================================================================
#include "mount_change_monitor.hpp"
//================================================================================================================================================
namespace gie {
    boost::filesystem::path const mount_change_monitor_t::m_self_fd = boost::filesystem::path{"/proc/self/fd"};


    void mount_change_monitor_t::process_notify_event_(fanotify_event_metadata const& event){
        GIE_CHECK(event.vers==FANOTIFY_METADATA_VERSION);

        GIE_SCOPE_EXIT( [&](){GIE_CHECK_ERRNO( close(event.fd)!= -1);} );

        GIE_CHECK( (event.mask & FAN_Q_OVERFLOW)==0 );

        boost::system::error_code ec;

        auto const file_info_symlink = m_self_fd / std::to_string(event.fd);
        auto const exe_symlink = boost::filesystem::path("/proc") / std::to_string(event.pid) / "exe";
        auto const exe_path = boost::filesystem::read_symlink(exe_symlink, ec) ;

        if(ec){
            GIE_DEBUG_LOG("failed to read exe path from "<<exe_symlink);
        }

        auto const& insert_r = m_pid_cache.insert( cached_pid_t{event.pid} );

    };



    void mount_change_monitor_t::async_read_events_(){
        m_fanotify_asio_handle.async_read_some(boost::asio::buffer(m_buffer), [this](boost::system::error_code const & ec, long unsigned int const& size){
            if(!ec){
                this->read_events_(ec, size);
            } else {
                GIE_DEBUG_LOG("CANCELED: " << ec.message());

                if(ec==boost::system::errc::operation_canceled){

                } else {
                    GIE_THROW(gie::exception::unexpected() << gie::exception::error_code_einfo(ec));
                }
            }
        });
    };




    void mount_change_monitor_t::read_events_(boost::system::error_code const & ec, long unsigned int const & size){
        //GIE_LOG("got " << size << " bytes" );

        fanotify_event_metadata tmp;
        size_t idx = 0;

        while(idx<size){
            //GIE_DEBUG_LOG("IDX: "<< idx);
            GIE_CHECK(idx+sizeof(fanotify_event_metadata)<=size);
            memcpy(&tmp, m_buffer.data()+idx, sizeof(fanotify_event_metadata));
            process_notify_event_(tmp);

            idx+=tmp.event_len;
        }
        GIE_CHECK(idx==size);

        async_read_events_();
    };


}
//================================================================================================================================================
