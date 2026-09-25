#include "nvs_config.h"

#ifdef ARDUINO

#include <Arduino.h>
#include <Preferences.h>

namespace NVSConfig
{
    static Preferences preferences;

    constexpr const char* NVS_NAMESPACE = "mesh_config";

    bool begin()
    {
        return preferences.begin(NVS_NAMESPACE, false);
    }

    bool save_node_id(uint16_t node_id)
    {
        return preferences.putUShort("node_id", node_id) > 0;
    }

    bool load_node_id(uint16_t& node_id)
    {
        if (!preferences.isKey("node_id"))
        {
            return false;
        }

        node_id = preferences.getUShort("node_id", 0);
        return true;
    }

    bool save_node_name(const char* node_name)
    {
        if (node_name == nullptr)
        {
            return false;
        }

        return preferences.putString("node_name", node_name) > 0;
    }

    bool load_node_name(char* node_name, size_t max_length)
    {
        if (node_name == nullptr || max_length == 0)
        {
            return false;
        }

        if (!preferences.isKey("node_name"))
        {
            return false;
        }

        String stored_name =
            preferences.getString("node_name", "");

        if (stored_name.length() >= max_length)
        {
            stored_name =
                stored_name.substring(0, max_length - 1);
        }

        stored_name.toCharArray(
            node_name,
            max_length
        );

        return true;
    }

    void clear()
    {
        preferences.clear();
    }
}

#else

namespace NVSConfig
{
    bool begin()
    {
        return true;
    }

    bool save_node_id(uint16_t)
    {
        return true;
    }

    bool load_node_id(uint16_t&)
    {
        return false;
    }

    bool save_node_name(const char*)
    {
        return true;
    }

    bool load_node_name(char*, size_t)
    {
        return false;
    }

    void clear()
    {
    }
}

#endif
