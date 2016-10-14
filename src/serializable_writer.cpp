//================================================================================================================================================
// FILE: serializable_writer.cpp
// (c) GIE 2016-10-14  12:33
//
//================================================================================================================================================
#include "serializable_writer.hpp"
//================================================================================================================================================
#include "async_writer.hpp"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>


#include <boost/archive/text_oarchive.hpp>
//================================================================================================================================================
namespace gie {

    boost::shared_ptr<void> make_serializable_writer(notify_callback_t& callback){

        auto const get_stdout= []{
            auto const& fn = fileno(stdout);
            GIE_CHECK_ERRNO( fn!=-1 );
            auto const& tmp = dup(fn);
            GIE_CHECK_ERRNO( tmp!=-1 );
            return tmp;
        };


        auto const& io_writer = boost::make_shared<gie::shared_io_service_t::element_type>();
        auto const& data_writer = boost::make_shared<gie::async_writer_t>(io_writer, boost::asio::posix::stream_descriptor{*io_writer, get_stdout()});


        callback = [data_writer](auto const pid, auto const& exe, auto const& file, auto const event_mask){

            auto const& shared_buffer= boost::make_shared<std::vector<char> >();

            boost::iostreams::stream<boost::iostreams::back_insert_device<std::vector<char> > > tmp_stream(*shared_buffer);


            {
                boost::archive::text_oarchive oa(tmp_stream);
                oa << pid << exe.native() << file.native() << event_mask;
            }

            //tmp_stream << exe << " ("<<pid<<"): ["<<gie::mount_change_monitor_t::event_mask2string(event_mask)<<"] " << file << std::endl;
            tmp_stream.flush();
            GIE_CHECK(!tmp_stream.bad());

            data_writer->async_write(shared_buffer);
        };


        return data_writer;
    }


}
//================================================================================================================================================
