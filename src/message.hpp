#pragma once

#include "fmt/base.h"
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

        result.text.reserve(result.size);

        // 2. Read 'text' from stream
        // NOTE: Coroutines still don't support alloca arrays (char[dynsize])
        char* tmp = new char[result.size];
        auto [err2, len2] = co_await asio::async_read(
            stream, asio::buffer(tmp, result.size), use_nothrow_awaitable
        );
        if (err2) {
            throw ConnectionClosed();
        }
        result.text.assign(tmp, result.size);

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

        // Convert from network endianess (big endian) to host
        // endianess (little endian)
        result.size = peppe::reverse_bytes(result.size);

        if (err1) {
            throw ConnectionClosed();
        }

        result.name.reserve(result.size);

        // 2. Read 'name' from stream
        // NOTE: Coroutines still don't support alloca arrays (char[dynsize])
        char* tmp = new char[result.size];
        auto [err2, len2] = co_await asio::async_read(
            stream, asio::buffer(tmp, result.size), use_nothrow_awaitable
        );
        if (err2) {
            throw ConnectionClosed();
        }
        result.name.assign(tmp, result.size);

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

struct Packet : public Variant<TextMessage, SetName> {
    using Variant<TextMessage, SetName>::Variant;

    static constexpr Packet text_message(std::string&& msg) {
        // TODO: name needs to have size < std::uint32_t::max()
        return { TextMessage{ .size = std::uint32_t(msg.size()),
                              .text = std::move(msg) } };
    }

    static constexpr Packet set_name(std::string&& name) {
        // TODO: name needs to have size < std::uint8_t::max()
        return { SetName{ .size = std::uint8_t(name.size()),
                          .name = std::move(name) } };
    }

    template<typename SyncWriteStream>
    void serialize(SyncWriteStream& stream) const {
        // Create concept to visit and ensure read and write
        // member functions.
        match([&stream](auto&& var) { var.write(stream); });
        // text_message.write(stream);
    }

    template<typename AsyncReadStream>
    static asio::awaitable<Packet> deserialize(AsyncReadStream& stream) {

        MessageType message_type;
        auto [err1, len1] = co_await asio::async_read(
            stream,
            asio::buffer(&message_type, sizeof(MessageType)),
            use_nothrow_awaitable
        );

        if (err1) {
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
            default: {
                throw ConnectionClosed();
                break;
            }
        }
    }
};

} // namespace peppe