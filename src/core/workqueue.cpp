#include "core/workqueue.h"

namespace core
{

WorkQueue& getWorkQueue()
{
    return WorkQueue::getInstance();
}

} // namespace core
