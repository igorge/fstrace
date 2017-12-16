cmake_minimum_required(VERSION 3.9 FATAL_ERROR)

project(fstrace LANGUAGES CXX)

set(BINARY_NAME fstrace)

#set(CMAKE_VERBOSE_MAKEFILE ON)

#set(CMAKE_CXX_STANDARD 17)
#set(CMAKE_CXX_STANDARD_REQUIRED ON)
#set(CMAKE_CXX_EXTENSIONS OFF)

#find_package(Boost COMPONENTS filesystem system thread program_options serialization date_time locale)
find_package(Boost COMPONENTS filesystem thread program_options serialization locale)
############################################################################

#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvtable-verify=std")
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread")
set(SANITAZERS "${SANITAZERS} -fsanitize=address")
set(SANITAZERS "${SANITAZERS} -fsanitize=undefined")
set(SANITAZERS "${SANITAZERS} -fsanitize=float-cast-overflow")
set(SANITAZERS "${SANITAZERS} -fno-sanitize-recover=all")

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Wno-deprecated -Wno-unused-parameter")

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -fno-omit-frame-pointer ${SANITAZERS} -DBOOST_MULTI_INDEX_ENABLE_SAFE_MODE -DBOOST_MULTI_INDEX_ENABLE_INVARIANT_CHECKING")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -DNDEBUG -O2 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} -DNDEBUG -Os")

############################################################################
add_subdirectory(../../libgie/ ${PROJECT_BINARY_DIR}/extlib/gie )

############################################################################
add_executable(${BINARY_NAME} main.cpp)

target_compile_features( ${BINARY_NAME} PRIVATE cxx_std_17 )

#set_property(
#        TARGET ${BINARY_NAME}
#        PROPERTY CXX_CLANG_TIDY clang-tidy;-checks=performance-*,-checks=modernize-*)


target_sources(
        ${BINARY_NAME}
        PRIVATE
        gie/utils/linux/mount_info_parser.cpp
        mount_change_monitor.cpp
        serializable_writer.cpp
        gie/utils/linux/proc.cpp)

target_include_directories(
        ${BINARY_NAME}
        PRIVATE
        ${CMAKE_CURRENT_BINARY_DIR}
        ${PROJECT_SOURCE_DIR})

target_link_libraries(
        ${BINARY_NAME}
        PRIVATE
        Boost::filesystem Boost::thread Boost::program_options Boost::serialization Boost::locale
        libgie )
