//================================================================================================================================================
// FILE: test_dummy.cpp
// (c) GIE 2016-10-11  02:18
//
//================================================================================================================================================
#include "test_dummy.hpp"
//================================================================================================================================================
#include "gie/sio2/sio2_exceptions.hpp"
#include "gie/sio2/sio2_core.hpp"
#include "gie/exceptions.hpp"
//================================================================================================================================================
namespace gie {


    struct vector_reader_t {

        typedef unsigned int octet_type;

        template <class TTag = sio2::tag::not_specified, class T>
        void serialize(T& v){
            sio2::serialize_in<TTag>(v, *this);
        }

        octet_type read(){
            GIE_CHECK_EX( m_pos<m_data.size(), sio2::exception::underflow() );
            return m_data[m_pos++];
        }



        vector_reader_t(std::vector<char>&data) : m_data(data) {

        }

        std::vector<char>& m_data;
        size_t m_pos = 0;

    };


    struct test_t{
        unsigned int v;

    };

    template <class ReadWriteStream>
    void serialize(test_t& v, ReadWriteStream& stream){
        stream.template serialize <sio2::tag::octet_type>(v.v);

    }


    void test_dummy(){

        std::vector<char> data = {1};

        vector_reader_t vr{data};

        test_t v;


        vr.serialize(v);


        GIE_UNEXPECTED();

    }


}
//================================================================================================================================================
