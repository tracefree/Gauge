#include "string_id.hpp"
#include <string>

using namespace Gauge;

Pool<std::string> StringID::string_pool;
std::unordered_map<std::string, Handle<std::string>> StringID::id_map;

StringID Gauge::operator""_id(const char* p_string, unsigned long) {
    std::string string(p_string);
    if (StringID::id_map.contains(string)) {
        return StringID::id_map[p_string];
    }

    return StringID(string);
}