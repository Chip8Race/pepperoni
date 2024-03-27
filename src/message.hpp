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
    TextMessage = 0,
    SetName = 1,
    PeerDiscovery = 2,
};

struct TextMessage {
    static constexpr auto msg_type = MessageType::TextMessage;
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

// TODO: serialization exception

struct Packet {
    MessageType message_type;
    // FIXME: Refactor this to variant with serializable types
    // std::uint32_t size;
    // char* msg;
    TextMessage text_message;

    static constexpr Packet message(std::string&& msg) {
        Packet res{ .message_type = MessageType::TextMessage,
                    .text_message{ .size = std::uint32_t(msg.size()),
                                   .text = std::move(msg) } };
        return res;
    }

    // TODO: Deal with streams here
    // awaitable<void> serialize(tcp::socket& socket) const {
    //     asio::error_code error;
    //     std::vector<asio::const_buffer> packet_data {
    //         asio::buffer(&type, sizeof(std::uint8_t)),
    //         asio::buffer(&size, sizeof(std::uint32_t)),
    //         asio::buffer(msg, size)};

    //     auto [err, _] = co_await asio::async_write(
    //         socket,
    //         packet_data,
    //         use_nothrow_awaitable
    //     );
    //     if (err == asio::error::eof) {
    //         // fmt::print(stderr, "Error: {}\n", err.message());
    //         throw ConnectionClosed();
    //     }
    //     co_return;
    // }

    template<typename SyncWriteStream>
    void serialize(SyncWriteStream& stream) const {
        fmt::print(stderr, "Pre message write\n");
        text_message.write(stream);
        fmt::print(
            stderr,
            "Sent TextMessage({}, '{}')\n",
            text_message.size,
            text_message.text
        );
    }

    // template<typename SyncWriteStream>
    // void serialize(SyncWriteStream& write_stream) const {
    //     asio::error_code error;
    //     auto net_size = peppe::reverse_bytes(size);
    //     std::vector<asio::const_buffer> packet_data{
    //         asio::buffer(&type, sizeof(std::uint8_t)),
    //         asio::buffer(&net_size, sizeof(std::uint32_t)),
    //         asio::buffer(msg, size)
    //     };

    //     asio::error_code err;
    //     asio::write(write_stream, packet_data, err);
    //     if (err) {
    //         // fmt::print(stderr, "Error: {}\n", err.message());
    //         throw ConnectionClosed();
    //     }
    // }

    template<typename AsyncReadStream>
    static asio::awaitable<Packet> deserialize(AsyncReadStream& stream) {
        fmt::print(stderr, "Pre message read\n");
        Packet result;

        auto [err1, len1] = co_await asio::async_read(
            stream,
            asio::buffer(&result.message_type, sizeof(std::uint8_t)),
            use_nothrow_awaitable
        );

        if (err1) {
            throw ConnectionClosed();
        }

        switch (result.message_type) {
            case MessageType::TextMessage: {
                result.text_message = co_await TextMessage::read(stream);
                fmt::print(
                    stderr,
                    "Received TextMessage({}, '{}')\n",
                    result.text_message.size,
                    result.text_message.text
                );
                break;
            }
            default: {
                break;
            }
        }

        co_return result;
    }
    // template<typename AsyncReadStream>
    // static awaitable<Packet> deserialize(AsyncReadStream& read_stream) {
    //     Packet result{
    //         .type = 0,
    //         .size = 0,
    //         .msg = nullptr,
    //     };

    //     // fmt::print(stderr, "Before first async read\n");
    //     auto [err1, len1] = co_await asio::async_read(
    //         read_stream,
    //         asio::buffer(&result.type, sizeof(std::uint8_t)),
    //         use_nothrow_awaitable
    //     );

    //     if (err1) {
    //         // fmt::print(stderr, "Error: {}\n", err1.message());
    //         throw ConnectionClosed();
    //     }

    //     switch (result.type) {
    //         case 0: {
    //             // fmt::print(stderr, "Before second async read\n");
    //             auto [err2, len2] = co_await asio::async_read(
    //                 read_stream,
    //                 asio::buffer(&result.size, sizeof(std::uint32_t)),
    //                 use_nothrow_awaitable
    //             );
    //             // Convert from network endianess (big endian) to host
    //             // endianess (little endian)
    //             result.size = peppe::reverse_bytes(result.size);

    //             if (err2) {
    //                 // fmt::print(stderr, "Error: {}\n", err2.message());
    //                 throw ConnectionClosed();
    //             }

    //             result.msg = new char[result.size];

    //             auto [err3, len3] = co_await asio::async_read(
    //                 read_stream,
    //                 asio::buffer(result.msg, result.size),
    //                 use_nothrow_awaitable
    //             );
    //             // fmt::print(stderr, "After msg content\n");
    //             if (err3) {
    //                 // fmt::print(stderr, "Error: {}\n", err3.message());
    //                 throw ConnectionClosed();
    //             }
    //             break;
    //         }
    //         default: {
    //             // TODO: read rest of bytes
    //             throw UnknownMsg();
    //         }
    //     }

    //     co_return result;
    // }
};

} // namespace peppe