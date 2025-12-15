#pragma once
#include "types.hpp"

#ifdef _WIN32
#   define WIN32_LEAN_AND_MEAN
#   include <Windows.h>
#elif defined (__linux__)
    // linux includes here
#else
#   error "OS isn't specified"
#endif // WIN32

namespace io {
    namespace native {
        struct Battery {
            IO_NODISCARD static bool in_use() noexcept {
#           ifdef _WIN32
                SYSTEM_POWER_STATUS status;
                if (::GetSystemPowerStatus(&status)) {
                    // ACLineStatus == 0 -> on battery; BatteryFlag == 128 -> no system battery
                    if (status.ACLineStatus == 0 && status.BatteryFlag != 128) return true;
                }
                return false;
#           elif defined(__linux__)
                // read /sys/class/power_supply or use upower.
                (void)0;
                return false;
#           else
#               error "Not implemented"
#           endif
            } // in_use
        }; // struct Battery
    } // namespace native
    static native::Battery battery;
} // namespace io