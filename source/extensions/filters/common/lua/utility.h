#pragma once

#include "common/protobuf/utility.h"

#include "extensions/filters/common/lua/lua_tinker.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Lua {

class Script;

class Utility {
public:
  static void protobufListValue2LuaTable(const ProtobufWkt::ListValue& listValue,
                                         ::lua_tinker::table* table, Script* sa);
  static void protobufStatus2LuaTable(const ProtobufWkt::Struct& s1, ::lua_tinker::table* t1,
                                      Script* sa);
};

} // namespace Lua
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
