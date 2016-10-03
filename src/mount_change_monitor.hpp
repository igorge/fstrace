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
        typedef boost::filesystem::path path_t;
        typedef decltype(fanotify_event_metadata::pid) pid_t;
        typedef decltype(fanotify_event_metadata::mask) event_mask_t;

        typedef boost::function<void(pid_t const, path_t const& /*exe*/, path_t const& /*file*/, event_mask_t const)> callback_t;

        static auto const event_buffer_size = 4*1024;

        mount_change_monitor_t(mount_change_monitor_t&&) = delete;
        mount_change_monitor_t(mount_change_monitor_t const &) = delete;
        mount_change_monitor_t& operator=(mount_change_monitor_t const&) = delete;

        mount_change_monitor_t(callback_t && callback): m_callback(std::move(callback)) {
            m_buffer.resize(event_buffer_size);

            async_read_events_();
        };

        ~mount_change_monitor_t(){}


        void add_mark(std::string const& mount_point){
            GIE_CHECK_ERRNO( fanotify_mark(m_fanotify_asio_handle.native_handle(), FAN_MARK_ADD | FAN_MARK_MOUNT, FAN_ACCESS | FAN_MODIFY |  FAN_CLOSE | FAN_OPEN  | FAN_ONDIR, 0, mount_point.c_str())==0 );
        }

    private:
        std::vector<char> m_buffer;
        callback_t m_callback;
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


        static void event_bit2char_(event_mask_t const em, event_mask_t const bit, char const c, std::string& str){
            if( (em & bit) == bit) str+=c;
        }

        static boost::filesystem::path const m_self_fd;

    public:
        static std::string event_mask2string(event_mask_t const em){
            std::string tmp;
            tmp.reserve(4);

            event_bit2char_(em,FAN_OPEN,'O',tmp);
            event_bit2char_(em,FAN_ACCESS,'R',tmp);
            event_bit2char_(em,FAN_MODIFY,'W',tmp);
            event_bit2char_(em,FAN_CLOSE_NOWRITE,'c',tmp);
            event_bit2char_(em,FAN_CLOSE_WRITE,'C',tmp);

            return tmp;
        }

    };



}
//================================================================================================================================================
#endif
//================================================================================================================================================
