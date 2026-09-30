// SPDX-License-Identifier: AGPL-3.0-only
// What a client keeps between syncs, on the generated types: each room's
// state by (type, state_key), its timeline since the last gap, its summary,
// counts, account data and ephemeral events; the rooms by membership; the
// account's own account data and presence. Each /sync answer is applied as
// the client-server API says (Syncing; Redactions; room versions for what a
// redaction keeps).
export module loom.state;

import std;
import splice;
export import knot;
export import loom.ev;
export import loom.cs.sync;

export namespace loom::client {

// The string a choice holds: the one its alternative names, or the one kept.
template <class... Alternatives>
constexpr std::string_view choice_text(const splice::variant<Alternatives...>& one) {
  return splice::visit(
      [](const auto& held) -> std::string_view {
        using type = std::remove_cvref_t<decltype(held)>;
        if constexpr (requires { type::json_value; })
          return type::json_value;
        else
          return held;
      },
      one);
}

// What a redaction leaves of an event's content, by room version (the
// specification's room versions, "Redactions"): the keys kept for each type.
// A version not written as a number is taken to follow the latest rules.
// An object as its keys, each value kept as its JSON text: for cutting an
// object by key without reading what is under the keys.
using members = std::map<std::string, knot::raw, std::less<>>;

// The to-device events of a sync, each kept as it came.
struct to_device_events {
  std::vector<knot::raw> events;
};
consteval auto json_schema(knot::type<to_device_events>) { return knot::schema<to_device_events>(); }

struct redaction_rules {
  int version = 11;

  static constexpr redaction_rules of(std::string_view room_version) {
    int number = 0;
    for (char c : room_version) {
      if (c < '0' || c > '9')
        return {11};
      number = number * 10 + (c - '0');
      if (number > 1000)
        return {11};
    }
    return {room_version.empty() ? 1 : number};
  }

  // Whether the whole of a type's content is kept.
  constexpr bool keeps_all(std::string_view type) const { return version >= 11 && type == "m.room.create"; }

  constexpr std::vector<std::string_view> kept(std::string_view type) const {
    std::vector<std::string_view> keys;
    if (type == "m.room.member") {
      keys.push_back("membership");
      if (version >= 9)
        keys.push_back("join_authorised_via_users_server");
    } else if (type == "m.room.create") {
      if (version < 11)
        keys.push_back("creator");
    } else if (type == "m.room.join_rules") {
      keys.push_back("join_rule");
      if (version >= 8)
        keys.push_back("allow");
    } else if (type == "m.room.power_levels") {
      for (std::string_view one : {"ban", "events", "events_default", "kick", "redact", "state_default", "users",
                                   "users_default"})
        keys.push_back(one);
      if (version >= 11)
        keys.push_back("invite");
    } else if (type == "m.room.aliases") {
      if (version <= 5)
        keys.push_back("aliases");
    } else if (type == "m.room.history_visibility") {
      keys.push_back("history_visibility");
    } else if (type == "m.room.redaction") {
      if (version >= 11)
        keys.push_back("redacts");
    }
    return keys;
  }

  // The content a redaction leaves: the kept keys, and from version 11 on,
  // of a member event's third_party_invite, only its signed. The content is
  // read once as its keys, each value kept as its text: no tree.
  constexpr knot::raw redact(std::string_view type, const knot::raw& content) const {
    if (keeps_all(type))
      return content;
    members left;
    const auto all = knot::try_read<members>(content.text);
    if (!all)
      return knot::raw{"{}"};
    for (std::string_view key : kept(type))
      if (const auto found = all->find(key); found != all->end())
        left.emplace(found->first, found->second);
    if (version >= 11 && type == "m.room.member")
      if (const auto invite = all->find("third_party_invite"); invite != all->end())
        if (const auto inner = knot::try_read<members>(invite->second.text))
          if (const auto signed_ = inner->find("signed"); signed_ != inner->end())
            left.emplace("third_party_invite", knot::raw{knot::to_json_string(members{{signed_->first, signed_->second}})});
    return knot::raw{knot::to_json_string(left)};
  }
};

// A room's state: the latest event for each (type, state_key).
struct room_state {
  std::map<std::pair<std::string, std::string>, ev::timeline_event, std::less<>> events;

