#pragma once

#include <chrono>
#include <iostream>
#include <ratio>
#include <type_traits>

template<typename Resolution = std::chrono::milliseconds>
class Timer
{
public:
    template<typename ReturnType = float>
    ReturnType elapsed() const
    {
        return std::chrono::duration<ReturnType>(ClockType::now() - m_start)
                .count();
    }

    using ClockType = std::chrono::steady_clock;

private:
    ClockType::time_point m_start = ClockType::now();
};

