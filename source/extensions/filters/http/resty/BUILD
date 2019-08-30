licenses(["notice"])  # Apache 2

# Resty L7 HTTP filter base on lua (https://www.lua.org/, http://luajit.org/)
# Public docs: docs/root/configuration/http_filters/resty_filter.rst

load(
    "//bazel:envoy_build_system.bzl",
    "envoy_cc_library",
    "envoy_package",
)

envoy_package()

envoy_cc_library(
    name = "resty_filter_lib",
    srcs = [
        "plugin.cc",
        "plugins_manager.cc",
        "resty_filter.cc",
        "script_action.cc",
        "utility.cc",
        "wrappers.cc",
        "resty_wrappers.cc",
    ],
    hdrs = [
        "plugin.h",
        "plugins_manager.h",
        "resty_filter.h",
        "script_action.h",
        "script_action.inl",
        "utility.h",
        "wrappers.h",
        "resty_wrappers.h",
    ],
    deps = [
        "//include/envoy/http:codes_interface",
        "//include/envoy/http:filter_interface",
        "//include/envoy/upstream:cluster_manager_interface",
        "//source/common/buffer:buffer_lib",
        "//source/common/common:enum_to_int",
        "//source/common/config:utility_lib",
        "//source/common/crypto:utility_lib",
        "//source/common/http:message_lib",
        "//source/common/http:utility_lib",
        "//source/common/protobuf:message_validator_lib",
        "//source/extensions/filters/common/lua:lua_lib",
        "//source/extensions/filters/common/lua:wrappers_lib",
        "//source/extensions/filters/http:well_known_names",
        "//source/extensions/filters/network:well_known_names",
        "//include/envoy/stream_info:stream_info_interface",
        "//include/envoy/http:header_map_interface",
        "@envoy_api//envoy/api/v2:lds_cc",
        "@envoy_api//envoy/api/v2/listener:listener_cc",
        "@envoy_api//envoy/config/filter/http/resty/v2:resty_cc",
        "@envoy_api//envoy/config/filter/network/http_connection_manager/v2:http_connection_manager_cc",
    ],
)

envoy_cc_library(
    name = "config",
    srcs = ["config.cc"],
    hdrs = ["config.h"],
    deps = [
        "//include/envoy/registry",
        "//source/common/config:filter_json_lib",
        "//source/extensions/filters/http:well_known_names",
        "//source/extensions/filters/http/common:factory_base_lib",
        "//source/extensions/filters/http/resty:resty_filter_lib",
        #"//api/envoy/config/filter/http/resty/v2:resty_proto_cc",
    ],
)
