#pragma once

#include "common/common/logger.h"
#include "envoy/http/filter.h"
#include "extensions/filters/common/lua/lua.h"
#include "extensions/filters/common/lua/lua_tinker.h"
#include "extensions/filters/http/resty/plugin.h"
#include "envoy/config/filter/http/resty/v2/resty.pb.h" 
#include "extensions/filters/common/lua/script_action.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {


class RestyPluginManager: Logger::Loggable<Logger::Id::resty> {

    public:

        using RestyEnablePlugins = envoy::config::filter::http::resty::v2::EnablePlugins ;
        using ScriptAction = Envoy::Extensions::Filters::Common::Lua::ScriptAction;

        RestyPluginManager(const RestyEnablePlugins& enablePluginList);
        virtual ~RestyPluginManager();

    public:
        bool initAllPlugin();
        void scriptError(const Filters::Common::Lua::LuaException& e);
        virtual void scriptLog(spdlog::level::level_enum level, const char* message);

    public:

        Http::FilterHeadersStatus doDecodeHeaders(Http::HeaderMap& headers, bool end_stream);
        Http::FilterDataStatus doDecodeData(Buffer::Instance& data, bool end_stream);
        Http::FilterTrailersStatus doDecodeTrailers(Http::HeaderMap& trailers);

        Http::FilterHeadersStatus doEncodeHeaders(Http::HeaderMap& headers, bool end_stream);
        Http::FilterDataStatus doEncodeData(Buffer::Instance& data, bool end_stream);
        Http::FilterTrailersStatus doEncodeTrailers(Http::HeaderMap& trailers);

    private:

        RestyPluginMap restyPluginMap_;
        const RestyEnablePlugins* enable_plugin_list_;

};

using RestyPluginManagerConstSharedPtr = std::shared_ptr<RestyPluginManager>;

} //namespace Resty
} //namespace HttpFilters
} //namespace Extensions
} //namespace Envoy


