// loom.bot:fiber -- stackful coroutines on one io_context: a fiber runs
// until what it waits for is not there yet, and is resumed by the loop when
// it is. Each handler of a bot is one; so is the sync.
module;
#include <boost/asio.hpp>
#include <boost/context/fiber.hpp>
#include <boost/context/fixedsize_stack.hpp>
export module loom.bot:fiber;

import std;

export namespace loom::bot::detail {

namespace asio = boost::asio;
namespace ctx = boost::context;

class fiber_on {
 public:
  explicit fiber_on(asio::io_context& io) : io_(io) {}
  fiber_on(const fiber_on&) = delete;
  fiber_on& operator=(const fiber_on&) = delete;

  // Started at the loop's next turn; kept alive by the loop's list of them.
  template <class Body> static fiber_on& start(asio::io_context& io, std::list<std::unique_ptr<fiber_on>>& all, Body body) {
    auto& made = *all.emplace_back(std::make_unique<fiber_on>(io));
    // A megabyte of stack: TLS and HTTP run on it.
    made.self_ = ctx::fiber(std::allocator_arg, ctx::fixedsize_stack(1 << 20), [&made, body = std::move(body)](ctx::fiber&& caller) mutable {
      made.caller_ = std::move(caller);
      body(made);
      made.done_ = true;
      return std::move(made.caller_);
    });
    asio::post(io, [&made] { made.resume(); });
    return made;
  }
  // From inside: back to the loop until resumed.
  void suspend() { caller_ = std::move(caller_).resume(); }
  // From the loop: on where it left off.
  void resume() {
    if (self_ && !done_)
      self_ = std::move(self_).resume();
  }
  // Resumed at the loop's next turn: what a completion does.
  void wake() {
    asio::post(io_, [this] { resume(); });
  }
  [[nodiscard]] asio::io_context& io() const { return io_; }
  [[nodiscard]] bool done() const { return done_; }

 private:
  asio::io_context& io_;
  ctx::fiber self_, caller_;
  bool done_ = false;
};

// An asynchronous operation awaited by the fiber that starts it: `start` is
// given the completion handler, the fiber suspends, and what the handler was
// called with is returned once it has been.
template <class... Args, class Start> std::tuple<Args...> await(fiber_on& fiber, Start&& start) {
  std::optional<std::tuple<Args...>> got;
  start([&got, &fiber](Args... args) {
    got.emplace(std::move(args)...);
    fiber.wake();
  });
  while (!got)
    fiber.suspend();
  return std::move(*got);
}

}  // namespace loom::bot::detail
