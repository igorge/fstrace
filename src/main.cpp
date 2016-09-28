#include "gie/exceptions.hpp"
#include "gie/easy_start/safe_main.hpp"
#include "gie/debug.hpp"

#include <boost/locale.hpp>
#include <boost/filesystem.hpp>
#include <boost/system/system_error.hpp>

#include <iostream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/fanotify.h>


int main(int argc, char *argv[]) {

    return ::gie::main([&](){

        auto const old_loc  = std::locale::global(boost::locale::generator().generate(""));
        std::locale loc;
        GIE_DEBUG_LOG(  "The previous locale is: " << old_loc.name( )  );
        GIE_DEBUG_LOG(  "The current locale is: " << loc.name( )  );
        boost::filesystem::path::imbue(std::locale());



        auto const fa_notify_handle = fanotify_init(FAN_CLASS_NOTIF /*| FAN_UNLIMITED_QUEUE | FAN_UNLIMITED_MARKS*/, O_RDWR);

        //auto ec = boost::system::error_code(errno, boost::system::system_category());

        //std::cout << ec.message() << std::endl;

        GIE_CHECK_ERRNO(fa_notify_handle!=-1);
        GIE_CHECK_ERRNO( close(fa_notify_handle)!= -1);

    });

}