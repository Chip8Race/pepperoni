#pragma once

#include "peer_table.hpp"
#include "utils.hpp"

#include <charconv>
#include <fmt/core.h>
#include <optional>
#define TOML_EXCEPTIONS 0
#include <toml++/toml.hpp>

namespace peppe {

struct Config {
    std::string name = "Me";
    int port = 2504;
    PeerTable peer_table;

    [[nodiscard]] static std::optional<Config> load_toml(
        std::string_view filename
    ) {
        // Parse toml table
        toml::parse_result toml_res = toml::parse_file(filename);
        if (toml_res.failed()) {
            fmt::print(
                "Config loading failed {}", toml_res.error().description()
            );
            return std::nullopt;
        }

        toml::table toml = std::move(toml_res).table();
        Config result{};

        // Load name
        auto name_opt = toml["name"].value<std::string>();
        if (name_opt.has_value()) {
            result.name = std::move(*name_opt);
        }

        // Load port
        const auto port_opt = toml["port"].value<int>();
        if (port_opt.has_value()) {
            result.port = *port_opt;
        }

        // Load peers
        if (toml::array* peers_arr = toml["peers"].as_array()) {
            peers_arr->for_each([&result](auto&& peer_str) {
                if constexpr (toml::is_string<decltype(peer_str)>) {

                    // Split ip string and then parse contents
                    const auto splitted = split(*peer_str, ':');
                    if (splitted.size() == 2) {

                        // Parse peer ip address
                        asio::error_code ec;
                        asio::ip::address peer_address =
                            asio::ip::address::from_string(
                                std::string(splitted[0]), ec
                            );
                        if (ec) {
                            fmt::print(
                                "Failed parsing peer ip address: {}",
                                splitted[0]
                            );
                            return;
                        }

                        // Parse peer port
                        int peer_port = -1;
                        auto parse_port_result = std::from_chars(
                            splitted[1].data(),
                            splitted[1].data() + splitted[1].size(),
                            peer_port
                        );
                        if (parse_port_result.ec != std::errc{}) {
                            fmt::print(
                                "Failed parsing peer port: {}", splitted[1]
                            );
                            return;
                        }

                        // Emplace peer
                        result.peer_table.push_back(Peer{
                            // FIXME: Unecessary allocation
                            .address = std::move(peer_address),
                            .port = peer_port,
                        });
                    }
                }
            });
        }

        return result;
    }
};

} // namespace peppe