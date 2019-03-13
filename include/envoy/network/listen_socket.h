#pragma once

#include <memory>
#include <vector>

#include "envoy/api/v2/core/base.pb.h"
#include "envoy/common/exception.h"
#include "envoy/common/pure.h"
#include "envoy/network/address.h"
#include "envoy/network/io_handle.h"

#include "absl/strings/string_view.h"
#include "absl/types/optional.h"

namespace Envoy {
namespace Network {

// Optional variant of setsockopt(2) optname. The idea here is that if the option is not supported
// on a platform, we can make this the empty value. This allows us to avoid proliferation of #ifdef.
typedef absl::optional<std::pair<int, int>> SocketOptionName;

/**
 * Base class for Sockets
 */
class Socket {
public:
  virtual ~Socket() {}

  /**
   * @return the local address of the socket.
   */
  virtual const Address::InstanceConstSharedPtr& localAddress() const PURE;

  /**
   * Set the local address of the socket. On accepted sockets the local address defaults to the
   * one at which the connection was received at, which is the same as the listener's address, if
   * the listener is bound to a specific address.
   *
   * @param local_address the new local address.
   */
  virtual void setLocalAddress(const Address::InstanceConstSharedPtr& local_address) PURE;

  /**
   * @return IoHandle for the underlying connection
   */
  virtual IoHandle& ioHandle() PURE;

  /**
   * @return const IoHandle for the underlying connection
   */
  virtual const IoHandle& ioHandle() const PURE;

  /**
   * @return the type (stream or datagram) of the socket.
   */
  virtual Address::SocketType socketType() const PURE;

  /**
   * Close the underlying socket.
   */
  virtual void close() PURE;

  /**
   * Visitor class for setting socket options.
   */
  class Option {
  public:
    virtual ~Option() {}

    /**
     * @param socket the socket on which to apply options.
     * @param state the current state of the socket. Significant for options that can only be
     *        set for some particular state of the socket.
     * @return true if succeeded, false otherwise.
     */
    virtual bool setOption(Socket& socket,
                           envoy::api::v2::core::SocketOption::SocketState state) const PURE;

    /**
     * @param vector of bytes to which the option should append hash key data that will be used
     *        to separate connections based on the option. Any data already in the key vector must
     *        not be modified.
     */
    virtual void hashKey(std::vector<uint8_t>& key) const PURE;

    /**
     * Contains details about what this option applies to a socket.
     */
    struct Details {
      SocketOptionName name_;
      std::string value_; ///< Binary string representation of an option's value.

      bool operator==(const Details& other) const {
        return name_ == other.name_ && value_ == other.value_;
      }
    };

    /**
     * @param socket The socket for which we want to know the options that would be applied.
     * @param state The state at which we would apply the options.
     * @return What we would apply to the socket at the provided state. Empty if we'd apply nothing.
     */
    virtual absl::optional<Details>
    getOptionDetails(const Socket& socket,
                     envoy::api::v2::core::SocketOption::SocketState state) const PURE;
  };

  typedef std::shared_ptr<const Option> OptionConstSharedPtr;
  typedef std::vector<OptionConstSharedPtr> Options;
  typedef std::shared_ptr<Options> OptionsSharedPtr;

  static OptionsSharedPtr& appendOptions(OptionsSharedPtr& to, const OptionsSharedPtr& from) {
    to->insert(to->end(), from->begin(), from->end());
    return to;
  }

  static bool applyOptions(const OptionsSharedPtr& options, Socket& socket,
                           envoy::api::v2::core::SocketOption::SocketState state) {
    if (options == nullptr) {
      return true;
    }
    for (const auto& option : *options) {
      if (!option->setOption(socket, state)) {
        return false;
      }
    }
    return true;
  }

  /**
   * Add a socket option visitor for later retrieval with options().
   */
  virtual void addOption(const OptionConstSharedPtr&) PURE;

  /**
   * Add socket option visitors for later retrieval with options().
   */
  virtual void addOptions(const OptionsSharedPtr&) PURE;

  /**
   * @return the socket options stored earlier with addOption() and addOptions() calls, if any.
   */
  virtual const OptionsSharedPtr& options() const PURE;
};

typedef std::unique_ptr<Socket> SocketPtr;
typedef std::shared_ptr<Socket> SocketSharedPtr;

/**
 * A socket passed to a connection. For server connections this represents the accepted socket, and
 * for client connections this represents the socket being connected to a remote address.
 *
 * TODO(jrajahalme): Hide internals (e.g., fd) from listener filters by providing callbacks filters
 * may need (set/getsockopt(), peek(), recv(), etc.)
 */
class ConnectionSocket : public virtual Socket {
public:
  virtual ~ConnectionSocket() {}

