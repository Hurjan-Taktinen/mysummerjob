#pragma once

#include "event/event.h"
#include "utils/singleton.h"
#include "logs/log.h"
#include "eventqueue.h"

#include <memory>
#include <map>
#include <typeindex>
#include <optional>
#include <cassert>
#include <mutex>
#include <type_traits>

namespace core
{

class SystemRegistry;

struct SystemBase
{
    virtual ~SystemBase() = default;
    virtual void preinit(SystemRegistry&) = 0;
    virtual void init(const SystemRegistry&) = 0;
};

class SystemRegistry final : public utils::Singleton<SystemRegistry>
{
    friend class utils::Singleton<SystemRegistry>;

public:
    SystemRegistry() { m_Log = logs::Log::create("System registry"); }
    ~SystemRegistry() { m_Subsystems.clear(); }

    template<class T>
    auto registerSubsystem(const std::weak_ptr<T>& ptr)
    {
        std::unique_lock {m_Mutex};

        std::type_index info = typeid(T);
        auto [it, inserted] = m_Subsystems.emplace(info, ptr);
        if(inserted)
        {
            m_Log->info("Succesfully added system({})", info.name());
        }
        else
        {
            m_Log->warn("Failed to add system({})", info.name());
            assert(false);
        }
    }

    template<class T>
    auto getSubsystem() const -> std::shared_ptr<T>
    {
        std::unique_lock {m_Mutex};

        std::type_index info = typeid(T);
        auto res = m_Subsystems.find(info);
        if(res != m_Subsystems.end())
        {
            if(auto ptr = res->second.lock(); ptr)
            {
                return std::dynamic_pointer_cast<T>(ptr);
            }
        }

        m_Log->warn("Failed to find system({})", info.name());
        return nullptr;
    }

private:
    std::map<std::type_index, std::weak_ptr<event::EventService>> m_Subsystems;
    mutable std::mutex m_Mutex;
    mutable logs::Logger m_Log;
};

[[nodiscard]] SystemRegistry& getSystemRegistry();
} // namespace core
