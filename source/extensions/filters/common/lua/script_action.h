#pragma once

#include "extensions/filters/common/lua/script.h"
#include "envoy/http/filter.h"
#include "common/common/lock_guard.h"
#include "common/common/thread.h"
#include "envoy/http/header_map.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Lua {


struct DeletePair
{
    template<typename Ty1, typename Ty2>
        inline void operator() (const std::pair<Ty1, Ty2> &ptr) const
        {
            if (ptr.second)
                delete ptr.second;
        }
};


#define CLEAR_MAP_DATA(m) \
std::for_each(m.begin(),m.end(),DeletePair()); \
m.clear();



class ScriptAction:public Script
{
public:
    ScriptAction();
    ScriptAction(int64_t threadId);
    ~ScriptAction();

    enum Step{
        INIT_PLUGIN = 1,
        DO_DECODE_HEADER = 2,
        DO_DECODE_DATA = 3,
        DO_DECODE_TRAILERS = 4,
        END_DECODE,
        DO_ENCODE_HEADER,
        DO_ENCODE_DATA,
        DO_ENCODE_TRAILERS,
    };



public:
    bool init(const std::string&);
    void unInit();

public:
    void registerActionInterface();

    void createThreadScriptAction(int64_t threadId);
    ScriptAction* getThreadScriptAction(const std::thread::id& threadId);


    bool checkPluginSchema(const std::string& name,Table& config);
    bool initPlugin(const std::string& name,Table& config);


    bool doScriptStep(Step step, Envoy::Http::StreamFilterCallbacks* decoderCallback, 
            Envoy::Http::StreamFilterCallbacks* encoderCallback,const std::string& name,
            Table& config,uint32_t& status);

    bool scriptDecodeHeaders(const std::string& name,Table& config);
    bool directResponse(Http::Code& error_code,const char* body);
    bool direct200Response(const char* body);

public:
    void scriptLog(int level,const char *);

public:
    template <typename R>
    inline R Run(Envoy::Http::StreamFilterCallbacks*, const std::string&);
    template <typename R, typename T1>
    inline R Run(Envoy::Http::StreamFilterCallbacks*, const std::string&, const T1&);
    template <typename R, typename T1, typename T2>
    inline R Run(Envoy::Http::StreamFilterCallbacks*, const std::string&, const T1&,  const T2&);
    template <typename R, typename T1, typename T2, typename T3>
    inline R Run(Envoy::Http::StreamFilterCallbacks*, const std::string&, const T1&,  const T2&, const T3&);
    template <typename R, typename T1, typename T2, typename T3, typename T4>
    inline R Run(Envoy::Http::StreamFilterCallbacks*, const std::string&, const T1&,  const T2&, const T3&, const T4&);
    template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
    inline R Run(Envoy::Http::StreamFilterCallbacks*, const std::string&, const T1&,  const T2&, const T3&, const T4&, const T5&);

protected:
    Envoy::Http::StreamFilterCallbacks * _stream;

private:
    std::map<std::thread::id,ScriptAction*> _threadScriptActionMap GUARDED_BY(_sa_lock); 
    Envoy::Thread::MutexBasicLockable _sa_lock;

    std::string _path;
    int64_t _threadId;

};

#include "extensions/filters/common/lua/script_action.inl"

extern ScriptAction gScriptAction;


} // namespace Lua
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
