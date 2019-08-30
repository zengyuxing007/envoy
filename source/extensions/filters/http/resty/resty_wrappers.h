#pragma once

#include "envoy/http/filter.h"
#include "envoy/upstream/cluster_manager.h"
#include "envoy/stream_info/stream_info.h"
#include "common/crypto/utility.h"

#include "extensions/filters/common/lua/lua.h"
#include "extensions/filters/common/lua/wrappers.h"
#include "extensions/filters/http/resty/wrappers.h"
#include "extensions/filters/http/resty/utility.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

namespace {
const ProtobufWkt::Struct& getMetadata(Http::StreamFilterCallbacks* callbacks) {
  if (callbacks->route() == nullptr || callbacks->route()->routeEntry() == nullptr) {
    return ProtobufWkt::Struct::default_instance();
  }
  const auto& metadata = callbacks->route()->routeEntry()->metadata();
  const auto& filter_it = metadata.filter_metadata().find(HttpFilterNames::get().Resty);
  if (filter_it == metadata.filter_metadata().end()) {
    return ProtobufWkt::Struct::default_instance();
  }
  return filter_it->second;
}
} // namespace



class Filter;
class RestyPluginManager;

/**
 * Callbacks used by a stream handler to access the filter.
 */
class FilterCallbacks {
public:
  virtual ~FilterCallbacks() = default;

  /**
   * Add data to the connection manager buffer.
   * @param data supplies the data to add.
   */
  virtual void addData(Buffer::Instance& data) PURE;

  /**
   * @return const Buffer::Instance* the currently buffered body.
   */
  virtual const Buffer::Instance* bufferedBody() PURE;

  /**
   * Continue filter iteration if iteration has been paused due to an async call.
   */
  virtual void continueIteration() PURE;

  /**
   * Called when headers have been modified by a script. This can only happen prior to headers
   * being continued.
   */
  virtual void onHeadersModified() PURE;

  /**
   * Perform an immediate response.
   * @param headers supplies the response headers.
   * @param body supplies the optional response body.
   * @param state supplies the active Lua state.
   */
  virtual void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, lua_State* state) PURE;

  /**
   * @return const ProtobufWkt::Struct& the value of metadata inside the lua filter scope of current
   * route entry.
   */
  virtual const ProtobufWkt::Struct& metadata() const PURE;

  /**
   * @return StreamInfo::StreamInfo& the current stream info handle. This handle is mutable to
   * accommodate write API e.g. setDynamicMetadata().
   */
  virtual StreamInfo::StreamInfo& streamInfo() PURE;

  /**
   * @return const Network::Connection* the current network connection handle.
   */
  virtual const Network::Connection* connection() const PURE;
};



struct DecoderCallbacks : public FilterCallbacks {
    DecoderCallbacks(Filter* parent);//: parent_(parent) {}

    virtual ~DecoderCallbacks();

    // FilterCallbacks
    void addData(Buffer::Instance& data) override {
      return callbacks_->addDecodedData(data, false);
    }
    const Buffer::Instance* bufferedBody() override { return callbacks_->decodingBuffer(); }
    void continueIteration() override { return callbacks_->continueDecoding(); }
    void onHeadersModified() override { callbacks_->clearRouteCache(); }
    void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, lua_State* state) override;

    const ProtobufWkt::Struct& metadata() const override { return getMetadata(callbacks_); }
    StreamInfo::StreamInfo& streamInfo() override { return callbacks_->streamInfo(); }
    const Network::Connection* connection() const override { return callbacks_->connection(); }


    operator Http::StreamDecoderFilterCallbacks* () {
        return callbacks_;
    }

    Filter* parent_;
    Http::StreamDecoderFilterCallbacks* callbacks_{};


  };

struct EncoderCallbacks : public FilterCallbacks {
    EncoderCallbacks(Filter* parent);// : parent_(parent) {}

    virtual ~EncoderCallbacks();

    // FilterCallbacks
    void addData(Buffer::Instance& data) override {
      return callbacks_->addEncodedData(data, false);
    }
    const Buffer::Instance* bufferedBody() override { return callbacks_->encodingBuffer(); }
    void continueIteration() override { return callbacks_->continueEncoding(); }
    void onHeadersModified() override {}
    void respond(Http::HeaderMapPtr&& headers, Buffer::Instance* body, lua_State* state) override;

    const ProtobufWkt::Struct& metadata() const override { return getMetadata(callbacks_); }
    StreamInfo::StreamInfo& streamInfo() override { return callbacks_->streamInfo(); }
    const Network::Connection* connection() const override { return callbacks_->connection(); }

    operator Http::StreamEncoderFilterCallbacks* () {
        return callbacks_;
    }

    Filter* parent_;
    Http::StreamEncoderFilterCallbacks* callbacks_{};
};







/**
 * A wrapper for a currently running request/response. This is the primary handle passed to Lua.
 * The script interacts with Envoy entirely through this handle.
 */
