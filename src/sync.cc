// /sync: what it answers, read into types, and a store of the rooms a client
// keeps between syncs, which each answer is applied to.
export module loom.sync;

import std;
export import knot;
export import loom.events;

export namespace loom {

struct timeline {
  std::vector<room_event> events;
  std::optional<bool> limited;
  std::optional<std::string> prev_batch;
};
consteval auto json_schema(knot::type<timeline>) { return knot::schema<timeline>(); }

struct state_events {
  std::vector<room_event> events;
};
consteval auto json_schema(knot::type<state_events>) { return knot::schema<state_events>(); }

struct joined_room {
  std::optional<loom::timeline> timeline;
  std::optional<state_events> state;
};
consteval auto json_schema(knot::type<joined_room>) { return knot::schema<joined_room>(); }

struct invite_state {
  std::vector<stripped_event> events;
};
consteval auto json_schema(knot::type<invite_state>) { return knot::schema<invite_state>(); }

struct invited_room {
  std::optional<loom::invite_state> invite_state;
};
consteval auto json_schema(knot::type<invited_room>) { return knot::schema<invited_room>(); }

struct left_room {
  std::optional<loom::timeline> timeline;
  std::optional<state_events> state;
};
consteval auto json_schema(knot::type<left_room>) { return knot::schema<left_room>(); }

struct rooms {
  std::optional<std::map<std::string, joined_room>> join;
  std::optional<std::map<std::string, invited_room>> invite;
  std::optional<std::map<std::string, left_room>> leave;
};
consteval auto json_schema(knot::type<rooms>) { return knot::schema<rooms>(); }

struct sync_response {
  std::string next_batch;
  std::optional<loom::rooms> rooms;
};
consteval auto json_schema(knot::type<sync_response>) { return knot::schema<sync_response>(); }

// A room as a client keeps it: its name and topic, who is in it, and the
// timeline since the last gap -- prev_batch, to page back from.
struct room {
  std::optional<std::string> name;
  std::optional<std::string> topic;
  std::map<std::string, std::string> members;  // user -> membership
  std::vector<room_event> timeline;
  std::optional<std::string> prev_batch;
};

// What a client keeps between syncs: where the next one starts, and its
// rooms. Each /sync answer is applied to it.
struct store {
  std::optional<std::string> since;
  std::map<std::string, room> joined;
  std::set<std::string> invited;

  constexpr void apply(const sync_response& sync) {
    since = sync.next_batch;
    if (!sync.rooms)
      return;
    if (sync.rooms->join)
      for (const auto& [id, got] : *sync.rooms->join) {
        invited.erase(id);
        room& kept = joined[id];
        if (got.state)
          for (const room_event& one : got.state->events)
            apply_state(kept, one);
        if (got.timeline) {
          // A gap: what is kept no longer runs on into what comes.
          if (got.timeline->limited.value_or(false))
            kept.timeline.clear();
          if (got.timeline->prev_batch && kept.timeline.empty())
            kept.prev_batch = got.timeline->prev_batch;
          for (const room_event& one : got.timeline->events) {
            if (one.state_key)
              apply_state(kept, one);
            kept.timeline.push_back(one);
          }
        }
      }
    if (sync.rooms->invite)
      for (const auto& [id, got] : *sync.rooms->invite)
        invited.insert(id);
    if (sync.rooms->leave)
      for (const auto& [id, got] : *sync.rooms->leave) {
        joined.erase(id);
        invited.erase(id);
      }
  }

 private:
  static constexpr void apply_state(room& kept, const room_event& one) {
    if (one.content.is<content::name>())
      kept.name = one.content.as<content::name>().name;
    else if (one.content.is<content::topic>())
      kept.topic = one.content.as<content::topic>().topic;
    else if (one.content.is<content::member>() && one.state_key)
      kept.members[*one.state_key] = one.content.as<content::member>().membership;
  }
};

}  // namespace loom
