//================================================================================================================================================
// FILE: mount_change_monitor.cpp
// (c) GIE 2016-10-03  17:48
//
//================================================================================================================================================
#include "mount_change_monitor.hpp"
//================================================================================================================================================
namespace gie {

    void mount_change_monitor_t::process_notify_event_(fanotify_event_metadata const& event){
        GIE_CHECK(event.vers==FANOTIFY_METADATA_VERSION);

        GIE_SCOPE_EXIT( [&](){GIE_CHECK_ERRNO( close(event.fd)!= -1);} );

        GIE_CHECK( (event.mask & FAN_Q_OVERFLOW)==0 );

        boost::system::error_code ec;

        auto const file_info_symlink = m_self_fd / std::to_string(event.fd);
        auto const exe_symlink = boost::filesystem::path("/proc") / std::to_string(event.pid) / "exe";
        auto const exe_path = boost::filesystem::read_symlink(exe_symlink, ec) ;

        auto const& proc_info = m_pid_cache.get_insert_modify(
                event.pid,
                [&](){ return cached_pid_t{event.pid, exe_path}; },
                [&](cached_pid_t& v){
                    if(!ec && (exe_path!=v.exe_path)){ // on error return cached path, otherwise update path due exec
                        GIE_DEBUG_LOG("EXE CHANGED ("<<event.pid<<") from "<<v.exe_path<<" to "<< exe_path);
                        v.exe_path = exe_path;
                    }
                }
        );

        m_callback(event.pid, proc_info.exe_path , boost::filesystem::read_symlink(file_info_symlink), event.mask);

        m_pid_cache.gc( boost::chrono::seconds(30) );
    };




    void mount_change_monitor_t::async_read_events_(){
        m_fanotify_asio_handle.async_read_some(boost::asio::buffer(m_buffer), make_custom_alloc_handler([this](boost::system::error_code const & ec, long unsigned int const& size){
            if(!ec){
                this->read_events_(ec, size);
            } else {
                if(ec==boost::system::errc::operation_canceled ){
                    GIE_DEBUG_LOG("CANCELED: " << ec.message());

                } else {
                    GIE_THROW(gie::exception::unexpected() << gie::exception::error_code_einfo(ec));
                }
            }
        }, make_ref_wrapper(m_allocator)));
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

        if(!is_aborted_()){
            async_read_events_();
        }
    };


}
//================================================================================================================================================
