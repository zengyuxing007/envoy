#include "extensions/filters/http/resty/resty_wrappers.h"
#include "extensions/filters/http/resty/resty_filter.h"
#include "common/http/header_map_impl.h"
#include "common/http/message_impl.h"
#include "common/common/enum_to_int.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {



DecoderCallbacks::DecoderCallbacks(Filter* parent):parent_(parent)
{}

DecoderCallbacks::~DecoderCallbacks() {}


EncoderCallbacks::EncoderCallbacks(Filter* parent):parent_(parent)
{}


EncoderCallbacks::~EncoderCallbacks() {}


void DecoderCallbacks::respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body,
                                       lua_State*) {
  callbacks_->encodeHeaders(std::move(headers), body == nullptr);
  if (body && !parent_->destroyed()) {
    callbacks_->encodeData(*body, true);
  }
}

void EncoderCallbacks::respond(Http::HeaderMapPtr&&, Buffer::Instance*, lua_State* state) {
  // TODO(mattklein123): Support response in response path if nothing has been continued
  // yet.
  luaL_error(state, "respond not currently supported in the response path");
}




///////

RestyHandleWrapper::RestyHandleWrapper(Filters::Common::Lua::Coroutine& coroutine,
                                         Http::HeaderMap& headers, bool end_stream, RestyPluginManager* filter,
                                         FilterCallbacks& callbacks)
    : coroutine_(coroutine), headers_(headers), end_stream_(end_stream), plugin_manager_(filter),
      callbacks_(callbacks), yield_callback_([this]() {
        if (state_ == State::Running) {
          throw Filters::Common::Lua::LuaException("script performed an unexpected yield");
        }
      }) {}

Http::FilterDataStatus RestyHandleWrapper::onData(Buffer::Instance& data, bool end_stream) {
  ASSERT(!end_stream_);
  end_stream_ = end_stream;
  saw_body_ = true;

  if (state_ == State::WaitForBodyChunk) {
    ENVOY_LOG(trace, "resuming for next body chunk");
    Filters::Common::Lua::LuaDeathRef<Filters::Common::Lua::BufferWrapper> wrapper(
        Filters::Common::Lua::BufferWrapper::create(coroutine_.luaState(), data), true);
    state_ = State::Running;
    coroutine_.resume(1, yield_callback_);
  } else if (state_ == State::WaitForBody && end_stream_) {
    ENVOY_LOG(debug, "resuming body due to end stream");
    callbacks_.addData(data);
    state_ = State::Running;
    coroutine_.resume(luaBody(coroutine_.luaState()), yield_callback_);
  } else if (state_ == State::WaitForTrailers && end_stream_) {
    ENVOY_LOG(debug, "resuming nil trailers due to end stream");
    state_ = State::Running;
    coroutine_.resume(0, yield_callback_);
  }

  if (state_ == State::HttpCall || state_ == State::WaitForBody) {
    ENVOY_LOG(trace, "buffering body");
    return Http::FilterDataStatus::StopIterationAndBuffer;
  } else if (state_ == State::Responded) {
    return Http::FilterDataStatus::StopIterationNoBuffer;
  } else {
    headers_continued_ = true;
    return Http::FilterDataStatus::Continue;
  }
}

Http::FilterTrailersStatus RestyHandleWrapper::onTrailers(Http::HeaderMap& trailers) {
  ASSERT(!end_stream_);
  end_stream_ = true;
  trailers_ = &trailers;

  if (state_ == State::WaitForBodyChunk) {
    ENVOY_LOG(debug, "resuming nil body chunk due to trailers");
    state_ = State::Running;
    coroutine_.resume(0, yield_callback_);
  } else if (state_ == State::WaitForBody) {
    ENVOY_LOG(debug, "resuming body due to trailers");
    state_ = State::Running;
    coroutine_.resume(luaBody(coroutine_.luaState()), yield_callback_);
  }

  if (state_ == State::WaitForTrailers) {
    // Mimic a call to trailers which will push the trailers onto the stack and then resume.
    state_ = State::Running;
    coroutine_.resume(luaTrailers(coroutine_.luaState()), yield_callback_);
  }

  Http::FilterTrailersStatus status = (state_ == State::HttpCall || state_ == State::Responded)
                                          ? Http::FilterTrailersStatus::StopIteration
                                          : Http::FilterTrailersStatus::Continue;

  if (status == Http::FilterTrailersStatus::Continue) {
    headers_continued_ = true;
  }

  return status;
}

