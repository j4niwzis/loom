// SPDX-License-Identifier: AGPL-3.0-only
// Numeric VoIP v0 versions and absent party IDs remain valid Matrix events.
import std;
import splice;
import knot;
import loom.ev;
import gtest;

#include "gtest/gtest-macros.h"

TEST(CallCompatibility, LegacyInviteAndHangupAreTyped) {
  auto invite = knot::try_read<loom::ev::timeline_event>(R"({"type":"m.call.invite",
    "content":{"call_id":"legacy","version":0,"lifetime":60000,
      "offer":{"type":"offer","sdp":"v=0\r\n"}},
    "event_id":"$invite","sender":"@a:example.org","origin_server_ts":1})");
  ASSERT_TRUE(invite);
  ASSERT_TRUE(invite->content.is<loom::ev::m_call_invite_content_t>());
  const auto& content = invite->content.as<loom::ev::m_call_invite_content_t>();
  EXPECT_FALSE(content.party_id);
  EXPECT_EQ(spl::visit(spl::overloaded{[](std::int64_t version) { return version; },
      [](const std::string&) { return std::int64_t{-1}; }}, content.version), 0);
  auto hangup = knot::try_read<loom::ev::timeline_event>(R"({"type":"m.call.hangup",
    "content":{"call_id":"legacy","version":0},
    "event_id":"$hangup","sender":"@a:example.org","origin_server_ts":2})");
  ASSERT_TRUE(hangup);
  ASSERT_TRUE(hangup->content.is<loom::ev::m_call_hangup_content_t>());
  EXPECT_FALSE(hangup->content.as<loom::ev::m_call_hangup_content_t>().reason);
}
