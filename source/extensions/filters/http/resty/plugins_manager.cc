#include "extensions/filters/http/resty/plugins_manager.h"

#include <google/protobuf/struct.pb.h>

#include "common/common/enum_to_int.h"
#include "envoy/upstream/cluster_manager.h"

#include "extensions/filters/common/lua/lua.h"
#include "extensions/filters/common/lua/utility.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

RestyPluginManager::RestyPluginManager(const RestyEnablePlugins& enablePluginList,Upstream::ClusterManager& cluster_manager)
    : cluster_manager_(cluster_manager),enable_plugin_list_(enablePluginList)
{}

RestyPluginManager::~RestyPluginManager() {}

std::shared_ptr<Table> RestyPluginManager::pluginConfigToTable(ScriptAction* sa,
                                                               const RestyPluginProto& p) {
  Table* table = sa->newNullTable();
  std::shared_ptr<Table> tablePtr(table);
  if (p.has_config()) {
    /// change struct to Table (lua)
    Filters::Common::Lua::Utility::protobufStatus2LuaTable(p.config(), table, sa);
  }
  return tablePtr;
}

bool RestyPluginManager::checkPluginSchema() {
  auto tid = std::this_thread::get_id();
  ScriptAction* sa = gScriptAction.getThreadScriptAction(tid);

  if (sa == NULL) {
    ENVOY_LOG(error, "checkPluginSchema error: not found ScriptAction by threadId-{}", tid);
    return false;
  }

  int size = enable_plugin_list_.plugins_size();
  ENVOY_LOG(info, "try to checkPluginSchema---size:{}", size);
  int i(0);
  for (; i < size; ++i) {
    auto p = enable_plugin_list_.plugins(i);
    ENVOY_LOG(debug, "try to enable plugin: {}", p.name());
    auto table = pluginConfigToTable(sa, p);
    if (!sa->checkPluginSchema(p.name(), *table))
      return false;
  }
  return true;
}

bool RestyPluginManager::initAllPlugin() {
  auto tid = std::this_thread::get_id();
  ScriptAction* sa = gScriptAction.getThreadScriptAction(tid);

  if (sa == NULL) {
    ENVOY_LOG(error, "initAllPlugin error: not found ScriptAction by threadId-{}", tid);
    return false;
  }

  int size = enable_plugin_list_.plugins_size();
  ENVOY_LOG(info, "try to initAllPlugin---size:{}", size);
  // TODO thread safe??
  int i(0);
  for (; i < size; ++i) {
    auto p = enable_plugin_list_.plugins(i);
    ENVOY_LOG(debug, "try to enable plugin: {}", p.name());
    auto p_config_table = pluginConfigToTable(sa, p);
    if (!sa->initPlugin(p.name(), *p_config_table))
      return false;
  }
  return true;
}

bool RestyPluginManager::doStep(StreamHandleRef& handle,Step step, uint32_t& returnStatus) {
  auto tid = std::this_thread::get_id();
  ScriptAction* sa = gScriptAction.getThreadScriptAction(tid);

  if (sa == NULL) {
    ENVOY_LOG(error, "doStep:{} error: not found ScriptAction by threadId-{}", step, tid);
    return false;
  }

  int size = enable_plugin_list_.plugins_size();
  int i(0);
  for (; i < size; ++i) {
    auto p = enable_plugin_list_.plugins(i);
    auto p_config_table = pluginConfigToTable(sa, p);
    uint32_t intStatus(0);
    if (!sa->doScriptStep(step,handle.get(), p.name(), *p_config_table,intStatus)) {
      ENVOY_LOG(error, "doScriptStep excute error");
      return false;
    }
    returnStatus = intStatus;
    if (isStopIteration(intStatus)) {
      ENVOY_LOG(debug, "--- return StopIteration");
      break;
    } else {
      ENVOY_LOG(debug, "Step:{} --- return {} ", step, intStatus);
    }
  }
  return true;
}

