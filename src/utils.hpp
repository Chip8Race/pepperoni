#pragma once

#include <algorithm>
#include <cstdint>
#include <string_view>
#include <variant>
#include <vector>

template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

namespace peppe {

template<typename... V>
struct Variant : public std::variant<V...> {
    template<typename... F>
    void match(F&&... funcs) const {
        std::visit(overloaded{ std::forward<F>(funcs)... }, *this);
    }
};

template<typename T>
T reverse_bytes(T x) {
    char* const ptr = reinterpret_cast<char*>(&x);
    std::reverse(ptr, ptr + sizeof(T));
    return *reinterpret_cast<T*>(ptr);
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