// Room events: the envelope, and the content chosen by the event's "type"
// with knot::tagged -- read straight into its type, the ones loom knows;
// any other kept as a knot::value.
export module loom.events;

import std;
export import knot;

export namespace loom {

namespace content {

struct message {
  std::string msgtype;
  std::string body;
  std::optional<std::string> format;
  std::optional<std::string> formatted_body;
};
consteval auto json_schema(knot::type<message>) { return knot::schema<message>().tag("m.room.message"); }

struct member {
  std::string membership;  // join, invite, leave, ban, knock
  std::optional<std::string> displayname;
  std::optional<std::string> avatar_url;
};
consteval auto json_schema(knot::type<member>) { return knot::schema<member>().tag("m.room.member"); }

struct name {
  std::string name;
};
consteval auto json_schema(knot::type<name>) { return knot::schema<name>().tag("m.room.name"); }

struct topic {
  std::string topic;
};
consteval auto json_schema(knot::type<topic>) { return knot::schema<topic>().tag("m.room.topic"); }

struct create {
  std::optional<std::string> creator;
  std::optional<std::string> room_version;
};
consteval auto json_schema(knot::type<create>) { return knot::schema<create>().tag("m.room.create"); }

}  // namespace content

using room_content = knot::tagged<"type", content::message, content::member, content::name, content::topic,
                                  content::create, knot::value>;

// A room event as /sync and /messages give it.
struct room_event {
  std::string type;
  room_content content;
  std::string event_id;
  std::string sender;
  std::optional<std::int64_t> origin_server_ts;
  std::optional<std::string> state_key;  // there for a state event
};
consteval auto json_schema(knot::type<room_event>) { return knot::schema<room_event>(); }

// A state event as an invite shows it: stripped of all but its type,
// content, sender and state key.
struct stripped_event {
  std::string type;
  room_content content;
  std::string sender;
  std::string state_key;
};
consteval auto json_schema(knot::type<stripped_event>) { return knot::schema<stripped_event>(); }

}  // namespace loom
