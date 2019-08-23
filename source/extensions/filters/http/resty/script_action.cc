#include "extensions/filters/http/resty/script_action.h"
#include "extensions/filters/common/lua/lua.h"
#include "extensions/filters/common/lua/wrappers.h"
#include "extensions/filters/http/resty/wrappers.h"
#include "envoy/http/codes.h"


namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {


ScriptAction gScriptAction;

ScriptAction::ScriptAction():_threadId(0)
{
    _threadScriptActionMap.clear();
}

ScriptAction::ScriptAction(int64_t threadId):_threadId(threadId)
{
    _threadScriptActionMap.clear();
}

ScriptAction::~ScriptAction()
{
    _threadId = 0;
    CLEAR_MAP_DATA(_threadScriptActionMap);
}

bool ScriptAction::init( const std::string& path )
{
    ENVOY_LOG(debug,"ScriptAction::init --");
    Script::init( path );
    _path = path;
    return true;
}

void ScriptAction::unInit()
{
    Script::unInit();
    _path = "";
}

void ScriptAction::registerActionInterface()
{

    CLASS_ADD(ScriptAction);
    CLASS_DEF(ScriptAction, scriptLog);
    CLASS_DEF(ScriptAction, directResponse);
    CLASS_DEF(ScriptAction, direct200Response);
    lua_tinker::set( _L, "_ScriptAction", this );

    //CLASS_ADD(Http::LowerCaseString);
    //CLASS_ADD(Http::HeaderMap);
    //CLASS_DEF(Http::HeaderMap,byteSize);
    //CLASS_DEF(Http::HeaderMap,get);
    //CLASS_DEF(Http::HeaderMap,addCopy);
    //CLASS_DEF(Http::HeaderMap,setReferenceKey);

    /*
    CLASS_ADD(Envoy::Extensions::HttpFilters::Resty::RestyHeaderMap);
    CLASS_DEF(Envoy::Extensions::HttpFilters::Resty::RestyHeaderMap,get);
    CLASS_DEF(Envoy::Extensions::HttpFilters::Resty::RestyHeaderMap,add);
    CLASS_DEF(Envoy::Extensions::HttpFilters::Resty::RestyHeaderMap,remove);
    CLASS_DEF(Envoy::Extensions::HttpFilters::Resty::RestyHeaderMap,replace);
    */

    /*
    _L->registerType<Filters::Common::Lua::BufferWrapper>();
    _L->registerType<Filters::Common::Lua::MetadataMapWrapper>();
    _L->registerType<Filters::Common::Lua::MetadataMapIterator>();
    _L->registerType<Filters::Common::Lua::ConnectionWrapper>();
    _L->registerType<Filters::Common::Lua::SslConnectionWrapper>();
    _L->registerType<HeaderMapWrapper>();
    _L->registerType<HeaderMapIterator>();
    _L->registerType<StreamInfoWrapper>();
    _L->registerType<DynamicMetadataMapWrapper>();
    _L->registerType<DynamicMetadataMapIterator>();
    //_L->registerType<StreamHandleWrapper>();
    _L->registerType<PublicKeyWrapper>();
    */

}


void ScriptAction::createThreadScriptAction(int64_t threadId){
    ENVOY_LOG(info,"int64 threadid:{}",threadId);
    const std::thread::id& thread = std::this_thread::get_id();

    ScriptAction* sa = new ScriptAction(threadId);
    sa->init(_path);

    {
        Thread::LockGuard guard(_sa_lock);
        _threadScriptActionMap[thread] = sa;
    }
}

ScriptAction* ScriptAction::getThreadScriptAction(const std::thread::id& threadId) {

    {
        Thread::LockGuard guard(_sa_lock);
        if(_threadScriptActionMap.find(threadId) != _threadScriptActionMap.end())
            return _threadScriptActionMap[threadId];
    }
    ENVOY_LOG(debug,"not found thread[{}] script action",threadId);
    return NULL;
}

void ScriptAction::scriptLog(int level, const char * msg ) {
    switch(level){
        case 0:
            ENVOY_LOG( trace,"[thread-{}]: {}",_threadId,msg );
            break;
        case 1:
            ENVOY_LOG( debug,"[thread-{}]: {}",_threadId,msg );
            break;
        case 2:
            ENVOY_LOG( info,"[thread-{}]: {}",_threadId,msg );
            break;
        case 3:
            ENVOY_LOG( warn,"[thread-{}]: {}",_threadId,msg );
            break;
        case 4:
            ENVOY_LOG( error,"[thread-{}]: {}",_threadId,msg );
            break;
        case 5:
            ENVOY_LOG( critical,"[thread-{}]: {}",_threadId,msg );
            break;
        default:
            ENVOY_LOG( info,"[thread-{}]: {}",_threadId,msg );
            break;
    }
}


bool ScriptAction::checkPluginSchema(const std::string& name,Table& config){

    bool result(false);
    try {
        ENVOY_LOG(debug,"ScriptAction::checkSchema[{}] invoke lua function",name);
        static char buffer[64] = "check_schema"; 
        result = Run<bool>(NULL,buffer,name.c_str(),config);
    }
    catch (const Filters::Common::Lua::LuaException& e) {
        ENVOY_LOG(error,"{} plugin config error: {}",name,e.what());
        return false;
    }
    return result;
}


bool ScriptAction::initPlugin(const std::string& name,Table& config){

    bool result(false);
    try {
        ENVOY_LOG(debug,"ScriptAction::initPlugin[{}] invoke lua init function",name);
        static char buffer[64] = "init_plugin"; 
        result = Run<bool>(NULL,buffer,name.c_str(),config);
    }
    catch (const Filters::Common::Lua::LuaException& e) {
        ENVOY_LOG(error,"init Plugin error: {}",e.what());
        return false;
    }
    return result;
}


bool ScriptAction::doScriptStep(Step step, Envoy::Http::StreamFilterCallbacks* decoderCallback, 
        Envoy::Http::StreamFilterCallbacks* encoderCallback,const std::string& name,
        Table& config,uint32_t& status){

    ENVOY_LOG(debug,"do step {}: plugin {}",step,name);
    // 定义ScriptAction::Step
    //
    static char buffer[][64] = { "begin","init_plugin","decodeHeader","decoderData","decodeTrailers","END_DECODE"
                                  "encodeHeader","encodeData","encodeTrailers"
                                };
    int i = step;

    Envoy::Http::StreamFilterCallbacks* stream;
    if (step < END_DECODE)
        stream = decoderCallback;
    else 
        stream = encoderCallback;

    Envoy::Http::StreamFilterCallbacks * old_stream = _stream;

    bool result(false);
    try {
        _stream = stream;
        status = Run<uint32_t>(stream,buffer[i],name.c_str(),config);
        ENVOY_LOG(debug,"run script function: {}, return status:{}",buffer[i],status);
        result = true;
    }
    catch (const Filters::Common::Lua::LuaException& e) {
        ENVOY_LOG(error,"Plugin error: {}",e.what());
        _stream = old_stream;
        return false;
    }
    _stream = old_stream;
    return result;
}


bool ScriptAction::directResponse(Http::Code& error_code,const char* body) {

    Envoy::Http::StreamDecoderFilterCallbacks* stream = dynamic_cast<Envoy::Http::StreamDecoderFilterCallbacks*>(_stream);
    stream->sendLocalReply(error_code, body, nullptr,absl::nullopt, "");
    return true;
}

bool ScriptAction::direct200Response(const char* body) {
    Envoy::Http::StreamDecoderFilterCallbacks* stream = dynamic_cast<Envoy::Http::StreamDecoderFilterCallbacks*>(_stream);
    stream->sendLocalReply(Http::Code::OK, body, nullptr,absl::nullopt, "");
    return true;
}




} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
