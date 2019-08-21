#pragma once

#include "extensions/filters/common/lua/lua_tinker.h"
#include <string>
#include "common/common/logger.h"

namespace Envoy {
namespace Extensions {
namespace Filters {
namespace Common {
namespace Lua {

class Script:public Logger::Loggable<Logger::Id::script>
{
public:
    Script();
    virtual ~Script();

public:
    virtual bool init(const std::string&);
    virtual void unInit();
    bool runScript(const std::string&);
    bool reload();
    virtual void registerActionInterface() {}

    Table* newNullTable()
    {
        Table* table = new Table(_L);
        return table;
    }


public:
    template <typename T1>
    inline Table setRawTable(const T1&);
    template <typename T1>
    inline bool getRawTable(Table&, T1&);
    template <typename T1, typename T2>
    inline Table setRawTable(const T1&, const T2&);
    template <typename T1, typename T2>
    inline bool getRawTable(Table&, T1&, T2&);
    template <typename T1, typename T2, typename T3>
    inline Table setRawTable(const T1&, const T2&, const T3&);
    template <typename T1, typename T2, typename T3>
    inline bool getRawTable(Table&, T1&, T2&, T3&);
    template <typename T1, typename T2, typename T3, typename T4>
    inline Table setRawTable(const T1&, const T2&, const T3&, const T4&);
    template <typename T1, typename T2, typename T3, typename T4>
    inline bool getRawTable(Table&, T1&, T2&, T3&, T4&);
    template <typename T, template <typename E, typename = std::less<E>, typename = std::allocator<E> > class Cont>
    inline Table setSetTable(const Cont<T>&);
    template <typename T, template <typename E, typename = std::less<E>, typename = std::allocator<E> > class Cont>
    inline bool getSetTable(Table&, Cont<T>&);
    template <typename T, template <typename E, typename = std::allocator<E> > class Cont>
    inline Table setVecTable(const Cont<T>&);
    template <typename T, template <typename E, typename = std::allocator<E> > class Cont>
    inline bool getVecTable(Table&, Cont<T>&);
    template <typename T1, typename T2, template <typename E1, typename E2, typename = std::less<E1>, typename = std::allocator<std::pair<const E1, E2> > > class Cont>
    inline Table setMapTable(const Cont<T1, T2>&);

    template <typename T1, typename T2, template <typename E1, typename E2, typename = std::less<E1>, typename = std::allocator<std::pair<const E1, E2> > > class Cont>
    inline void setMapTable(Table& table,const Cont<T1, T2>&);

    template <typename T1, typename T2, template <typename E1, typename E2, typename = std::less<E1>, typename = std::allocator<std::pair<const E1, E2> > > class Cont>
    inline bool getMapTable(Table&, Cont<T1, T2>&);

protected:
    std::string _path;
    lua_State * _L;
};

#define CLASS_DEF(klass, member)    \
    lua_tinker::class_def<klass>(_L, #member, &klass::member)
#define CLASS_ADD(klass)	\
    lua_tinker::class_add<klass>(_L, #klass);

#include "extensions/filters/common/lua/script.inl"

} // namespace Lua
} // namespace Common
} // namespace Filters
} // namespace Extensions
} // namespace Envoy