  constexpr const ev::timeline_event* find(std::string_view type, std::string_view state_key = "") const {
    const auto found = events.find(std::pair<std::string, std::string>(type, state_key));
    return found == events.end() ? nullptr : &found->second;
  }

  // The content of a state event, as its type, where it is there and fits.
  template <class Content>
  constexpr const Content* content(std::string_view type, std::string_view state_key = "") const {
    const auto* one = find(type, state_key);
    if (!one || !one->content.template is<Content>())
      return nullptr;
    return &one->content.template as<Content>();
  }

  constexpr void set(const ev::timeline_event& one) {
    if (one.state_key)
      events.insert_or_assign(std::pair<std::string, std::string>(one.type, *one.state_key), one);
  }

  constexpr std::optional<std::string> name() const {
    if (const auto* got = content<ev::m_room_name_content_t>("m.room.name"))
      return got->name;
    return std::nullopt;
  }
  constexpr std::optional<std::string> topic() const {
    if (const auto* got = content<ev::m_room_topic_content_t>("m.room.topic"))
      return got->topic;
    return std::nullopt;
  }
  constexpr std::optional<std::string> avatar_url() const {
    if (const auto* got = content<ev::m_room_avatar_content_t>("m.room.avatar"))
      return got->url;
    return std::nullopt;
  }
  constexpr std::optional<std::string> canonical_alias() const {
    if (const auto* got = content<ev::m_room_canonical_alias_content_t>("m.room.canonical_alias"))
      return got->alias;
    return std::nullopt;
  }
  constexpr bool encrypted() const { return find("m.room.encryption") != nullptr; }
  constexpr std::string room_version() const {
    if (const auto* got = content<ev::m_room_create_content_t>("m.room.create"))
      if (got->room_version)
        return *got->room_version;
    return "1";  // m.room.create's default
  }
  // A user's membership, as its string ("join", "leave", ...).
  constexpr std::optional<std::string> membership(std::string_view user) const {
    if (const auto* got = content<ev::m_room_member_content_t>("m.room.member", user))
      return std::string(choice_text(got->membership));
    return std::nullopt;
  }
  constexpr std::optional<std::string> display_name(std::string_view user) const {
    if (const auto* got = content<ev::m_room_member_content_t>("m.room.member", user))
      return got->displayname;
    return std::nullopt;
  }
  // The users whose membership is the one asked.
  constexpr std::vector<std::string> members(std::string_view membership = "join") const {
    std::vector<std::string> out;
    for (const auto& [key, one] : events)
      if (key.first == "m.room.member" && one.content.template is<ev::m_room_member_content_t>() &&
          choice_text(one.content.template as<ev::m_room_member_content_t>().membership) == membership)
        out.push_back(key.second);
    return out;
  }
};

struct room_summary {
  std::vector<std::string> heroes;
  std::int64_t joined_members = 0;
  std::int64_t invited_members = 0;
};

struct unread_counts {
  std::int64_t highlight = 0;
  std::int64_t notification = 0;
};

using other_event = ev::basic_event<ev::other_content>;
using stripped_state = std::map<std::pair<std::string, std::string>, ev::stripped_event<ev::state_content>, std::less<>>;

struct joined_room {
  room_state state;
  // The timeline since the last gap, oldest first; prev_batch pages back from
  // its start.
  std::vector<ev::timeline_event> timeline;
  std::optional<std::string> prev_batch;
  room_summary summary;
  unread_counts unread;
  std::map<std::string, unread_counts, std::less<>> unread_by_thread;
  std::map<std::string, other_event, std::less<>> account_data;  // by type
  std::vector<std::string> typing;
  std::vector<other_event> ephemeral;  // the last sync's, receipts among them
};

struct left_room {
  room_state state;
  std::vector<ev::timeline_event> timeline;
  std::optional<std::string> prev_batch;
  std::map<std::string, other_event, std::less<>> account_data;
};

struct state {
  std::optional<std::string> since;
  std::map<std::string, joined_room, std::less<>> joined;
  std::map<std::string, stripped_state, std::less<>> invited;
  std::map<std::string, stripped_state, std::less<>> knocked;
  std::map<std::string, left_room, std::less<>> left;
  std::map<std::string, other_event, std::less<>> account_data;  // by type
  std::map<std::string, other_event, std::less<>> presence;      // by user
  std::vector<knot::raw> to_device;                               // the last sync's
  std::optional<knot::raw> device_lists;
  std::map<std::string, std::int64_t> one_time_keys_count;

