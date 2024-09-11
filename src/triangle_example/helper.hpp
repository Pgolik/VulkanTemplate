#pragma once

#include <fmt/format.h>
#include <functional>
#include <stdlib.h>
#include <vector>

template <typename R>
auto vulcan_function_wrapper(auto f, auto... args) {
    std::vector<R> result {};
    uint32_t size {};
    f(args..., &size, nullptr);
    if(size <= 0) { return result; }
    result.resize(size);
    f(args..., &size, result.data());
    return result;
}
