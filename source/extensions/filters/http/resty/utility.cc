#include "extensions/filters/http/resty/utility.h"

#include "common/common/fmt.h"
#include "common/config/utility.h"
#include "common/protobuf/message_validator_impl.h"

#include "extensions/filters/common/lua/lua_tinker.h"
#include "extensions/filters/common/lua/utility.h"
#include "extensions/filters/http/resty/script_action.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

std::shared_ptr<Table> Utility::ProtoConfigToTable(ScriptAction* sa, const RestyPluginProto& p) {
  Table* table = sa->newNullTable();
  std::shared_ptr<Table> tablePtr(table);
  if (p.has_config()) {
    /// change struct to Table (lua)
    Filters::Common::Lua::Utility::protobufStatus2LuaTable(p.config(), table, sa);
  }
  return tablePtr;
}

void Utility::validateRestySchema(const RestyEnablePlugins& enablePlugins) {
  auto tid = std::this_thread::get_id();
  ScriptAction* sa = &Envoy::Extensions::HttpFilters::Resty::gScriptAction;

  if (sa == NULL) {
    throw RestyException(
        fmt::format("checkPluginSchema error: not found ScriptAction by threadId {}", tid));
  }
  auto& plugins = enablePlugins.plugins();
  for (auto& plugin : plugins) {
    auto table = ProtoConfigToTable(sa, plugin);
    if (!sa->checkPluginSchema(plugin.name(), *table))
      throw RestyException(
          fmt::format("checkPluginSchema error: sechma of plugin {} error.", plugin.name()));
  }
}

void Utility::validateRestySchema(const Listener& listener) {
  auto& filterChains = listener.filter_chains();
  for (auto& filterChain : filterChains) {
    auto& filters = filterChain.filters();
    const auto& filter =
        std::find_if(filters.begin(), filters.end(), [](const envoy::api::v2::listener::Filter& f) {
          return f.name() == NetworkFilterNames::get().HttpConnectionManager;
        });

    if (filter == filters.end()) {
      continue;
    }

    auto emptyConfig = std::make_shared<HttpConnectionManager>();
    auto validationVisitor = Envoy::ProtobufMessage::StrictValidationVisitorImpl();
    Config::Utility::translateOpaqueConfig((*filter).typed_config(), (*filter).config(),
                                           validationVisitor, *emptyConfig);
    auto& httpFilters = emptyConfig->http_filters();

    const auto& httpFilter = std::find_if(
        httpFilters.begin(), httpFilters.end(),
        [](const envoy::config::filter::network::http_connection_manager::v2::HttpFilter& h) {
          return h.name() == HttpFilterNames::get().Resty;
        });
    if (httpFilter == httpFilters.end()) {
      break;
    }
    auto emptyRestyConfig = std::make_shared<RestyEnablePlugins>();
    Config::Utility::translateOpaqueConfig((*httpFilter).typed_config(), (*httpFilter).config(),
                                           validationVisitor, *emptyRestyConfig);
    validateRestySchema(*emptyRestyConfig);
    break;
  }
}

} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
