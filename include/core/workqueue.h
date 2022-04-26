#pragma once

#include "riften/thiefpool.hpp"
#include "utils/singleton.h"

#include <memory>
#include <map>
#include <mutex>

namespace core
{

class WorkQueue final : public utils::Singleton<WorkQueue>
{
    friend class utils::Singleton<WorkQueue>;

public:
    template<class F, class... Args>
    decltype(auto) submitWork(F&& f, Args&&... args)
    {
        std::unique_lock lock{m_Mutex};
        return m_Pool.enqueue(std::forward<F>(f), std::forward<Args>(args)...);
    }

private:
    WorkQueue() = default;
    riften::Thiefpool m_Pool;
    std::mutex m_Mutex;
};

[[nodiscard]] WorkQueue& getWorkQueue();

} // namespace core
