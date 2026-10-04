// SPDX-License-Identifier: AGPL-3.0-only
import std;
import knot;
import loom.ev;
import gtest;
#include "gtest/gtest-macros.h"

TEST(events, unsigned_thread_construction) {
  loom::ev::unsigned_data data;
  auto& thread = data.m_relations.emplace().m_thread.emplace();
  auto& latest = thread.latest_event.emplace();
  EXPECT_EQ(thread.count, 0);
  EXPECT_FALSE(thread.current_user_participated);
  EXPECT_EQ(latest.origin_server_ts, 0);
}

TEST(events, unsigned_thread_json) {
  auto value = knot::try_read<loom::ev::unsigned_data>(R"({"m.relations":{"m.thread":{"count":2,"current_user_participated":true,"latest_event":{"event_id":"$reply","sender":"@a:example.org","origin_server_ts":42,"content":{"body":"hello","future":7}},"future_summary":true}},"future_unsigned":9})");
  ASSERT_TRUE(value.has_value());
  ASSERT_TRUE(value->m_relations.has_value());
  ASSERT_TRUE(value->m_relations->m_thread.has_value());
  const auto& thread = *value->m_relations->m_thread;
  EXPECT_EQ(thread.count, 2);
  EXPECT_TRUE(thread.current_user_participated);
  ASSERT_TRUE(thread.latest_event.has_value());
  EXPECT_EQ(thread.latest_event->event_id, "$reply");
  EXPECT_EQ(thread.latest_event->content.body, "hello");
  auto roundtrip = knot::try_read<loom::ev::unsigned_data>(knot::to_json_string(*value));
  ASSERT_TRUE(roundtrip.has_value());
  EXPECT_TRUE(knot::to_json_string(*roundtrip).contains("future_summary"));
  EXPECT_TRUE(knot::to_json_string(*roundtrip).contains("future_unsigned"));
  auto empty = knot::try_read<loom::ev::unsigned_data>(R"({"m.relations":{"m.thread":{"count":0,"current_user_participated":false}}})");
  ASSERT_TRUE(empty.has_value());
  EXPECT_EQ(empty->m_relations->m_thread->count, 0);
  EXPECT_FALSE(empty->m_relations->m_thread->current_user_participated);
}
