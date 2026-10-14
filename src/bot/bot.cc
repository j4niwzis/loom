// loom.bot -- a bot as straight-line code: each handler a fiber, waiting
// for what it needs and saying what it says, on one io_context.
//
//   loom::bot::run(session, [](loom::bot::fiber& f) {
//     auto mine = f.subscribe({.sender = owner});
//     for (;;) {
//       auto asked = mine.next();                       // suspends until it comes
//       f.reply(asked, loom::bot::markdown("**pong**"));
//     }
//   });
//
// Events are broadcast: every inbox whose filter an event matches gets it,
// queued until its fiber asks, so no fiber takes another's. Nothing runs at
// once: a fiber switches only where it waits (HTTP, a timer, next()).
module;
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
export module loom.bot;

export import :fiber;
import :https;
import std;
import knot;
import splice;
import loom;
import loom.cs.room_send;
import loom.cs.sync;
import loom.cs.typing;
import loom.cs.message_pagination;

namespace asio = boost::asio;

export namespace loom::bot {

// Where the bot is, and who: its homeserver's host, its token, its id.
struct session {
  std::string host;
  std::string access_token;
  std::string user_id;
  // Where the sync's place is kept between runs; none: from now, each run.
  std::optional<std::filesystem::path> since_file;
};

// ---- What is said ----------------------------------------------------------
struct text {
  std::string plain;
};
struct html {
  std::string markup;
  std::optional<std::string> plain;  // its text, where said; else made from the markup
};
struct markdown {
  std::string source;
};
using content = spl::variant<text, html, markdown>;

namespace detail {
inline std::string escaped(std::string_view in) {
  std::string out;
  for (const char c : in)
    switch (c) {
      case '&': out += "&amp;"; break;
      case '<': out += "&lt;"; break;
      case '>': out += "&gt;"; break;
      case '"': out += "&quot;"; break;
      default: out += c;
    }
  return out;
}
// A line's marks: `code`, **bold**, _italic_ (at word edges), [text](url).
inline std::string inline_html(std::string_view line) {
  std::string out;
  bool bold = false, italic = false;
  for (std::size_t i = 0; i < line.size();) {
    if (line[i] == '`')
      if (const auto end = line.find('`', i + 1); end != std::string_view::npos) {
        out += "<code>" + escaped(line.substr(i + 1, end - i - 1)) + "</code>";
        i = end + 1;
        continue;
      }
    if (line.substr(i).starts_with("**")) {
      out += bold ? "</b>" : "<b>";
      bold = !bold;
      i += 2;
      continue;
    }
    if (line[i] == '_') {
      const bool before = i == 0 || !std::isalnum(static_cast<unsigned char>(line[i - 1]));
      const bool after = i + 1 >= line.size() || !std::isalnum(static_cast<unsigned char>(line[i + 1]));
      if ((!italic && before && !after) || (italic && after && !before)) {
        out += italic ? "</i>" : "<i>";
        italic = !italic;
        ++i;
        continue;
      }
    }
    if (line[i] == '[')
      if (const auto close = line.find("](", i); close != std::string_view::npos)
        if (const auto end = line.find(')', close); end != std::string_view::npos) {
          out += "<a href=\"" + escaped(line.substr(close + 2, end - close - 2)) + "\">" + escaped(line.substr(i + 1, close - i - 1)) + "</a>";
          i = end + 1;
          continue;
        }
    out += escaped(line.substr(i, 1));
    ++i;
  }
  if (italic) out += "</i>";
  if (bold) out += "</b>";
  return out;
}
inline std::string markdown_html(std::string_view source) {
  std::string out;
  bool fenced = false;
  for (const auto piece : source | std::views::split('\n')) {
    const std::string_view line(piece.begin(), piece.end());
    if (line.starts_with("```")) {
      out += fenced ? "</code></pre>" : "<pre><code>";
      fenced = !fenced;
    } else if (fenced) {
      out += escaped(line) + "\n";
    } else {
      out += inline_html(line) + "<br>";
    }
  }
  if (fenced) out += "</code></pre>";
  return out;
}
inline std::string text_of_html(std::string_view markup) {
  std::string out;
  bool tag = false;
  for (const char c : markup) {
    if (c == '<') tag = true;
    else if (c == '>') tag = false;
    else if (!tag) out += c;
  }
  for (const auto& [from, to] : std::array<std::pair<std::string_view, std::string_view>, 4>{
           {{"&lt;", "<"}, {"&gt;", ">"}, {"&quot;", "\""}, {"&amp;", "&"}}})
    for (std::size_t at; (at = out.find(from)) != std::string::npos;) out.replace(at, from.size(), to);
  return out;
}
// What a content is sent as: its text, and its HTML.
struct said {
  std::string plain, markup;
};
inline said said_of(const content& c) {
  return spl::visit(spl::overloaded{[](const text& t) { return said{t.plain, escaped(t.plain)}; },
                                    [](const html& h) { return said{h.plain.value_or(text_of_html(h.markup)), h.markup}; },
                                    [](const markdown& m) { return said{m.source, markdown_html(m.source)}; }},
                    c);
}
// What a bot reads of a room's history: messages and reactions typed, the
// rest kept as it came.
using looked_content = knot::tagged<"type", loom::ev::m_room_message_content_t, loom::ev::m_reaction_content_t, knot::raw>;
struct looked_event {
  looked_content content;
  std::string event_id;
  std::string sender;
  std::string type;  // what the content is chosen by
  knot::raw rest;
  friend consteval auto json_schema(knot::type<looked_event>) { return knot::schema<looked_event>().member<"rest">(knot::rest); }
};
struct looked_history {
  std::vector<looked_event> chunk;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<looked_history>) { return knot::schema<looked_history>().member<"rest">(knot::rest); }
};
}  // namespace detail

