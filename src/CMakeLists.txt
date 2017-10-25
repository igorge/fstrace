cmake_minimum_required(VERSION 3.7 FATAL_ERROR)
project(fstrace)
set(proj_exe_name fstrace)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wno-deprecated -Wno-unused-parameter")

find_package(Boost COMPONENTS filesystem system thread program_options serialization date_time locale)
############################################################################

#add_definitions(-D_FILE_OFFSET_BITS=64)

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvtable-verify=std")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
set(SANITAZERS "${SANITAZERS} -fsanitize=address")
set(SANITAZERS "${SANITAZERS} -fsanitize=undefined")
set(SANITAZERS "${SANITAZERS} -fsanitize=float-cast-overflow")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-omit-frame-pointer ${SANITAZERS} -DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE -DBOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG -O2 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DNDEBUG -Os")

############################################################################
add_subdirectory(../../libgie/ ${PROJECT_BINARY_DIR}/extlib/gie )

set(gie_include_dir ${gie_include_dir} ${libgie_SOURCE_DIR}/include)
set(gie_link_libs ${gie_link_libs} libgie)

############################################################################

include_directories(
        ${CMAKE_CURRENT_BINARY_DIR} ${Boost_INCLUDE_DIRS} ${PROJECT_SOURCE_DIR} ${gie_include_dir} )

############################################################################


set(SOURCE_FILES
        main.cpp
        gie/utils/linux/mount_info_parser.cpp
        mount_change_monitor.cpp
        serializable_writer.cpp
        gie/utils/linux/proc.cpp)


############################################################################

add_executable(${proj_exe_name} ${SOURCE_FILES})

target_link_libraries(
        ${proj_exe_name}
        ${Boost_LIBRARIES}
        ${gie_link_libs} )
