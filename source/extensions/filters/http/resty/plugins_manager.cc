#include "extensions/filters/http/resty/plugins_manager.h"
#include "extensions/filters/common/lua/lua.h"
#include <google/protobuf/struct.pb.h>

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {


    RestyPluginManager::RestyPluginManager(const RestyEnablePlugins& enablePluginList):enable_plugin_list_(enablePluginList)
    {
    }

    RestyPluginManager::~RestyPluginManager() {

    }

    bool RestyPluginManager::initAllPlugin()
    {
        auto tid = std::this_thread::get_id();
        ScriptAction* sa = Envoy::Extensions::Filters::Common::Lua::gScriptAction.getThreadScriptAction(tid);

        if(sa == NULL){
            ENVOY_LOG(error,"initAllPlugin error: not found ScriptAction by threadId-{}",tid);
            return false;
        }

        int size = enable_plugin_list_.plugins_size();
        ENVOY_LOG(info,"try to initAllPlugin---size:{}",size);
        //TODO thread safe??
        int i(0);
        for( ; i < size; ++i ) {
            auto p = enable_plugin_list_.plugins(i);
            ENVOY_LOG(debug,"try to enable plugin: {}",p.name());
            Table p_config_table = sa->newNullTable();
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
                sa->setMapTable(p_config_table,config_map);
            }
            sa->initPlugin(p.name(),p_config_table);
        }
        return true;
    }

    bool RestyPluginManager::doStep(ScriptAction::Step step,int& returnStatus)
    {
        auto tid = std::this_thread::get_id();
        ScriptAction* sa = Envoy::Extensions::Filters::Common::Lua::gScriptAction.getThreadScriptAction(tid);

        int status(0);

        if(sa == NULL){
            ENVOY_LOG(error,"doStep:{} error: not found ScriptAction by threadId-{}",step,tid);
            status = 1;
            return false;
        }

        int size = enable_plugin_list_.plugins_size();
        int i(0);
        for( ;i < size; ++i ) {
            auto p = enable_plugin_list_.plugins(i);
            Table p_config_table = sa->newNullTable();
            if(p.has_config())
            {
                auto p_config = p.config();
                ///change struct to Table (lua)
                ENVOY_LOG(debug,"plugin config fields size:{}",p_config.fields_size());

                auto fields_map = p_config.fields();
                for(auto &iter: fields_map) {
                    p_config_table.set(iter.first.c_str(),iter.second.string_value().c_str());
                }
            }

            int intStatus(0);
            sa->doScriptStep(step,decoder_callbacks_,encoder_callbacks_,p.name(),p_config_table,intStatus);
            if(intStatus == 0)
                continue;
            else
                break;
        }
        returnStatus = status;
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

        auto route = decoder_callbacks_->route();

        //TODO
        if(!route){
            ENVOY_LOG(debug,"not any route found");  
            return Http::FilterHeadersStatus::Continue;
        }

        auto routeEntry = route->routeEntry(); 
        if (!routeEntry) {  
            ENVOY_LOG(debug,"not any routeEntry found");  
            return Http::FilterHeadersStatus::Continue;
        }

        Http::FilterHeadersStatus status = Http::FilterHeadersStatus::Continue;
        int intStatus(0);
        ENVOY_LOG(info,"RestyPluginManager -- doDecodeHeaders");

        try {
           doStep(ScriptAction::Step::DO_DECODE_HEADER,intStatus);

        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
            status = Http::FilterHeadersStatus::StopIteration;
        }

        if(intStatus) {
            //// TODO
        }
        else{
        }

        return status;
    }

    Http::FilterDataStatus RestyPluginManager::doDecodeData(Buffer::Instance& data, bool end_stream) {
        Http::FilterDataStatus status = Http::FilterDataStatus::Continue;
        int intStatus(0);

        ENVOY_LOG(info,"RestyPluginManager -- doDecodeData");
        try {
           doStep(ScriptAction::Step::DO_DECODE_DATA,intStatus);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
            //TODO
            status = Http::FilterDataStatus::StopIterationNoBuffer;
        }
        return status;
    }

    Http::FilterTrailersStatus RestyPluginManager::doDecodeTrailers(Http::HeaderMap& trailers) {
        Http::FilterTrailersStatus status = Http::FilterTrailersStatus::Continue;
        int intStatus(0);
        ENVOY_LOG(info,"RestyPluginManager -- doDecodeTrailers");
        try {
           doStep(ScriptAction::Step::DO_DECODE_TRAILERS,intStatus);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
            status = Http::FilterTrailersStatus::StopIteration;
        }

        return status;
    }




    /////////////////encode

    Http::FilterHeadersStatus RestyPluginManager::doEncodeHeaders(Http::HeaderMap& headers, bool end_stream) {
        Http::FilterHeadersStatus status = Http::FilterHeadersStatus::Continue;
        int intStatus(0);
        ENVOY_LOG(info,"RestyPluginManager -- doEncodeHeaders");

        try {
            doStep(ScriptAction::Step::DO_ENCODE_HEADER,intStatus);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }

    Http::FilterDataStatus RestyPluginManager::doEncodeData(Buffer::Instance& data, bool end_stream) {
        Http::FilterDataStatus status = Http::FilterDataStatus::Continue;

        int intStatus(0);
        ENVOY_LOG(info,"RestyPluginManager -- doEncodeData");
        try {
            doStep(ScriptAction::Step::DO_ENCODE_DATA,intStatus);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }

    Http::FilterTrailersStatus RestyPluginManager::doEncodeTrailers(Http::HeaderMap& trailers) {
        Http::FilterTrailersStatus status = Http::FilterTrailersStatus::Continue;
        int intStatus(0);
        ENVOY_LOG(info,"RestyPluginManager -- doEncodeTrailers");
        try {
            doStep(ScriptAction::Step::DO_ENCODE_TRAILERS,intStatus);
        } catch (const Filters::Common::Lua::LuaException& e) {
            scriptError(e);
        }

        return status;
    }



} //namespace Resty
} //namespace HttpFilters
} //namespace Extensions
} //namespace Envoy
