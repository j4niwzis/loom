// loom, run when the program runs and -- with LOOM_CONSTEXPR_TESTS -- while
// it is compiled: identifiers, what is sent, what comes back, and a /sync
// applied to the store.
import std;
import loom;
import gtest;

#include "gtest/gtest-macros.h"
#include "constexpr_test.h"

CONSTEXPR_TEST(Ids, Grammar) {
  CONSTEXPR_EXPECT_TRUE(loom::user_id::parse("@alice:example.org").has_value());
  CONSTEXPR_EXPECT_TRUE(loom::user_id::parse("@alice:[::1]:8448").has_value());
  CONSTEXPR_EXPECT_FALSE(loom::user_id::parse("@alice").has_value());
  CONSTEXPR_EXPECT_FALSE(loom::user_id::parse("!room:example.org").has_value());
  CONSTEXPR_EXPECT_FALSE(loom::room_id::parse("!room:exa mple.org").has_value());
  CONSTEXPR_EXPECT_TRUE(loom::event_id::parse("$acR1l0raoZnm60CBwAVgqbZqoO/mYU81xysh1u7XcJk").has_value());
  const auto room = loom::room_id::parse("!abc:example.org:8448");
  CONSTEXPR_EXPECT_TRUE(room.has_value());
  if (room) {
    CONSTEXPR_EXPECT_EQ(room->localpart(), "abc");
    CONSTEXPR_EXPECT_EQ(room->server(), "example.org:8448");
  }
  CONSTEXPR_EXPECT_EQ(loom::percent_encoded("!abc:example.org"), "%21abc%3Aexample.org");
}

CONSTEXPR_TEST(Api, Requests) {
  const loom::request login = loom::login{.user = "alice", .password = "hunter2"}.to_send();
  CONSTEXPR_EXPECT_EQ(login.method_name(), "POST");
  CONSTEXPR_EXPECT_EQ(login.target, "/_matrix/client/v3/login");
  CONSTEXPR_EXPECT_EQ(login.body,
                      R"({"identifier":{"type":"m.id.user","user":"alice"},"password":"hunter2","type":"m.login.password"})");
  CONSTEXPR_EXPECT_FALSE(login.authenticated);

  const loom::request sync = loom::sync{.since = "s72594_4483_1934", .timeout_ms = 30000}.to_send();
  CONSTEXPR_EXPECT_EQ(sync.method_name(), "GET");
  CONSTEXPR_EXPECT_EQ(sync.target, "/_matrix/client/v3/sync?since=s72594_4483_1934&timeout=30000");

  const loom::request send =
      loom::send_message{.room = "!abc:example.org", .txn_id = "loom.1", .message = {.msgtype = "m.text", .body = "hi"}}
          .to_send();
  CONSTEXPR_EXPECT_EQ(send.method_name(), "PUT");
  CONSTEXPR_EXPECT_EQ(send.target, "/_matrix/client/v3/rooms/%21abc%3Aexample.org/send/m.room.message/loom.1");
  CONSTEXPR_EXPECT_EQ(send.body, R"({"body":"hi","msgtype":"m.text"})");
  CONSTEXPR_EXPECT_TRUE(send.authenticated);

  loom::transactions ids;
  CONSTEXPR_EXPECT_EQ(ids.next(), "loom.1");
  CONSTEXPR_EXPECT_EQ(ids.next(), "loom.2");
}

CONSTEXPR_TEST(Api, Responses) {
  const auto logged_in = loom::read<loom::login>(
      200, R"({"user_id":"@alice:example.org","access_token":"syt_abc","device_id":"GHTYAJCE","home_server":"example.org"})");
  CONSTEXPR_EXPECT_TRUE(logged_in.has_value());
  if (logged_in) {
    CONSTEXPR_EXPECT_EQ(logged_in->user_id, "@alice:example.org");
    CONSTEXPR_EXPECT_EQ(logged_in->access_token, "syt_abc");
  }
  const auto limited = loom::read<loom::whoami>(
      429, R"({"errcode":"M_LIMIT_EXCEEDED","error":"Too many requests","retry_after_ms":2000})");
  CONSTEXPR_EXPECT_FALSE(limited.has_value());
  if (!limited) {
    CONSTEXPR_EXPECT_EQ(limited.error().status, 429);
    CONSTEXPR_EXPECT_EQ(limited.error().errcode, "M_LIMIT_EXCEEDED");
    CONSTEXPR_EXPECT_TRUE(limited.error().retry_after_ms == 2000);
  }
  const auto left = loom::read<loom::leave>(200, "{}");
  CONSTEXPR_EXPECT_TRUE(left.has_value());
}

CONSTEXPR_TEST(Sync, Applied) {
  const auto got = loom::read<loom::sync>(200, R"({
    "next_batch": "s72595_4483_1934",
    "rooms": {
      "join": {
        "!abc:example.org": {
          "state": {"events": [
            {"type": "m.room.name", "state_key": "", "content": {"name": "Party"},
             "event_id": "$1", "sender": "@alice:example.org", "origin_server_ts": 1}
          ]},
          "timeline": {"limited": false, "prev_batch": "t34-23535_0_0", "events": [
            {"type": "m.room.member", "state_key": "@bob:example.org", "content": {"membership": "join"},
             "event_id": "$2", "sender": "@bob:example.org", "origin_server_ts": 2},
            {"type": "m.room.message", "content": {"msgtype": "m.text", "body": "hello"},
             "event_id": "$3", "sender": "@bob:example.org", "origin_server_ts": 3},
            {"type": "org.example.custom", "content": {"x": 1},
             "event_id": "$4", "sender": "@bob:example.org", "origin_server_ts": 4}
          ]}
        }
      },
      "invite": {"!def:example.org": {"invite_state": {"events": [
        {"type": "m.room.name", "state_key": "", "content": {"name": "Secret"}, "sender": "@carol:example.org"}
      ]}}}
    }
  })");
  CONSTEXPR_EXPECT_TRUE(got.has_value());
  if (!got)
    return;
  loom::store kept;
  kept.apply(*got);
  CONSTEXPR_EXPECT_TRUE(kept.since == "s72595_4483_1934");
  CONSTEXPR_EXPECT_EQ(kept.joined.size(), 1u);
  const auto found = kept.joined.find("!abc:example.org");
  CONSTEXPR_EXPECT_TRUE(found != kept.joined.end());
  if (found == kept.joined.end())
    return;
  const loom::room& room = found->second;
  CONSTEXPR_EXPECT_TRUE(room.name == "Party");
  CONSTEXPR_EXPECT_EQ(room.members.size(), 1u);
  CONSTEXPR_EXPECT_TRUE(room.prev_batch == "t34-23535_0_0");
  CONSTEXPR_EXPECT_EQ(room.timeline.size(), 3u);
  if (room.timeline.size() == 3) {
    CONSTEXPR_EXPECT_TRUE(room.timeline[1].content.is<loom::content::message>());
    if (room.timeline[1].content.is<loom::content::message>())
      CONSTEXPR_EXPECT_EQ(room.timeline[1].content.as<loom::content::message>().body, "hello");
    CONSTEXPR_EXPECT_TRUE(room.timeline[2].content.is<knot::raw>());  // no type here for it: kept as its text
  }
  CONSTEXPR_EXPECT_TRUE(kept.invited.contains("!def:example.org"));
}
