#include "mount_change_monitor.hpp"
#include "mount_info_parser.hpp"

#include "gie/sio/util-sio.hpp"
#include "gie/util-scope-exit.hpp"
#include "gie/exceptions.hpp"
#include "gie/easy_start/safe_main.hpp"
#include "gie/debug.hpp"

#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>
#include <boost/filesystem.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/filtered.hpp>

#include <iostream>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>

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


        gie::mount_change_monitor_t fsmonitor;

        char a;
        std::cin >> a;

    });

}