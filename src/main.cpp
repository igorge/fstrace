#include "gie/asio/simple_service.hpp"
#include "gie/util-scope-exit.hpp"
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


using async_io_t = gie::simple_asio_service_t<>;


int main(int argc, char *argv[]) {

    return ::gie::main([&](){

        async_io_t io;

        auto const old_loc  = std::locale::global(boost::locale::generator().generate(""));
        std::locale loc;
        GIE_DEBUG_LOG(  "The previous locale is: " << old_loc.name( )  );
        GIE_DEBUG_LOG(  "The current locale is: " << loc.name( )  );
        boost::filesystem::path::imbue(std::locale());



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


        std::function<void(const boost::system::error_code, const long unsigned int)> read_events = [&](const boost::system::error_code& ec, const long unsigned int& size){
            if(!ec){
                GIE_LOG("got " << size << "bytes" );
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