void RestyPluginManager::scriptError(const Filters::Common::Lua::LuaException& e) {
  scriptLog(spdlog::level::err, e.what());
}

void RestyPluginManager::scriptLog(spdlog::level::level_enum level, const char* message) {
  switch (level) {
  case spdlog::level::trace:
    ENVOY_LOG(trace, "script log: {}", message);
    return;
  case spdlog::level::debug:
    ENVOY_LOG(debug, "script log: {}", message);
    return;
  case spdlog::level::info:
    ENVOY_LOG(info, "script log: {}", message);
    return;
  case spdlog::level::warn:
    ENVOY_LOG(warn, "script log: {}", message);
    return;
  case spdlog::level::err:
    ENVOY_LOG(error, "script log: {}", message);
    return;
  case spdlog::level::critical:
    ENVOY_LOG(critical, "script log: {}", message);
    return;
  case spdlog::level::off:
    NOT_IMPLEMENTED_GCOVR_EXCL_LINE;
  }
}

Http::FilterHeadersStatus RestyPluginManager::intToHeaderStatus(uint32_t intStatus) {
  if (intStatus > enumToInt(Envoy::Http::FilterHeadersStatus::Max))
    return Http::FilterHeadersStatus::StopIteration;
  return static_cast<Http::FilterHeadersStatus>(intStatus);
}

Http::FilterDataStatus RestyPluginManager::intToDataStatus(uint32_t intStatus) {
  intStatus = intStatus % 10;
  if (intStatus > enumToInt(Envoy::Http::FilterDataStatus::Max))
    return Http::FilterDataStatus::StopIterationNoBuffer;
  return static_cast<Http::FilterDataStatus>(intStatus);
}

Http::FilterTrailersStatus RestyPluginManager::intToTrailerStatus(uint32_t intStatus) {
  intStatus = intStatus % 20;
  if (intStatus > enumToInt(Envoy::Http::FilterTrailersStatus::Max))
    return Http::FilterTrailersStatus::StopIteration;
  return static_cast<Http::FilterTrailersStatus>(intStatus);
}

bool RestyPluginManager::isStopIteration(uint32_t status) {

  if (status < enumToInt(Envoy::Http::FilterHeadersStatus::Max)) {
    Http::FilterHeadersStatus s = intToHeaderStatus(status);
    return ((s == Http::FilterHeadersStatus::StopIteration) ||
            (s == Http::FilterHeadersStatus::StopAllIterationAndBuffer) ||
            (s == Http::FilterHeadersStatus::StopAllIterationAndWatermark));
  }
  if (status >= 10 && status < 20) {
    Http::FilterDataStatus s = intToDataStatus(status);
    return ((s == Http::FilterDataStatus::StopIterationAndBuffer) ||
            (s == Http::FilterDataStatus::StopIterationAndWatermark) ||
            (s == Http::FilterDataStatus::StopIterationNoBuffer));
  }
  if (status >= 20) {
    Http::FilterTrailersStatus s = intToTrailerStatus(status);
    return ((s == Http::FilterTrailersStatus::StopIteration));
  }
  ENVOY_LOG(error, "isStopIteration -- unknow status:{}", status);
  return true;
}

///////////////////////////////////////////////////////////////////
//////////////// decode