  // An answer applied. use_state_after says whether the request asked for
  // state_after: then the state there is the state at the end of the
  // timeline, and the timeline's state events are not applied again.
  constexpr void apply(const cs::sync::response& sync, bool use_state_after = false) {
    since = sync.next_batch;
    if (sync.account_data && sync.account_data->events)
      for (const auto& one : *sync.account_data->events)
        account_data.insert_or_assign(one.type, one);
    if (sync.presence && sync.presence->events)
      for (const auto& one : *sync.presence->events)
        if (one.sender)
          presence.insert_or_assign(*one.sender, one);
    to_device.clear();
    if (sync.to_device)
      if (auto got = knot::try_read<to_device_events>(sync.to_device->text))
        to_device = std::move(got->events);
    if (sync.device_lists)
      device_lists = sync.device_lists;
    if (sync.device_one_time_keys_count)
      one_time_keys_count = *sync.device_one_time_keys_count;
    if (!sync.rooms)
      return;
    const auto& rooms = *sync.rooms;
    if (rooms.join)
      for (const auto& [id, got] : *rooms.join) {
        invited.erase(id);
        knocked.erase(id);
        auto was_left = left.find(id);
        joined_room& kept = joined[id];
        if (was_left != left.end()) {
          kept.state = std::move(was_left->second.state);
          left.erase(was_left);
        }
        apply_joined(kept, got, use_state_after);
      }
    if (rooms.invite)
      for (const auto& [id, got] : *rooms.invite) {
        left.erase(id);
        knocked.erase(id);
        stripped_state& kept = invited[id];
        if (got.invite_state && got.invite_state->events)
          for (const auto& one : *got.invite_state->events)
            kept.insert_or_assign(std::pair<std::string, std::string>(one.type, one.state_key), one);
      }
    if (rooms.knock)
      for (const auto& [id, got] : *rooms.knock) {
        left.erase(id);
        invited.erase(id);
        stripped_state& kept = knocked[id];
        if (got.knock_state && got.knock_state->events)
          for (const auto& one : *got.knock_state->events)
            kept.insert_or_assign(std::pair<std::string, std::string>(one.type, one.state_key), one);
      }
    if (rooms.leave)
      for (const auto& [id, got] : *rooms.leave) {
        invited.erase(id);
        knocked.erase(id);
        left_room& kept = left[id];
        if (auto was = joined.find(id); was != joined.end()) {
          kept.state = std::move(was->second.state);
          kept.account_data = std::move(was->second.account_data);
          joined.erase(was);
        }
        apply_state_and_timeline(kept.state, kept.timeline, kept.prev_batch, got.state ? &*got.state : nullptr,
                                 got.state_after ? &*got.state_after : nullptr,
                                 got.timeline ? &*got.timeline : nullptr, use_state_after);
        if (got.account_data && got.account_data->events)
          for (const auto& one : *got.account_data->events)
            kept.account_data.insert_or_assign(one.type, one);
      }
  }

 private:
  template <class Joined>
  static constexpr void apply_joined(joined_room& kept, const Joined& got, bool use_state_after) {
    apply_state_and_timeline(kept.state, kept.timeline, kept.prev_batch, got.state ? &*got.state : nullptr,
                             got.state_after ? &*got.state_after : nullptr, got.timeline ? &*got.timeline : nullptr,
                             use_state_after);
    // The summary's fields come only when they change.
    if (got.summary) {
      if (got.summary->m_heroes)
        kept.summary.heroes = *got.summary->m_heroes;
      if (got.summary->m_joined_member_count)
        kept.summary.joined_members = *got.summary->m_joined_member_count;
      if (got.summary->m_invited_member_count)
        kept.summary.invited_members = *got.summary->m_invited_member_count;
    }
    if (got.unread_notifications) {
      kept.unread.highlight = got.unread_notifications->highlight_count.value_or(0);
      kept.unread.notification = got.unread_notifications->notification_count.value_or(0);
    }
    if (got.unread_thread_notifications)
      for (const auto& [thread, counts] : *got.unread_thread_notifications)
        kept.unread_by_thread.insert_or_assign(
            thread, unread_counts{counts.highlight_count.value_or(0), counts.notification_count.value_or(0)});
    if (got.account_data && got.account_data->events)
      for (const auto& one : *got.account_data->events)
        kept.account_data.insert_or_assign(one.type, one);
    kept.ephemeral.clear();
    if (got.ephemeral && got.ephemeral->events)
      for (const auto& one : *got.ephemeral->events) {
        if (one.content.template is<ev::m_typing_content_t>())
          kept.typing = one.content.template as<ev::m_typing_content_t>().user_ids;
        kept.ephemeral.push_back(one);
      }
  }

