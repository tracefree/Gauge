#pragma once

#include "handle.hpp"

#include <queue>
#include <vector>

namespace Gauge {

template <typename T>
struct Pool {
    std::vector<T> data;
    std::vector<uint> generations;
    std::queue<uint> free_list;

    Handle<T> Allocate(T p_data);
    T* Get(Handle<T> p_handle);
    void Free(Handle<T> p_handle);

    Pool(uint p_initial_size = 2048);
};

template <typename T>
Pool<T>::Pool(uint p_initial_size) {
    data.resize(p_initial_size);
    generations.resize(p_initial_size);
    for (uint i = 0; i < p_initial_size - 1; ++i) {
        free_list.push(i);
    }
}

template <typename T>
Handle<T> Pool<T>::Allocate(T p_data) {
    if (free_list.empty()) [[unlikely]] {
        uint new_size = 2 * data.size();
        data.resize(new_size);
        generations.resize(new_size);
        for (uint i = new_size - 1; i >= (new_size / 2); --i) {
            free_list.push(i);
        }
    }
    uint index = free_list.front();
    data[index] = p_data;
    free_list.pop();
    return {
        .index = index,
        .generation = generations[index],
    };
}

template <typename T>
T* Pool<T>::Get(Handle<T> p_handle) {
    if (p_handle.generation != generations[p_handle.index]) {
        return nullptr;
    }

    return &data[p_handle.index];
}

template <typename T>
void Pool<T>::Free(Handle<T> p_handle) {
    generations[p_handle.index]++;
    free_list.push(p_handle.index);
}

}  // namespace Gauge