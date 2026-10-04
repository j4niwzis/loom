// SPDX-License-Identifier: AGPL-3.0-only
import std;
import loom;
import loom.cs.content_repo;
import loom.cs.room_send;
import gtest;
#include "gtest/gtest-macros.h"

namespace {
struct chunks {
  std::vector<std::string> pieces;
  std::size_t at = 0;
  int begins = 0;
  struct iterator {
    using value_type = std::string;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;
    chunks* from;
    const std::string& operator*() const { return from->pieces[from->at]; }
    iterator& operator++() { ++from->at; return *this; }
    void operator++(int) { ++*this; }
    bool operator==(std::default_sentinel_t) const { return from->at == from->pieces.size(); }
  };
  iterator begin() {
    if (++begins != 1) throw std::runtime_error("range read twice");
    return {this};
  }
  std::default_sentinel_t end() const { return {}; }
};
static_assert(std::ranges::input_range<chunks>);
static_assert(!std::ranges::forward_range<chunks>);

TEST(Stream, JsonAcrossSinglePassChunks) {
  chunks input{{"", "{\"user_", "id\":\"@a:", "example.org\",\"device_id\":\"d\"}", ""}};
  auto got = loom::read_chunks<loom::whoami>(200, input);
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->user_id, "@a:example.org");
  EXPECT_EQ(got->device_id, "d");
  EXPECT_EQ(input.begins, 1);
}

TEST(Stream, ErrorAcrossSinglePassChunks) {
  chunks input{{"{\"errcode\":\"M_LIMIT_", "EXCEEDED\",\"error\":\"slow down\",",
                "\"retry_after_ms\":1200,\"session\":\"s\"}"}};
  auto got = loom::read_chunks<loom::whoami>(429, input);
  ASSERT_FALSE(got.has_value());
  EXPECT_EQ(got.error().status, 429);
  EXPECT_EQ(got.error().errcode, "M_LIMIT_EXCEEDED");
  EXPECT_EQ(got.error().retry_after_ms, 1200);
  EXPECT_EQ(got.error().session, "s");
  EXPECT_EQ(input.begins, 1);
}

TEST(Stream, CharacterRangeAndMalformedJson) {
  std::istringstream input(R"({"user_id":"@a:example.org"})");
  input >> std::noskipws;
  auto got = loom::read<loom::whoami>(200, std::ranges::istream_view<char>(input));
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(got->user_id, "@a:example.org");
  chunks broken{{"{\"user_id\":\"unfinished"}};
  auto failed = loom::read_chunks<loom::whoami>(200, broken);
  ASSERT_FALSE(failed.has_value());
  EXPECT_EQ(failed.error().errcode, "M_BAD_JSON");
  // The string-view overload excludes a literal's terminating NUL.
  EXPECT_TRUE(loom::read<loom::whoami>(200, R"({"user_id":"@a:x"})").has_value());
}

TEST(Stream, UploadMetadataAndBorrowedBody) {
  loom::cs::upload_content upload{.filename = "photo one.png", .body = std::string(32768, 'x'), .content_type = "image/png"};
  const auto head = upload.to_head();
  EXPECT_EQ(head.method_name(), "POST");
  EXPECT_EQ(head.content_type, "image/png");
  EXPECT_TRUE(head.authenticated);
  EXPECT_TRUE(head.target.ends_with("?filename=photo%20one.png"));
  std::size_t bytes = 0;
  upload.write_body([&](std::string_view piece) {
    EXPECT_EQ(piece.data(), upload.body.data());
    bytes += piece.size();
  });
  EXPECT_EQ(bytes, upload.body.size());
  const auto owned = upload.to_send();
  EXPECT_EQ(owned.content_type, head.content_type);
  EXPECT_EQ(owned.body, upload.body);
  EXPECT_EQ(owned.target, head.target);
}

TEST(Stream, JsonWriterBorrowsLongTextAndMatchesBufferedRequest) {
  loom::send_message endpoint{.room = "!r:x", .txn_id = "one", .message = {.msgtype = "m.text", .body = std::string(32768, 'z')}};
  bool borrowed = false;
  std::string written;
  endpoint.write_body([&](std::string_view piece) {
    borrowed |= piece.data() == endpoint.message.body.data();
    written.append(piece);
  });
  EXPECT_TRUE(borrowed);
  EXPECT_EQ(written, endpoint.to_send().body);
  auto content = knot::try_read<loom::content::message>(written);
  ASSERT_TRUE(content.has_value());
  EXPECT_EQ(content->body, endpoint.message.body);
  loom::cs::send_message generated{.room_id = "!r:x", .event_type = "m.room.message", .txn_id = "one", .body = knot::raw{written}};
  std::string generated_bytes;
  generated.write_body([&](std::string_view piece) { generated_bytes.append(piece); });
  EXPECT_EQ(generated_bytes, written);
  EXPECT_EQ(generated.to_head().target, endpoint.to_head().target);
}

TEST(Stream, BinaryResponseAndDownloadSink) {
  using endpoint = loom::cs::get_content;
  const std::string bytes("a\0b", 3);
  auto owned = loom::read<endpoint>(200, bytes);
  ASSERT_TRUE(owned.has_value());
  EXPECT_EQ(owned->bytes, bytes);
  chunks input{{bytes, "", "tail"}};
  std::string written;
  auto got = loom::read_chunks_to<endpoint>(200, input, [&](std::string_view piece) {
    EXPECT_EQ(piece.data(), input.pieces[input.at].data());
    written.append(piece);
  });
  ASSERT_TRUE(got.has_value());
  EXPECT_EQ(written, bytes + "tail");
  EXPECT_EQ(input.begins, 1);
}

TEST(Stream, DownloadErrorDoesNotReachSink) {
  chunks input{{"{\"errcode\":\"M_NOT_FOUND\",", "\"error\":\"gone\"}"}};
  int writes = 0;
  auto got = loom::read_chunks_to<loom::cs::get_content>(404, input, [&](std::string_view) { ++writes; });
  ASSERT_FALSE(got.has_value());
  EXPECT_EQ(got.error().errcode, "M_NOT_FOUND");
  EXPECT_EQ(writes, 0);
}

TEST(Stream, SinkFailuresPropagate) {
  loom::send_message endpoint{.message = {.msgtype = "m.text", .body = "hello"}};
  auto fail = [](std::string_view) { throw std::runtime_error("transport failed"); };
  EXPECT_THROW(endpoint.write_body(fail), std::runtime_error);
  chunks input{{"first", "second"}};
  EXPECT_THROW((void)loom::read_chunks_to<loom::cs::get_content>(200, input, fail), std::runtime_error);
  EXPECT_EQ(input.at, 0u);
}
}  // namespace