// ---- What is heard -----------------------------------------------------------
// A message in a room, as the sync brought it.
struct message {
  std::string room_id;
  std::string event_id;
  std::string sender;
  std::string body;  // the new text, for an edit
  bool edit = false;
  std::optional<std::string> thread_root;
  std::optional<std::string> reply_to;
};
// Which messages an inbox takes: each part said narrows it.
struct filter {
  std::optional<std::string> room;
  std::optional<std::string> sender;
  std::optional<std::string> thread_root;
  std::optional<std::string> reply_to;
  bool edits = true;
  [[nodiscard]] bool takes(const message& m) const {
    return (!room || *room == m.room_id) && (!sender || *sender == m.sender) && (!thread_root || thread_root == m.thread_root) &&
           (!reply_to || reply_to == m.reply_to) && (edits || !m.edit);
  }
};

class client;
class fiber;

// What one fiber hears: every message its filter takes, kept in order until
// it asks -- at most `kKept`, the oldest let go first (and counted).
class inbox {
 public:
  static constexpr std::size_t kKept = 512;
  inbox(client& c, detail::fiber_on& f, filter wants);
  ~inbox();
  inbox(const inbox&) = delete;
  inbox& operator=(const inbox&) = delete;
  // The next message, waited for.
  message next();
  // The next message, or none if `for_ms` passes first.
  std::optional<message> next(std::chrono::milliseconds for_ms);
  [[nodiscard]] std::size_t dropped() const { return dropped_; }

 private:
  friend class client;
  void put(const message& m) {
    if (!wants_.takes(m)) return;
    if (queue_.size() == kKept) {
      queue_.pop_front();
      ++dropped_;
    }
    queue_.push_back(m);
    if (waiting_) fiber_.wake();
  }
  client& client_;
  detail::fiber_on& fiber_;
  filter wants_;
  std::deque<message> queue_;
  std::size_t dropped_ = 0;
  bool waiting_ = false;
};

// The bot: its session, its loop, its fibers and their inboxes.
class client {
 public:
  explicit client(session s) : session_(std::move(s)), tls_(asio::ssl::context::tls_client) {
    tls_.set_default_verify_paths();
    tls_.set_verify_mode(asio::ssl::verify_peer);
  }
  [[nodiscard]] const session& settings() const { return session_; }
  [[nodiscard]] asio::io_context& io() { return io_; }

