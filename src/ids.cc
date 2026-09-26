// Matrix's identifiers (client-server API, appendix "Identifier grammar"):
// a user, a room, an alias, an event -- a sigil, a local part, and but for
// events a server -- and a piece of a path, as it is sent.
export module loom.ids;

import std;

export namespace loom {

namespace detail {
constexpr bool digit(char c) { return c >= '0' && c <= '9'; }
constexpr bool alpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
constexpr bool hex(char c) { return digit(c) || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'); }
}  // namespace detail

// A server name: a DNS name, an IPv4 address, or an IPv6 one in brackets;
// perhaps a port.
constexpr bool server_name(std::string_view text) {
  std::string_view port;
  if (text.empty())
    return false;
  if (text.front() == '[') {
    const auto close = text.find(']');
    if (close == std::string_view::npos || close == 1)
      return false;
    for (char c : text.substr(1, close - 1))
      if (!detail::hex(c) && c != ':' && c != '.')
        return false;
    port = text.substr(close + 1);
  } else {
    const auto colon = text.find(':');
    const std::string_view host = text.substr(0, colon);
    if (host.empty() || host.size() > 255)
      return false;
    for (char c : host)
      if (!detail::alpha(c) && !detail::digit(c) && c != '-' && c != '.')
        return false;
    port = colon == std::string_view::npos ? std::string_view() : text.substr(colon);
  }
  if (!port.empty()) {
    if (port.front() != ':' || port.size() < 2 || port.size() > 6)
      return false;
    for (char c : port.substr(1))
      if (!detail::digit(c))
        return false;
  }
  return true;
}

// An identifier with its sigil: @user:server, !room:server, #alias:server;
// $event, with a server only in the oldest room versions.
template <char Sigil, bool Server>
struct identifier {
  std::string text;

  static constexpr std::optional<identifier> parse(std::string_view said) {
    if (said.size() < 2 || said.size() > 255 || said.front() != Sigil)
      return std::nullopt;
    for (char c : said)
      if (static_cast<unsigned char>(c) < 0x21 || c == 0x7f)
        return std::nullopt;
    const auto colon = said.find(':');
    if (colon == std::string_view::npos) {
      if constexpr (Server)
        return std::nullopt;
      return identifier{std::string(said)};
    }
    if (colon == 1 || !server_name(said.substr(colon + 1)))
      return std::nullopt;
    return identifier{std::string(said)};
  }

  constexpr std::string_view localpart() const {
    const std::string_view all = text;
    return all.substr(1, all.find(':') - 1);
  }
  constexpr std::string_view server() const {
    const std::string_view all = text;
    const auto colon = all.find(':');
    return colon == std::string_view::npos ? std::string_view() : all.substr(colon + 1);
  }

  friend constexpr bool operator==(const identifier&, const identifier&) = default;
  friend constexpr auto operator<=>(const identifier&, const identifier&) = default;
};

using user_id = identifier<'@', true>;
using room_id = identifier<'!', true>;
using room_alias = identifier<'#', true>;
using event_id = identifier<'$', false>;

// A piece of a path or of a query, percent-encoded (RFC 3986): all but the
// unreserved characters.
constexpr std::string percent_encoded(std::string_view text) {
  constexpr char digits[] = "0123456789ABCDEF";
  std::string out;
  for (char c : text) {
    if (detail::alpha(c) || detail::digit(c) || c == '-' || c == '.' || c == '_' || c == '~') {
      out += c;
    } else {
      const auto byte = static_cast<unsigned char>(c);
      out += '%';
      out += digits[byte >> 4];
      out += digits[byte & 15];
    }
  }
  return out;
}

}  // namespace loom
