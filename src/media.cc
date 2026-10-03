// SPDX-License-Identifier: AGPL-3.0-only
// loom.media -- A content URI (mxc://server/media-id), read once where it
// comes in, and the paths it is fetched at: whole, or as a thumbnail; the
// authenticated media API's first, the legacy one's after it.
export module loom.media;

import std;

export namespace loom::media {

// What an mxc:// names: the server it is on, and its id there.
struct mxc {
  std::string server;
  std::string media_id;
  friend bool operator==(const mxc&, const mxc&) = default;
};

// A content URI, as the specification writes one: a server name (a host,
// maybe a port, an IPv6 literal in brackets) and a media ID of letters,
// digits, '-' and '_'. None for anything else -- "..", '/', '?' put into a
// path would ask the homeserver, with the user's token, for another
// endpoint than media.
[[nodiscard]] inline std::optional<mxc> mxc_of(std::string_view uri) {
  constexpr std::string_view scheme = "mxc://";
  if (!uri.starts_with(scheme))
    return std::nullopt;
  const std::string_view rest = uri.substr(scheme.size());
  const auto slash = rest.find('/');
  if (slash == std::string_view::npos)
    return std::nullopt;
  const std::string_view server = rest.substr(0, slash);
  const std::string_view media_id = rest.substr(slash + 1);
  const auto plain = [](std::string_view text, std::string_view also) {
    return !text.empty() && std::ranges::all_of(text, [&](char c) {
      return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || also.contains(c);
    });
  };
  if (!plain(server, ".-:[]") || !plain(media_id, "-_") || server.starts_with('.'))
    return std::nullopt;
  return mxc{std::string(server), std::string(media_id)};
}

// A thumbnail asked for: its size, and whether cropped to it or scaled.
struct thumbnail {
  int size = 0;
  bool crop = false;
};
// Where it is fetched, in order of asking: the authenticated media API
// (v1, Matrix 1.11), then the legacy one (v3) for servers before it.
[[nodiscard]] inline std::array<std::string, 2> paths_of(const mxc& one) {
  const std::string at = one.server + "/" + one.media_id;
  return {"/_matrix/client/v1/media/download/" + at, "/_matrix/media/v3/download/" + at};
}
[[nodiscard]] inline std::array<std::string, 2> paths_of(const mxc& one, const thumbnail& asked) {
  const std::string at =
      std::format("{0}/{1}?width={2}&height={2}&method={3}", one.server, one.media_id, asked.size, asked.crop ? "crop" : "scale");
  return {"/_matrix/client/v1/media/thumbnail/" + at, "/_matrix/media/v3/thumbnail/" + at};
}

}  // namespace loom::media
