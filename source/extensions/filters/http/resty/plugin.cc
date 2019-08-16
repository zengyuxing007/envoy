#include "extensions/filters/http/resty/plugin.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {



RestyPlugin::RestyPlugin(std::string& name,const RestyPluginConfig* plugin_config):
    name_(name),plugin_config_(plugin_config)
{
    ENVOY_LOG(debug,"constructor RestyPlugin: {}",name);
}


RestyPlugin::~RestyPlugin() {
}

bool RestyPlugin::decodeHeaders(bool end_stream)
{
    ENVOY_LOG(debug,"plugin {} - decodeHeaders",name_);
    return true;
}




//namespace end
}
}
}
}


