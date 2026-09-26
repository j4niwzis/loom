# loom

A Matrix client library for C++26, as modules -- sans I/O. Each endpoint of
the client-server API is a value made into what to send (method, target, a
JSON body) and what comes back is read straight into its response type, or
into the homeserver's error; sending it is the caller's (Boost.Beast, or any
HTTP client). JSON is knot's: typed, no tree on the way, events' content
chosen by their "type" with `knot::tagged`.

```cpp
import loom;

loom::request r = loom::login{.user = "alice", .password = "…"}.to_send();
// r.method_name() == "POST", r.target == "/_matrix/client/v3/login", r.body: the JSON
// … send it; with the status and body that come back:
auto session = loom::read<loom::login>(status, body);   // std::expected<login::response, loom::error>

loom::store kept;                                         // what a client keeps between syncs
auto sync = loom::read<loom::sync>(status, body);
if (sync) kept.apply(*sync);                              // rooms, names, members, timelines
```

- `loom.ids`: user, room, event ids and aliases, by the identifier grammar;
  percent-encoding for paths.
- `loom.events`: room events, their content typed where loom knows the type
  (`m.room.message`, `m.room.member`, `m.room.name`, `m.room.topic`,
  `m.room.create`) and kept as a `knot::value` otherwise.
- `loom.api`: versions, login, whoami, sync, send, messages, join, leave;
  typed errors -- errcode, and `retry_after_ms` for rate limits;
  transaction ids.
- `loom.sync`: the /sync answer's types and a store it is applied to.

Tests are `CONSTEXPR_TEST`s, as alef's and tern's: run by the program, and --
with `-DLOOM_CONSTEXPR_TESTS=ON` -- by the compiler as well.

End-to-end encryption is not here yet.

## Building

CMake 4.3.4 or newer, Ninja, clang with libc++ and `import std`. knot and
googletest come through cmake-everywhere, pinned; knot is private, so git has
to be able to authenticate, or `-DCPM_knot_SOURCE=<path>` given.
