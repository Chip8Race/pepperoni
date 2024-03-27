#pragma once

#include <asio/awaitable.hpp>
#include <asio/co_spawn.hpp>
#include <asio/connect.hpp>
#include <asio/detached.hpp>
#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/read_until.hpp>
#include <asio/signal_set.hpp>
#include <asio/steady_timer.hpp>
#include <asio/write.hpp>

#include <algorithm>
#include <chrono>
#include <fmt/core.h>

using asio::ip::tcp;
using namespace asio::ip;
using asio::awaitable;
using asio::co_spawn;
using asio::detached;
using asio::use_awaitable;
using std::chrono::steady_clock;
namespace this_coro = asio::this_coro;
using namespace std::literals::chrono_literals;

// #if defined(ASIO_ENABLE_HANDLER_TRACKING)
// #    def ine use_awaitable \
//        asio::use_awaitable_t(__FILE__, __LINE__, __PRETTY_FUNCTION__)
// #endif
#include "message.hpp"

#include <list>
#include <optional>
#include <string>

using namespace asio;
using namespace peppe;

awaitable<void> timeout(steady_clock::duration duration) {
    asio::steady_timer timer(co_await this_coro::executor);
    timer.expires_after(duration);
    co_await timer.async_wait(use_nothrow_awaitable);
    // fmt::print(stderr, "Timed out\n");
}

using asio::ip::tcp;

struct PeerConnection {
    std::optional<std::string> name;
    tcp::socket socket;
};

class ConnectionTable {
public:
    // Ctor
    ConnectionTable() = default;
    // Dtor
    ~ConnectionTable() = default;

    void remove(PeerConnection* conn) {
        m_connection_table.remove_if([&conn](auto c) {
            return conn->socket.local_endpoint() == c->socket.local_endpoint();
        });
    }

    void add(PeerConnection* conn) { m_connection_table.push_back(conn); }

    // awaitable<void> send_all(Packet const& packet) {
    //     for (auto* conn : m_connection_table) {
    //         co_await packet.serialize(conn->socket);
    //     }
    // }

    void send_all(const Packet& packet) {
        for (auto* conn : m_connection_table) {
            packet.serialize(conn->socket);
        }
    }

private:
    std::list<PeerConnection*> m_connection_table;
};