int RestyHandleWrapper::luaRespond(lua_State* state) {
  ASSERT(state_ == State::Running);

  if (headers_continued_) {
    luaL_error(state, "respond() cannot be called if headers have been continued");
  }

  luaL_checktype(state, 2, LUA_TTABLE);
  size_t body_size;
  const char* raw_body = luaL_optlstring(state, 3, nullptr, &body_size);
  Http::HeaderMapPtr headers = buildHeadersFromTable(state, 2);

  uint64_t status;
  if (headers->Status() == nullptr ||
      !absl::SimpleAtoi(headers->Status()->value().getStringView(), &status) || status < 200 ||
      status >= 600) {
    luaL_error(state, ":status must be between 200-599");
  }

  Buffer::InstancePtr body;
  if (raw_body != nullptr) {
    body = std::make_unique<Buffer::OwnedImpl>(raw_body, body_size);
    headers->insertContentLength().value(body_size);
  }

  // Once we respond we treat that as the end of the script even if there is more code. Thus we
  // yield.
  callbacks_.respond(std::move(headers), body.get(), state);
  state_ = State::Responded;
  return lua_yield(state, 0);
}

Http::HeaderMapPtr RestyHandleWrapper::buildHeadersFromTable(lua_State* state, int table_index) {
  Http::HeaderMapPtr headers(new Http::HeaderMapImpl());

  // Build a header map to make the request. We iterate through the provided table to do this and
  // check that we are getting strings.
  lua_pushnil(state);
  while (lua_next(state, table_index) != 0) {
    // Uses 'key' (at index -2) and 'value' (at index -1).
    const char* key = luaL_checkstring(state, -2);
    const char* value = luaL_checkstring(state, -1);
    headers->addCopy(Http::LowerCaseString(key), value);

    // Removes 'value'; keeps 'key' for next iteration. This is the input for lua_next() so that
    // it can push the next key/value pair onto the stack.
    lua_pop(state, 1);
  }

  return headers;
}

int RestyHandleWrapper::luaHttpCall(lua_State* state) {
  ASSERT(state_ == State::Running);

  const std::string cluster = luaL_checkstring(state, 2);
  luaL_checktype(state, 3, LUA_TTABLE);
  size_t body_size;
  const char* body = luaL_optlstring(state, 4, nullptr, &body_size);
  int timeout_ms = luaL_checkint(state, 5);
  if (timeout_ms < 0) {
    return luaL_error(state, "http call timeout must be >= 0");
  }

  if (plugin_manager_->clusterManager().get(cluster) == nullptr) {
    return luaL_error(state, "http call cluster invalid. Must be configured");
  }

  Http::MessagePtr message(new Http::RequestMessageImpl(buildHeadersFromTable(state, 3)));

  // Check that we were provided certain headers.
  if (message->headers().Path() == nullptr || message->headers().Method() == nullptr ||
      message->headers().Host() == nullptr) {
    return luaL_error(state, "http call headers must include ':path', ':method', and ':authority'");
  }

  if (body != nullptr) {
    message->body() = std::make_unique<Buffer::OwnedImpl>(body, body_size);
    message->headers().insertContentLength().value(body_size);
  }

  absl::optional<std::chrono::milliseconds> timeout;
  if (timeout_ms > 0) {
    timeout = std::chrono::milliseconds(timeout_ms);
  }

  http_request_ = plugin_manager_->clusterManager().httpAsyncClientForCluster(cluster).send(
      std::move(message), *this, Http::AsyncClient::RequestOptions().setTimeout(timeout));
  if (http_request_) {
    state_ = State::HttpCall;
    return lua_yield(state, 0);
  } else {
    // Immediate failure case. The return arguments are already on the stack.
    ASSERT(lua_gettop(state) >= 2);
    return 2;
  }
}

