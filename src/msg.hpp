#pragma once

#include <asio/experimental/as_tuple.hpp>
#include <asio/experimental/awaitable_operators.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/read.hpp>
#include <asio/write.hpp>

using asio::ip::tcp;

#include "error.hpp"
#include "utils.hpp"

constexpr auto use_nothrow_awaitable
    = asio::experimental::as_tuple(asio::use_awaitable);

// TODO: serialization exception

struct Packet {
    std::uint8_t type;
    // FIXME: Refactor this to variant with serializable types
    std::uint32_t size;
    char* msg;

    static constexpr Packet message(std::string const& msg) {
        Packet res {.type = 0, .size = std::uint32_t(msg.size())};
        res.msg = new char[msg.size()];
        res.msg[msg.size()] = '\0';
        msg.copy(res.msg, msg.size());
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

    void serialize(tcp::socket& socket) const {
        asio::error_code error;
        auto net_size = peppe::reverse_bytes(size);
        std::vector<asio::const_buffer> packet_data {
            asio::buffer(&type, sizeof(std::uint8_t)),
            asio::buffer(&net_size, sizeof(std::uint32_t)),
            asio::buffer(msg, size)};

        asio::error_code err;
        asio::write(
            socket,
            packet_data,
            err
        );
        if (err) {
            // fmt::print(stderr, "Error: {}\n", err.message());
            throw ConnectionClosed();
        }
    }

    // TODO: Deal with streams here
    static awaitable<Packet> deserialize(tcp::socket& socket) {
        Packet result {
            .type = 0,
            .size = 0,
            .msg = nullptr,
        };

        // fmt::print(stderr, "Before first async read\n");
        auto [err1, len1] = co_await asio::async_read(
            socket,
            asio::buffer(&result.type, sizeof(std::uint8_t)),
            use_nothrow_awaitable
        );

        if (err1) {
            // fmt::print(stderr, "Error: {}\n", err1.message());
            throw ConnectionClosed();
        }

        switch (result.type) {
            case 0:
            {
                // fmt::print(stderr, "Before second async read\n");
                auto [err2, len2] = co_await asio::async_read(
                    socket,
                    asio::buffer(&result.size, sizeof(std::uint32_t)),
                    use_nothrow_awaitable
                );
                // Convert from network endianess (big endian) to host
                // endianess (little endian)
                result.size = peppe::reverse_bytes(result.size);
                
                if (err2) {
                    // fmt::print(stderr, "Error: {}\n", err2.message());
                    throw ConnectionClosed();
                }

                result.msg = new char[result.size];

                auto [err3, len3] = co_await asio::async_read(
                    socket,
                    asio::buffer(result.msg, result.size),
                    use_nothrow_awaitable
                );
                // fmt::print(stderr, "After msg content\n");
                if (err3) {
                    // fmt::print(stderr, "Error: {}\n", err3.message());
                    throw ConnectionClosed();
                }
                break;
            }
            default:
                throw UnknownMsg();
        }

        co_return result;
    }
};