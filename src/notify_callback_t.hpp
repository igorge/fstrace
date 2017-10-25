//================================================================================================================================================
// FILE: notify_callback_t.h
// (c) GIE 2016-10-14  12:34
//
//================================================================================================================================================
#ifndef H_GUARD_NOTIFY_CALLBACK_T_2016_10_14_12_34
#define H_GUARD_NOTIFY_CALLBACK_T_2016_10_14_12_34
//================================================================================================================================================
#pragma once
//================================================================================================================================================
#include <boost/filesystem/path.hpp>
#include <sys/fanotify.h>
#include <functional>
//================================================================================================================================================
namespace {

    typedef decltype(fanotify_event_metadata::pid) pid_t;
    typedef decltype(fanotify_event_metadata::mask) event_mask_t;
    typedef boost::filesystem::path path_t;

    typedef std::function<void(pid_t, path_t, path_t, event_mask_t)> notify_callback_t;
}
//================================================================================================================================================
#endif
//================================================================================================================================================
