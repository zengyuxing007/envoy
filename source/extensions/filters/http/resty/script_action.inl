template <typename R>
inline R ScriptAction::Run(Envoy::Http::StreamFilterCallbacks* stream, const std::string& script)
{
	Envoy::Http::StreamFilterCallbacks* saveStreamFilterCallbacks1 = _stream;
	_stream = stream;
	R ret = lua_tinker::call<R>(_L, script.c_str());
	_stream = saveStreamFilterCallbacks1;
	return ret;
}

template <typename R, typename T1>
inline R ScriptAction::Run(Envoy::Http::StreamFilterCallbacks* stream, const std::string& script, const T1& t1)
{
	Envoy::Http::StreamFilterCallbacks* saveStreamFilterCallbacks1 = _stream;
	_stream = stream;
	R ret = lua_tinker::call<R, T1>(_L, script.c_str(), t1);
	_stream = saveStreamFilterCallbacks1;
	return ret;
}

template <typename R, typename T1, typename T2>
inline R ScriptAction::Run(Envoy::Http::StreamFilterCallbacks* stream, const std::string& script, const T1& t1,  const T2& t2)
{
	Envoy::Http::StreamFilterCallbacks* saveStreamFilterCallbacks1 = _stream;
	_stream = stream;
	R ret = lua_tinker::call<R, T1, T2>(_L, script.c_str(), t1, t2);
	_stream = saveStreamFilterCallbacks1;
	return ret;
}

template <typename R, typename T1, typename T2, typename T3>
inline R ScriptAction::Run(Envoy::Http::StreamFilterCallbacks* stream, const std::string& script, const T1& t1,  const T2& t2, const T3& t3)
{
	Envoy::Http::StreamFilterCallbacks* saveStreamFilterCallbacks1 = _stream;
	_stream = stream;
	R ret = lua_tinker::call<R, T1, T2, T3>(_L, script.c_str(), t1, t2, t3);
	_stream = saveStreamFilterCallbacks1;
	return ret;
}

template <typename R, typename T1, typename T2, typename T3, typename T4>
inline R ScriptAction::Run(Envoy::Http::StreamFilterCallbacks* stream, const std::string& script, const T1& t1,  const T2& t2, const T3& t3, const T4& t4)
{
	Envoy::Http::StreamFilterCallbacks* saveStreamFilterCallbacks1 = _stream;
	_stream = stream;
	R ret = lua_tinker::call<R, T1, T2, T3, T4>(_L, script.c_str(), t1, t2, t3, t4);
	_stream = saveStreamFilterCallbacks1;
	return ret;
}

template <typename R, typename T1, typename T2, typename T3, typename T4, typename T5>
inline R ScriptAction::Run(Envoy::Http::StreamFilterCallbacks* stream, const std::string& script, const T1& t1,  const T2& t2, const T3& t3, const T4& t4, const T5& t5)
{
	Envoy::Http::StreamFilterCallbacks* saveStreamFilterCallbacks1 = _stream;
	_stream = stream;
	R ret = lua_tinker::call<R, T1, T2, T3, T4, T5>(_L, script.c_str(), t1, t2, t3, t4, t5);
	_stream = saveStreamFilterCallbacks1;
	return ret;
}
