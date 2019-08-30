#include "extensions/filters/http/resty/config.h"

#include "envoy/config/filter/http/resty/v2/resty.pb.validate.h"
#include "envoy/registry/registry.h"

#include "common/config/filter_json.h"

#include "extensions/filters/http/resty/resty_filter.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

Http::FilterFactoryCb RestyFilterConfig::createFilterFactoryFromProtoTyped(
    const envoy::config::filter::http::resty::v2::EnablePlugins& proto_config, const std::string&,
    Server::Configuration::FactoryContext& context) {
  RestyPluginManagerConstSharedPtr resty_plugin_manager(new RestyPluginManager{proto_config,context.clusterManager()});
  return [resty_plugin_manager](Http::FilterChainFactoryCallbacks& callbacks) -> void {
    callbacks.addStreamFilter(std::make_shared<Filter>(resty_plugin_manager));
  };
}

Http::FilterFactoryCb
RestyFilterConfig::createFilterFactory(const Json::Object& json_config,
                                       const std::string& stat_prefix,
                                       Server::Configuration::FactoryContext& context) {

  NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  /*
  envoy::config::filter::http::resty::v2::EnablePlugins proto_config;
  Config::FilterJson::translateRestyFilter(json_config, proto_config);
  return createFilterFactoryFromProtoTyped(proto_config, stat_prefix, context);
  */
}

/**
 * Static registration for the Resty filter. @see RegisterFactory.
 */
REGISTER_FACTORY(RestyFilterConfig, Server::Configuration::NamedHttpFilterConfigFactory);

} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions 
} // namespace Envoy
