#pragma once

#include "asio/ip/address_v4.hpp"
#include "fmt/base.h"
#include <array>
#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

using asio::ip::tcp;

#include "error.hpp"
#include "utils.hpp"

namespace peppe {

constexpr auto use_nothrow_awaitable =
    asio::experimental::as_tuple(asio::use_awaitable);

enum class MessageType : std::uint8_t {
    TextMessageType = 0,
    SetNameType = 1,
    PeerDiscoveryType = 2,
};

struct TextMessage {
    static constexpr auto msg_type = MessageType::TextMessageType;
    // TODO: Size not needed
    std::uint32_t size;
    std::string text;

    template<typename AsyncReadStream>
    static asio::awaitable<TextMessage> read(AsyncReadStream& stream) {
        TextMessage result;

        // 1. Read 'size' from stream
        auto [err1, len1] = co_await asio::async_read(
            stream,
            asio::buffer(&result.size, sizeof(std::uint32_t)),
            use_nothrow_awaitable
        );
        // Convert from network endianess (big endian) to host
        // endianess (little endian)
        result.size = peppe::reverse_bytes(result.size);
        if (err1) {
            throw ConnectionClosed();
        }

        // 2. Read 'text' from stream
        // Allocate necessary space for the 'text'
        result.text.resize(result.size);
        auto [err2, len2] = co_await asio::async_read(
            stream,
            asio::buffer(result.text.data(), result.size),
            use_nothrow_awaitable
        );
        if (err2) {
            throw ConnectionClosed();
        }

        co_return result;
    }

    template<typename SyncWriteStream>
    void write(SyncWriteStream& stream) const {
        auto net_size = peppe::reverse_bytes(size);
        std::vector<asio::const_buffer> packet_data{
            asio::buffer(&msg_type, sizeof(std::uint8_t)),
            asio::buffer(&net_size, sizeof(std::uint32_t)),
            asio::buffer(text.data(), size)
        };

        asio::error_code err;
        asio::write(stream, packet_data, err);
        if (err) {
            throw ConnectionClosed();
        }
    }
};

struct SetName {
    static constexpr auto msg_type = MessageType::SetNameType;
    std::uint8_t size;
    std::string name;

    template<typename AsyncReadStream>
    static asio::awaitable<SetName> read(AsyncReadStream& stream) {
        SetName result;

        // 1. Read 'size' from stream
        auto [err1, len1] = co_await asio::async_read(
            stream,
            asio::buffer(&result.size, sizeof(std::uint8_t)),
            use_nothrow_awaitable
        );
        if (err1) {
            throw ConnectionClosed();
        }

        // Convert from network endianess (big endian) to host
        // endianess (little endian)
        result.size = peppe::reverse_bytes(result.size);

        // 2. Read 'name' from stream
        // Allocate necessary space for the 'name'
        result.name.resize(result.size);
        auto [err2, len2] = co_await asio::async_read(
            stream,
            asio::buffer(result.name.data(), result.size),
            use_nothrow_awaitable
        );
        if (err2) {
            throw ConnectionClosed();
        }

        co_return result;
    }

    template<typename SyncWriteStream>
    void write(SyncWriteStream& stream) const {
        auto net_size = peppe::reverse_bytes(size);
        std::vector<asio::const_buffer> packet_data{
            asio::buffer(&msg_type, sizeof(std::uint8_t)),
            asio::buffer(&net_size, sizeof(std::uint8_t)),
            asio::buffer(name.data(), size)
        };

        asio::error_code err;
        asio::write(stream, packet_data, err);
        if (err) {
            throw ConnectionClosed();
        }
    }
};

using Ipv4Bytes = asio::ip::address_v4::bytes_type;
using Ipv6Bytes = asio::ip::address_v6::bytes_type;

struct PeerDiscovery {

    static constexpr auto msg_type = MessageType::PeerDiscoveryType;
    std::vector<Ipv4Bytes> ipv4_addresses;
    std::vector<Ipv6Bytes> ipv6_addresses;