class RestyHandleWrapper : public Filters::Common::Lua::BaseLuaObject<RestyHandleWrapper>,
                            public Http::AsyncClient::Callbacks {
public:
  /**
   * The state machine for a stream handler. In the current implementation everything the filter
   * does is a discrete state. This may become sub-optimal as we add other things that might
   * cause the filter to block.
   * TODO(mattklein123): Consider whether we should split the state machine into an overall state
   * and a blocking reason type.
   */
  enum class State {
    // Lua code is currently running or the script has finished.
    Running,
    // Lua script is blocked waiting for the next body chunk.
    WaitForBodyChunk,
    // Lua script is blocked waiting for the full body.
    WaitForBody,
    // Lua script is blocked waiting for trailers.
    WaitForTrailers,
    // Lua script is blocked waiting for the result of an HTTP call.
    HttpCall,
    // Lua script has done a direct response.
    Responded
  };

  RestyHandleWrapper(Filters::Common::Lua::Coroutine& coroutine, Http::HeaderMap& headers,
                      bool end_stream, RestyPluginManager* filter, FilterCallbacks& callbacks);

  Envoy::Http::FilterDataStatus onData(Buffer::Instance& data, bool end_stream);
  Envoy::Http::FilterTrailersStatus onTrailers(Http::HeaderMap& trailers);

  void onReset() {
    if (http_request_) {
      http_request_->cancel();
      http_request_ = nullptr;
    }
  }

  static ExportedFunctions exportedFunctions() {
    return {{"headers", static_luaHeaders},
            {"body", static_luaBody},
            {"bodyChunks", static_luaBodyChunks},
            {"trailers", static_luaTrailers},
            {"metadata", static_luaMetadata},
            {"httpCall", static_luaHttpCall},
            {"respond", static_luaRespond},
            {"streamInfo", static_luaStreamInfo},
            {"connection", static_luaConnection},
            {"importPublicKey", static_luaImportPublicKey},
            {"verifySignature", static_luaVerifySignature}};
  }

private:
  /**
   * Perform an HTTP call to an upstream host.
   * @param 1 (string): The name of the upstream cluster to call. This cluster must be configured.
   * @param 2 (table): A table of HTTP headers. :method, :path, and :authority must be defined.
   * @param 3 (string): Body. Can be nil.
   * @param 4 (int): Timeout in milliseconds for the call.
   * @return headers (table), body (string/nil)
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaHttpCall);

  /**
   * Perform an inline response. This call is currently only valid on the request path. Further
   * filter iteration will stop. No further script code will run after this call.
   * @param 1 (table): A table of HTTP headers. :status must be defined.
   * @param 2 (string): Body. Can be nil.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaRespond);

  /**
   * @return a handle to the headers.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaHeaders);

  /**
   * @return a handle to the full body or nil if there is no body. This call will cause the script
   *         to yield until the entire body is received (or if there is no body will return nil
   *         right away).
   *         NOTE: This call causes Envoy to buffer the body. The max buffer size is configured
   *         based on the currently active flow control settings.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaBody);

  /**
   * @return an iterator that allows the script to iterate through all body chunks as they are
   *         received. The iterator will yield between body chunks. Envoy *will not* buffer
   *         the body chunks in this case, but the script can look at them as they go by.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaBodyChunks);

  /**
   * @return a handle to the trailers or nil if there are no trailers. This call will cause the
   *         script to yield if Envoy does not yet know if there are trailers or not.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaTrailers);

  /**
   * @return a handle to the metadata.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaMetadata);

  /**
   * @return a handle to the stream info.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaStreamInfo);

  /**
   * @return a handle to the network connection.
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaConnection);

  /**
   * Verify cryptographic signatures.
   * @param 1 (string) hash function(including SHA1, SHA224, SHA256, SHA384, SHA512)
   * @param 2 (void*)  pointer to public key
   * @param 3 (string) signature
   * @param 4 (int)    length of signature
   * @param 5 (string) clear text
   * @param 6 (int)    length of clear text
   * @return (bool, string) If the first element is true, the second element is empty; otherwise,
   * the second element stores the error message
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaVerifySignature);

  /**
   * Import public key.
   * @param 1 (string) keyder string
   * @param 2 (int)    length of keyder string
   * @return pointer to public key
   */
  DECLARE_LUA_FUNCTION(RestyHandleWrapper, luaImportPublicKey);

  /**
   * This is the closure/iterator returned by luaBodyChunks() above.
   */
  DECLARE_LUA_CLOSURE(RestyHandleWrapper, luaBodyIterator);

  static Http::HeaderMapPtr buildHeadersFromTable(lua_State* state, int table_index);

  // Filters::Common::Lua::BaseLuaObject
  void onMarkDead() override {
    // Headers/body/trailers wrappers do not survive any yields. The user can request them
    // again across yields if needed.
    headers_wrapper_.reset();
    body_wrapper_.reset();
    trailers_wrapper_.reset();
    metadata_wrapper_.reset();
    stream_info_wrapper_.reset();
    connection_wrapper_.reset();
    public_key_wrapper_.reset();
  }

  // Http::AsyncClient::Callbacks
  void onSuccess(Http::MessagePtr&&) override;
  void onFailure(Http::AsyncClient::FailureReason) override;

  Filters::Common::Lua::Coroutine& coroutine_;
  Http::HeaderMap& headers_;
  bool end_stream_;
  bool headers_continued_{};
  bool buffered_body_{};
  bool saw_body_{};
  RestyPluginManager* plugin_manager_;
  FilterCallbacks& callbacks_;
  Http::HeaderMap* trailers_{};
  Filters::Common::Lua::LuaDeathRef<HeaderMapWrapper> headers_wrapper_;
  Filters::Common::Lua::LuaDeathRef<Filters::Common::Lua::BufferWrapper> body_wrapper_;
  Filters::Common::Lua::LuaDeathRef<HeaderMapWrapper> trailers_wrapper_;
  Filters::Common::Lua::LuaDeathRef<Filters::Common::Lua::MetadataMapWrapper> metadata_wrapper_;
  Filters::Common::Lua::LuaDeathRef<StreamInfoWrapper> stream_info_wrapper_;
  Filters::Common::Lua::LuaDeathRef<Filters::Common::Lua::ConnectionWrapper> connection_wrapper_;
  Filters::Common::Lua::LuaDeathRef<PublicKeyWrapper> public_key_wrapper_;
  State state_{State::Running};
  std::function<void()> yield_callback_;
  Http::AsyncClient::Request* http_request_{};
};




} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
