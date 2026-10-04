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
- `loom.events`: the small convenience event model, its content typed where loom knows the type
  (`m.room.message`, `m.room.member`, `m.room.name`, `m.room.topic`,
  `m.room.create`) and kept as `knot::raw` otherwise.
- `loom.api`: versions, login, whoami, sync, send, messages, join, leave;
  typed errors -- errcode, and `retry_after_ms` for rate limits;
  transaction ids.
- `loom.sync`: the /sync answer's types and a store it is applied to.

Tests are `CONSTEXPR_TEST`s, as alef's and tern's: run by the program, and --
with `-DLOOM_CONSTEXPR_TESTS=ON` -- by the compiler as well.

For the full generated API, import `loom.cs.<part>` and use `loom::cs`
endpoints. `loom.ev` supplies the generated event model and `loom.state`
supplies `loom::client::state`, including membership transitions and
redactions. The shorter names above remain available for existing callers.

## Streaming requests and responses

Every endpoint exposes `to_head()` and `write_body(sink)`. The header value
owns its method, target, authentication flag and content type, so it can be
sent before serialization starts. The sink is a concrete callable receiving
`std::string_view` chunks. Consume or copy each chunk before returning; it
may borrow endpoint storage or temporary serializer storage. The endpoint
must remain alive and unchanged during the call. Sink exceptions propagate.

```cpp
auto head = endpoint.to_head();
// Configure the transport with head.method_name(), head.target,
// head.authenticated and head.content_type; use chunked transfer if needed.
endpoint.write_body([&](std::string_view chunk) { transport.write(chunk); });
```

`to_send()` still returns a buffered `loom::request`. It uses the same
writer and preserves the content type, including media uploads. The
generator emits both interfaces; generated files and handwritten endpoints
follow the same convention. For an upload whose bytes come from a file or
another stream, use the upload endpoint's `to_head()` and send that source
directly through the transport; no endpoint body string is needed.

`read<Endpoint>(status, characters)` accepts a single-pass character range
as well as a string view. `read_chunks<Endpoint>(status, chunks)` accepts a
single-pass range of character chunks, including empty chunks and tokens
split at arbitrary boundaries. Both return the usual typed response or
homeserver error. Raw response types return owned bytes without JSON parsing.

For downloads, `read_chunks_to<Endpoint>(status, chunks, sink)` forwards
successful response chunks directly to a sink and returns
`std::expected<void, loom::error>`. On an HTTP error it decodes the error and
does not pass its body to the sink. Range and sink exceptions propagate;
bytes already delivered before a transport failure remain the caller's to
discard or resume. Transport buffering and response-size limits remain the
caller's responsibility.

## Retained state and encryption

`loom::client::state` suppresses duplicate event IDs in each retained
timeline. Redactions find timeline targets through an index built for the
incoming batch. Set `state.timeline_limit` to cap the number of retained
events per joined or left room; zero retains state but no timeline. The
default is unlimited for compatibility. The limit bounds retained events,
not the memory used to decode an individual response.

Local eviction sets `room.timeline_truncated` and clears `prev_batch`,
because that token refers to a boundary before the evicted events. A new
server gap re-establishes the boundary. Clients using a limit must handle
the lost history explicitly, with their own storage or a new history fetch.

The optional `crypto` component provides Olm/Megolm through vodozemac,
key backup, cross-signing and verification helpers. It requires OpenSSL
and the Rust toolchain for the vodozemac bindings. The protocol API does no
network I/O; the crypto machine uses filesystem paths and a caller-supplied
concrete `Keeper` for its persistent store. Keepers must make successful
writes durable and replace files atomically where crash recovery requires it.

Failed saves remain dirty and can be retried with `flush()`. An existing
empty or unreadable store is an error; it must not silently create a new
device identity. Applications must handle persistence failures before
continuing operations that require durable crypto state.

## Building

CMake 4.3.4 or newer, Ninja, clang with libc++ and `import std`. knot and
googletest come through cmake-everywhere, pinned; knot is private, so git has
to be able to authenticate, or `-DCPM_knot_SOURCE=<path>` given.

## Licence

GNU Affero General Public License, version 3 only (`AGPL-3.0-only`) -- the
text is in `LICENSE`. A program that uses this library is a work based on
it; whoever interacts with such a program over a network is offered its
source, as the licence's section 13 says.
