#pragma once

#include <format>
#include <gauge/core/pool.hpp>

#include <string>
#include <unordered_map>

namespace Gauge {

class StringID {
   public:
    static Pool<std::string> string_pool;
    static std::unordered_map<std::string, Handle<std::string>> id_map;

    Handle<std::string> handle;

    StringID() {}
    StringID(Handle<std::string> p_handle) : handle(p_handle) {}
    StringID(std::string p_string) {
        if (!id_map.contains(p_string)) {
            handle = string_pool.Allocate(p_string);
            id_map[p_string] = handle;
        } else {
            handle = id_map[p_string];
        }
    }
    StringID(const char* p_string) : Gauge::StringID(std::string(p_string)) {}

    inline operator std::string*() const {
        return string_pool.Get(handle);
    }

    inline operator std::string&() const {
        return *string_pool.Get(handle);
    }

    inline operator uint64_t() const {
        return (uint64_t)handle;
    }

    size_t hash() const {
        return handle.hash();
    }

    bool operator==(const StringID& rhs) const {
        return handle == rhs.handle;
    }

    class HashFunction {
       public:
        size_t operator()(const StringID& string_id) const {
            return string_id.handle.hash();
        }
    };

    template <typename T>
    using Map = std::unordered_map<StringID, T, StringID::HashFunction>;
};

StringID operator""_id(const char* p_string, unsigned long);

}  // namespace Gauge

namespace std {
template <>
struct formatter<Gauge::StringID> {
    constexpr auto parse(format_parse_context& ctx) {
        return formatter<string>().parse(ctx);
    }

    auto format(const Gauge::StringID& obj, format_context& ctx) const {
        return formatter<std::string>().format(string(obj), ctx);
    }
};
}  // namespace std