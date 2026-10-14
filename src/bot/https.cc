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

std::optional<answer> send(fiber_on& fiber, asio::ssl::context& tls, const std::string& host, const std::string& token,
                           const loom::request& asked, std::chrono::seconds wait) {
  beast::ssl_stream<beast::tcp_stream> stream(fiber.io(), tls);
  // SNI and the name the certificate is checked against: OpenSSL's C API
  // takes a char*, which is what the string holds.
  SSL_set_tlsext_host_name(stream.native_handle(), host.c_str());
  stream.set_verify_callback(asio::ssl::host_name_verification(host));
  asio::ip::tcp::resolver resolver(fiber.io());
  auto [resolved, found] = await<error_code, asio::ip::tcp::resolver::results_type>(
      fiber, [&](auto done) { resolver.async_resolve(host, "443", std::move(done)); });
  if (resolved)
    return std::nullopt;
  beast::get_lowest_layer(stream).expires_after(wait + std::chrono::seconds(15));
  auto [connected, at] = await<error_code, asio::ip::tcp::endpoint>(
      fiber, [&](auto done) { beast::get_lowest_layer(stream).async_connect(found, std::move(done)); });
  if (connected)
    return std::nullopt;
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
