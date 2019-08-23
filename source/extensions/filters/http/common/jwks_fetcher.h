#pragma once

#include "envoy/api/v2/core/http_uri.pb.h"
#include "envoy/common/pure.h"
#include "envoy/upstream/cluster_manager.h"

#include "jwt_verify_lib/jwks.h"

namespace Envoy {
namespace Extensions {
namespace HttpFilters {
namespace Common {

class JwksFetcher;
using JwksFetcherPtr = std::unique_ptr<JwksFetcher>;
/**
 * JwksFetcher interface can be used to retrieve remote JWKS
 * (https://tools.ietf.org/html/rfc7517) data structures returning a concrete,
 * type-safe representation. An instance of this interface is designed to
 * retrieve one JWKS at a time.
 */
class JwksFetcher {
public:
  class JwksReceiver {
  public:
    enum class Failure {
      /* A network error occurred causing JWKS retrieval failure. */
      Network,
      /* A failure occurred when trying to parse the retrieved JWKS data. */
      InvalidJwks,
    };

    virtual ~JwksReceiver() = default;
    /*
     * Successful retrieval callback.
     * of the returned JWKS object.
     * @param jwks the JWKS object retrieved.
     */
    virtual void onJwksSuccess(google::jwt_verify::JwksPtr&& jwks) PURE;
    /*
     * Retrieval error callback.
     * * @param reason the failure reason.
     */
    virtual void onJwksError(Failure reason) PURE;
  };