  // A request from a fiber: the homeserver's answer read into the endpoint's
  // response, or nothing. Rate limits waited out and tried again.
  template <class Endpoint, class Response = typename Endpoint::response>
  std::optional<Response> call(detail::fiber_on& f, const Endpoint& asked, std::chrono::seconds wait = std::chrono::seconds(30)) {
    for (int tries = 0; tries < 5; ++tries) {
      const auto got = detail::send(f, tls_, session_.host, session_.access_token, asked.to_send(), wait);
      if (!got) return std::nullopt;
      if (got->status == 429) {
        sleep(f, std::chrono::milliseconds(1000 << tries));
        continue;
      }
      if (got->status != 200) return std::nullopt;
      if (auto read = knot::try_read<Response>(std::string_view(got->body))) return std::move(*read);
      return std::nullopt;
    }
    return std::nullopt;
  }
  void sleep(detail::fiber_on& f, std::chrono::milliseconds for_ms) {
    asio::steady_timer timer(io_, for_ms);
    detail::await<boost::system::error_code>(f, [&](auto done) { timer.async_wait(std::move(done)); });
  }
  // A transaction id never used before by this client.
  std::string txn() {
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    return std::format("loom.bot.{}.{}", std::chrono::duration_cast<std::chrono::microseconds>(now).count(), ++sent_);
  }
  // Fibers started, each with a fiber& to say and hear through; the sync too.
  template <class... Handlers> void run(Handlers... handlers);
  template <class Handler> void once(Handler handler);

 private:
  friend class inbox;
  void sync_forever(detail::fiber_on& f);
  void deliver(const message& m) {
    for (inbox* each : inboxes_) each->put(m);
  }
  session session_;
  asio::io_context io_;
  asio::ssl::context tls_;
  std::list<std::unique_ptr<detail::fiber_on>> fibers_;
  std::vector<inbox*> inboxes_;
  std::uint64_t sent_ = 0;
  std::size_t live_ = 0;
};

inline inbox::inbox(client& c, detail::fiber_on& f, filter wants) : client_(c), fiber_(f), wants_(std::move(wants)) {
  client_.inboxes_.push_back(this);
}
inline inbox::~inbox() { std::erase(client_.inboxes_, this); }
inline message inbox::next() {
  while (queue_.empty()) {
    waiting_ = true;
    fiber_.suspend();
    waiting_ = false;
  }
  message m = std::move(queue_.front());
  queue_.pop_front();
  return m;
}
inline std::optional<message> inbox::next(std::chrono::milliseconds for_ms) {
  if (queue_.empty()) {
    bool lapsed = false;
    asio::steady_timer timer(client_.io(), for_ms);
    timer.async_wait([&](boost::system::error_code e) {
      if (!e) {
        lapsed = true;
        if (waiting_) fiber_.wake();
      }
    });
    while (queue_.empty() && !lapsed) {
      waiting_ = true;
      fiber_.suspend();
      waiting_ = false;
    }
    timer.cancel();
    if (queue_.empty()) return std::nullopt;
  }
  message m = std::move(queue_.front());
  queue_.pop_front();
  return m;
}

// What a handler is given: its inboxes, and what it says.
class fiber {
 public:
  fiber(client& c, detail::fiber_on& f) : client_(c), fiber_(f) {}
  [[nodiscard]] inbox subscribe(filter wants) { return inbox(client_, fiber_, std::move(wants)); }
  void sleep(std::chrono::milliseconds for_ms) { client_.sleep(fiber_, for_ms); }

  std::optional<std::string> send(const std::string& room, const content& c) { return post(room, message_of(c)); }
  std::optional<std::string> reply(const message& to, const content& c) {
    auto m = message_of(c);
    m.m_relates_to = relation{};
    m.m_relates_to->m_in_reply_to = relation::m_in_reply_to_t{};
    m.m_relates_to->m_in_reply_to->event_id = to.event_id;
    if (to.thread_root) {  // in its thread, where it is in one
      m.m_relates_to->rel_type = relation::rel_type_t(relation::rel_type_values::m_thread{});
      m.m_relates_to->event_id = to.thread_root;
      m.m_relates_to->is_falling_back = false;
    }
    return post(to.room_id, m);
  }
  std::optional<std::string> thread(const std::string& room, const std::string& root, const content& c) {
    auto m = message_of(c);
    m.m_relates_to = relation{};
    m.m_relates_to->rel_type = relation::rel_type_t(relation::rel_type_values::m_thread{});
    m.m_relates_to->event_id = root;
    m.m_relates_to->is_falling_back = true;
    m.m_relates_to->m_in_reply_to = relation::m_in_reply_to_t{};
    m.m_relates_to->m_in_reply_to->event_id = root;
    return post(room, m);
  }
  std::optional<std::string> edit(const std::string& room, const std::string& event_id, const content& c) {
    const auto s = detail::said_of(c);
    auto m = message_of(text{"* " + s.plain});
    m.formatted_body = "* " + s.markup;
    m.m_new_content = room_message::m_new_content_t{.msgtype = std::string("m.text"), .body = s.plain,
                                                    .format = std::string("org.matrix.custom.html"), .formatted_body = s.markup, .rest = {}};
    m.m_relates_to = relation{};
    m.m_relates_to->rel_type = relation::rel_type_t(relation::rel_type_values::m_replace{});
    m.m_relates_to->event_id = event_id;
    return post(room, m);
  }
  std::optional<std::string> react(const std::string& room, const std::string& event_id, std::string key) {
    using reaction = loom::ev::m_reaction_content_t;
    reaction r{};
    r.m_relates_to = reaction::reaction_relates_to_t{};
    r.m_relates_to->rel_type = reaction::reaction_relates_to_t::rel_type_t(reaction::reaction_relates_to_t::rel_type_values::m_annotation{});
    r.m_relates_to->event_id = event_id;
    r.m_relates_to->key = std::move(key);
    return post_event(room, "m.reaction", r);
  }
  bool typing(const std::string& room, bool on, std::chrono::milliseconds for_ms = std::chrono::milliseconds(30000)) {
    loom::cs::set_typing asked{};
    asked.user_id = client_.settings().user_id;
    asked.room_id = room;
    asked.body.typing = on;
    if (on) asked.body.timeout = for_ms.count();
    return client_.call(fiber_, asked, std::chrono::seconds(20)).has_value();
  }

