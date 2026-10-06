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

// Available before the body is written, so a transport can send its headers
// first. Bodies are written synchronously to the caller's concrete sink.
struct request_head {
  spl::variant<method::get, method::post, method::put, method::delete_> method;
  std::string target;
  bool authenticated = true;
  std::string content_type = "application/json";

  constexpr std::string_view method_name() const {
    return spl::visit([](auto one) { return decltype(one)::name; }, method);
  }
};

// What to send, from the homeserver's base URL; with the access token as
// "Authorization: Bearer", where authenticated.
struct request {
  spl::variant<method::get, method::post, method::put, method::delete_> method;
  std::string target;
  std::string body;  // JSON, media bytes, or nothing
  bool authenticated = true;
  std::string content_type = "application/json";

  constexpr std::string_view method_name() const {
    return spl::visit([](auto one) { return decltype(one)::name; }, method);
  }
};

// What a homeserver said went wrong: its status, errcode -- M_FORBIDDEN,
// M_UNKNOWN_TOKEN, M_LIMIT_EXCEEDED... -- and how long to wait, where it
// says so.
// Interactive authentication (the spec's User-Interactive Authentication
// API), as a 401 asks for it: the flows the server offers, each its stages,
// what is done of them, and what the stages that say anything say -- the
// terms to agree to. Read once, here, into types. A CAPTCHA, whichever the
// server uses, is done on its own page (the stage's fallback), not here.
namespace auth_stage {
struct dummy {
  friend constexpr bool operator==(dummy, dummy) = default;
};
struct password {
  friend constexpr bool operator==(password, password) = default;
};
struct registration_token {
  friend constexpr bool operator==(registration_token, registration_token) = default;
};
struct terms {
  friend constexpr bool operator==(terms, terms) = default;
};
struct captcha {
  friend constexpr bool operator==(captcha, captcha) = default;
};
struct email {
  friend constexpr bool operator==(email, email) = default;
};
struct msisdn {
  friend constexpr bool operator==(msisdn, msisdn) = default;
};
struct sso {
  friend constexpr bool operator==(sso, sso) = default;
};
struct other {
  std::string name;
  friend bool operator==(const other&, const other&) = default;
};
}  // namespace auth_stage
using auth_stage_t = spl::variant<auth_stage::dummy, auth_stage::password, auth_stage::registration_token, auth_stage::terms,
                                     auth_stage::captcha, auth_stage::email, auth_stage::msisdn, auth_stage::sso,
                                     auth_stage::other>;
[[nodiscard]] inline auth_stage_t auth_stage_of(std::string_view name) {
  static const std::unordered_map<std::string_view, auth_stage_t> known = {
      {"m.login.dummy", auth_stage::dummy{}},
      {"m.login.password", auth_stage::password{}},
      {"m.login.registration_token", auth_stage::registration_token{}},
      {"m.login.terms", auth_stage::terms{}},
      {"m.login.recaptcha", auth_stage::captcha{}},
      {"m.login.email.identity", auth_stage::email{}},
      {"m.login.msisdn", auth_stage::msisdn{}},
      {"m.login.sso", auth_stage::sso{}}};
  if (const auto found = known.find(name); found != known.end())
    return found->second;
  return auth_stage::other{std::string(name)};
}
// The stage's name, as the server is answered with it.
[[nodiscard]] inline std::string name_of(const auth_stage_t& stage) {
  return spl::visit(spl::overloaded{[](auth_stage::dummy) { return std::string("m.login.dummy"); },
                                          [](auth_stage::password) { return std::string("m.login.password"); },
                                          [](auth_stage::registration_token) { return std::string("m.login.registration_token"); },
                                          [](auth_stage::terms) { return std::string("m.login.terms"); },
                                          [](auth_stage::captcha) { return std::string("m.login.recaptcha"); },
                                          [](auth_stage::email) { return std::string("m.login.email.identity"); },
                                          [](auth_stage::msisdn) { return std::string("m.login.msisdn"); },
                                          [](auth_stage::sso) { return std::string("m.login.sso"); },
                                          [](const auth_stage::other& one) { return one.name; }},
                       stage);
}
// A document to agree to, as m.login.terms gives it: its name and where it
// is, in English where it is in it, else in the first language it has.
struct auth_policy {
  std::string name;
  std::string url;
  std::string version;
};
struct interactive_auth {
  std::vector<std::vector<auth_stage_t>> flows;
  std::vector<auth_stage_t> completed;
  std::vector<auth_policy> terms;
};

struct error {
  int status = 0;
  std::string errcode;
  std::string message;
  std::optional<std::int64_t> retry_after_ms;
  // Asked for interactive authentication (a 401 with flows): its session,
  // to be answered in.
  std::optional<std::string> session;
  // And what it asks: its flows, what is done, the stages' parameters.
  std::optional<interactive_auth> auth{};
};

