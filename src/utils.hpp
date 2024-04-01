#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// explicit deduction guide (not needed as of C++20)
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

namespace peppe {

template<typename... V>
struct Variant : public std::variant<V...> {
    using std::variant<V...>::variant;
    template<typename... F>
    void match(F&&... funcs) const {
        std::visit(overloaded{ std::forward<F>(funcs)... }, *this);
    }

    template<typename... F>
    void match(F&&... funcs) {
        std::visit(overloaded{ std::forward<F>(funcs)... }, *this);
    }
};

template<typename T>
    requires std::is_arithmetic_v<T>
[[nodiscard]] T reverse_bytes(T number) {
    auto data = std::bit_cast<std::array<char, sizeof(T)>>(number);
    std::ranges::reverse(data);
    return std::bit_cast<T>(data);
}

[[nodiscard]] constexpr std::vector<std::string_view>
split(std::string_view str, char separator) {
    uint32_t num_sparator = 0;
    for (uint32_t i = 0; i < str.length(); ++i)
        if (str[i] != separator && (i == 0 || str[i - 1] == separator)) {
            ++num_sparator;
        }

    std::vector<std::string_view> res(num_sparator);
    uint32_t idx = 0;
    uint32_t begin = 0;
    uint32_t end = 0;
    for (uint32_t i = 0; i < str.length(); ++i) {
        if (str[i] == separator || str[i] == '\n') {
            if (end != begin) {
                std::string_view tmp(str.data(), end);
                tmp.remove_prefix(begin);
                res[idx++] = tmp;
            }
            begin = ++end;
        }
        else {
            ++end;
        }
    }
    std::string_view tmp(str.data(), end);
    tmp.remove_prefix(begin);

    if (!tmp.empty())
        res[idx] = tmp;
    return res;
}

} // namespace peppe