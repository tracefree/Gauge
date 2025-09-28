#pragma once

#include <gauge/common.hpp>
#include <gauge/core/pool.hpp>
#include <gauge/core/string_id.hpp>

#include <print>
#include <unordered_map>

namespace Gauge {

template <typename R, typename... Ts>
concept IsResource = requires(R resource, StringID p_id, Ts... p_arguments) {
    resource = R::Load(p_id, p_arguments...);
    resource.Unload();
};

class ResourceManager {
   public:
    enum LoadStatus {
        UNLOADED,
        LOADING,
        LOADED,
    };

    enum Usage {
        UNLOAD_WHEN_UNUSED,
        KEEP_LOADED,
    };

   private:
    template <IsResource R>
    inline static Pool<R> pool;

    template <IsResource R>
    struct ResourceInfo {
        Handle<R> handle{};
        uint reference_count = 0;
        LoadStatus status = UNLOADED;
        Usage usage = UNLOAD_WHEN_UNUSED;
    };

    template <IsResource R>
    inline static StringID::Map<ResourceInfo<R>> resources;

   public:
    template <IsResource R, typename... Ts>
    static R* Load(StringID p_id, Ts... p_arguments) {
        if (resources<R>.contains(p_id)) {
            std::println("Resource {} is already loaded", p_id);
            ResourceManager::Reference<R>(p_id);
            auto& info = resources<R>[p_id];
            return pool<R>.Get(info.handle);
        }

        std::println("Loading {}", p_id);
        R resource = R::Load(p_id, p_arguments...);
        auto handle = pool<R>.Allocate(resource);
        resources<R>[p_id] = ResourceInfo<R>{
            .handle = handle,
            .reference_count = 1,
            .status = LOADED};
        return pool<R>.Get(handle);
    }

    template <IsResource R>
    static void Reference(StringID p_id) {
        if (!resources<R>.contains(p_id)) {
            std::println("Can not reference {}, resource is not loaded", p_id);
            return;
        }
        auto& info = resources<R>[p_id];
        info.reference_count++;
        std::println("Referencing {}: {} [+1]", p_id, info.reference_count);
    }

    template <IsResource R>
    static void Unreference(StringID p_id) {
        if (!resources<R>.contains(p_id)) {
            std::println("Can not dereference {}, resource is not loaded", p_id);
            return;
        }
        auto& info = resources<R>[p_id];
        if (info.reference_count > 1) {
            info.reference_count--;
            return;
        }
        info.reference_count = 0;
        std::println("Dereferencing {}: {} [-1]", p_id, info.reference_count);
        if (info.usage == KEEP_LOADED) {
            std::println("Keeping {} loaded", p_id);
            return;
        } else if (info.usage == UNLOAD_WHEN_UNUSED) {
            std::println("Unloading {}", p_id);
            R* resource = pool<R>.Get(info.handle);
            resource->Unload();
            pool<R>.Free(info.handle);
            resources<R>.erase(p_id);
            return;
        }
    }
};

}  // namespace Gauge