namespace detail {
template <class Endpoint>
constexpr request collect_request(const Endpoint& endpoint) {
  auto head = endpoint.to_head();
  std::string body;
  endpoint.write_body([&](std::string_view piece) { body.append(piece); });
  return {std::move(head.method), std::move(head.target), std::move(body),
          head.authenticated, std::move(head.content_type)};
}

struct flow_body {
  std::vector<std::string> stages;
};
consteval auto json_schema(knot::type<flow_body>) { return knot::schema<flow_body>(); }
// A policy of m.login.terms: its version beside one object per language --
// read as what each key holds, its languages then read as they are.
struct policy_language {
  std::string name;
  std::string url;
};
consteval auto json_schema(knot::type<policy_language>) { return knot::schema<policy_language>(); }
struct terms_params {
  std::map<std::string, std::map<std::string, knot::raw>> policies;
};
consteval auto json_schema(knot::type<terms_params>) { return knot::schema<terms_params>(); }
struct params_body {
  std::optional<terms_params> terms;
};
consteval auto json_schema(knot::type<params_body>) {
  return knot::schema<params_body>().member<"terms">(knot::key("m.login.terms"));
}
struct error_body {
  std::optional<std::string> errcode;
  std::optional<std::string> error;
  std::optional<std::int64_t> retry_after_ms;
  std::optional<std::string> session;
  std::optional<std::vector<flow_body>> flows;
  std::optional<std::vector<std::string>> completed;
  std::optional<params_body> params;
};
consteval auto json_schema(knot::type<error_body>) { return knot::schema<error_body>(); }
// The policies as the terms stage lists them: each its name and link in
// English, else in the first language given.
inline std::vector<auth_policy> policies_of(const terms_params& given) {
  return given.policies | std::views::transform([](const auto& entry) {
           const auto& [id, fields] = entry;
           auth_policy out{.name = id};
           if (const auto version = fields.find("version"); version != fields.end())
             if (auto read = knot::try_read<std::string>(std::string_view(version->second.text)))
               out.version = std::move(*read);
           const auto language = fields.contains("en") ? fields.find("en")
                                 : std::ranges::find_if(fields, [](const auto& one) { return one.first != "version"; });
           if (language != fields.end())
             if (auto read = knot::try_read<policy_language>(std::string_view(language->second.text))) {
               out.name = std::move(read->name);
               out.url = std::move(read->url);
             }
           return out;
         }) |
         std::ranges::to<std::vector>();
}

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
constexpr std::string text(const spl::variant<Alternatives...>& one) {
  return spl::visit(
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
namespace detail {
template <class Endpoint, class Range>
constexpr std::expected<typename Endpoint::response, error> read_response(int status, Range&& body) {
  if (status >= 200 && status < 300) {
    if constexpr (requires { Endpoint::raw_response; }) {
      return typename Endpoint::response{std::ranges::to<std::string>(std::forward<Range>(body))};
    } else {
      auto got = knot::try_read<typename Endpoint::response>(std::forward<Range>(body));
      if (!got)
        return std::unexpected(error{status, "M_BAD_JSON", std::string(got.error().message), std::nullopt});
      return std::move(*got);
    }
  }
  error out{status, "", "", std::nullopt};
  if (auto said = knot::try_read<detail::error_body>(std::forward<Range>(body))) {
    out.errcode = said->errcode.value_or("");
    out.message = said->error.value_or("");
    out.retry_after_ms = said->retry_after_ms;
    out.session = said->session;
    if (said->flows) {
      const auto stages_of = [](const std::vector<std::string>& names) {
        return names | std::views::transform([](const std::string& name) { return auth_stage_of(name); }) |
               std::ranges::to<std::vector>();
      };
      out.auth = interactive_auth{
          .flows = *said->flows | std::views::transform([&](const flow_body& flow) { return stages_of(flow.stages); }) |
                   std::ranges::to<std::vector>(),
          .completed = stages_of(said->completed.value_or(std::vector<std::string>{})),
          .terms = said->params && said->params->terms ? policies_of(*said->params->terms) : std::vector<auth_policy>{}};
    }
  }
  return std::unexpected(std::move(out));
}
}  // namespace detail

template <class Endpoint>
constexpr std::expected<typename Endpoint::response, error> read(int status, std::string_view body) {
  return detail::read_response<Endpoint>(status, body);
}

// The constraint selects ranges that are not already handled as text; in
// particular a string literal must not be read including its trailing NUL.
template <class Endpoint, std::ranges::input_range Range>
  requires std::same_as<std::ranges::range_value_t<Range>, char> &&
           (!std::convertible_to<Range, std::string_view>)
constexpr std::expected<typename Endpoint::response, error> read(int status, Range&& body) {
  return detail::read_response<Endpoint>(status, std::forward<Range>(body));
}

template <class Endpoint, std::ranges::input_range Chunks>
constexpr std::expected<typename Endpoint::response, error> read_chunks(int status, Chunks&& chunks) {
  return read<Endpoint>(status, std::forward<Chunks>(chunks) | std::views::join);
}

// Download bytes straight to a sink. On an HTTP error, only the error is
// decoded; no error-body bytes are passed to the download sink. Exceptions
// from the range or sink propagate to the caller.
template <class Endpoint, std::ranges::input_range Chunks, class Sink>
  requires (Endpoint::raw_response)
constexpr std::expected<void, error> read_chunks_to(int status, Chunks&& chunks, Sink&& sink) {
  if (status >= 200 && status < 300) {
    for (auto&& chunk : chunks) {
      const std::string_view piece(chunk);
      if (!piece.empty())
        std::invoke(sink, piece);
    }
    return {};
  }
  return std::unexpected(read_chunks<Endpoint>(status, std::forward<Chunks>(chunks)).error());
}

// GET /_matrix/client/versions: what the homeserver speaks.
struct versions {
  struct response {
    std::vector<std::string> versions;
  };
  constexpr request_head to_head() const { return {method::get{}, "/_matrix/client/versions", false}; }
  template <class Sink>
  constexpr void write_body(Sink&&) const {}
  constexpr request to_send() const { return detail::collect_request(*this); }
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

  constexpr request_head to_head() const { return {method::post{}, std::string(detail::client) + "/login", false}; }
  template <class Sink>
  constexpr void write_body(Sink&& sink) const {
    knot::write_chunks(std::forward<Sink>(sink),
                      body{.identifier = {.user = user}, .password = password, .device_id = device_id,
                           .initial_device_display_name = initial_device_display_name});
  }
  constexpr request to_send() const { return detail::collect_request(*this); }
};
consteval auto json_schema(knot::type<login::response>) { return knot::schema<login::response>(); }

// GET /account/whoami.
struct whoami {
  struct response {
    std::string user_id;
    std::optional<std::string> device_id;
  };
  constexpr request_head to_head() const { return {method::get{}, std::string(detail::client) + "/account/whoami"}; }
  template <class Sink>
  constexpr void write_body(Sink&&) const {}
  constexpr request to_send() const { return detail::collect_request(*this); }
};
consteval auto json_schema(knot::type<whoami::response>) { return knot::schema<whoami::response>(); }

// GET /sync: from where the last one ended, waiting up to timeout_ms for
// something new.
struct sync {
  std::optional<std::string> since;
  std::optional<std::int64_t> timeout_ms;
  std::optional<std::string> filter;
  using response = sync_response;

  constexpr request_head to_head() const {
    std::string target = std::string(detail::client) + "/sync";
    if (filter)
      detail::query(target, "filter", *filter);
    if (since)
      detail::query(target, "since", *since);
    if (timeout_ms)
      detail::query(target, "timeout", detail::decimal(*timeout_ms));
    return {method::get{}, std::move(target)};
  }
  template <class Sink>
  constexpr void write_body(Sink&&) const {}
  constexpr request to_send() const { return detail::collect_request(*this); }
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
  constexpr request_head to_head() const {
    return {method::put{},
            std::string(detail::client) + "/rooms/" + percent_encoded(room) + "/send/m.room.message/" +
                percent_encoded(txn_id)};
  }
  template <class Sink>
  constexpr void write_body(Sink&& sink) const { knot::write_chunks(std::forward<Sink>(sink), message); }
  constexpr request to_send() const { return detail::collect_request(*this); }
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
  constexpr request_head to_head() const {
    std::string target = std::string(detail::client) + "/rooms/" + percent_encoded(room) + "/messages";
    detail::query(target, "dir", "b");
    if (from)
      detail::query(target, "from", *from);
    if (limit)
      detail::query(target, "limit", detail::decimal(*limit));
    return {method::get{}, std::move(target)};
  }
  template <class Sink>
  constexpr void write_body(Sink&&) const {}
  constexpr request to_send() const { return detail::collect_request(*this); }
};
consteval auto json_schema(knot::type<messages::response>) { return knot::schema<messages::response>(); }

// POST /join/{room or alias}.
struct join {
  std::string room;
  struct response {
    std::string room_id;
  };
  constexpr request_head to_head() const {
    return {method::post{}, std::string(detail::client) + "/join/" + percent_encoded(room)};
  }
  template <class Sink>
  constexpr void write_body(Sink&& sink) const { std::invoke(sink, std::string_view("{}")); }
  constexpr request to_send() const { return detail::collect_request(*this); }
};
consteval auto json_schema(knot::type<join::response>) { return knot::schema<join::response>(); }

// POST /rooms/{room}/leave.
struct leave {
  std::string room;
  using response = empty;
  constexpr request_head to_head() const {
    return {method::post{}, std::string(detail::client) + "/rooms/" + percent_encoded(room) + "/leave"};
  }
  template <class Sink>
  constexpr void write_body(Sink&& sink) const { std::invoke(sink, std::string_view("{}")); }
  constexpr request to_send() const { return detail::collect_request(*this); }
};

// Transaction ids for sending: each once. Kept, with its count, across
// restarts where messages may be sent again.
struct transactions {
  std::string prefix = "loom";
  std::uint64_t count = 0;
  constexpr std::string next() { return prefix + "." + detail::decimal(static_cast<std::int64_t>(++count)); }
};

}  // namespace loom