void RestyHandleWrapper::onSuccess(Http::MessagePtr&& response) {
  ASSERT(state_ == State::HttpCall || state_ == State::Running);
  ENVOY_LOG(debug, "async HTTP response complete");
  http_request_ = nullptr;

  // We need to build a table with the headers as return param 1. The body will be return param 2.
  lua_newtable(coroutine_.luaState());
  response->headers().iterate(
      [](const Http::HeaderEntry& header, void* context) -> Http::HeaderMap::Iterate {
        lua_State* state = static_cast<lua_State*>(context);
        lua_pushlstring(state, header.key().getStringView().data(),
                        header.key().getStringView().length());
        lua_pushlstring(state, header.value().getStringView().data(),
                        header.value().getStringView().length());
        lua_settable(state, -3);
        return Http::HeaderMap::Iterate::Continue;
      },
      coroutine_.luaState());

  // TODO(mattklein123): Avoid double copy here.
  if (response->body() != nullptr) {
    lua_pushstring(coroutine_.luaState(), response->bodyAsString().c_str());
  } else {
    lua_pushnil(coroutine_.luaState());
  }

  // In the immediate failure case, we are just going to immediately return to the script. We
  // have already pushed the return arguments onto the stack.
  if (state_ == State::HttpCall) {
    state_ = State::Running;
    markLive();

    try {
      coroutine_.resume(2, yield_callback_);
      markDead();
    } catch (const Filters::Common::Lua::LuaException& e) {
      plugin_manager_->scriptError(e);
    }

    if (state_ == State::Running) {
      headers_continued_ = true;
      callbacks_.continueIteration();
    }
  }
}

void RestyHandleWrapper::onFailure(Http::AsyncClient::FailureReason) {
  ASSERT(state_ == State::HttpCall || state_ == State::Running);
  ENVOY_LOG(debug, "async HTTP failure");

  // Just fake a basic 503 response.
  Http::MessagePtr response_message(new Http::ResponseMessageImpl(Http::HeaderMapPtr{
      new Http::HeaderMapImpl{{Http::Headers::get().Status,
                               std::to_string(enumToInt(Http::Code::ServiceUnavailable))}}}));
  response_message->body() = std::make_unique<Buffer::OwnedImpl>("upstream failure");
  onSuccess(std::move(response_message));
}

int RestyHandleWrapper::luaHeaders(lua_State* state) {
  ASSERT(state_ == State::Running);

  if (headers_wrapper_.get() != nullptr) {
    headers_wrapper_.pushStack();
  } else {
    headers_wrapper_.reset(HeaderMapWrapper::create(state, headers_,
                                                    [this] {
                                                      // If we are about to do a modifiable header
                                                      // operation, blow away the route cache. We
                                                      // could be a little more intelligent about
                                                      // when we do this so the performance would be
                                                      // higher, but this is simple and will get the
                                                      // job done for now. This is a NOP on the
                                                      // encoder path.
                                                      if (!headers_continued_) {
                                                        callbacks_.onHeadersModified();
                                                      }

                                                      return !headers_continued_;
                                                    }),
                           true);
  }
  return 1;
}

int RestyHandleWrapper::luaBody(lua_State* state) {
  ASSERT(state_ == State::Running);

  if (end_stream_) {
    if (!buffered_body_ && saw_body_) {
      return luaL_error(state, "cannot call body() after body has been streamed");
    } else if (callbacks_.bufferedBody() == nullptr) {
      ENVOY_LOG(debug, "end stream. no body");
      return 0;
    } else {
      if (body_wrapper_.get() != nullptr) {
        body_wrapper_.pushStack();
      } else {
        body_wrapper_.reset(
            Filters::Common::Lua::BufferWrapper::create(state, *callbacks_.bufferedBody()), true);
      }
      return 1;
    }
  } else if (saw_body_) {
    return luaL_error(state, "cannot call body() after body streaming has started");
  } else {
    ENVOY_LOG(debug, "yielding for full body");
    state_ = State::WaitForBody;
    buffered_body_ = true;
    return lua_yield(state, 0);
  }
}

