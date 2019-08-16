#pragma once

#include "envoy/config/filter/http/resty/v2/resty.pb.h"
#include "envoy/config/filter/http/resty/v2/resty.pb.validate.h"

#include "extensions/filters/http/common/factory_base.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

/**
 * Config registration for the Resty filter. @see NamedHttpFilterConfigFactory.
 */
class RestyFilterConfig : public Common::FactoryBase<envoy::config::filter::http::resty::v2::EnablePlugins> {
public:
  RestyFilterConfig() : FactoryBase(HttpFilterNames::get().Resty) {}

  Http::FilterFactoryCb
  createFilterFactory(const Json::Object& json_config, const std::string& stats_prefix,
                      Server::Configuration::FactoryContext& context) override;

private:
  Http::FilterFactoryCb
  createFilterFactoryFromProtoTyped(const envoy::config::filter::http::resty::v2::EnablePlugins& proto_config,
                                    const std::string&,
                                    Server::Configuration::FactoryContext& context) override;
};

} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
