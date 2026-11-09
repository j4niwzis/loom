// loom.bot:https -- loom's requests sent from a fiber: HTTPS with Beast and
// OpenSSL, the fiber suspended while it is under way. A connection a
// request; what came back, or nothing where nothing could.
module;
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <openssl/ssl.h>
export module loom.bot:https;

import std;
import loom.api;
import :fiber;

namespace asio = boost::asio;
namespace beast = boost::beast;
namespace http = beast::http;
using error_code = boost::system::error_code;

export namespace loom::bot::detail {

struct answer {
  int status = 0;
  std::string body;
};

// Configuration is parsed once, before request routing. Unsupported proxies
// fail closed; proxy credentials must never be copied into diagnostics.
struct http_proxy {
  std::string host;
  std::string port;
};
enum class proxy_config_error { unsupported_scheme, invalid_authority };
using proxy_config = std::expected<std::optional<http_proxy>, proxy_config_error>;

proxy_config read_environment_proxy() {
  const char* given = std::getenv("HTTPS_PROXY");
  if (given == nullptr || *given == 0) given = std::getenv("https_proxy");
  if (given == nullptr || *given == 0) return std::nullopt;
  std::string_view authority(given);
  if (!authority.starts_with("http://"))
    return std::unexpected(proxy_config_error::unsupported_scheme);
  authority.remove_prefix(7);
  if (authority.ends_with('/')) authority.remove_suffix(1);
  if (authority.empty() || authority.contains('@') || authority.find_first_of("/?#") != std::string_view::npos)
    return std::unexpected(proxy_config_error::invalid_authority);
  http_proxy out;
  if (authority.front() == '[') {
    const auto close = authority.find(']');
    if (close == std::string_view::npos || (close + 1 < authority.size() && authority[close + 1] != ':'))
      return std::unexpected(proxy_config_error::invalid_authority);
    out.host = authority.substr(1, close - 1);
    out.port = close + 1 == authority.size() ? "80" : std::string(authority.substr(close + 2));
  } else {
    const auto colon = authority.rfind(':');
    out.host = authority.substr(0, colon);
    out.port = colon == std::string_view::npos ? "80" : std::string(authority.substr(colon + 1));
  }
  if (out.host.empty() || out.port.empty() || out.port.find_first_not_of("0123456789") != std::string::npos)
    return std::unexpected(proxy_config_error::invalid_authority);
  return out;
}
const proxy_config& environment_proxy() {
  static const proxy_config configured = read_environment_proxy();
  return configured;
}

std::optional<answer> send(fiber_on& fiber, asio::ssl::context& tls, const std::string& host, const std::string& token,
                           const loom::request& asked, std::chrono::seconds wait) {
  beast::ssl_stream<beast::tcp_stream> stream(fiber.io(), tls);
  // SNI and the name the certificate is checked against: OpenSSL's C API
  // takes a char*, which is what the string holds.
  SSL_set_tlsext_host_name(stream.native_handle(), host.c_str());
  stream.set_verify_callback(asio::ssl::host_name_verification(host));
  const auto& configured = environment_proxy();
  if (!configured) return std::nullopt;
  const auto& proxy = *configured;
  const std::string& connect_host = proxy ? proxy->host : host;
  const std::string connect_port = proxy ? proxy->port : "443";
  asio::ip::tcp::resolver resolver(fiber.io());
  auto [resolved, found] = await<error_code, asio::ip::tcp::resolver::results_type>(
      fiber, [&](auto done) { resolver.async_resolve(connect_host, connect_port, std::move(done)); });
  if (resolved)
    return std::nullopt;
  beast::get_lowest_layer(stream).expires_after(wait + std::chrono::seconds(15));
  auto [connected, at] = await<error_code, asio::ip::tcp::endpoint>(
      fiber, [&](auto done) { beast::get_lowest_layer(stream).async_connect(found, std::move(done)); });
  if (connected)
    return std::nullopt;
  if (proxy) {
    const std::string authority = host + ":443";
    http::request<http::empty_body> tunnel(http::verb::connect, authority, 11);
    tunnel.set(http::field::host, authority);
    auto [sent, bytes] = await<error_code, std::size_t>(fiber, [&](auto done) {
      http::async_write(beast::get_lowest_layer(stream), tunnel, std::move(done));
    });
    if (sent) return std::nullopt;
    beast::flat_buffer buffer;
    http::response_parser<http::empty_body> response;
    response.skip(true);
    auto [read, count] = await<error_code, std::size_t>(fiber, [&](auto done) {
      http::async_read(beast::get_lowest_layer(stream), buffer, response, std::move(done));
    });
    if (read || response.get().result_int() != 200 || buffer.size() != 0) return std::nullopt;
  }
  auto [shaken] = await<error_code>(fiber, [&](auto done) { stream.async_handshake(asio::ssl::stream_base::client, std::move(done)); });
  if (shaken)
    return std::nullopt;
  http::request<http::string_body> req(http::string_to_verb(asked.method_name()), asked.target, 11);
  req.set(http::field::host, host);
  req.set(http::field::user_agent, "loom.bot/0.1");
  if (asked.authenticated)
    req.set(http::field::authorization, "Bearer " + token);
  if (!asked.body.empty()) {
    req.set(http::field::content_type, asked.content_type);
    req.body() = asked.body;
  }
  req.prepare_payload();
  auto [wrote, written] = await<error_code, std::size_t>(fiber, [&](auto done) { http::async_write(stream, req, std::move(done)); });
  if (wrote)
    return std::nullopt;
  beast::flat_buffer buffer;
  http::response<http::string_body> got;
  auto [read, length] = await<error_code, std::size_t>(fiber, [&](auto done) { http::async_read(stream, buffer, got, std::move(done)); });
  if (read)
    return std::nullopt;
  return answer{static_cast<int>(got.result_int()), std::move(got.body())};
}

}  // namespace loom::bot::detail
