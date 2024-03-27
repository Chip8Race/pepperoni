#include "config.hpp"
#include "events.hpp"
#include "frontend.hpp"
#include "peer_listener.hpp"

#include <optional>

void print_config(const peppe::Config& config) {
    fmt::print("name: '{}'\n", config.name);
    fmt::print("port: '{}'\n", config.port);
    fmt::print("peers:\n");
    for (const auto& peer : config.peer_table) {
        fmt::print("- ip/port: '{}:{}'\n", peer.address.to_string(), peer.port);
    }
}

int main(int argc, const char* argv[]) {
    using namespace peppe;

    // Load config
    auto config = Config::load_toml((argc < 2) ? "config.toml" : argv[1])
                      .value_or(Config{});
    print_config(config);

    // Launch input handler
    const int num_threads_hint = int(std::thread::hardware_concurrency());
    asio::io_context io_context(num_threads_hint);
    auto frontend = Frontend();
    std::jthread frontend_thread([&frontend] { frontend.start(); });

    // Launch peer listener
    PeerListener peer_listener(io_context, std::move(config.peer_table));
    peer_listener.set_port(config.port);
    co_spawn(io_context, peer_listener.listener(), detached);

    // Setup signal handlers and run async event loop
    asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });
    io_context.run();
}

//////////////////////////////////////////////////////////////////////

// #include <asio/ip/address.hpp>
// #include <fmt/core.h>
// #include <variant>
// // #include <ftxui/component/screen_interactive.hpp>

// #include "config.hpp"
// #include "frontend.hpp"

// #include "message.hpp"

// int main() {
//     using namespace peppe;
//     Packet msg = TextMessage{};
//     msg.match([](TextMessage& text_msg) { fmt::print("Is TextMessage\n"); }
//               // [](auto&&) {}
//     );
//     // std::visit()
// }