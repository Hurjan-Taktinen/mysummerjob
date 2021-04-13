#include "core/subsystemregistry.h"

namespace core
{
SystemRegistry& getSystemRegistry()
{
    return SystemRegistry::getInstance();
}
} // namespace core
