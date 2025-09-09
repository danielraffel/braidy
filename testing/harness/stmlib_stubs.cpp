// Stubs for stmlib static members needed for testing

#include "../../eurorack/stmlib/utils/random.h"

namespace stmlib {
    // Define the static member for Random class
    uint32_t Random::rng_state_ = 0x42424242;  // Default seed
}