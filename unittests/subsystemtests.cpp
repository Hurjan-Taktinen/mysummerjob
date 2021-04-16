#include "catch2/catch.hpp"

#include "core/subsystemregistry.h"
#include "core/workqueue.h"
#include "event/event.h"

#include <chrono>
#include <memory>
#include <string>
#include <thread>

struct RequesterEvent final : public event::EventBase
{
    std::string name;
    int pl = 0;
};

struct ProviderEvent final : public event::EventBase
{
    std::string name;
    static inline int pl = 0;
};

struct ProviderEvent2 final : public event::EventBase
{
    std::string name;
    static inline int pl = 0;
};

struct ProviderIf :
    public event::EventService,
    public core::Sub<ProviderEvent>,
    public core::Sub<ProviderEvent2>

{
    virtual ~ProviderIf() = default;
    using core::Sub<ProviderEvent>::handleEvent;
    using core::Sub<ProviderEvent2>::handleEvent;
};
struct RequesterIf :
    public event::EventService,
    public core::Sub<RequesterEvent>
{
    virtual ~RequesterIf() = default;
    using core::Sub<RequesterEvent>::handleEvent;
};

struct Fooman final :
    public core::SystemBase,
    public RequesterIf,
    public std::enable_shared_from_this<Fooman>
{
    Fooman()
    {
        log = logs::Log::create("Fooman");
    }

    void preinit(core::SystemRegistry& reg) override
    {
        reg.registerSubsystem<RequesterIf>(shared_from_this());
    }

    void init(const core::SystemRegistry& reg) override
    {
        auto ptr = reg.getSubsystem<ProviderIf>();

        auto payload = ProviderEvent {};
        payload.name = "fooman fooman fooman fooman";

        auto payload2 = ProviderEvent2 {};
        payload2.name = "fooman2 fooman2 fooman2 fooman2";

        for(int i = 0; i < 6; ++i)
        {
            auto pl = payload;
            auto pl2 = payload2;
            event::sendUnicast(ptr, pl);
            event::sendUnicast(ptr, pl2);
        }
    }

    void handleEvent(RequesterEvent&& e) override
    {
        log->info("RequesterEvent[{} says hello {}]", e.name, e.pl);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        log->info("sleep over");
    }

    logs::Logger log;
};

struct Barman final :
    public core::SystemBase,
    public ProviderIf,
    public std::enable_shared_from_this<Barman>
{
    static inline int count = 0;
    Barman()
    {
        count++;
        log = logs::Log::create("Barman");
    }

    void preinit(core::SystemRegistry& reg) override
    {
        reg.registerSubsystem<ProviderIf>(shared_from_this());
    }

    void init(const core::SystemRegistry& reg) override
    {
        auto ptr = reg.getSubsystem<RequesterIf>();

        auto event = RequesterEvent {};
        event.pl = 42;
        event.name = "barman";
        for(int i = 0; i < 6; ++i)
        {
            auto pl = event;
            event::sendUnicast(ptr, pl);
        }
    }

    void handleEvent(ProviderEvent&& e) override
    {
        log->info("ProviderEvent[{} says hello {}]", e.name, e.pl);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        log->info("sleep over");
    }

    void handleEvent(ProviderEvent2&& e) override
    {
        log->info("ProviderEvent222[{} says hello {}]", e.name, e.pl);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        log->info("sleep over");
    }

    logs::Logger log;
};

TEST_CASE("subsys")
{
    logs::Log::init();

    auto foo = std::make_shared<Fooman>();
    auto bar = std::make_shared<Barman>();

    auto& reg = core::getSystemRegistry();

    foo->preinit(reg);
    bar->preinit(reg);

    foo->init(reg);
    bar->init(reg);
}
