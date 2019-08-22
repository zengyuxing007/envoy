licenses(["notice"])  # Apache 2

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "lua_lib",
    srcs = [
            "lua.cc",
            "lua_tinker.cc",
            "script.cc",
            "script_action.cc",
            "utility.cc",
           ],

    hdrs = [
            "lua.h",
            "lua_tinker.h",
            "script.h",
            "script.inl",
            "script_action.h",
            "script_action.inl",
            "utility.h",
           ],

    external_deps = [
        "luajit",
    ],
    deps = [
        "//include/envoy/thread_local:thread_local_interface",
        "//include/envoy/http:filter_interface",
        "//source/common/common:assert_lib",
        "//source/common/common:c_smart_ptr_lib",
        "//source/common/common:lock_guard_lib",
        "//source/common/common:thread_lib",
        "//source/common/protobuf",
    ],
)

envoy_cc_library(
    name = "wrappers_lib",
    srcs = ["wrappers.cc"],
    hdrs = ["wrappers.h"],
    deps = [
        ":lua_lib",
        "//include/envoy/buffer:buffer_interface",
        "//source/common/protobuf",
    ],
)

