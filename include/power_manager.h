#pragma once

#include <cstdint>

namespace MeshPowerManager
{
    using RadioPowerHook = void (*)();

    void init();

    void notify_activity();

    void set_radio_hooks(
        RadioPowerHook suspend_hook,
        RadioPowerHook resume_hook
    );

    void sleep_if_idle(uint32_t idle_time_ms);
}
