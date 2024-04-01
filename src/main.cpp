#include "config.hpp"
#include "events.hpp"
#include "frontend.hpp"
#include "peer_listener.hpp"

#include <optional>

void print_config(const peppe::Config& config) {
    fmt::print("name: '{}'\n", config.name);
    fmt::print("port: '{}'\n", config.port);
    fmt::print("initial_peers:\n");
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

    // Launch Frontend in a separate thread
    const int num_threads_hint = int(std::thread::hardware_concurrency());
    asio::io_context io_context(num_threads_hint);
    auto frontend = Frontend(std::string(config.name));
    std::jthread frontend_thread([&frontend] { frontend.start(); });

    // Launch peer listener with an async runtime
    PeerListener peer_listener(io_context, std::move(config.peer_table));
    peer_listener.set_port(config.port);
    peer_listener.set_client_name(config.name);
    co_spawn(io_context, peer_listener.listener(), detached);

    // Setup signal handlers and run async event loop
    asio::signal_set signals(io_context, SIGINT, SIGTERM);
    signals.async_wait([&](auto, auto) { io_context.stop(); });
    io_context.run();
}

//////////////////////////////////////////////////////////////////////

// #include "peer_session.hpp"
// #include "utils.hpp"
// #include <fmt/core.h>

// int main() {
//     uint32_t num = 0x11223344;
//     fmt::print("0x{:0x}\n", num);
//     fmt::print("0x{:0x}\n", peppe::reverse_bytes(num));
// }