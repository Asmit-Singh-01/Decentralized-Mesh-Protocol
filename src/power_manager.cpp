#include "power_config.h"
#include "power_manager.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <esp_sleep.h>

namespace MeshPowerManager
{
    static uint32_t last_activity_ms = 0;
    static bool initialized = false;

    static RadioPowerHook radio_suspend_hook = nullptr;
    static RadioPowerHook radio_resume_hook = nullptr;

    void init()
    {
        last_activity_ms = millis();
        initialized = true;
    }

    void notify_activity()
    {
        last_activity_ms = millis();
    }

    void set_radio_hooks(
        RadioPowerHook suspend_hook,
        RadioPowerHook resume_hook
    )
    {
        radio_suspend_hook = suspend_hook;
        radio_resume_hook = resume_hook;
    }

    void sleep_if_idle(uint32_t idle_time_ms)
    {
#if ENABLE_MESH_POWER_SAVE

        if (!initialized)
        {
            return;
        }

        if (idle_time_ms <= MESH_MIN_SLEEP_MS)
        {
            return;
        }

        uint32_t sleep_ms = MESH_BEACON_INTERVAL_MS;

        if (idle_time_ms < MESH_BEACON_INTERVAL_MS)
        {
            sleep_ms =
                MESH_BEACON_INTERVAL_MS - idle_time_ms;
        }

        if (sleep_ms < MESH_MIN_SLEEP_MS)
        {
            sleep_ms = MESH_MIN_SLEEP_MS;
        }

        // Safely suspend the radio before entering light sleep.
        if (radio_suspend_hook)
        {
            radio_suspend_hook();
        }

        esp_sleep_enable_timer_wakeup(
            static_cast<uint64_t>(sleep_ms) * 1000ULL
        );

        esp_light_sleep_start();

        // Reinitialize the radio after waking up.
        if (radio_resume_hook)
        {
            radio_resume_hook();
        }

        last_activity_ms = millis();

#endif
    }
}

#else

namespace MeshPowerManager
{
    void init()
    {
    }

    void notify_activity()
    {
    }

    void set_radio_hooks(
        RadioPowerHook,
        RadioPowerHook
    )
    {
    }

    void sleep_if_idle(uint32_t)
    {
    }
}

#endif
