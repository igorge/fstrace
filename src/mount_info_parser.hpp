//================================================================================================================================================
// FILE: mount_info_parser.h
// (c) GIE 2016-10-01  23:08
//
//================================================================================================================================================
#ifndef H_GUARD_MOUNT_INFO_PARSER_2016_10_01_23_08
#define H_GUARD_MOUNT_INFO_PARSER_2016_10_01_23_08
//================================================================================================================================================
#include "mount_info.hpp"
#include <boost/range/any_range.hpp>
//================================================================================================================================================
namespace gie {




//
//    typedef boost::tuple<
//            int,
//            int,
//            boost::tuple<int,int>,
//            std::string,
//            std::string,
//            std::vector<std::string>,
//            std::vector<boost::tuple<std::string,boost::optional<std::string>>>,
//            boost::tuple<std::string,boost::optional<std::string>>,
//            std::string,
//            std::vector<std::string>
//    > mountinfo_t;


    typedef std::vector<mountinfo_t> mounts_t;


    typedef boost::any_range<char const, boost::forward_traversal_tag> parse_forward_range_t;

    // parse /proc/<pid>/mountinfo
    mounts_t parse_mounts(parse_forward_range_t const& in);

}
//================================================================================================================================================
#endif
//================================================================================================================================================
