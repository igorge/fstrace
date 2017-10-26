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
#include "timeout_cache.hpp"

#include "gie/simple_caching_allocator.hpp"
#include "gie/asio/custom_alloc_handler.hpp"
#include "gie/asio/simple_service.hpp"
#include "gie/exceptions.hpp"
#include "gie/util-scope-exit.hpp"
#include "gie/debug.hpp"

#include <boost/filesystem.hpp>

#include <sys/fanotify.h>
//================================================================================================================================================
namespace gie {

    namespace exception {
        struct fanotify : virtual root {};
        struct fanotify_mark : virtual fanotify {};
    }


    struct mount_change_monitor_t {
        typedef boost::filesystem::path path_t;
        typedef decltype(fanotify_event_metadata::pid) pid_t;
        typedef decltype(fanotify_event_metadata::mask) event_mask_t;

        using async_io_t = boost::asio::io_service;

        struct cached_pid_t{
            pid_t pid;
            boost::filesystem::path exe_path;
        };

        struct cached_pid_key_extractor_t{
            typedef pid_t result_type;
            result_type operator()(cached_pid_t const& v) const { return v.pid; }
            result_type& operator()(cached_pid_t& v) const { return v.pid; }
        };

        typedef timeout_cache_t<cached_pid_t, cached_pid_key_extractor_t> pid_cache_t;

        pid_cache_t m_pid_cache;

        typedef boost::function<void(pid_t const, path_t const& /*exe*/, path_t const& /*file*/, event_mask_t const)> callback_t;

        static auto const event_buffer_size = 4*1024;

        mount_change_monitor_t(mount_change_monitor_t&&) = delete;
        mount_change_monitor_t(mount_change_monitor_t const &) = delete;
        mount_change_monitor_t& operator=(mount_change_monitor_t const&) = delete;

        template <class T1, class T2>
        mount_change_monitor_t(simple_allocator_i& allocator, T1 && io, T2 && callback)
                : m_allocator(allocator)
                , m_io(io)
                , m_callback(std::forward<T2>(callback))
        {
            m_buffer.resize(event_buffer_size);

            async_read_events_();
        };

        ~mount_change_monitor_t(){
            GIE_DEBUG_TRACE_INOUT();

            try {
                GIE_DEBUG_LOG("m_fanotify_asio_handle.cancel()");
                this->abort_();
            } catch (...) {
                GIE_UNEXPECTED_IN_DTOR();
            }
        }


        void add_mark(std::string const& mount_point){
            GIE_CHECK_ERRNO_EX(
                    fanotify_mark(m_fanotify_asio_handle.native_handle(), FAN_MARK_ADD | FAN_MARK_MOUNT, FAN_ACCESS | FAN_MODIFY |  FAN_CLOSE | FAN_OPEN  | FAN_ONDIR, 0, mount_point.c_str())==0,
                    exception::fanotify_mark() << exception::error_str_einfo(mount_point) );
        }

        auto io_service() -> async_io_t& {
            return m_io;
        }

        void abort(){
            abort_();
        }

    private:
        std::vector<char> m_buffer;
        simple_allocator_i& m_allocator;
        async_io_t& m_io;
        callback_t m_callback;
        bool m_aborted = false;

        boost::asio::posix::stream_descriptor m_fanotify_asio_handle = ([&](){
            auto const fanotify_fd = fanotify_init(FAN_CLASS_NOTIF | FAN_NONBLOCK /*| FAN_UNLIMITED_QUEUE | FAN_UNLIMITED_MARKS*/, O_RDONLY);
            GIE_CHECK_ERRNO(fanotify_fd!=-1);
            try {
                return boost::asio::posix::stream_descriptor{io_service(), fanotify_fd};
            } catch (...) {
                GIE_CHECK_ERRNO( close(fanotify_fd)!= -1);
                throw;
            }
        })();

        void abort_(){
            GIE_DEBUG_TRACE_INOUT();
            m_fanotify_asio_handle.cancel();

            m_io.post([this]{
                m_aborted = true;
                m_fanotify_asio_handle.cancel();
            });
        }
        bool is_aborted_()const{
            return m_aborted;
        }

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
