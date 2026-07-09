#pragma once

// Local shim so editors and tools can include "cursor.hpp" from
// `ui/wm/include`. This forwards to the real cursor header in
// `ui/cursor/include` without requiring global include path adjustments.

#include "../../cursor/include/cursor.hpp"
