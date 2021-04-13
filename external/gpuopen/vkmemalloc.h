#pragma once

// An proxy include file for the real vk_mem_alloc, so I don't need to redo
// compiler pragmas elsewhere

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunknown-pragmas"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness"
#pragma clang diagnostic ignored "-Wnullability-extension"
#pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#pragma clang diagnostic ignored "-Wmissing-field-initializers"
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wnested-anon-types"
#pragma clang diagnostic ignored "-Wunused-variable"

#include "vulkan/vulkan.h"
#include "vk_mem_alloc.h"

#pragma clang diagnostic pop
#pragma GCC diagnostic pop
