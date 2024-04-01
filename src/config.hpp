#pragma once

#include "peer_table.hpp"
#include "utils.hpp"

#include <charconv>
#include <fmt/core.h>
#include <optional>
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

namespace peppe {

constexpr int default_port = 2504;

struct Config {
    std::string name = "Me";
    int port = default_port;
    PeerTable peer_table;

    [[nodiscard]] static std::optional<Config> load_toml(
        std::string_view filename
    );
};

} // namespace peppe