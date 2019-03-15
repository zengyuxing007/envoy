#pragma once

#include "envoy/network/transport_socket.h"

namespace Envoy {
namespace Network {

class TransportSocketOptionsImpl : public TransportSocketOptions {
public:
  TransportSocketOptionsImpl(absl::string_view override_server_name = "",
                             bool if_send_proxy_protocol = false,
                             const char* default_downstream_color = NULL)
      : override_server_name_(override_server_name.empty()
                                  ? absl::nullopt
                                  : absl::optional<std::string>(override_server_name)),
        config_send_proxy_protocol_(if_send_proxy_protocol),
        default_downstream_color_(default_downstream_color) {}

  // Network::TransportSocketOptions
  const absl::optional<std::string>& serverNameOverride() const override {
    return override_server_name_;
  }
  void hashKey(std::vector<uint8_t>& key) const override;

  bool isSendProxyProtocol() const override { return config_send_proxy_protocol_; }

  const std::string& getDefaultDownStreamColor() const override {
    return default_downstream_color_;
  }

private:
  const absl::optional<std::string> override_server_name_;
  bool config_send_proxy_protocol_;
  std::string default_downstream_color_;
};

} // namespace Network
} // namespace Envoy