  // The room's latest messages that a filter takes, oldest first, each with
  // whether this bot has reacted to it -- what a bot started again catches
  // up on.
  struct looked {
    message said;
    bool reacted = false;
  };
  std::vector<looked> recent(const std::string& room, const filter& wants, std::int64_t last = 50) {
    loom::cs::get_room_events asked{};
    asked.room_id = room;
    asked.dir = loom::cs::get_room_events::dir_t(loom::cs::get_room_events::dir_values::b{});
    asked.limit = last;
    std::vector<looked> out;
    const auto got = client_.call<loom::cs::get_room_events, detail::looked_history>(fiber_, asked, std::chrono::seconds(20));
    if (!got) return out;
    std::set<std::string> reacted;
    for (const auto& e : got->chunk)
      spl::visit(spl::overloaded{[&](const loom::ev::m_reaction_content_t& r) {
                                   if (e.sender == client_.settings().user_id && r.m_relates_to && r.m_relates_to->event_id)
                                     reacted.insert(*r.m_relates_to->event_id);
                                 },
                                 [](const auto&) {}},
                 e.content.data());
    for (const auto& e : got->chunk | std::views::reverse)
      spl::visit(spl::overloaded{[&](const loom::ev::m_room_message_content_t& said) {
                                   message m{.room_id = room, .event_id = e.event_id, .sender = e.sender, .body = said.body};
                                   if (said.m_new_content && said.m_new_content->body) {
                                     m.body = *said.m_new_content->body;
                                     m.edit = true;
                                   }
                                   if (wants.takes(m)) out.push_back({std::move(m), reacted.contains(e.event_id)});
                                 },
                                 [](const auto&) {}},
                 e.content.data());
    return out;
  }

 private:
  using room_message = loom::ev::m_room_message_content_t;
  using relation = room_message::m_relates_to_t;
  static room_message message_of(const content& c) {
    const auto s = detail::said_of(c);
    room_message m{};
    m.msgtype = "m.text";
    m.body = s.plain;
    m.format = "org.matrix.custom.html";
    m.formatted_body = s.markup;
    return m;
  }
  std::optional<std::string> post(const std::string& room, const room_message& m) { return post_event(room, "m.room.message", m); }
  template <class Content> std::optional<std::string> post_event(const std::string& room, std::string type, const Content& body) {
    std::string json;
    knot::write(json, body);
    loom::cs::send_message asked{.room_id = room, .event_type = std::move(type), .txn_id = client_.txn(), .body = knot::raw{std::move(json)}};
    if (auto sent = client_.call(fiber_, asked, std::chrono::seconds(20))) return std::move(sent->event_id);
    return std::nullopt;
  }
  client& client_;
  detail::fiber_on& fiber_;
};

