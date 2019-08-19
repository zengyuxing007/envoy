template <typename T, template <typename E, typename = std::allocator<E> > class Cont>
inline Table Script::setVecTable( const Cont<T>& value )
{
    Table table( _L );
    typename Cont<T>::const_iterator cit = value.begin();
    for ( ; cit != value.end(); ++ cit )
        table.put<T>( *cit );
    return table;
}

template <typename T, template <typename E, typename = std::allocator<E> > class Cont>
inline bool Script::getVecTable( Table& table, Cont<T>& value )
{
    size_t count = table.size();
    for ( size_t i = 1; i <= count; ++ i )
        value.push_back( table.fetch<T>(i) );
    return true;
}

template <typename T, template <typename E, typename = std::less<E>, typename = std::allocator<E> > class Cont>
inline Table Script::setSetTable( const Cont<T>& value )
{
    Table table( _L );
    typename Cont<T>::const_iterator cit = value.begin();
    for ( ; cit != value.end(); ++ cit )
        table.put<T>( *cit );
    return table;
}

template <typename T, template <typename E, typename = std::less<E>, typename = std::allocator<E> > class Cont>
inline bool Script::getSetTable( Table& table, Cont<T>& value )
{
    size_t count = table.size();
    for ( size_t i = 1; i <= count; ++ i )
        value.insert( table.fetch<T>( i ) );
    return true;
}

template <typename T1, typename T2, template <typename E1, typename E2, typename = std::less<E1>, typename = std::allocator<std::pair<const E1, E2> > > class Cont >
inline Table Script::setMapTable( const Cont<T1, T2>& value )
{
    Table table( _L );
    typename Cont<T1, T2>::const_iterator cit = value.begin();
    for ( ; cit != value.end(); ++ cit )
    {
        Table elem( _L );
        elem.put<T1>( cit->first );
        elem.put<T2>( cit->second );
        table.put( elem );
    }
    return table;
}


template <typename T1, typename T2, template <typename E1, typename E2, typename = std::less<E1>, typename = std::allocator<std::pair<const E1, E2> > > class Cont >
inline void Script::setMapTable(Table& table, const Cont<T1, T2>& value )
{
    typename Cont<T1, T2>::const_iterator cit = value.begin();
    for ( ; cit != value.end(); ++ cit )
    {
        Table elem( _L );
        elem.put<T1>( cit->first );
        elem.put<T2>( cit->second );
        table.put( elem );
    }
}



template <typename T1, typename T2, template <typename E1, typename E2, typename = std::less<E1>, typename = std::allocator<std::pair<const E1, E2> > > class Cont>
inline bool Script::getMapTable( Table& table, Cont<T1, T2>& value )
{
    size_t count = table.size();
    for ( size_t i = 1; i <= count; ++ i )
    {
        Table elem = table.fetch<Table>( i );
        if ( elem.size() == 2 )
            value.insert( std::make_pair( elem.fetch<T1>(1), elem.fetch<T2>(2) ) );
    }
    return true;
}


template <typename T1>
inline Table Script::setRawTable( const T1& t1 )
{
    Table table( _L );
    table.put<T1>( t1 );
    return table;
}

template <typename T1, typename T2>
inline Table Script::setRawTable( const T1& t1, const T2& t2 )
{
    Table table( _L );
    table.put<T1>( t1 );
    table.put<T2>( t2 );
    return table;
}

template <typename T1, typename T2, typename T3>
inline Table Script::setRawTable( const T1& t1, const T2& t2, const T3& t3 )
{
    Table table( _L );
    table.put<T1>( t1 );
    table.put<T2>( t2 );
    table.put<T3>( t3 );
    return table;
}

template <typename T1, typename T2, typename T3, typename T4>
inline Table Script::setRawTable( const T1& t1, const T2& t2, const T3& t3, const T4& t4 )
{
    Table table( _L );
    table.put<T1>( t1 );
    table.put<T2>( t2 );
    table.put<T3>( t3 );
    table.put<T4>( t4 );
    return table;
}

template <typename T1>
inline bool Script::getRawTable( Table& table, T1& t1 )
{
    if ( table.size() < 1 )
        return false;
    t1 = table.fetch<T1>( 1 );
    return true;
}

template <typename T1, typename T2>
inline bool Script::getRawTable( Table& table, T1& t1, T2& t2 )
{
    if ( table.size() < 2 )
        return false;
    t1 = table.fetch<T1>( 1 );
    t2 = table.fetch<T2>( 2 );
    return true;
}

template <typename T1, typename T2, typename T3>
inline bool Script::getRawTable( Table& table, T1& t1, T2& t2, T3& t3 )
{
    if ( table.size() < 3 )
        return false;
    t1 = table.fetch<T1>( 1 );
    t2 = table.fetch<T2>( 2 );
    t3 = table.fetch<T3>( 3 );
    return true;
}

template <typename T1, typename T2, typename T3, typename T4>
inline bool Script::getRawTable( Table& table, T1& t1, T2& t2, T3& t3, T4& t4 )
{
    if ( table.size() < 4 )
        return false;
    t1 = table.fetch<T1>( 1 );
    t2 = table.fetch<T2>( 2 );
    t3 = table.fetch<T3>( 3 );
    t4 = table.fetch<T4>( 4 );
    return true;   
}

