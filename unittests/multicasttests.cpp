#include "catch2/catch.hpp"

#include "core/subsystemregistry.h"
#include "core/workqueue.h"
#include "event/event.h"

struct keypressed final : public event::EventBase
{
    std::string name;
    int code = 0;
};

struct keyreleased final : public event::EventBase
{
    std::string name;
    int code = 0;
};

struct keylistener :
    public event::EventService,
    public core::Sub<keypressed>,
    public core::Sub<keyreleased>

{
    virtual ~keylistener() = default;
    using core::Sub<keypressed>::handleEvent;
};

struct Booman final :
    public core::SystemBase,
    public keylistener,
    public std::enable_shared_from_this<Booman>
{
    Booman() { log = logs::Log::create("Booman"); }

    void preinit(core::SystemRegistry&) override
    {
        // reg.registerSubsystem<RequesterIf>(shared_from_this());
        auto& que = core::getWorkQueue();
        que.subscribeToMulticast(
                event::EventType::KeyPressed, shared_from_this());
    }

    void init(const core::SystemRegistry&) override
    {
        // auto ptr = reg.getSubsystem<ProviderIf>();
    }

    void handleEvent(keypressed&& e) override
    {
        log->info("keypressed[{}] ", e.code);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        eventcount += 1;
    }

    void handleEvent(keyreleased&& e) override
    {
        log->info("keyreleased[{}] ", e.code);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        eventcount += 1;
    }

    int eventcount = 0;

    logs::Logger log;
};

struct Fanman final :
    public core::SystemBase,
    public keylistener,
    public std::enable_shared_from_this<Fanman>
{
    static inline int count = 0;
    Fanman()
    {
        count++;
        log = logs::Log::create("Fanman");
    }

    void preinit(core::SystemRegistry&) override
    {
        auto& que = core::getWorkQueue();
        que.subscribeToMulticast(
                event::EventType::KeyPressed, shared_from_this());
    }

    void init(const core::SystemRegistry&) override
    {
        auto event = keypressed {};
        event.code = 42;
        event.name = "space";
        for(int i = 0; i < 5; ++i)
        {
            auto pl = event;
            pl.code++;
            event::sendMulticast(event::EventType::KeyPressed, pl);
        }
    }

    void handleEvent(keypressed&& e) override
    {
        log->info("keypressed[{}] ", e.code);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        eventcount += 1;
    }

    void handleEvent(keyreleased&& e) override
    {
        log->info("keyreleased[{}] ", e.code);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        eventcount += 1;
    }

    int eventcount = 0;
    logs::Logger log;
};

TEST_CASE("multicast")
{
    // logs::Log::init();

    auto foo = std::make_shared<Booman>();
    auto bar = std::make_shared<Fanman>();

    auto& reg = core::getSystemRegistry();

    foo->preinit(reg);
    bar->preinit(reg);

    foo->init(reg);
    bar->init(reg);

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    REQUIRE(5 == foo->eventcount);
    REQUIRE(5 == bar->eventcount);
}
