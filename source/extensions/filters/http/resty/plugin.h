#pragma once
#include "envoy/http/filter.h"

#include "extensions/filters/common/lua/lua_tinker.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

struct RestyPluginConfig {
  std::string name_;
  Table config_;
};

class RestyPlugin : public Logger::Loggable<Logger::Id::resty> {

public:
  RestyPlugin(std::string& name, const RestyPluginConfig* plugin_config);
  virtual ~RestyPlugin();

public:
  bool decodeHeaders(bool end_stream);

private:
  std::string name_;
  const RestyPluginConfig* plugin_config_;
};

typedef std::map<std::string, RestyPlugin*> RestyPluginMap;

// namespace end
} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
