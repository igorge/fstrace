//================================================================================================================================================
// FILE: mount_change_monitor.h
// (c) GIE 2016-10-03  17:48
//
//================================================================================================================================================
#ifndef H_GUARD_MOUNT_CHANGE_MONITOR_2016_10_03_17_48
#define H_GUARD_MOUNT_CHANGE_MONITOR_2016_10_03_17_48
//================================================================================================================================================
#pragma once
//================================================================================================================================================
#include "gie/asio/simple_service.hpp"

#include "gie/exceptions.hpp"
#include "gie/util-scope-exit.hpp"
#include "gie/debug.hpp"

#include <boost/filesystem.hpp>

#include <sys/fanotify.h>
//================================================================================================================================================
namespace gie {

    using async_io_t = gie::simple_asio_service_t<>;


    struct mount_change_monitor_t {

        static auto const event_buffer_size = 4*1024;

        mount_change_monitor_t(mount_change_monitor_t&&) = delete;
        mount_change_monitor_t(mount_change_monitor_t const &) = delete;
        mount_change_monitor_t& operator=(mount_change_monitor_t const&) = delete;

        mount_change_monitor_t(){
            m_buffer.resize(event_buffer_size);

            async_read_events_();
        };

        ~mount_change_monitor_t(){}

    private:
        std::vector<char> m_buffer;
        async_io_t m_io;

        boost::asio::posix::stream_descriptor m_fanotify_asio_handle = ([&](){
            auto const fanotify_fd = fanotify_init(FAN_CLASS_NOTIF | FAN_NONBLOCK /*| FAN_UNLIMITED_QUEUE | FAN_UNLIMITED_MARKS*/, O_RDONLY);
            GIE_CHECK_ERRNO(fanotify_fd!=-1);
            try {
                return boost::asio::posix::stream_descriptor{m_io.service(), fanotify_fd};
            } catch (...) {
                GIE_CHECK_ERRNO( close(fanotify_fd)!= -1);
                throw;
            }
        })();

        void async_read_events_();
        void process_notify_event_(fanotify_event_metadata const& event);
        void read_events_(boost::system::error_code const & ec, long unsigned int const & size);

        static boost::filesystem::path const m_self_fd;
    };



}
//================================================================================================================================================
#endif
//================================================================================================================================================
