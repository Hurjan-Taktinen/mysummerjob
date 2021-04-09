#include "logs/log.h"

#include "application.h"

int main()
{
    Log::init();
    INFO("MySummerJob Game ({}.{}.{})", 0, 0, 1);

    app::Application app;
    app.init();
}
