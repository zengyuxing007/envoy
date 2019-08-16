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

void Filter::onDestroy() {
  destroyed_ = true;
}



} // namespace Resty
} // namespace HttpFilters
} // namespace Extensions
} // namespace Envoy
