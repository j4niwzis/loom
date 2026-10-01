// SPDX-License-Identifier: AGPL-3.0-only
// The client-server API as values: each endpoint made into what to send --
// method, target, JSON body -- and what comes back read straight into its
// response, or into the homeserver's error. Sending is the caller's.
export module loom.api;

import std;
import splice;
export import knot;
export import loom.ids;
export import loom.events;
export import loom.sync;

export namespace loom {

namespace method {
struct get { static constexpr std::string_view name = "GET"; };
struct post { static constexpr std::string_view name = "POST"; };
struct put { static constexpr std::string_view name = "PUT"; };
struct delete_ { static constexpr std::string_view name = "DELETE"; };
}  // namespace method

// What to send, from the homeserver's base URL; with the access token as
// "Authorization: Bearer", where authenticated.
struct request {
  splice::variant<method::get, method::post, method::put, method::delete_> method;
  std::string target;
  std::string body;  // JSON, or nothing
  bool authenticated = true;

  constexpr std::string_view method_name() const {
    return splice::visit([](auto one) { return decltype(one)::name; }, method);
  }
};

// What a homeserver said went wrong: its status, errcode -- M_FORBIDDEN,
// M_UNKNOWN_TOKEN, M_LIMIT_EXCEEDED... -- and how long to wait, where it
// says so.
struct error {
  int status = 0;
  std::string errcode;
  std::string message;
  std::optional<std::int64_t> retry_after_ms;
  // Asked for interactive authentication (a 401 with flows): its session,
  // to be answered in.
  std::optional<std::string> session;
};

namespace detail {
struct error_body {
  std::optional<std::string> errcode;
  std::optional<std::string> error;
  std::optional<std::int64_t> retry_after_ms;
  std::optional<std::string> session;
};
consteval auto json_schema(knot::type<error_body>) { return knot::schema<error_body>(); }

inline constexpr std::string_view client = "/_matrix/client/v3";

constexpr void query(std::string& target, std::string_view key, std::string_view value) {
  target += target.find('?') == std::string::npos ? '?' : '&';
  target += key;
  target += '=';
  target += percent_encoded(value);
}

constexpr std::string decimal(std::int64_t n) {
  char digits[24];
  char* end = std::to_chars(digits, digits + 24, n).ptr;
  return std::string(digits, end);
}

// A parameter as the text a path or a query carries.
constexpr std::string text(const std::string& one) { return one; }
constexpr std::string text(std::string_view one) { return std::string(one); }
constexpr std::string text(const char* one) { return std::string(one); }
constexpr std::string text(bool one) { return one ? "true" : "false"; }
constexpr std::string text(std::int64_t one) { return decimal(one); }
constexpr std::string text(double one) {
  char digits[32];
  char* end = std::to_chars(digits, digits + 32, one).ptr;
  return std::string(digits, end);
}
constexpr std::string text(const knot::raw& one) { return one.text; }
// A choice: the string its alternative names, or the one it keeps.
template <class... Alternatives>
constexpr std::string text(const splice::variant<Alternatives...>& one) {
  return splice::visit(
      [](const auto& held) -> std::string {
        if constexpr (requires { std::remove_cvref_t<decltype(held)>::json_value; }) {
          return std::string(std::remove_cvref_t<decltype(held)>::json_value);
        } else {
          return text(held);
        }
      },
      one);
}

// A body, as JSON.
template <class Body>
constexpr std::string json(const Body& body) {
  return knot::to_json_string(body);
}
}  // namespace detail

// An answer with nothing in it that a client needs.
struct empty {};
consteval auto json_schema(knot::type<empty>) { return knot::schema<empty>(); }

// What came back: a 2xx read into the endpoint's response; anything else
// into the error.
template <class Endpoint>
constexpr std::expected<typename Endpoint::response, error> read(int status, std::string_view body) {
  if constexpr (requires { Endpoint::raw_response; }) {
    // Bytes, not JSON: what the content repository gives.
    if (status >= 200 && status < 300)
      return typename Endpoint::response{std::string(body)};
  }
  if (status >= 200 && status < 300) {
    auto got = knot::try_read<typename Endpoint::response>(body);
    if (!got)
      return std::unexpected(error{status, "M_BAD_JSON", std::string(got.error().message), std::nullopt});
    return std::move(*got);
  }
  error out{status, "", "", std::nullopt};
  if (auto said = knot::try_read<detail::error_body>(body)) {
    out.errcode = said->errcode.value_or("");
    out.message = said->error.value_or("");
    out.retry_after_ms = said->retry_after_ms;
    out.session = said->session;
  }
  return std::unexpected(std::move(out));
}

// GET /_matrix/client/versions: what the homeserver speaks.
struct versions {
  struct response {
    std::vector<std::string> versions;
  };
  constexpr request to_send() const { return {method::get{}, "/_matrix/client/versions", "", false}; }
};
consteval auto json_schema(knot::type<versions::response>) { return knot::schema<versions::response>(); }

// POST /login, with a password.
struct login {
  std::string user;  // the localpart, or the whole user id
  std::string password;
  std::optional<std::string> device_id;
  std::optional<std::string> initial_device_display_name;

