#pragma once


#include "envoy/http/filter.h"
//#include "envoy/upstream/cluster_manager.h"

#include "extensions/filters/common/lua/script_action.h"
#include "extensions/filters/http/resty/plugins_manager.h"
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
  Filter(RestyPluginManagerConstSharedPtr resty_plugin_manager) : resty_plugin_manager_(resty_plugin_manager) {
      resty_plugin_manager->initAllPlugin();
  }


  // Http::StreamFilterBase
  void onDestroy() override;

  // Http::StreamDecoderFilter
  Http::FilterHeadersStatus decodeHeaders(Http::HeaderMap& headers, bool end_stream) override {
    return resty_plugin_manager_->doDecodeHeaders(headers, end_stream);
  }
  Http::FilterDataStatus decodeData(Buffer::Instance& data, bool end_stream) override {
    return resty_plugin_manager_->doDecodeData(data, end_stream);
  }
  Http::FilterTrailersStatus decodeTrailers(Http::HeaderMap& trailers) override {
    return resty_plugin_manager_->doDecodeTrailers(trailers);
  }
  void setDecoderFilterCallbacks(Http::StreamDecoderFilterCallbacks& callbacks) override {
    decoder_callbacks_ = &callbacks;
  }

  // Http::StreamEncoderFilter
  Http::FilterHeadersStatus encode100ContinueHeaders(Http::HeaderMap&) override {
    return Http::FilterHeadersStatus::Continue;
  }
  Http::FilterHeadersStatus encodeHeaders(Http::HeaderMap& headers, bool end_stream) override {
    return resty_plugin_manager_->doEncodeHeaders(headers, end_stream);
  }
  Http::FilterDataStatus encodeData(Buffer::Instance& data, bool end_stream) override {
    return resty_plugin_manager_->doEncodeData(data, end_stream);
  };
  Http::FilterTrailersStatus encodeTrailers(Http::HeaderMap& trailers) override {
    return resty_plugin_manager_->doEncodeTrailers(trailers);
  };
  Http::FilterMetadataStatus encodeMetadata(Http::MetadataMap&) override {
    return Http::FilterMetadataStatus::Continue;
  }
  void setEncoderFilterCallbacks(Http::StreamEncoderFilterCallbacks& callbacks) override {
    encoder_callbacks_ = &callbacks;
  };

private:
   bool destroyed_{};
   RestyPluginManagerConstSharedPtr  resty_plugin_manager_;
   Http::StreamDecoderFilterCallbacks* decoder_callbacks_;
   Http::StreamEncoderFilterCallbacks* encoder_callbacks_;
};

} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy


