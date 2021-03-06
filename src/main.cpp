#include "logs/log.h"

#include "application.h"
#include "config/config.h"

#include <string_view>

int main(int argc, const char** argv)
{
    logs::Log::init();
    LGINFO("MySummerJob Game ({}.{}.{})", 0, 0, 1);

    std::string_view optionalConfigPath;
    if(argc > 1)
    {
        optionalConfigPath = argv[1];
    }

    config::Config::init(optionalConfigPath);

    auto app = std::make_shared<app::Application>();
    app->run();
}