  struct identifier {
    std::string type = "m.id.user";
    std::string user;
    friend consteval auto json_schema(knot::type<identifier>) { return knot::schema<identifier>(); }
  };
  struct body {
    std::string type = "m.login.password";
    login::identifier identifier;
    std::string password;
    std::optional<std::string> device_id;
    std::optional<std::string> initial_device_display_name;
    friend consteval auto json_schema(knot::type<body>) { return knot::schema<body>(); }
  };
  struct response {
    std::string user_id;
    std::string access_token;
    std::string device_id;
    std::optional<std::string> refresh_token;
    std::optional<std::int64_t> expires_in_ms;
  };

  constexpr request to_send() const {
    return {method::post{}, std::string(detail::client) + "/login",
            knot::to_json_string(body{.identifier = {.user = user}, .password = password, .device_id = device_id,
                                      .initial_device_display_name = initial_device_display_name}),
            false};
  }
};
consteval auto json_schema(knot::type<login::response>) { return knot::schema<login::response>(); }

// GET /account/whoami.
struct whoami {
  struct response {
    std::string user_id;
    std::optional<std::string> device_id;
  };
  constexpr request to_send() const { return {method::get{}, std::string(detail::client) + "/account/whoami", ""}; }
};
consteval auto json_schema(knot::type<whoami::response>) { return knot::schema<whoami::response>(); }

// GET /sync: from where the last one ended, waiting up to timeout_ms for
// something new.
struct sync {
  std::optional<std::string> since;
  std::optional<std::int64_t> timeout_ms;
  std::optional<std::string> filter;
  using response = sync_response;

  constexpr request to_send() const {
    std::string target = std::string(detail::client) + "/sync";
    if (filter)
      detail::query(target, "filter", *filter);
    if (since)
      detail::query(target, "since", *since);
    if (timeout_ms)
      detail::query(target, "timeout", detail::decimal(*timeout_ms));
    return {method::get{}, std::move(target), ""};
  }
};

// PUT /rooms/{room}/send/m.room.message/{txn}: a text message. The
// transaction id makes a retry of the same message the same message.
struct send_message {
  std::string room;
  std::string txn_id;
  content::message message;
  struct response {
    std::string event_id;
  };
  constexpr request to_send() const {
    return {method::put{},
            std::string(detail::client) + "/rooms/" + percent_encoded(room) + "/send/m.room.message/" +
                percent_encoded(txn_id),
            knot::to_json_string(message)};
  }
};
consteval auto json_schema(knot::type<send_message::response>) { return knot::schema<send_message::response>(); }

// GET /rooms/{room}/messages: a page of history, back from `from`.
struct messages {
  std::string room;
  std::optional<std::string> from;
  std::optional<std::int64_t> limit;
  struct response {
    std::optional<std::string> start;
    std::optional<std::string> end;
    std::vector<room_event> chunk;
  };
  constexpr request to_send() const {
    std::string target = std::string(detail::client) + "/rooms/" + percent_encoded(room) + "/messages";
    detail::query(target, "dir", "b");
    if (from)
      detail::query(target, "from", *from);
    if (limit)
      detail::query(target, "limit", detail::decimal(*limit));
    return {method::get{}, std::move(target), ""};
  }
};
consteval auto json_schema(knot::type<messages::response>) { return knot::schema<messages::response>(); }

// POST /join/{room or alias}.
struct join {
  std::string room;
  struct response {
    std::string room_id;
  };
  constexpr request to_send() const {
    return {method::post{}, std::string(detail::client) + "/join/" + percent_encoded(room), "{}"};
  }
};
consteval auto json_schema(knot::type<join::response>) { return knot::schema<join::response>(); }

// POST /rooms/{room}/leave.
struct leave {
  std::string room;
  using response = empty;
  constexpr request to_send() const {
    return {method::post{}, std::string(detail::client) + "/rooms/" + percent_encoded(room) + "/leave", "{}"};
  }
};

// Transaction ids for sending: each once. Kept, with its count, across
// restarts where messages may be sent again.
struct transactions {
  std::string prefix = "loom";
  std::uint64_t count = 0;
  constexpr std::string next() { return prefix + "." + detail::decimal(static_cast<std::int64_t>(++count)); }
};

}  // namespace loom
