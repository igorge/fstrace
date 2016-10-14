//================================================================================================================================================
#include "serializable_writer.hpp"
#include "test_dummy.hpp"
#include "async_writer.hpp"
#include "mount_change_monitor.hpp"

#include "gie/utils/linux/mount_info_parser.hpp"
#include "gie/utils/linux/proc.hpp"
#include "gie/easy_start/safe_main.hpp"

#include <boost/locale.hpp>

#include <boost/range/algorithm/copy.hpp>
#include <boost/range/adaptor/filtered.hpp>
#include <boost/program_options.hpp>
//================================================================================================================================================

std::set<std::string> const ignore_fs_types{"debugfs", "sysfs", "cgroup", "proc", "devtmpfs", "devpts", "pstore", "securityfs", "rpc_pipefs", "fusectl", "binfmt_misc", "fuseblk", "fuse"};

template <class Vec>
auto filter_mounts(Vec const& input, std::set<std::string> const& filter)->auto{
    return (input | boost::adaptors::filtered([&](gie::mountinfo_t const& mi){
        return filter.count(boost::get<0>(mi.fs_type))==0;
    }));
}




namespace po = boost::program_options;




int main(int argc, char *argv[]) {

    return ::gie::main([&](){

        auto const old_loc  = std::locale::global(boost::locale::generator().generate(""));
        std::locale loc;
        GIE_DEBUG_LOG(  "The previous locale is: " << old_loc.name( )  );
        GIE_DEBUG_LOG(  "The current locale is: " << loc.name( )  );
        boost::filesystem::path::imbue(std::locale());

        gie::test_dummy();

        po::options_description options_desc("Allowed options");
        po::variables_map       options_values;

        options_desc.add_options()
                ("help", "produce help message")
                ("serialize", "output in format suitable for consumption by other application via pipe: [4-octet-length][boost::archive::text_oarchive data: pid exe file event-mask]")
                ;

        po::store(po::command_line_parser(argc, argv).options(options_desc).run(), options_values);
        po::notify(options_values);

        if (options_values.count("help") ) { std::cout << options_desc <<std::endl; return EXIT_SUCCESS; }


        GIE_CHECK_ERRNO( signal(SIGPIPE, SIG_IGN)!=SIG_ERR );

        auto const & mounts = gie::parse_mounts(gie::read_proc_file("/proc/self/mountinfo"));
        GIE_DEBUG_LOG("MOUNTS: " <<  mounts.size());

        auto const& filtered_mounts = filter_mounts(mounts, ignore_fs_types);


        notify_callback_t callback;
        gie::serializable_writer_holder_t data_writer_holder;

        if (options_values.count("serialize")){
            data_writer_holder = gie::make_serializable_writer(callback);
        } else {
            callback = [](auto const pid, auto const& exe, auto const& file, auto const event_mask){
                std::cout << exe << " ("<<pid<<"): ["<<gie::mount_change_monitor_t::event_mask2string(event_mask)<<"] " << file << std::endl;
            };
        }

        gie::mount_change_monitor_t fsmonitor{boost::make_shared<gie::shared_io_service_t::element_type>(), [&,self_pid=getpgrp()](auto const pid, auto const& exe, auto const& file, auto const event_mask){
            if(pid!=self_pid){
                callback(pid, exe, file, event_mask);
            }
        }};

        for( auto&& i:filtered_mounts){
            GIE_DEBUG_LOG("MONITORING '" << boost::get<0>(i.fs_type) << "' @ '" <<i.mount_point<<"'");
            try { fsmonitor.add_mark(i.mount_point); } catch (gie::exception::fanotify_mark const& e) {
                auto const einfo = boost::get_error_info<gie::exception::error_str_einfo>(e);
                GIE_LOG("Failed to set monitoring for '" << *einfo << "'. Ignoring.");
            }
        }

        sigset_t set;
        GIE_CHECK( sigemptyset(&set)==0 );
        GIE_CHECK( sigaddset(&set, SIGINT)==0 );
        int sig=0;
        GIE_CHECK( pthread_sigmask( SIG_BLOCK, &set, NULL )==0 );
        GIE_CHECK(sigwait(&set, &sig)==0);
        GIE_CHECK(sig==SIGINT);

        GIE_DEBUG_LOG("Terminated.");

        return EXIT_SUCCESS;
    });

}