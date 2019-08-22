#include "extensions/filters/common/lua/utility.h"
#include "extensions/filters/common/lua/lua_tinker.h"
#include "extensions/filters/common/lua/script_action.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Lua {


 
void Utility::protobufListValue2LuaTable(const ProtobufWkt::ListValue& listValue,::Table* table,ScriptAction* sa) {

        if(listValue.values_size() <1) return;

        auto kind = listValue.values(0).kind_case();

        switch(kind) {
            case ProtobufWkt::Value::KIND_NOT_SET:
            case ProtobufWkt::Value::kNullValue:
                return;
            case ProtobufWkt::Value::kNumberValue:
                for (int i = 0; i < listValue.values_size(); i++) {
                    table->put(listValue.values(i).number_value());
                }
                return;
           case ProtobufWkt::Value::kStringValue:
                for (int i = 0; i < listValue.values_size(); i++) {
                    table->put(listValue.values(i).string_value().c_str());
                }
                return;
           case ProtobufWkt::Value::kBoolValue:
                for (int i = 0; i < listValue.values_size(); i++) {
                    table->put(listValue.values(i).bool_value());
                }
                return;
           case ProtobufWkt::Value::kStructValue: {
                for (int i = 0; i < listValue.values_size(); i++) {
                    const ProtobufWkt::Struct& tmpStruct = listValue.values(i).struct_value();
                    auto tmpTable = sa->newNullTable();
                    protobufStatus2LuaTable(tmpStruct,tmpTable,sa);
                    table->put(*tmpTable);
                    delete tmpTable;
                }
                return;
           }
           case ProtobufWkt::Value::kListValue: {
                for (int i = 0; i < listValue.values_size(); i++) {
                    const ProtobufWkt::ListValue& tmpList = listValue.values(i).list_value();
                    auto tmpTable = sa->newNullTable();
                    protobufListValue2LuaTable(tmpList,tmpTable,sa);
                    table->put(*tmpTable);
                    delete tmpTable;
                }
                return;
           }
           default:
                NOT_REACHED_GCOVR_EXCL_LINE;

        }
}


void Utility::protobufStatus2LuaTable(const ProtobufWkt::Struct& s1, ::Table* table,ScriptAction* sa){
      for (const auto& iter : s1.fields()) {
            auto kind = iter.second.kind_case();
            switch(kind) {
                case ProtobufWkt::Value::KIND_NOT_SET:
                case ProtobufWkt::Value::kNullValue:
                    //TODO
                    table->set(iter.first.c_str(),"");
                    break;
                case ProtobufWkt::Value::kNumberValue:
                    table->set(iter.first.c_str(),iter.second.number_value());
                    break;
                case ProtobufWkt::Value::kStringValue:
                    table->set(iter.first.c_str(),iter.second.string_value().c_str());
                    break;
                case ProtobufWkt::Value::kBoolValue:
                    table->set(iter.first.c_str(),iter.second.bool_value());
                    break;
                case ProtobufWkt::Value::kStructValue: {
                    const ProtobufWkt::Struct& tmpStruct = iter.second.struct_value();
                    auto tmpTable = sa->newNullTable();
                    protobufStatus2LuaTable(tmpStruct,tmpTable,sa);
                    table->set(iter.first.c_str(),*tmpTable);
                    delete tmpTable;
                    break;
                }
                case ProtobufWkt::Value::kListValue: {
                    const ProtobufWkt::ListValue& tmpList = iter.second.list_value();
                    auto tmpTable = sa->newNullTable();
                    protobufListValue2LuaTable(tmpList,tmpTable,sa);
                    table->set(iter.first.c_str(),*tmpTable);
                    delete tmpTable;
                    break;
                }
                default:
                    NOT_REACHED_GCOVR_EXCL_LINE;
           }
     }
}


} // namespace Lua
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy

