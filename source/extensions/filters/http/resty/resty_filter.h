#pragma once

#include "envoy/http/filter.h"

#include "common/buffer/buffer_impl.h"

#include "extensions/filters/http/resty/plugins_manager.h"
#include "extensions/filters/http/resty/script_action.h"
#include "extensions/filters/http/well_known_names.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {




/**
 * The HTTP Resty filter. Allows scripts to run in both the request an response flow.
 */
class Filter : public Http::StreamFilter, Logger::Loggable<Logger::Id::resty> {
public:
  enum class Error {
    PayloadTooLarge,
    JsonParseError,
    TemplateParseError,
    TransformationNotFound,
  };

  using StreamHandleRef = Filters::Common::Lua::LuaDeathRef<RestyHandleWrapper>; 

public:
  Filter(RestyPluginManagerConstSharedPtr resty_plugin_manager)
      : resty_plugin_manager_(resty_plugin_manager) {
     resty_plugin_manager->initAllPlugin();
     resty_plugin_manager->setFilter(this);
  }

  Upstream::ClusterManager& clusterManager() { return resty_plugin_manager_->cluster_manager_; }

  void scriptError(const Filters::Common::Lua::LuaException& e){
      resty_plugin_manager_->scriptError(e);
  }

  // Http::StreamFilterBase
  void onDestroy() override;

  void resetInternalState();
  void error(Error error, std::string msg = "");
  bool is_error();
  void requestError();
  void responseError();

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override {
    return resty_plugin_manager_->doDecodeHeaders(request_stream_wrapper_,headers, end_stream);
  }
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override {
    return resty_plugin_manager_->doDecodeData(request_stream_wrapper_,data, end_stream);
  }
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override {
    return resty_plugin_manager_->doDecodeTrailers(request_stream_wrapper_,trailers);
  }
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override {
    decoder_callbacks_ = &callbacks;
    resty_plugin_manager_->setDecoderFilterCallbacks(callbacks);
  }

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override {
    return Http::FilterHeadersStatus::Continue;
  }
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override {
    return resty_plugin_manager_->doEncodeHeaders(response_stream_wrapper_,headers, end_stream);
  }
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override {
    return resty_plugin_manager_->doEncodeData(response_stream_wrapper_,data, end_stream);
  };
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override {
    return resty_plugin_manager_->doEncodeTrailers(response_stream_wrapper_,trailers);
  };
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap&) override {
    return Http::FilterMetadataStatus::Continue;
  }
  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override {
    encoder_callbacks_ = &callbacks;
    resty_plugin_manager_->setEncoderFilterCallbacks(callbacks);
  };

  bool destroyed() { return destroyed_; }

private:
  bool destroyed_{};
  RestyPluginManagerConstSharedPtr resty_plugin_manager_;
  Http::StreamDecoderFilterCallbacks* decoder_callbacks_{};
  Http::StreamEncoderFilterCallbacks* encoder_callbacks_{};
  Http::HeaderMap* request_headers_{nullptr};
  Http::HeaderMap* response_headers_{nullptr};
  Buffer::OwnedImpl request_body_{};
  Buffer::OwnedImpl response_body_{};
  StreamHandleRef request_stream_wrapper_;
  StreamHandleRef response_stream_wrapper_;

  absl::optional<Error> error_;
  Http::Code error_code_;
  std::string error_messgae_;
};





} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
