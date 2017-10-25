//================================================================================================================================================
// FILE: serializable_writer.cpp
// (c) GIE 2016-10-14  12:33
//
//================================================================================================================================================
#include "serializable_writer.hpp"
//================================================================================================================================================
#include "allocator.hpp"
#include "async_writer.hpp"

#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/back_inserter.hpp>


#include <boost/archive/text_oarchive.hpp>
//================================================================================================================================================
namespace gie {

    notify_callback_t make_serializable_writer(caching_simple_allocator_t& caching_allocator){

        auto const get_stdout= []{
            auto const& fn = fileno(stdout);
            GIE_CHECK_ERRNO( fn!=-1 );
            auto const& tmp = dup(fn);
            GIE_CHECK_ERRNO( tmp!=-1 );
            return tmp;
        };


        std_caching_allocator_t allocator{caching_allocator};

        auto const& io_writer = boost::make_shared<gie::shared_io_service_t::element_type>();
        auto const& data_writer = boost::make_shared<gie::async_writer_t>(allocator, io_writer, boost::asio::posix::stream_descriptor{*io_writer, get_stdout()});

        return [data_writer, allocator](auto const pid, auto const& exe, auto const& file, auto const event_mask){

            auto const& shared_buffer = allocate_buffer(allocator);

            boost::iostreams::stream<boost::iostreams::back_insert_device< gie::buffer_t > > tmp_stream(*shared_buffer);

            {
                boost::archive::text_oarchive oa(tmp_stream);
                oa << pid << exe.native() << file.native() << event_mask;
            }

            //tmp_stream << exe << " ("<<pid<<"): ["<<gie::mount_change_monitor_t::event_mask2string(event_mask)<<"] " << file << std::endl;
            tmp_stream.flush();
            GIE_CHECK(!tmp_stream.bad());

            data_writer->async_write(shared_buffer);
        };

    }


}
//================================================================================================================================================
