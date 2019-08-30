#pragma once

#include "envoy/config/filter/http/resty/v2/resty.pb.h"
#include "envoy/http/filter.h"

#include "common/common/logger.h"

#include "extensions/filters/common/lua/lua.h"
#include "extensions/filters/common/lua/lua_tinker.h"
#include "extensions/filters/http/resty/plugin.h"
#include "extensions/filters/http/resty/script_action.h"
#include "extensions/filters/http/resty/resty_wrappers.h"


namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

class RestyPluginManager : Logger::Loggable<Logger::Id::resty> {

public:
  using RestyEnablePlugins = envoy::config::filter::http::resty::v2::EnablePlugins;
  using ScriptAction = Envoy::Extensions::HttpFilters::Resty::ScriptAction;
  using RestyPluginProto = envoy::config::filter::http::resty::v2::Plugin;
  using StreamHandleRef = Filters::Common::Lua::LuaDeathRef<RestyHandleWrapper>; 

  RestyPluginManager(const RestyEnablePlugins& enablePluginList,Upstream::ClusterManager& clusterManager);
  virtual ~RestyPluginManager();

  Http::FilterHeadersStatus intToHeaderStatus(uint32_t intStatus);
  Http::FilterDataStatus intToDataStatus(uint32_t intStatus);
  Http::FilterTrailersStatus intToTrailerStatus(uint32_t intStatus);

  bool isStopIteration(uint32_t status);

public:
  bool checkPluginSchema();
  bool initAllPlugin();
  void scriptError(const Filters::Common::Lua::LuaException& e);
  virtual void scriptLog(spdlog::level::level_enum level, const char* message);

  std::shared_ptr<Table> pluginConfigToTable(ScriptAction* sa, const RestyPluginProto& p);
  bool doStep(StreamHandleRef& handle,Step step, uint32_t& status);

public:
  Http::FilterHeadersStatus doDecodeHeaders(StreamHandleRef& handle,Http::HeaderMap& headers, bool end_stream);
  Http::FilterDataStatus doDecodeData(StreamHandleRef& handle,Buffer::Instance& data, bool end_stream);
  Http::FilterTrailersStatus doDecodeTrailers(StreamHandleRef& handle,Http::HeaderMap& trailers);

  Http::FilterHeadersStatus doEncodeHeaders(StreamHandleRef& handle,Http::HeaderMap& headers, bool end_stream);
  Http::FilterDataStatus doEncodeData(StreamHandleRef& handle,Buffer::Instance& data, bool end_stream);
  Http::FilterTrailersStatus doEncodeTrailers(StreamHandleRef& handle,Http::HeaderMap& trailers);

  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) {
    decoder_callbacks_.callbacks_ = &callbacks;
    decoder_callbacks_.parent_ = filter_;
  }

  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) {
    encoder_callbacks_.callbacks_ = &callbacks;
    encoder_callbacks_.parent_ = filter_;
  }

  Upstream::ClusterManager& clusterManager() { return cluster_manager_; }

  Filter* getFilter() { return filter_; }
  void setFilter(Filter* filter) { filter_ = filter; }

public:
  Upstream::ClusterManager& cluster_manager_;
private:
  RestyPluginMap restyPluginMap_;
  const RestyEnablePlugins enable_plugin_list_;
  //Http::StreamDecoderFilterCallbacks* decoder_callbacks_;
  //Http::StreamEncoderFilterCallbacks* encoder_callbacks_;
  DecoderCallbacks decoder_callbacks_{NULL};
  EncoderCallbacks encoder_callbacks_{NULL};

  Filter* filter_{};

};

using RestyPluginManagerConstSharedPtr = std::shared_ptr<RestyPluginManager>;

} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
