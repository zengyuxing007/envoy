#include "extensions/filters/common/lua/script.h"
#include <string>

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Lua {



Script::Script()
{
    _L = lua_open();
    luaL_openlibs( _L );
}

Script::~Script()
{

}

bool Script::init( const std::string& path )
{
    _path = path;
    registerActionInterface();
    lua_tinker::dofile( _L, _path.c_str() );
    return true;
}

bool Script::runScript( const std::string& buffer )
{
    registerActionInterface();
    lua_tinker::dostring( _L, buffer.c_str() );
    return true;
}

bool Script::reload()
{
    lua_close( _L );
    _L = lua_open();
    luaL_openlibs( _L );
    registerActionInterface();
    lua_tinker::dofile( _L, _path.c_str() );
    return true;
}

void Script::unInit()
{
    lua_close( _L );
    _L = NULL;
}


} // namespace Lua
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
