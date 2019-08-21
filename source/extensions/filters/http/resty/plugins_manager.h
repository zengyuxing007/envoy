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
            
        Http::FilterHeadersStatus intToHeaderStatus(uint32_t intStatus);
        Http::FilterDataStatus intToDataStatus(uint32_t intStatus);
        Http::FilterTrailersStatus intToTrailerStatus(uint32_t intStatus);

        bool isStopIteration(uint32_t status);

    public:
        bool initAllPlugin();
        void scriptError(const Filters::Common::Lua::LuaException& e);
        virtual void scriptLog(spdlog::level::level_enum level, const char* message);

        bool doStep(ScriptAction::Step step,uint32_t& status);
    public:

        Http::FilterHeadersStatus doDecodeHeaders(Http::HeaderMap& headers, bool end_stream);
        Http::FilterDataStatus doDecodeData(Buffer::Instance& data, bool end_stream);
        Http::FilterTrailersStatus doDecodeTrailers(Http::HeaderMap& trailers);

        Http::FilterHeadersStatus doEncodeHeaders(Http::HeaderMap& headers, bool end_stream);
        Http::FilterDataStatus doEncodeData(Buffer::Instance& data, bool end_stream);
        Http::FilterTrailersStatus doEncodeTrailers(Http::HeaderMap& trailers);

        void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) {
            decoder_callbacks_ = &callbacks;
        }

        void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) {
            encoder_callbacks_ = &callbacks;
        }

    private:

        RestyPluginMap restyPluginMap_;
        const RestyEnablePlugins enable_plugin_list_;
        Http::StreamDecoderFilterCallbacks* decoder_callbacks_;
        Http::StreamEncoderFilterCallbacks* encoder_callbacks_;

};

using RestyPluginManagerConstSharedPtr = std::shared_ptr<RestyPluginManager>;

} //namespace Resty
} //namespace HttpFilters
} //namespace Extensions
} //namespace Envoy


