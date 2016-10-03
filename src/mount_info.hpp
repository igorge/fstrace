//================================================================================================================================================
// FILE: mount_info.h
// (c) GIE 2016-10-02  22:02
//
//================================================================================================================================================
#ifndef H_GUARD_MOUNT_INFO_2016_10_02_22_02
#define H_GUARD_MOUNT_INFO_2016_10_02_22_02
//================================================================================================================================================
#include <boost/tuple/tuple.hpp>
#include <boost/optional.hpp>
#include <boost/fusion/adapted.hpp>

#include <vector>
#include <string>
//================================================================================================================================================
namespace gie{

    typedef boost::tuple<int,int> dev_majot_minor_t;
    typedef boost::tuple<std::string,boost::optional<std::string>> opt_string_pair_t;

    struct mountinfo_t {
        int     mount_id;
        int     parent_id;
        dev_majot_minor_t   st_dev;
        std::string     root;
        std::string     mount_point;
        std::vector<std::string>    permount_options;
        std::vector<opt_string_pair_t> opt_fields;
        opt_string_pair_t              fs_type;
        std::string                 mount_source;
        std::vector<std::string>    per_superblock_options;
    };


}


        BOOST_FUSION_ADAPT_STRUCT( gie::mountinfo_t,
            (int, mount_id)
            (int, parent_id)
            (gie::dev_majot_minor_t,st_dev)
            (std::string,     root)
            (std::string,     mount_point)
            (std::vector<std::string>,    permount_options)
            (std::vector<gie::opt_string_pair_t> , opt_fields)
            (gie::opt_string_pair_t,              fs_type)
            (std::string,                 mount_source)
            (std::vector<std::string>,    per_superblock_options)
        )


//================================================================================================================================================
#endif
//================================================================================================================================================