namespace detail {
// What a bot reads of a sync: its place, and each joined room's timeline,
// messages typed and every other event kept as it came (knot::raw) -- not
// the whole response, whose readers for every kind of event a bot never
// looks at took more than a gigabyte to compile.
using heard_content = knot::tagged<"type", loom::ev::m_room_message_content_t, knot::raw>;
struct heard_event {
  heard_content content;
  std::string event_id;
  std::string sender;
  std::string type;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<heard_event>) { return knot::schema<heard_event>().member<"rest">(knot::rest); }
};
struct heard_timeline {
  std::vector<heard_event> events;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<heard_timeline>) { return knot::schema<heard_timeline>().member<"rest">(knot::rest); }
};
struct heard_room {
  std::optional<heard_timeline> timeline;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<heard_room>) { return knot::schema<heard_room>().member<"rest">(knot::rest); }
};
struct heard_rooms {
  std::optional<std::map<std::string, heard_room>> join;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<heard_rooms>) { return knot::schema<heard_rooms>().member<"rest">(knot::rest); }
};
struct heard_sync {
  std::string next_batch;
  std::optional<heard_rooms> rooms;
  knot::raw rest;
  friend consteval auto json_schema(knot::type<heard_sync>) { return knot::schema<heard_sync>().member<"rest">(knot::rest); }
};
}  // namespace detail

// The sync: every new message handed to every inbox that takes it.
inline void client::sync_forever(detail::fiber_on& f) {
  std::optional<std::string> since;
  if (session_.since_file)
    if (std::ifstream in(*session_.since_file); in)
      if (std::string token; std::getline(in, token) && !token.empty()) since = token;
  // Started afresh: from now, not the rooms' history.
  while (!since) {
    loom::cs::sync first{};
    first.timeout = 0;
    if (auto got = call<loom::cs::sync, detail::heard_sync>(f, first, std::chrono::seconds(30))) since = got->next_batch;
    else sleep(f, std::chrono::seconds(5));
  }
  for (;;) {
    loom::cs::sync asked{};
    asked.since = since;
    asked.timeout = 25000;
    auto got = call<loom::cs::sync, detail::heard_sync>(f, asked, std::chrono::seconds(30));
    if (!got) {
      sleep(f, std::chrono::seconds(5));
      continue;
    }
    since = got->next_batch;
    if (session_.since_file) std::ofstream(*session_.since_file) << *since;
    if (!got->rooms || !got->rooms->join) continue;
    for (const auto& [room, joined] : *got->rooms->join) {
      if (!joined.timeline) continue;
      for (const auto& event : joined.timeline->events)
        spl::visit(spl::overloaded{[&](const loom::ev::m_room_message_content_t& said) {
                                     message m{.room_id = room, .event_id = event.event_id, .sender = event.sender, .body = said.body};
                                     if (said.m_new_content && said.m_new_content->body) {
                                       m.body = *said.m_new_content->body;
                                       m.edit = true;
                                     }
                                     if (said.m_relates_to) {
                                       const auto& rel = *said.m_relates_to;
                                       if (rel.m_in_reply_to) m.reply_to = rel.m_in_reply_to->event_id;
                                       if (rel.rel_type)
                                         spl::visit(spl::overloaded{[&](const loom::ev::m_room_message_content_t::m_relates_to_t::rel_type_values::m_thread&) {
                                                                      m.thread_root = rel.event_id;
                                                                    },
                                                                    [](const auto&) {}},
                                                    *rel.rel_type);
                                     }
                                     deliver(m);
                                   },
                                   [](const auto&) {}},
                   event.content.data());
    }
  }
}

template <class... Handlers> void client::run(Handlers... handlers) {
  detail::fiber_on::start(io_, fibers_, [this](detail::fiber_on& f) { sync_forever(f); });
  live_ = sizeof...(Handlers);
  (detail::fiber_on::start(io_, fibers_,
                           [this, handler = std::move(handlers)](detail::fiber_on& f) mutable {
                             fiber said(*this, f);
                             handler(said);
                             // The last handler done: the bot is (the sync with it).
                             if (--live_ == 0) io_.stop();
                           }),
   ...);
  io_.run();
}
// One handler run on its own, with no sync: what a command line says once.
template <class Handler> void client::once(Handler handler) {
  detail::fiber_on::start(io_, fibers_, [this, handler = std::move(handler)](detail::fiber_on& f) mutable {
    fiber said(*this, f);
    handler(said);
  });
  io_.run();
}

// A bot run: its handlers each a fiber, until they all end (or forever).
template <class... Handlers> void run(session s, Handlers... handlers) {
  client(std::move(s)).run(std::move(handlers)...);
}

}  // namespace loom::bot
