#pragma once

// Local shim so editors and tools can include "background.hpp" from
// `ui/wm/include`. This forwards to the real background header in
// `ui/bckg/include` without requiring global include path adjustments.

#include "../../bckg/include/background.hpp"
