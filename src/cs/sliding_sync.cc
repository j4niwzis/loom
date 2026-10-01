// SPDX-License-Identifier: AGPL-3.0-only
// Written by hand, not generated: Simplified Sliding Sync (MSC4186) is not in
// the specification yet, so no YAML of it to generate from. Its request and
// its answer, typed as the generated parts are -- what is not read kept as
// it came, in `rest`.
//
// POST /_matrix/client/unstable/org.matrix.simplified_msc3575/sync: the
// rooms of a window of a list sorted by activity, and the rooms subscribed
// to, each with the state asked for and its newest events; and the
// extensions -- account data, receipts, typing, to-device -- beside them.
export module loom.cs.sliding_sync;

import std;
export import splice;
export import knot;
export import loom.api;
export import loom.ev;
export import loom.cs.definitions;

export namespace loom::cs {

struct sliding_sync {
  struct body_t {
    // A list: the rooms of its window, by their activity, the newest first;
    // each with this state and this many of its newest events.
    struct list_t {
      std::vector<std::vector<std::int64_t>> ranges;
      std::vector<std::vector<std::string>> required_state;
      std::int64_t timeline_limit = 20;
      friend consteval auto json_schema(knot::type<list_t>) { return knot::schema<list_t>(); }
    };
    // A room followed whatever its place in a list: the one being read.
    struct subscription_t {
      std::vector<std::vector<std::string>> required_state;
      std::int64_t timeline_limit = 50;
      friend consteval auto json_schema(knot::type<subscription_t>) { return knot::schema<subscription_t>(); }
    };
    struct extension_t {
      std::optional<bool> enabled;
      std::optional<std::string> since;
      friend consteval auto json_schema(knot::type<extension_t>) { return knot::schema<extension_t>(); }
    };
    struct extensions_t {
      std::optional<extension_t> to_device;
      std::optional<extension_t> e2ee;
      std::optional<extension_t> account_data;
      std::optional<extension_t> receipts;
      std::optional<extension_t> typing;
      friend consteval auto json_schema(knot::type<extensions_t>) { return knot::schema<extensions_t>(); }
    };
    std::optional<std::string> conn_id;
    std::map<std::string, list_t> lists;
    std::optional<std::map<std::string, subscription_t>> room_subscriptions;
    std::optional<extensions_t> extensions;
    friend consteval auto json_schema(knot::type<body_t>) { return knot::schema<body_t>(); }
  };
  struct response_t {
    struct hero_t {
      std::string user_id;
      std::optional<std::string> displayname;
      std::optional<std::string> avatar_url;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<hero_t>) { return knot::schema<hero_t>().member<"rest">(knot::rest); }
    };
    // A room as the answer has it: what changed in it since the last.
    struct room_t {
      std::optional<std::string> name;
      std::optional<std::string> avatar;
      std::optional<std::vector<hero_t>> heroes;
      std::optional<bool> initial;
      std::optional<std::vector<loom::ev::timeline_event>> required_state;
      std::optional<std::vector<loom::ev::timeline_event>> timeline;
      std::optional<std::string> prev_batch;
      std::optional<bool> limited;
      std::optional<std::int64_t> num_live;
      std::optional<std::int64_t> bump_stamp;
      std::optional<std::int64_t> joined_count;
      std::optional<std::int64_t> invited_count;
      std::optional<std::int64_t> notification_count;
      std::optional<std::int64_t> highlight_count;
      std::optional<std::vector<loom::ev::stripped_event<loom::ev::state_content>>> invite_state;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<room_t>) { return knot::schema<room_t>().member<"rest">(knot::rest); }
    };
    struct list_t {
      std::optional<std::int64_t> count;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<list_t>) { return knot::schema<list_t>().member<"rest">(knot::rest); }
    };
    using other_event = loom::ev::basic_event<loom::ev::other_content>;
    struct account_data_t {
      std::optional<std::vector<other_event>> global;
      std::optional<std::map<std::string, std::vector<other_event>>> rooms;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<account_data_t>) { return knot::schema<account_data_t>().member<"rest">(knot::rest); }
    };
    // Receipts and typing: an event a room.
    struct by_room_t {
      std::optional<std::map<std::string, other_event>> rooms;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<by_room_t>) { return knot::schema<by_room_t>().member<"rest">(knot::rest); }
    };
    // MSC3885: what came for this device, and where to go on from.
    struct to_device_t {
      std::string next_batch;
      std::optional<std::vector<loom::ev::to_device_event>> events;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<to_device_t>) { return knot::schema<to_device_t>().member<"rest">(knot::rest); }
    };
    // MSC3884: whose devices changed, and how many one-time keys are left.
    struct device_lists_t {
      std::optional<std::vector<std::string>> changed;
      std::optional<std::vector<std::string>> left;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<device_lists_t>) { return knot::schema<device_lists_t>().member<"rest">(knot::rest); }
    };
    struct e2ee_t {
      std::optional<std::map<std::string, std::int64_t>> device_one_time_keys_count;
      std::optional<device_lists_t> device_lists;
      std::optional<std::vector<std::string>> device_unused_fallback_key_types;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<e2ee_t>) { return knot::schema<e2ee_t>().member<"rest">(knot::rest); }
    };
    struct extensions_t {
      std::optional<account_data_t> account_data;
      std::optional<by_room_t> receipts;
      std::optional<by_room_t> typing;
      std::optional<to_device_t> to_device;
      std::optional<e2ee_t> e2ee;
      knot::raw rest;
      friend consteval auto json_schema(knot::type<extensions_t>) { return knot::schema<extensions_t>().member<"rest">(knot::rest); }
    };
    std::string pos;
    std::optional<std::map<std::string, list_t>> lists;
    std::optional<std::map<std::string, room_t>> rooms;
    std::optional<extensions_t> extensions;
    knot::raw rest;
    friend consteval auto json_schema(knot::type<response_t>) { return knot::schema<response_t>().member<"rest">(knot::rest); }
  };
  // Where the last answer left off -- none, from the start -- and how long
  // the server may hold the request for news.
  std::optional<std::string> pos;
  std::optional<std::int64_t> timeout;
  body_t body;
  using response = response_t;
  constexpr request to_send() const {
    std::string target = "/_matrix/client/unstable/org.matrix.simplified_msc3575/sync";
    if (pos)
      detail::query(target, "pos", detail::text((*pos)));
    if (timeout)
      detail::query(target, "timeout", detail::text((*timeout)));
    return {method::post{}, std::move(target), detail::json(body), true};
  }
};

}  // namespace loom::cs
