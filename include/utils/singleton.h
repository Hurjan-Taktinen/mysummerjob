#pragma once

namespace utils
{

template<class T>
class Singleton
{
public:
    static T& getInstance()
    {
        static T instance;
        return instance;
    }

protected:
    Singleton() = default;
    Singleton(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton& operator=(Singleton&&) = delete;
};

} // namespace utils
