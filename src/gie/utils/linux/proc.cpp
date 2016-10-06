//================================================================================================================================================
// FILE: proc.cpp
// (c) GIE 2016-10-06  23:38
//
//================================================================================================================================================
#include "proc.hpp"
//================================================================================================================================================
#include "gie/util-scope-exit.hpp"
//================================================================================================================================================
namespace gie {

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

}
//================================================================================================================================================
