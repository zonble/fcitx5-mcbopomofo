#ifndef FMT
#define FMT

#include <fmt/core.h>

#if FMT_VERSION >= 1000000
// Syntax for fmt v10+
#define FMT_RUNTIME(x) fmt::runtime(x)
#else
// Safe syntax for fmt v9 or earlier
#define FMT_RUNTIME(x) (x)
#endif

#endif /* FMT */