    template<typename AsyncReadStream>
    static asio::awaitable<PeerDiscovery> read(AsyncReadStream& stream) {
        PeerDiscovery result;

        // 1. Read 'count_ipv4' from stream
        std::uint8_t count_ipv4;
        auto [err1, len1] = co_await asio::async_read(
            stream,
            asio::buffer(&count_ipv4, sizeof(std::uint8_t)),
            use_nothrow_awaitable
        );
        if (err1) {
            throw ConnectionClosed();
        }

        // 2. Read 'count_ipv6' from stream
        std::uint8_t count_ipv6;
        auto [err2, len2] = co_await asio::async_read(
            stream,
            asio::buffer(&count_ipv6, sizeof(std::uint8_t)),
            use_nothrow_awaitable
        );
        if (err2) {
            throw ConnectionClosed();
        }

        // 3. Read 'ipv4_addresses' from stream
        result.ipv4_addresses.resize(count_ipv4);
        auto [err3, len3] = co_await asio::async_read(
            stream,
            asio::buffer(
                result.ipv4_addresses.data(), sizeof(Ipv4Bytes) * count_ipv4
            ),
            use_nothrow_awaitable
        );
        if (err3) {
            throw ConnectionClosed();
        }
        // TODO: Figure out if I need to revert the bytes here

        // 4. Read 'ipv6_addresses' from stream
        result.ipv6_addresses.resize(count_ipv6);
        auto [err4, len4] = co_await asio::async_read(
            stream,
            asio::buffer(
                result.ipv6_addresses.data(), sizeof(Ipv6Bytes) * count_ipv6
            ),
            use_nothrow_awaitable
        );
        if (err4) {
            throw ConnectionClosed();
        }
        // TODO: Figure out if I need to revert the bytes here

        co_return result;
    }

    template<typename SyncWriteStream>
    void write(SyncWriteStream& stream) const {
        fmt::print(stderr, "Write Ipv4's:\n");
        for (const auto& address : ipv4_addresses) {
            fmt::print(
                stderr,
                "{}.{}.{}.{}\n",
                address[0],
                address[1],
                address[2],
                address[3]
            );
        }
        std::uint8_t count_ipv4 = ipv4_addresses.size();
        std::uint8_t count_ipv6 = ipv6_addresses.size();
        std::vector<asio::const_buffer> packet_data{
            asio::buffer(&msg_type, sizeof(std::uint8_t)),
            asio::buffer(&count_ipv4, sizeof(std::uint8_t)),
            asio::buffer(&count_ipv6, sizeof(std::uint8_t)),
            asio::buffer(
                ipv4_addresses.data(), sizeof(Ipv4Bytes) * ipv4_addresses.size()
            ),
            asio::buffer(
                ipv6_addresses.data(), sizeof(Ipv6Bytes) * ipv6_addresses.size()
            )
        };

        asio::error_code err;
        asio::write(stream, packet_data, err);
        if (err) {
            throw ConnectionClosed();
        }
    }
};

struct Packet : public Variant<TextMessage, SetName, PeerDiscovery> {
    using Variant<TextMessage, SetName, PeerDiscovery>::Variant;

    static constexpr Packet text_message(std::string&& msg) {
        // TODO: name needs to have size < std::uint32_t::max()
        return { TextMessage{ .size = std::uint32_t(msg.size()),
                              .text = std::move(msg) } };
    }

    static constexpr Packet set_name(std::string&& name) {
        // TODO: name needs to have size < std::uint8_t::max() (implement
        // security checks)
        return { SetName{ .size = std::uint8_t(name.size()),
                          .name = std::move(name) } };
    }

    static constexpr Packet peer_discovery(
        std::ranges::input_range auto&& ipv4_addresses,
        std::ranges::input_range auto&& ipv6_addresses
    ) {
        return { PeerDiscovery{
            .ipv4_addresses =
                std::vector(ipv4_addresses.begin(), ipv4_addresses.end()),
            .ipv6_addresses =
                std::vector(ipv6_addresses.begin(), ipv6_addresses.end()) } };
    }

    template<typename SyncWriteStream>
    void write(SyncWriteStream& stream) const {
        // Create concept to visit and ensure read and write
        // member functions.
        match([&stream](auto&& var) { var.write(stream); });
        // text_message.write(stream);
    }

    template<typename AsyncReadStream>
    static asio::awaitable<Packet> read(AsyncReadStream& stream) {
        [[clang::uninitialized]] MessageType message_type;
        auto [err, len] = co_await asio::async_read(
            stream,
            asio::buffer(&message_type, sizeof(MessageType)),
            use_nothrow_awaitable
        );

        if (err) {
            throw ConnectionClosed();
        }

        switch (message_type) {
            case MessageType::TextMessageType: {
                Packet result = co_await TextMessage::read(stream);
                co_return result;
                break;
            }
            case MessageType::SetNameType: {
                Packet result = co_await SetName::read(stream);
                co_return result;
                break;
            }
            case MessageType::PeerDiscoveryType: {
                Packet result = co_await PeerDiscovery::read(stream);
                co_return result;
                break;
            }
            default: {
                throw ConnectionClosed();
            }
        }
    }
};

} // namespace peppe