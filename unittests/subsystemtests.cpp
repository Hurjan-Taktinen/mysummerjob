#include "catch2/catch.hpp"

#include "core/subsystemregistry.h"
#include "core/eventqueue.h"
#include "event/event.h"

#include <memory>
#include <iostream>
#include <string>

struct RequesterEvent final : public event::EventBase
{
    std::string name;
    int pl = 0;
};

struct ProviderEvent final : public event::EventBase
{
    std::string name;
    int pl = 0;
};

struct ProviderIf : public event::EventService, public core::Sub<ProviderEvent>
{
    virtual ~ProviderIf() = default;
};
struct RequesterIf :
    public event::EventService,
    public core::Sub<RequesterEvent>
{
    virtual ~RequesterIf() = default;
};

struct Fooman final :
    public core::SystemBase,
    public RequesterIf,
    public std::enable_shared_from_this<Fooman>
{
    void preinit(core::SystemRegistry& reg) override
    {
        reg.registerSubsystem<RequesterIf>(shared_from_this());
    }

    void init(const core::SystemRegistry& reg) override
    {
        auto ptr = reg.getSubsystem<ProviderIf>();
        auto& queue = core::getEventQueue();
        {
            auto payload = ProviderEvent {};
            payload.pl = 10;
            payload.name = "fooman";
            queue.unicast(ptr, payload);
        }
    }

    void handleEvent(const RequesterEvent& e) override
    {
        std::cout << e.name << " says hello\n";
    }
};

struct Barman final :
    public core::SystemBase,
    public ProviderIf,
    public std::enable_shared_from_this<Barman>
{
    void preinit(core::SystemRegistry& reg) override
    {
        reg.registerSubsystem<ProviderIf>(shared_from_this());
    }

    void init(const core::SystemRegistry& reg) override
    {
        auto ptr = reg.getSubsystem<RequesterIf>();
        auto& queue = core::getEventQueue();

        {
            auto event = RequesterEvent {};
            event.pl = 42;
            event.name = "barman";
            queue.unicast(ptr, event);
        }
    }

    void handleEvent(const ProviderEvent& e) override
    {
        std::cout << e.name << " says hello == "<< e.pl << "\n";
    }
};

TEST_CASE("subsys")
{
    auto foo = std::make_shared<Fooman>();
    auto bar = std::make_shared<Barman>();

    auto& reg = core::getSystemRegistry();

    foo->preinit(reg);
    bar->preinit(reg);

    foo->init(reg);
    bar->init(reg);
}