  /**
   * @return the remote address of the socket.
   */
  virtual const Address::InstanceConstSharedPtr& remoteAddress() const PURE;

  /**
   * Restores the local address of the socket. On accepted sockets the local address defaults to the
   * one at which the connection was received at, which is the same as the listener's address, if
   * the listener is bound to a specific address. Call this to restore the address to a value
   * different from the one the socket was initially accepted at. This should only be called when
   * restoring the original destination address of a connection redirected by iptables REDIRECT. The
   * caller is responsible for making sure the new address is actually different.
   *
   * @param local_address the new local address.
   */
  virtual void restoreLocalAddress(const Address::InstanceConstSharedPtr& local_address) PURE;

  /**
   * Set the remote address of the socket.
   */
  virtual void setRemoteAddress(const Address::InstanceConstSharedPtr& remote_address) PURE;

  /**
   * @return true if the local address has been restored to a value that is different from the
   *         address the socket was initially accepted at.
   */
  virtual bool localAddressRestored() const PURE;

  /**
   * Set detected transport protocol (e.g. RAW_BUFFER, TLS).
   */
  virtual void setDetectedTransportProtocol(absl::string_view protocol) PURE;

  /**
   * @return detected transport protocol (e.g. RAW_BUFFER, TLS), if any.
   */
  virtual absl::string_view detectedTransportProtocol() const PURE;

  /**
   * Set requested application protocol(s) (e.g. ALPN in TLS).
   */
  virtual void
  setRequestedApplicationProtocols(const std::vector<absl::string_view>& protocol) PURE;

  /**
   * @return requested application protocol(s) (e.g. ALPN in TLS), if any.
   */
  virtual const std::vector<std::string>& requestedApplicationProtocols() const PURE;

  /**
   * Set requested server name (e.g. SNI in TLS).
   */
  virtual void setRequestedServerName(absl::string_view server_name) PURE;

  /**
   * @return requested server name (e.g. SNI in TLS), if any.
   */
  virtual absl::string_view requestedServerName() const PURE;
};

typedef std::unique_ptr<ConnectionSocket> ConnectionSocketPtr;

/**
 * Thrown when there is a runtime error binding a socket.
 */
class SocketBindException : public EnvoyException {
public:
  SocketBindException(const std::string& what, int error_number)
      : EnvoyException(what), error_number_(error_number) {}

  // This can't be called errno because otherwise the standard errno macro expansion replaces it.
  int errorNumber() const { return error_number_; }

private:
  const int error_number_;
};

namespace ProxyProtocol {
// Readme: https://www.haproxy.org/download/1.8/doc/proxy-protocol.txt

enum AddrType {
  AddrType_ipv4 = 1,
  AddrType_ipv6 = 2,
  AddrType_unix = 3,
};

#define PP2_TYPE_NETNS 0x30

#pragma pack(1)

struct pp2_tlv {
  uint8_t type;
  uint16_t length;
  uint8_t value[16];
};

struct proxy_protocol_data {
  uint8_t sig[12];
  uint8_t ver_cmd;
  uint8_t fam;
  uint16_t len;

  union {
    struct { /* for TCP/UDP over IPv4, len = 12 */
      uint32_t src_addr;
      uint32_t dst_addr;
      uint16_t src_port;
      uint16_t dst_port;
    } ip4;
    struct { /* for TCP/UDP over IPv6, len = 36 */
      uint8_t src_addr[16];
      uint8_t dst_addr[16];
      uint16_t src_port;
      uint16_t dst_port;
    } ip6;
    struct { /* for AF_UNIX sockets, len = 216 */
      uint8_t src_addr[108];
      uint8_t dst_addr[108];
    } unix;
  } addr;

  struct pp2_tlv tlv;

  //// extra info
  uint16_t length;    // little edian
  bool dest_is_local; //

  proxy_protocol_data() {
    memcpy(sig, "\x0d\x0a\x0d\x0a\x00\x0d\x0a\x51\x55\x49\x54\x0a", 12);
    // using PROXY
    ver_cmd = 0x21;

    length = 0;
    dest_is_local = false;
  }

  int size() { return length + 16; }
};

#pragma pack()

typedef std::shared_ptr<proxy_protocol_data> ProxyProtocolDataSharedPtr;

} // namespace ProxyProtocol

} // namespace Network
} // namespace Envoy