int RestyHandleWrapper::luaBodyChunks(lua_State* state) {
  ASSERT(state_ == State::Running);

  if (saw_body_) {
    luaL_error(state, "cannot call bodyChunks after body processing has begun");
  }

  // We are currently at the top of the stack. Push a closure that has us as the upvalue.
  lua_pushcclosure(state, static_luaBodyIterator, 1);
  return 1;
}

int RestyHandleWrapper::luaBodyIterator(lua_State* state) {
  ASSERT(state_ == State::Running);

  if (end_stream_) {
    ENVOY_LOG(debug, "body complete. no more body chunks");
    return 0;
  } else {
    ENVOY_LOG(debug, "yielding for next body chunk");
    state_ = State::WaitForBodyChunk;
    return lua_yield(state, 0);
  }
}

int RestyHandleWrapper::luaTrailers(lua_State* state) {
  ASSERT(state_ == State::Running);

  if (end_stream_ && trailers_ == nullptr) {
    ENVOY_LOG(debug, "end stream. no trailers");
    return 0;
  } else if (trailers_ != nullptr) {
    if (trailers_wrapper_.get() != nullptr) {
      trailers_wrapper_.pushStack();
    } else {
      trailers_wrapper_.reset(HeaderMapWrapper::create(state, *trailers_, []() { return true; }),
                              true);
    }
    return 1;
  } else {
    ENVOY_LOG(debug, "yielding for trailers");
    state_ = State::WaitForTrailers;
    return lua_yield(state, 0);
  }
}

int RestyHandleWrapper::luaMetadata(lua_State* state) {
  ASSERT(state_ == State::Running);
  if (metadata_wrapper_.get() != nullptr) {
    metadata_wrapper_.pushStack();
  } else {
    metadata_wrapper_.reset(
        Filters::Common::Lua::MetadataMapWrapper::create(state, callbacks_.metadata()), true);
  }
  return 1;
}

int RestyHandleWrapper::luaStreamInfo(lua_State* state) {
  ASSERT(state_ == State::Running);
  if (stream_info_wrapper_.get() != nullptr) {
    stream_info_wrapper_.pushStack();
  } else {
    stream_info_wrapper_.reset(StreamInfoWrapper::create(state, callbacks_.streamInfo()), true);
  }
  return 1;
}

int RestyHandleWrapper::luaConnection(lua_State* state) {
  ASSERT(state_ == State::Running);
  if (connection_wrapper_.get() != nullptr) {
    connection_wrapper_.pushStack();
  } else {
    connection_wrapper_.reset(
        Filters::Common::Lua::ConnectionWrapper::create(state, callbacks_.connection()), true);
  }
  return 1;
}

int RestyHandleWrapper::luaVerifySignature(lua_State* state) {
  // Step 1: get hash function
  absl::string_view hash = luaL_checkstring(state, 2);

  // Step 2: get key pointer
  auto ptr = lua_touserdata(state, 3);

  // Step 3: get signature
  const char* signature = luaL_checkstring(state, 4);
  int sig_len = luaL_checknumber(state, 5);
  const std::vector<uint8_t> sig_vec(signature, signature + sig_len);

  // Step 4: get clear text
  const char* clear_text = luaL_checkstring(state, 6);
  int text_len = luaL_checknumber(state, 7);
  const std::vector<uint8_t> text_vec(clear_text, clear_text + text_len);

  // Step 5: verify signature
  auto output = Common::Crypto::Utility::verifySignature(hash, reinterpret_cast<EVP_PKEY*>(ptr),
                                                         sig_vec, text_vec);

  lua_pushboolean(state, output.result_);
  if (output.result_) {
    lua_pushnil(state);
  } else {
    lua_pushlstring(state, output.error_message_.data(), output.error_message_.length());
  }
  return 2;
}

int RestyHandleWrapper::luaImportPublicKey(lua_State* state) {
  // Get byte array and the length.
  const char* str = luaL_checkstring(state, 2);
  int n = luaL_checknumber(state, 3);
  std::vector<uint8_t> key(str, str + n);

  if (public_key_wrapper_.get() != nullptr) {
    public_key_wrapper_.pushStack();
  } else {
    public_key_wrapper_.reset(
        PublicKeyWrapper::create(state, Common::Crypto::Utility::importPublicKey(key)), true);
  }

  return 1;
}




} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
