#pragma once

#include <asio/ip/address.hpp>
#include <vector>

struct Peer {
    std::string name;
    asio::ip::address address;
    int port;
};

using PeerTable = std::vector<Peer>;
// struct PeerTable {
//     std::vector<Peer> peers;

//     // [[nodiscard]] static PeerTable load(std::filesystem::path const&
//     // filename) {
//     //     // fmt::print(stderr, "Loading peers from '{}'\n",
//     filename.c_str());
//     //     //  TODO: Error handling
//     //     auto tmp = filename.c_str();
//     //     std::ifstream in_stream(filename.c_str());
//     //     std::vector<Peer> peers;

//     //     std::string name;
//     //     std::string address;
//     //     std::string port;

//     //     while (in_stream >> name >> address >> port) {
//     //         peers.emplace_back(
//     //             name,
//     //             asio::ip::address::from_string(address),
//     //             std::atoi(port.c_str())
//     //         );
//     //     }

//     //     return PeerTable {
//     //         .peers = std::move(peers),
//     //     };
//     // }
// };