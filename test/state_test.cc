// SPDX-License-Identifier: AGPL-3.0-only
// The client's state, applied from /sync answers: state and timeline, a gap,
// a redaction cut as the room version says, the rooms moving between
// memberships, and what the account keeps.
import std;
import knot;
import loom.state;
import gtest;

#include "gtest/gtest-macros.h"

namespace {

const std::string first = R"({
  "next_batch": "s1",
  "account_data": {"events": [{"type": "m.direct", "content": {"@b:x.org": ["!r:x.org"]}}]},
  "presence": {"events": [{"type": "m.presence", "sender": "@b:x.org", "content": {"presence": "online"}}]},
  "rooms": {
    "join": {"!r:x.org": {
      "summary": {"m.heroes": ["@b:x.org"], "m.joined_member_count": 2},
      "state": {"events": [
        {"type": "m.room.create", "state_key": "", "event_id": "$c", "sender": "@a:x.org", "origin_server_ts": 1,
         "content": {"room_version": "11"}},
        {"type": "m.room.member", "state_key": "@a:x.org", "event_id": "$m1", "sender": "@a:x.org", "origin_server_ts": 2,
         "content": {"membership": "join", "displayname": "A"}}
      ]},
      "timeline": {"limited": true, "prev_batch": "p0", "events": [
        {"type": "m.room.name", "state_key": "", "event_id": "$n", "sender": "@a:x.org", "origin_server_ts": 3,
         "content": {"name": "Garden"}},
        {"type": "m.room.member", "state_key": "@b:x.org", "event_id": "$m2", "sender": "@b:x.org", "origin_server_ts": 4,
         "content": {"membership": "join", "displayname": "B", "avatar_url": "mxc://x.org/b"}},
        {"type": "m.room.message", "event_id": "$t", "sender": "@b:x.org", "origin_server_ts": 5,
         "content": {"msgtype": "m.text", "body": "hello"}}
      ]},
      "ephemeral": {"events": [{"type": "m.typing", "content": {"user_ids": ["@b:x.org"]}}]},
      "unread_notifications": {"highlight_count": 1, "notification_count": 3}
    }},
    "invite": {"!i:x.org": {"invite_state": {"events": [
      {"type": "m.room.name", "state_key": "", "sender": "@c:x.org", "content": {"name": "Invited"}}
    ]}}}
  }
})";

const std::string second = R"({
  "next_batch": "s2",
  "rooms": {
    "join": {
      "!r:x.org": {"timeline": {"limited": false, "prev_batch": "p1", "events": [
        {"type": "m.room.redaction", "event_id": "$x", "sender": "@a:x.org", "origin_server_ts": 6,
         "content": {"redacts": "$m2", "reason": "spam"}},
        {"type": "m.room.redaction", "event_id": "$y", "sender": "@a:x.org", "origin_server_ts": 7,
         "content": {"redacts": "$t"}}
      ]}},
      "!i:x.org": {"timeline": {"events": []}}
    },
    "leave": {"!gone:x.org": {"timeline": {"events": []}}}
  }
})";

loom::cs::sync::response read(const std::string& text) {
  auto got = knot::try_read<loom::cs::sync::response>(text);
  EXPECT_TRUE(got.has_value()) << (got ? "" : got.error().message);
  return got ? std::move(*got) : loom::cs::sync::response{};
}

TEST(State, Applied) {
  loom::client::state kept;
  kept.apply(read(first));
  EXPECT_EQ(kept.since, "s1");
  ASSERT_TRUE(kept.joined.contains("!r:x.org"));
  const auto& room = kept.joined.at("!r:x.org");
  EXPECT_EQ(room.state.name(), "Garden");
  EXPECT_EQ(room.state.room_version(), "11");
  EXPECT_EQ(room.state.membership("@b:x.org"), "join");
  EXPECT_EQ(room.state.display_name("@a:x.org"), "A");
  EXPECT_EQ(room.state.members(), (std::vector<std::string>{"@a:x.org", "@b:x.org"}));
  EXPECT_EQ(room.timeline.size(), 3u);
  EXPECT_EQ(room.prev_batch, "p0");
  EXPECT_EQ(room.summary.heroes, (std::vector<std::string>{"@b:x.org"}));
  EXPECT_EQ(room.summary.joined_members, 2);
  EXPECT_EQ(room.typing, (std::vector<std::string>{"@b:x.org"}));
  EXPECT_EQ(room.unread.notification, 3);
  EXPECT_TRUE(kept.account_data.contains("m.direct"));
  EXPECT_TRUE(kept.presence.contains("@b:x.org"));
  ASSERT_TRUE(kept.invited.contains("!i:x.org"));

  kept.apply(read(second));
  EXPECT_EQ(kept.since, "s2");
  const auto& after = kept.joined.at("!r:x.org");
  // Not a gap: the timeline runs on, and prev_batch stays where it began.
  EXPECT_EQ(after.timeline.size(), 5u);
  EXPECT_EQ(after.prev_batch, "p0");
  // The member event redacted in state and timeline: version 11 keeps its
  // membership only.
  const auto* member = after.state.find("m.room.member", "@b:x.org");
  ASSERT_NE(member, nullptr);
  EXPECT_EQ(after.state.membership("@b:x.org"), "join");
  EXPECT_EQ(after.state.display_name("@b:x.org"), std::nullopt);
  ASSERT_TRUE(member->unsigned_.has_value());
  EXPECT_TRUE(member->unsigned_->redacted_because.has_value());
  // The message: nothing of its content left.
  EXPECT_TRUE(knot::to_json_string(after.timeline.at(2)).contains(R"("content":{})"));
  // Invited, then joined: no longer invited.
  EXPECT_FALSE(kept.invited.contains("!i:x.org"));
  EXPECT_TRUE(kept.joined.contains("!i:x.org"));
  EXPECT_TRUE(kept.left.contains("!gone:x.org"));
}

TEST(State, RedactionRulesByVersion) {
  const knot::raw member{
      R"({"membership":"join","displayname":"B","join_authorised_via_users_server":"@s:x.org",)"
      R"("third_party_invite":{"display_name":"b","signed":{"mxid":"@b:x.org"}}})"};
  EXPECT_EQ(knot::to_json_string(loom::client::redaction_rules::of("1").redact("m.room.member", member)),
            R"({"membership":"join"})");
  EXPECT_EQ(knot::to_json_string(loom::client::redaction_rules::of("9").redact("m.room.member", member)),
            R"({"join_authorised_via_users_server":"@s:x.org","membership":"join"})");
  EXPECT_EQ(knot::to_json_string(loom::client::redaction_rules::of("11").redact("m.room.member", member)),
            R"({"join_authorised_via_users_server":"@s:x.org","membership":"join",)"
            R"("third_party_invite":{"signed":{"mxid":"@b:x.org"}}})");
  const knot::raw create{R"({"creator":"@a:x.org","room_version":"10","m.federate":false})"};
  EXPECT_EQ(knot::to_json_string(loom::client::redaction_rules::of("10").redact("m.room.create", create)),
            R"({"creator":"@a:x.org"})");
  EXPECT_EQ(knot::to_json_string(loom::client::redaction_rules::of("11").redact("m.room.create", create)),
            knot::to_json_string(create));
  EXPECT_EQ(loom::client::redaction_rules::of("org.example.custom").version, 11);
}

}  // namespace
