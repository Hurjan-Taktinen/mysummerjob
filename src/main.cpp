#include "logs/log.h"

int main()
{
    Log::init();

    INFO("MySummerJob Game ({},{},{})", 0, 0, 1);
}
