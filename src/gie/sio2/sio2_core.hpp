//================================================================================================================================================
// FILE: sio2_integral.h
// (c) GIE 2016-10-11  01:18
//
//================================================================================================================================================
#ifndef H_GUARD_SIO2_INTEGRAL_2016_10_11_01_18
#define H_GUARD_SIO2_INTEGRAL_2016_10_11_01_18
//================================================================================================================================================
#pragma once
//================================================================================================================================================
#include <type_traits>
#include <cinttypes>
#include <limits>
//================================================================================================================================================
namespace gie { namespace sio2 {

//        octet serializer
//
//        ReadStream model:
//        struct ReadStream {
//        typedef integral_good_enought_for_octet_t octet_type
//          ReadStream& operator()(T& v){
//              serialize_in(v, *this);
//              return *this;
//          }
//
//          octet_type read();
//        };
//
//        serialize(value, [Read|Write]Stream) -- generic serialization function

        namespace tag {
            struct type_tag {};

            struct not_specified : type_tag {};

            struct integral_type : type_tag {};
            struct octet_type : integral_type {};
            struct int32_le_type: integral_type {};

        }

        std::uint_fast8_t const octet_mask=0xFF;


        //dispatch serialize via serialize_in
        // TTag is discarded here because serialize() must be only defined for structs
       // template <class T, class ReadWriteStream> void serialize(T&v, ReadWriteStream& stream);

        template <class TTag,class ReadStream, class T>
        typename std::enable_if<std::is_same<TTag, tag::not_specified>::value , void>::type
        serialize_in(T& v, ReadStream& read_stream){
            return serialize(v, read_stream);
        }

        template <class TTag, class WriteStream, class T>
        typename std::enable_if<std::is_same<TTag, tag::not_specified>::value , void>::type
        serialize_out(T& v, WriteStream& write_stream){
            return serialize(v, write_stream);
        }



        template <class TTag, class ReadStream, class T>
        typename std::enable_if<std::is_same<TTag, tag::octet_type>::value , void>::type
        serialize_in(T& v, ReadStream& op){
            typedef typename ReadStream::octet_type octet_type;
            octet_type const value = op.read();

            static_assert(!std::numeric_limits<octet_type>::is_signed, "octet must not be signed");
            static_assert(std::numeric_limits<octet_type>::digits>=8, "octet must contain atleast 8 bits");

            assert(std::numeric_limits<T>::digits==8 || (value & ~static_cast<octet_type>(octet_mask))==0 );

            static_assert(!std::numeric_limits<T>::is_signed, "target octet type must not be signed");
            static_assert(std::numeric_limits<T>::digits>=8, "target octet type must contain atleast 8 bits");

            v = static_cast<T>( value );
        }



    }}
//================================================================================================================================================
#endif
//================================================================================================================================================