Http::FilterHeadersStatus RestyPluginManager::doDecodeHeaders(StreamHandleRef& handle,Http::HeaderMap& headers,
                                                              bool end_stream) {
  auto route = decoder_callbacks_.callbacks_->route();
  if (!route) {
    ENVOY_LOG(debug, "not any route found");
    return Http::FilterHeadersStatus::Continue;
  }

  auto routeEntry = route->routeEntry();
  if (!routeEntry) {
    ENVOY_LOG(debug, "not any routeEntry found");
    return Http::FilterHeadersStatus::Continue;
  }

  Http::FilterHeadersStatus status = Http::FilterHeadersStatus::Continue;
  uint32_t intStatus(0);
  ENVOY_LOG(info, "RestyPluginManager -- doDecodeHeaders");

  auto tid = std::this_thread::get_id();
  ScriptAction* sa = gScriptAction.getThreadScriptAction(tid);

  if (sa == NULL) {
    ENVOY_LOG(error, "doDecodeHeaders  error: not found ScriptAction by threadId-{}", tid);
    return Http::FilterHeadersStatus::Continue;
  }

  Filters::Common::Lua::CoroutinePtr coroutine = sa->createCoroutine();
  handle.reset(RestyHandleWrapper::createUsingSpecifiedName("RestyHandleWrapper",coroutine->luaState(), *coroutine, headers, end_stream,this, decoder_callbacks_), false);
  //handle.reset(RestyHandleWrapper::create(coroutine->luaState(), *coroutine, headers, end_stream,this, decoder_callbacks_), true);
  
  RestyHandleWrapper* resty_handle_wrapper = handle.get();

  ENVOY_LOG(debug,"resty_handle_wrapper addr: {}",static_cast<void*>(resty_handle_wrapper));

  if (!doStep(handle,Step::DO_DECODE_HEADER, intStatus)) {
    ENVOY_LOG(error, "doStep --- DO_DECODE_HEADER -- error: {}",
              "excute script error,please see more in log");
    return Http::FilterHeadersStatus::StopIteration;
  }

  ENVOY_LOG(debug, "doDecodeHeaders -- status:{}", intStatus);

  return intToHeaderStatus(intStatus);
}

Http::FilterDataStatus RestyPluginManager::doDecodeData(StreamHandleRef& handle,Buffer::Instance& data, bool end_stream) {
  uint32_t intStatus(0);
  ENVOY_LOG(info, "RestyPluginManager -- doDecodeData");
  if (!doStep(handle,Step::DO_DECODE_DATA, intStatus)) {
    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
  return intToDataStatus(intStatus);
}

Http::FilterTrailersStatus RestyPluginManager::doDecodeTrailers(StreamHandleRef& handle,Http::HeaderMap& trailers) {
  uint32_t intStatus(0);
  ENVOY_LOG(info, "RestyPluginManager -- doDecodeTrailers");
  if (!doStep(handle,Step::DO_DECODE_TRAILERS, intStatus)) {
    return Http::FilterTrailersStatus::StopIteration;
  }
  return intToTrailerStatus(intStatus);
}

/////////////////encode
Http::FilterHeadersStatus RestyPluginManager::doEncodeHeaders(StreamHandleRef& handle,Http::HeaderMap& headers,
                                                              bool end_stream) {
  uint32_t intStatus(0);
  ENVOY_LOG(info, "RestyPluginManager -- doEncodeHeaders");
  if (!doStep(handle,Step::DO_ENCODE_HEADER, intStatus)) {
    return Http::FilterHeadersStatus::StopIteration;
  }
  return intToHeaderStatus(intStatus);
}

Http::FilterDataStatus RestyPluginManager::doEncodeData(StreamHandleRef& handle,Buffer::Instance& data, bool end_stream) {
  uint32_t intStatus(0);
  ENVOY_LOG(info, "RestyPluginManager -- doEncodeData");
  if (!doStep(handle,Step::DO_ENCODE_DATA, intStatus)) {
    return Http::FilterDataStatus::StopIterationNoBuffer;
  }
  return intToDataStatus(intStatus);
}

Http::FilterTrailersStatus RestyPluginManager::doEncodeTrailers(StreamHandleRef& handle,Http::HeaderMap& trailers) {
  uint32_t intStatus(0);
  ENVOY_LOG(info, "RestyPluginManager -- doEncodeTrailers");
  if (!doStep(handle,Step::DO_ENCODE_TRAILERS, intStatus)) {
    return Http::FilterTrailersStatus::StopIteration;
  }
  return intToTrailerStatus(intStatus);
}

} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
