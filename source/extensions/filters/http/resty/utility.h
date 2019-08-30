#pragma once

#include "envoy/api/v2/lds.pb.h"
#include "envoy/api/v2/listener/listener.pb.h"
#include "envoy/config/filter/http/resty/v2/resty.pb.h"
#include "envoy/config/filter/network/http_connection_manager/v2/http_connection_manager.pb.h"

#include "common/protobuf/utility.h"
#include "envoy/http/filter.h"

#include "extensions/filters/common/lua/lua_tinker.h"
#include "extensions/filters/http/well_known_names.h"
#include "extensions/filters/network/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {


class ScriptAction;

class Utility {
public:
  using RestyEnablePlugins = envoy::config::filter::http::resty::v2::EnablePlugins;
  using RestyPluginProto = envoy::config::filter::http::resty::v2::Plugin;
  using Listener = envoy::api::v2::Listener;
  using NetworkFilterNames = Envoy::Extensions::NetworkFilters::NetworkFilterNames;
  using HttpFilterNames = Envoy::Extensions::HttpFilters::HttpFilterNames;
  using HttpConnectionManager =
      envoy::config::filter::network::http_connection_manager::v2::HttpConnectionManager;

  static std::shared_ptr<Table> ProtoConfigToTable(ScriptAction* sa, const RestyPluginProto& p);

  static void validateRestySchema(const RestyEnablePlugins& plugins);
  static void validateRestySchema(const Listener& listener);
};

class RestyException : public Envoy::EnvoyException {
public:
  RestyException(const std::string& message) : EnvoyException(message) { ; }
};




} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
