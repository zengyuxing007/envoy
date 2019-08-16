#include "extensions/filters/http/resty/plugins_manager.h"
#include "extensions/filters/common/lua/lua.h"
#include <google/protobuf/struct.pb.h>

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {


    RestyPluginManager::RestyPluginManager(const RestyEnablePlugins& enablePluginList):enable_plugin_list_(&enablePluginList)
    {
    }

    RestyPluginManager::~RestyPluginManager() {

    }

    bool RestyPluginManager::initAllPlugin()
    {
        //check config if is valid
        auto tid = std::this_thread::get_id();
        ScriptAction* sa = Envoy::Extensions::Filters::Common::Lua::gScriptAction.getThreadScriptAction(tid);

        if(sa == NULL){
            ENVOY_LOG(error,"initAllPlugin error: not found ScriptAction by threadId-{}",tid);
            return false;
        }

        int size = enable_plugin_list_->plugins_size();
        ENVOY_LOG(info,"try to initAllPlugin---size:{}",size);
        int i = 0;
        // thread safe??
        //auto plugins = enable_plugin_list_->mutable_plugins();
        //for(auto &p: plugins ) {

        {
            auto p = enable_plugin_list_->plugins(0);
            //envoy::config::filter::http::resty::v2::Plugin* p_ptr = enable_plugin_list_->mutable_plugins(i);
            //const envoy::config::filter::http::resty::v2::Plugin& p = *p_ptr;
            //const envoy::config::filter::http::resty::v2::Plugin& p = enable_plugin_list_->mutable_plugins(i);

            ENVOY_LOG(debug,"try to enable plugin: {}",p.name());
            if(p.has_config())
            {
                auto p_config = p.config();
                ///change struct to Table (lua)
                ENVOY_LOG(debug,"plugin config fields size:{}",p_config.fields_size());

                auto fields_map = p_config.fields();
                std::map<std::string,std::string> config_map;
                for(auto &iter: fields_map) {
                    config_map[iter.first] = iter.second.string_value();
                }
                Table p_config_table = sa->setMapTable(config_map);
                sa->initPlugin(p.name(),p_config_table);
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



    ///////////////////////////////////////////////////////////////////
    //////////////// decode

    Http::FilterHeadersStatus RestyPluginManager::doDecodeHeaders(Http::HeaderMap& headers, bool end_stream) {
        Http::FilterHeadersStatus status = Http::FilterHeadersStatus::Continue;

        try {
           // status = resty_plugin_manager_->doDecodeHeaders();
           ENVOY_LOG(info,"RestyPluginManager -- doDecodeHeaders");
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }

    Http::FilterDataStatus RestyPluginManager::doDecodeData(Buffer::Instance& data, bool end_stream) {
        Http::FilterDataStatus status = Http::FilterDataStatus::Continue;

        try {
            //status = resty_pluginn_manager_->onData(data, end_stream);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }

    Http::FilterTrailersStatus RestyPluginManager::doDecodeTrailers(Http::HeaderMap& trailers) {
        Http::FilterTrailersStatus status = Http::FilterTrailersStatus::Continue;
        try {
            //status = resty_pluginn_manager_->onDecodeTrailers(trailers);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }




    /////////////////encode

    Http::FilterHeadersStatus RestyPluginManager::doEncodeHeaders(Http::HeaderMap& headers, bool end_stream) {
        Http::FilterHeadersStatus status = Http::FilterHeadersStatus::Continue;

        try {
            // status = resty_plugin_manager_->doEncodeHeaders();
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }

    Http::FilterDataStatus RestyPluginManager::doEncodeData(Buffer::Instance& data, bool end_stream) {
        Http::FilterDataStatus status = Http::FilterDataStatus::Continue;

        try {
            //status = resty_pluginn_manager_->onData(data, end_stream);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }

    Http::FilterTrailersStatus RestyPluginManager::doEncodeTrailers(Http::HeaderMap& trailers) {
        Http::FilterTrailersStatus status = Http::FilterTrailersStatus::Continue;
        try {
            //status = resty_pluginn_manager_->onEncodeTrailers(trailers);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }



} //namespace Resty
} //namespace HttpFilters
} //namespace Extensions
} //namespace Envoy
