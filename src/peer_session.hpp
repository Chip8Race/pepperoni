#pragma once

#include "asio/ip/address.hpp"
#include "connection_table.hpp"
#include "events.hpp"
#include "fmt/base.h"
#include "message.hpp"

#include <asio/use_future.hpp>
#include <fmt/core.h>
#include <ranges>

namespace peppe {

class PeerSession : public std::enable_shared_from_this<PeerSession> {
public:
    // Ctor
    PeerSession(
        ConnectionTable& conn_table,
        tcp::socket socket,
        const std::optional<std::string>& client_name_opt
    )
        : m_connection{ std::nullopt, std::move(socket) }
        , m_connection_table_ref(conn_table) {
        m_connection_table_ref.add(&m_connection);
        const auto ep = m_connection.socket.remote_endpoint();
        fmt::print(
            stderr, "Connected ({}:{})\n", ep.address().to_string(), ep.port()
        );
        EventManager::send(BackendEvent{ PeerConnected{} });

        // When the session starts, the first packet sent is set name
        if (client_name_opt.has_value()) {
            Packet::set_name(std::string(client_name_opt.value()))
                .write(m_connection.socket);
            fmt::print(stderr, "Sent SetName\n");
        }

        // Also send known peers
        auto known_peers = m_connection_table_ref.connected_peers();
        auto ivp4_addresses_bytes =
            known_peers |
            std::views::filter([](const auto& addr) { return addr.is_v4(); }) |
            std::views::transform([](const auto& addr) {
                return addr.to_v4().to_bytes();
            });
        auto ivp6_addresses_bytes =
            known_peers |
            std::views::filter([](const auto& addr) { return addr.is_v6(); }) |
            std::views::transform([](const auto& addr) {
                return addr.to_v6().to_bytes();
            });

        Packet::peer_discovery(ivp4_addresses_bytes, ivp6_addresses_bytes)
            .write(m_connection.socket);
        fmt::print(stderr, "Sent PeerDiscovery\n");
    }

    // Dtor
    ~PeerSession() {
        m_connection_table_ref.remove(&m_connection);
        const auto ep = m_connection.socket.remote_endpoint();
        fmt::print(
            stderr,
            "Disconnected ({}:{})\n",
            ep.address().to_string(),
            ep.port()
        );
        EventManager::send(BackendEvent{ PeerDisconnected{} });
    }

    awaitable<void> start() {
        co_spawn(
            m_connection.socket.get_executor(),
            [self = shared_from_this()] { return self->reader(); },
            detached
        );

        co_return;
    }

    awaitable<void> reader() {
        try {
            while (true) {
                auto packet = co_await Packet::read(m_connection.socket);
                const auto ep = m_connection.socket.remote_endpoint();
                auto from = m_connection.name.value_or(
                    fmt::format("{}:{}", ep.address().to_string(), ep.port())
                );

                packet.match(
                    [&from](TextMessage& text_msg) {
                        fmt::print(stderr, "'{}' > {}\n", from, text_msg.text);
                        EventManager::send(BackendEvent{
                            ReceiveMessage{ from, text_msg.text } });
                    },
                    [this](SetName& set_name) {
                        m_connection.name = set_name.name;
                    },
                    [](PeerDiscovery& peer_discovery) {
                        fmt::print(
                            stderr,
                            "IPv4 Addresses ({}):\n",
                            peer_discovery.ipv4_addresses.size()
                        );
                        for (const auto& address :
                             peer_discovery.ipv4_addresses) {
                            fmt::print(
                                stderr,
                                "{}.{}.{}.{}\n",
                                address[0],
                                address[1],
                                address[2],
                                address[3]
                            );
                        }
                    },
                    // Default case
                    [](auto&&) {}
                );
            }
        }
        catch (ConnectionClosed&) {
            // fmt::print(stderr, "ConnectionClosed\n");
        }
    }

private:
    PeerConnection m_connection;
    ConnectionTable& m_connection_table_ref;
};

} // namespace peppe