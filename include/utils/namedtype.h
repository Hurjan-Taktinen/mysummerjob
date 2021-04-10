#pragma once

#include <utility>

#include <iostream>

namespace utils
{

template<class T, class Param>
struct NamedType
{
    explicit constexpr NamedType(const T& value) : value(value) {}
    explicit constexpr NamedType(T&& value) : value(std::move(value)) {}
    constexpr operator T() { return value; }

    constexpr bool operator==(const T& rhs) const { return value == rhs; }
    constexpr bool operator!=(const T& rhs) const { return !(value == rhs); }

    constexpr auto& get() & { return value; }
    constexpr auto& get() const& { return value; }
    auto&& take() { return std::move(value); }

private:
    T value;
};

} // namespace utils
