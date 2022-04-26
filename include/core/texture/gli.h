
// A proxy include for GLI to suppress compile warnings coming from inside
// library

#pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wnullability-completeness"
// #pragma clang diagnostic ignored "-Wnullability-extension"
// #pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
// #pragma clang diagnostic ignored "-Wmissing-field-initializers"

#pragma clang diagnostic ignored "-Wdeprecated-anon-enum-enum-conversion"
#pragma clang diagnostic ignored "-Wignored-qualifiers"
#pragma clang diagnostic ignored "-Wunused-parameter"
#include "gli/gli.hpp"

#pragma clang diagnostic pop
