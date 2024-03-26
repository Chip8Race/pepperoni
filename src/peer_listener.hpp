#pragma once

#include "events.hpp"
#include "peer_session.hpp"
#include "peer_table.hpp"

#include <asio/read_until.hpp>
#include <barrier>
#include <future>
#include <iostream>
#include <memory>

namespace peppe {

class PeerListener : public EventListener<PeerListener, FrontendEvent> {
public:
    // Ctor
    PeerListener(asio::io_context& io_context, PeerTable&& table)
        : m_io_context(io_context)
        , m_initial_peers(std::move(table)) {}

    // Dtor
    ~PeerListener() = default;

    void set_port(asio::ip::port_type port) { m_port = port; }

    void on_event(const FrontendEvent& event) {
        event.match(
            [this](const SendMessage& sm) {
                m_connection_table.send_all(Packet::message(sm.message));
            },
            [](const Terminate& t) {}
        );
    }

    awaitable<void> connect_to_peer(
        tcp::socket&& socket,
        tcp::endpoint&& endpoint,
        std::string&& name
    ) {
        // fmt::print(stderr, "connect_to_peer() IN\n");
        auto [error] =
            co_await socket.async_connect(endpoint, use_nothrow_awaitable);
        if (error) {
            // fmt::print(stderr, "connect_to_peer() OUT ({})\n",
            // error.message());
            co_return;
        }

        auto self_shared = std::make_shared<PeerSession>(
            m_connection_table, std::move(socket), std::move(name)
        );
        co_await self_shared->start();
        // fmt::print(stderr, "connect_to_peer() OUT\n");
        co_return;
    }

    awaitable<void> connect_to_peers() {
        // fmt::print(stderr, "connect_to_peers() IN\n");

        if (m_initial_peers.peers.size() > 0) {
            for (auto& peer : m_initial_peers.peers) {
                // Create socket and connect
                auto endpoint = tcp::endpoint(peer.address, peer.port);
                auto socket = tcp::socket(m_io_context, endpoint.protocol());

                // TODO: Add a timeout to the connection attempt
                // since I cant try to connect to all at once
                // co_spawn(
                //     m_io_context,
                //     connect_to_peer(std::move(tmp.value()),
                //     std::move(endpoint)), detached);
                co_await connect_to_peer(
                    std::move(socket),
                    std::move(endpoint),
                    std::string(peer.name)
                );
            }
        }

        // fmt::print(stderr, "connect_to_peers() OUT\n");
        co_return;
    }

    awaitable<void> listener() {
        // Try to connect to known peers
        co_spawn(m_io_context, connect_to_peers(), detached);

        tcp::acceptor acceptor(m_io_context, { tcp::v4(), m_port });
        fmt::print(stderr, "Listening on port '{}'\n", m_port);

        while (true) {
            auto socket = co_await acceptor.async_accept(use_awaitable);
            auto self_shared = std::make_shared<PeerSession>(
                m_connection_table, std::move(socket)
            );
            co_spawn(socket.get_executor(), self_shared->start(), detached);
        }
    }

private:
    asio::io_context& m_io_context;
    asio::ip::port_type m_port = 2501;
    ConnectionTable m_connection_table;
    PeerTable m_initial_peers;
};

} // namespace peppe