#include "extensions/filters/http/resty/resty_filter.h"

#include <memory>

#include "envoy/http/codes.h"

#include "common/buffer/buffer_impl.h"
#include "common/common/assert.h"
#include "common/common/enum_to_int.h"
#include "common/crypto/utility.h"
#include "common/http/message_impl.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Resty {

struct RcDetailsValues {
  // The fault filter injected an abort for this request.
  const std::string RestyError = "resty_filter_error";
};
typedef ConstSingleton<RcDetailsValues> RcDetails;

void Filter::onDestroy() {
  //  destroyed_ = true;
  resetInternalState();
}

void Filter::requestError() {
  ENVOY_LOG(debug, "requestError");
  ASSERT(is_error());
  decoder_callbacks_->sendLocalReply(error_code_, error_messgae_, nullptr, absl::nullopt,
                                     RcDetails::get().RestyError);
}

void Filter::responseError() {
  ENVOY_LOG(debug, "responseError");
  ASSERT(is_error());
  response_headers_->Status()->value(enumToInt(error_code_));
  Buffer::OwnedImpl data(error_messgae_);
  response_headers_->removeContentType();
  response_headers_->insertContentLength().value(data.length());
  encoder_callbacks_->addEncodedData(data, false);
}

void Filter::resetInternalState() {
  request_body_.drain(request_body_.length());
  response_body_.drain(response_body_.length());
}

void Filter::error(Error error, std::string msg) {
  error_ = error;
  resetInternalState();
  switch (error) {
  case Error::PayloadTooLarge: {
    error_messgae_ = "payload too large";
    error_code_ = Http::Code::PayloadTooLarge;
    break;
  }
  case Error::JsonParseError: {
    error_messgae_ = "bad request";
    error_code_ = Http::Code::BadRequest;
    break;
  }
  case Error::TemplateParseError: {
    error_messgae_ = "bad request";
    error_code_ = Http::Code::BadRequest;
    break;
  }
  case Error::TransformationNotFound: {
    error_messgae_ = "transformation for function not found";
    error_code_ = Http::Code::NotFound;
    break;
  }
  }
  if (!msg.empty()) {
    if (error_messgae_.empty()) {
      error_messgae_ = std::move(msg);
    } else {
      error_messgae_ = error_messgae_ + ": " + msg;
    }
  }
}

bool Filter::is_error() { return error_.has_value(); }

} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
