cmake_minimum_required(VERSION 3.5)
project(fstrace)
set(proj_exe_name fstrace)

#set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -Wall -Wextra -Wno-deprecated -Wno-unused-parameter")

find_package(Boost COMPONENTS filesystem system thread serialization date_time locale)
############################################################################

#add_definitions(-D_FILE_OFFSET_BITS=64)

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
        mount_info_parser.cpp
        mount_change_monitor.cpp)


############################################################################

add_executable(${proj_exe_name} ${SOURCE_FILES})

target_link_libraries(
        ${proj_exe_name}
        ${Boost_LIBRARIES}
        ${gie_link_libs} )