  template <class State, class StateAfter, class Timeline>
  static constexpr void apply_state_and_timeline(room_state& state, std::vector<ev::timeline_event>& timeline,
                                                 std::optional<std::string>& prev_batch, const State* before,
                                                 const StateAfter* after, const Timeline* got,
                                                 bool use_state_after) {
    if (use_state_after) {
      if (after && after->events)
        for (const auto& one : *after->events)
          state.set(one);
    } else if (before && before->events) {
      for (const auto& one : *before->events)
        state.set(one);
    }
    if (!got)
      return;
    // A gap: what is kept no longer runs on into what comes.
    if (got->limited.value_or(false))
      timeline.clear();
    if (timeline.empty())
      prev_batch = got->prev_batch;
    const redaction_rules rules = redaction_rules::of(state.room_version());
    for (const auto& one : got->events) {
      if (one.state_key && !use_state_after)
        state.set(one);
      timeline.push_back(one);
      if (one.type == "m.room.redaction")
        redact(state, timeline, one, rules);
    }
  }

  // What a redaction redacts, wherever it is kept: its content cut to what the
  // room version keeps, and the redaction in its unsigned.
  static constexpr void redact(room_state& state, std::vector<ev::timeline_event>& timeline,
                               const ev::timeline_event& redaction, const redaction_rules& rules) {
    std::optional<std::string> target = redaction.redacts;
    if (redaction.content.template is<ev::m_room_redaction_content_t>())
      if (const auto& redacts = redaction.content.template as<ev::m_room_redaction_content_t>().redacts)
        target = *redacts;
    if (!target)
      return;
    // The event cut by its keys: its content to what the rules keep, and
    // the redaction put in its unsigned; then read back into its type.
    const auto cut = [&](ev::timeline_event& one) {
      auto all = knot::try_read<members>(knot::to_json_string(one));
      if (!all)
        return;
      const auto content = all->find("content");
      knot::raw left = rules.redact(one.type, content != all->end() ? content->second : knot::raw{});
      all->insert_or_assign("content", std::move(left));
      all->insert_or_assign("unsigned",
                            knot::raw{knot::to_json_string(members{{"redacted_because",
                                                                    knot::raw{knot::to_json_string(redaction)}}})});
      if (auto made = knot::try_read<ev::timeline_event>(knot::to_json_string(*all)))
        one = std::move(*made);
    };
    for (auto& one : timeline)
      if (one.event_id == *target && &one != &timeline.back())
        cut(one);
    for (auto& [key, one] : state.events)
      if (one.event_id == *target)
        cut(one);
  }
};

// ---- What a client reads of the state and says in a room, typed here once ----

// Who has read up to where, as m.receipt says it: its content is event, then
// kind, then user, to when -- read here into that; the private kind reads up
// to the same place, only for the user.
struct read_receipt {
  std::string event_id;
  std::string user;
  std::optional<std::int64_t> ts;
};
namespace receipt_kind {
struct read {};
struct other {};
}  // namespace receipt_kind
using receipt_kind_t = splice::variant<receipt_kind::read, receipt_kind::other>;
inline receipt_kind_t receipt_kind_of(std::string_view key) {
  static constexpr std::array<std::string_view, 2> reads{"m.read", "m.read.private"};
  return std::ranges::contains(reads, key) ? receipt_kind_t{receipt_kind::read{}} : receipt_kind_t{receipt_kind::other{}};
}
struct receipt_at {
  std::optional<std::int64_t> ts;
  friend consteval auto json_schema(knot::type<receipt_at>) { return knot::schema<receipt_at>(); }
};
using receipts_by_event = std::map<std::string, std::map<std::string, std::map<std::string, receipt_at>>>;

// The read receipts an ephemeral event carries: none where it is not m.receipt.
inline std::vector<read_receipt> receipts_of(const other_event& one) {
  std::vector<read_receipt> out;
  splice::visit(splice::overloaded{[&](const ev::m_receipt_content_t& content) {
                                     const auto all = knot::try_read<receipts_by_event>(content.rest.text);
                                     if (!all)
                                       return;
                                     for (const auto& [event_id, kinds] : *all)
                                       for (const auto& [kind, users] : kinds)
                                         splice::visit(splice::overloaded{[&](receipt_kind::read) {
                                                                            for (const auto& [user, at] : users)
                                                                              out.push_back({event_id, user, at.ts});
                                                                          },
                                                                          [](receipt_kind::other) {}},
                                                       receipt_kind_of(kind));
                                   },
                                   [](const auto&) {}},
                one.content.data());
  return out;
}

// m.direct: each person's direct rooms, as the user's account data says.
using direct_rooms_t = std::map<std::string, std::vector<std::string>>;
inline direct_rooms_t direct_rooms(const state& all) {
  direct_rooms_t out;
  if (const auto found = all.account_data.find("m.direct"); found != all.account_data.end())
    splice::visit(splice::overloaded{[&](const ev::m_direct_content_t& content) {
                                       if (auto got = knot::try_read<direct_rooms_t>(content.rest.text))
                                         out = std::move(*got);
                                     },
                                     [](const auto&) {}},
                  found->second.content.data());
  return out;
}
inline bool is_direct(const state& all, std::string_view room) {
  return std::ranges::any_of(direct_rooms(all), [&](const auto& one) { return std::ranges::contains(one.second, room); });
}

// The images of the packs (MSC2545) a room offers: the user's own, the room's,
// and those of other rooms the user took everywhere -- each shortcode once,
// the first found, and only those the use asked for allows.
namespace image_use {
struct emoticon {};
struct sticker {};
struct other {};
}  // namespace image_use
using image_use_t = splice::variant<image_use::emoticon, image_use::sticker, image_use::other>;
inline image_use_t image_use_of(std::string_view word) {
  if (word == "emoticon")
    return image_use::emoticon{};
  if (word == "sticker")
    return image_use::sticker{};
  return image_use::other{};
}
struct pack_image {
  std::string shortcode;
  std::string url;
};
namespace detail {
// Whether a usage list lets an image be used so: missing or empty, as any.
inline bool allows(const auto& usage, const image_use_t& wanted) {
  if (!usage || usage->empty())
    return true;
  return std::ranges::any_of(*usage, [&](const auto& one) {
    return image_use_of(choice_text(one)).index() == wanted.index();
  });
}
inline void add_pack(const auto& content, const image_use_t& wanted, std::vector<pack_image>& out) {
  for (const auto& [shortcode, image] : content.images) {
    const bool allowed = image.usage ? allows(image.usage, wanted)
                                     : !content.pack || allows(content.pack->usage, wanted);
    if (allowed && image.url.starts_with("mxc://") && !std::ranges::contains(out, shortcode, &pack_image::shortcode))
      out.push_back({shortcode, image.url});
  }
}
// A state or account-data event's pack, whichever of its names it came under.
inline void add_packs_of(const auto& content, const image_use_t& wanted, std::vector<pack_image>& out) {
  splice::visit(splice::overloaded{[&](const ev::m_room_image_pack_content_t& one) { add_pack(one, wanted, out); },
                                   [&](const ev::im_ponies_room_emotes_content_t& one) { add_pack(one, wanted, out); },
                                   [&](const ev::im_ponies_user_emotes_content_t& one) { add_pack(one, wanted, out); },
                                   [](const auto&) {}},
                content);
}
}  // namespace detail
inline std::vector<pack_image> images(const state& all, const joined_room& room, const image_use_t& wanted) {
  std::vector<pack_image> out;
  if (const auto own = all.account_data.find("im.ponies.user_emotes"); own != all.account_data.end())
    detail::add_packs_of(own->second.content.data(), wanted, out);
  for (const auto& [key, one] : room.state.events)
    detail::add_packs_of(one.content.data(), wanted, out);
  // The packs of other rooms taken everywhere: room, then the state keys.
  const auto everywhere = [&](const auto& rooms) {
    for (const auto& [room_id, keys] : rooms)
      if (const auto joined = all.joined.find(room_id); joined != all.joined.end())
        for (const auto& [key, one] : joined->second.state.events)
          if (keys.contains(key.second))
            detail::add_packs_of(one.content.data(), wanted, out);
  };
  for (const std::string_view type : {"im.ponies.emote_rooms", "m.image_pack.rooms"})
    if (const auto chosen = all.account_data.find(type); chosen != all.account_data.end())
      splice::visit(splice::overloaded{[&](const ev::im_ponies_emote_rooms_content_t& one) { everywhere(one.rooms); },
                                       [&](const ev::m_image_pack_rooms_content_t& one) { everywhere(one.rooms); },
                                       [](const auto&) {}},
                    chosen->second.content.data());
  return out;
}

// Whether a message mentions a user: as its m.mentions says, and before
// those, by the user's ID in its text.
inline bool mentions(const ev::m_room_message_content_t& content, std::string_view user) {
  if (const auto& said = content.m_mentions)
    return (said->user_ids && std::ranges::contains(*said->user_ids, user)) || said->room.value_or(false);
  return content.body.find(user) != std::string::npos;
}

// ---- What is said: the contents a client sends, made ----

inline constexpr std::string_view kHtml = "org.matrix.custom.html";

// Text, rich where HTML is given, an answer to a message where one is, and
// who it mentions (Matrix 1.7).
struct text_said {
  std::string body;
  std::optional<std::string> html;
  std::optional<std::string> reply_to;
  std::vector<std::string> mentions;
};
inline ev::m_room_message_m_text_content_t text_message(const text_said& said) {
  ev::m_room_message_m_text_content_t content;
  content.body = said.body;
  if (said.html) {
    content.format = std::string(kHtml);
    content.formatted_body = *said.html;
  }
  if (said.reply_to)
    content.m_relates_to.emplace().m_in_reply_to.emplace().event_id = *said.reply_to;
  if (!said.mentions.empty())
    content.m_mentions.emplace().user_ids = said.mentions;
  return content;
}
// An edit of a message: the new text in m.new_content, "* " before it for
// clients that do not know edits.
inline ev::m_room_message_m_text_content_t edit_message(std::string_view event, const std::string& text,
                                                         const std::optional<std::string>& html) {
  ev::m_room_message_m_text_content_t content;
  content.body = "* " + text;
  if (html) {
    content.format = std::string(kHtml);
    content.formatted_body = "* " + *html;
  }
  auto& now = content.m_new_content.emplace();
  now.msgtype = "m.text";
  now.body = text;
  if (html) {
    now.format = std::string(kHtml);
    now.formatted_body = *html;
  }
  auto& relates = content.m_relates_to.emplace();
  relates.rel_type = ev::m_room_message_m_text_content_t::m_relates_to_t::rel_type_values::m_replace{};
  relates.event_id = std::string(event);
  return content;
}
// A reaction: a key, an emoji or an image's mxc URI, on a message.
inline ev::m_reaction_content_t reaction(std::string_view target, std::string_view key) {
  ev::m_reaction_content_t content;
  auto& relates = content.m_relates_to.emplace();
  relates.rel_type = ev::m_reaction_content_t::reaction_relates_to_t::rel_type_values::m_annotation{};
  relates.event_id = std::string(target);
  relates.key = std::string(key);
  return content;
}
// A picture or a file sent: where it is kept, its name, its caption as its
// body (its name where it has none), what it is -- an answer where one is.
struct media_said {
  std::string uri;
  std::string name;
  std::string caption;
  std::string mimetype;
  std::int64_t size = 0;
  std::optional<std::string> reply_to;
};
namespace detail {
inline void fill_media(auto& content, const media_said& said) {
  content.body = said.caption.empty() ? said.name : said.caption;
  content.filename = said.name;
  content.url = said.uri;
  auto& info = content.info.emplace();
  info.mimetype = said.mimetype;
  info.size = said.size;
  if (said.reply_to)
    content.m_relates_to.emplace().m_in_reply_to.emplace().event_id = *said.reply_to;
}
}  // namespace detail
inline ev::m_room_message_m_image_content_t picture_message(const media_said& said, std::int64_t width, std::int64_t height) {
  ev::m_room_message_m_image_content_t content;
  detail::fill_media(content, said);
  content.info->w = width;
  content.info->h = height;
  return content;
}
inline ev::m_room_message_m_file_content_t file_message(const media_said& said) {
  ev::m_room_message_m_file_content_t content;
  detail::fill_media(content, said);
  return content;
}

}  // namespace loom::client
