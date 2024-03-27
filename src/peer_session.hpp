#pragma once

#include "connection_table.hpp"
#include "events.hpp"
#include "fmt/base.h"
#include "message.hpp"

#include <asio/use_future.hpp>
#include <fmt/core.h>
#include <type_traits>
#include <variant>

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
        }
        auto set_name_packet =
            Packet::set_name(std::string(client_name_opt.value()));
        set_name_packet.serialize(m_connection.socket);
        fmt::print(stderr, "Sent SetName\n");
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
        // fmt::print(stderr, "start() IN\n");

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
                auto packet = co_await Packet::deserialize(m_connection.socket);
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