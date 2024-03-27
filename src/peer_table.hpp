#pragma once

#include <asio/ip/address.hpp>
#include <vector>

namespace peppe {

struct Peer {
    std::string name;
    asio::ip::address address;
    int port;
};

using PeerTable = std::vector<Peer>;

} // namespace peppe
