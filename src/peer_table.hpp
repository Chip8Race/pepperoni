#pragma once

#include <asio/ip/address.hpp>
#include <vector>

namespace peppe {

struct Peer {
    asio::ip::address address;
    int port;
};

using PeerTable = std::vector<Peer>;

} // namespace peppe
