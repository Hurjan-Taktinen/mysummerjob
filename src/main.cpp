#include "logs/log.h"

#include <thread>

int main()
{
    Log::init();
    INFO("MySummerJob Game ({}.{}.{})", 0, 0, 1);

    auto t1 = std::thread([] {
        auto log = Log::create("vulkan context");
        log->info("nice graphics");
    });

    auto t2 = std::thread([] {
        auto log = Log::create("Key Events");
        log->warn("KeyCode 0x66({}) isPressed({})", 'E', true);
    });

    auto t3 = std::thread([] {
        auto log = Log::create("Timer");
        log->error("Timer expired {}", 123);
    });

    auto log = Log::create("main");
    log->info("Testing log format");

    t1.join();
    t2.join();
    t3.join();
}
