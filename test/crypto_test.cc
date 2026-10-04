// SPDX-License-Identifier: AGPL-3.0-only
// Exercise the real machine with a keeper that can fail writes.
import std;
import loom.crypto;
import gtest;
#include "gtest/gtest-macros.h"

namespace {
struct io_state {
  bool fail = false;
  bool throw_on_write = false;
  int writes = 0;
};
struct keeper {
  io_state* state;
  std::optional<std::string> read_file(const std::filesystem::path& path) const {
    std::ifstream in(path, std::ios::binary);
    if (!in) return std::nullopt;
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
  }
  bool write_file(const std::filesystem::path& path, std::string_view text, bool) const {
    ++state->writes;
    if (state->throw_on_write) throw std::runtime_error("injected write failure");
    if (state->fail) return false;
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    out.close();
    std::filesystem::permissions(path, std::filesystem::perms::owner_read | std::filesystem::perms::owner_write);
    return !out.fail();
  }
};
class CryptoStore : public testing::Test {
 protected:
  io_state io;
  std::filesystem::path directory;
  std::filesystem::path file;
  void SetUp() override {
    static std::atomic<unsigned> sequence = 0;
    directory = std::filesystem::temp_directory_path() /
        ("loom-crypto-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + std::to_string(++sequence));
    std::filesystem::create_directories(directory);
    file = directory / "store";
  }
  void TearDown() override {
    std::error_code ignored;
    std::filesystem::remove_all(directory, ignored);
  }
  auto open() { return loom::crypto::olm_machine<keeper>::open(keeper{&io}, file, "@a:x", "device"); }
};

TEST_F(CryptoStore, FailedSaveRemainsRetryable) {
  auto machine = open();
  const auto identity = machine.ed25519();
  io.fail = true;
  EXPECT_THROW(machine.remember_encrypted("!room:x"), std::runtime_error);
  io.fail = false;
  machine.flush();
  auto restored = open();
  EXPECT_EQ(restored.ed25519(), identity);
  EXPECT_TRUE(restored.was_encrypted("!room:x"));
  const int writes = io.writes;
  machine.flush();
  EXPECT_EQ(io.writes, writes);
}

TEST_F(CryptoStore, ThrowingKeeperAndRepeatedFailureRemainRetryable) {
  auto machine = open();
  io.throw_on_write = true;
  EXPECT_THROW(machine.went_on_to("sync-token"), std::runtime_error);
  EXPECT_THROW(machine.flush(), std::runtime_error);
  io.throw_on_write = false;
  machine.flush();
  EXPECT_EQ(open().to_device_since(), "sync-token");
}

TEST_F(CryptoStore, EmptyExistingStoreDoesNotReplaceIdentity) {
  auto machine = open();
  { std::ofstream truncate(file, std::ios::binary | std::ios::trunc); }
  const int writes = io.writes;
  EXPECT_THROW((void)open(), std::runtime_error);
  EXPECT_EQ(io.writes, writes);
  EXPECT_EQ(std::filesystem::file_size(file), 0u);
}

TEST_F(CryptoStore, MissingKeyDoesNotReplaceIdentity) {
  auto machine = open();
  std::filesystem::remove(std::filesystem::path(file).concat(".key"));
  const int writes = io.writes;
  EXPECT_THROW((void)open(), std::runtime_error);
  EXPECT_EQ(io.writes, writes);
}
}  // namespace
