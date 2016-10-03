#include "mount_info_parser.hpp"

#include "gie/sio/util-sio.hpp"
#include "gie/asio/simple_service.hpp"
#include "gie/util-scope-exit.hpp"
#include "gie/exceptions.hpp"
#include "gie/easy_start/safe_main.hpp"
#include "gie/debug.hpp"

#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
#include <boost/filesystem.hpp>

#include <boost/optional/optional_io.hpp>
#include <boost/fusion/sequence/io.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <iostream>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/fanotify.h>


using async_io_t = gie::simple_asio_service_t<>;

//namespace std {
//
//    template<class T>
//    std::ostream &operator<<(ostream &os, const vector<T> &v) {
//        os << "v:"<<v.size()<<"[";
//        for (auto &&ii:v) {
//            os << " " << ii;
//        }
//        os << "]";
//        return os;
//    }
//
//}


std::vector<char> read_proc_file(std::string const& path){
    auto const handle = fopen(path.c_str(), "r");
    GIE_CHECK_ERRNO(handle!= nullptr);
    GIE_SCOPE_EXIT([&](){  GIE_CHECK(fclose(handle)==0);  });

    auto const inc_step = 4*1024;

    std::vector<char> tmp;
    tmp.resize(inc_step);
    std::vector<char>::size_type i = 0;

    for(;;){
        auto const bytes_to_read = tmp.size()-i;
        auto const bytes_read = fread(tmp.data()+i, 1, tmp.size()-i, handle);

        if(bytes_to_read !=bytes_read) {
            assert(bytes_read<bytes_to_read);
            tmp.resize(i+bytes_read);
            break;
        } else {
            i +=bytes_read;
            tmp.resize(tmp.size()+inc_step);
        }
    }

    return tmp;
}

std::set<std::string> const ignore_fs_types{"sysfs", "cgroup", "proc", "devtmpfs", "devpts", "pstore", "securityfs", "rpc_pipefs", "fusectl", "binfmt_misc", "fuseblk", "fuse"};

template <class Vec>
auto filter_mounts(Vec const& input, std::set<std::string> const& filter)->auto{
    return (input | boost::adaptors::filtered([&](gie::mountinfo_t const& mi){
        return filter.count(boost::get<0>(mi.fs_type))==0;
    }));
}


int main(int argc, char *argv[]) {

    return ::gie::main([&](){

        auto const old_loc  = std::locale::global(boost::locale::generator().generate(""));
        std::locale loc;
        GIE_DEBUG_LOG(  "The previous locale is: " << old_loc.name( )  );
        GIE_DEBUG_LOG(  "The current locale is: " << loc.name( )  );
        boost::filesystem::path::imbue(std::locale());


        std::vector<char> isbuffer = read_proc_file("/proc/self/mountinfo");

        auto const & mounts = gie::parse_mounts(isbuffer);
        std::cout << "MOUNTS: " <<  mounts.size() << std::endl;

        auto const& filtered_mounts = filter_mounts(mounts, ignore_fs_types);

        for( auto&& i:filtered_mounts){
            std::cout << "fstype: " << boost::get<0>(i.fs_type) << " @ " <<i.mount_point<< "\n";
        }

        return 0;

        async_io_t io;


        auto fanotify_asio_handle = ([&](){
            auto const fanotify_fd = fanotify_init(FAN_CLASS_NOTIF | FAN_NONBLOCK /*| FAN_UNLIMITED_QUEUE | FAN_UNLIMITED_MARKS*/, O_RDONLY);
            GIE_CHECK_ERRNO(fanotify_fd!=-1);
            try {
                return boost::asio::posix::stream_descriptor{io.service(), fanotify_fd};
            } catch (...) {
                GIE_CHECK_ERRNO( close(fanotify_fd)!= -1);
                throw;
            }
        })();

        GIE_CHECK_ERRNO( fanotify_mark(fanotify_asio_handle.native_handle(), FAN_MARK_ADD | FAN_MARK_MOUNT, FAN_ACCESS | FAN_MODIFY |  FAN_CLOSE | FAN_OPEN  | FAN_ONDIR, 0, "/")==0 );


        std::vector<char> buffer; buffer.resize(4*1024);

        auto const self_fd = boost::filesystem::path{"/proc/self/fd"};

        auto const process_notify_event = [&](fanotify_event_metadata const& event){
            GIE_CHECK(event.vers==FANOTIFY_METADATA_VERSION);

            GIE_SCOPE_EXIT( [&](){GIE_CHECK_ERRNO( close(event.fd)!= -1);} );

            boost::system::error_code ec;

            auto const file_info_symlink = self_fd / std::to_string(event.fd);
            auto const exe_symlink = boost::filesystem::path("/proc") / std::to_string(event.pid) / "exe";
            auto const exe_path = boost::filesystem::read_symlink(exe_symlink, ec) ;
            GIE_DEBUG_LOG( exe_path << ": " <<  boost::filesystem::read_symlink(file_info_symlink));

        };


        std::function<void(const boost::system::error_code, const long unsigned int)> read_events = [&](const boost::system::error_code& ec, const long unsigned int& size){
            if(!ec){
                //GIE_LOG("got " << size << " bytes" );

                fanotify_event_metadata tmp;
                size_t idx = 0;

                while(idx<size){
                    //GIE_DEBUG_LOG("IDX: "<< idx);
                    GIE_CHECK(idx+sizeof(fanotify_event_metadata)<=size);
                    memcpy(&tmp, buffer.data()+idx, sizeof(fanotify_event_metadata));
                    process_notify_event(tmp);

                    idx+=tmp.event_len;
                }
                 GIE_CHECK(idx==size);

                fanotify_asio_handle.async_read_some(boost::asio::buffer(buffer), read_events);
            } else {
                GIE_DEBUG_LOG("error: " << ec.message());
                GIE_THROW(gie::exception::unexpected() << gie::exception::error_code_einfo(ec));
            }
        };

        fanotify_asio_handle.async_read_some(boost::asio::buffer(buffer), read_events);

        char a;
        std::cin >> a;
        //while(true){
//            auto const size = read(fanotify_fd, buffer.data(), buffer.size());
        //}


    });

}