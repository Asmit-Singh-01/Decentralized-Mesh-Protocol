#include <Arduino.h>
#include <Update.h>

class OTAUpdater {
public:
    // 1. Initialize OTA partition handler
    bool beginOTA(size_t total_size) {
        if (!Update.begin(total_size)) {
            Update.printError(Serial);
            return false;
        }
        Serial.println("OTA Begin: Partition ready.");
        return true;
    }

    // 2. Process incoming binary chunks and write to flash
    bool writeChunk(const uint8_t* chunk_data, size_t length) {
        if (Update.write(const_cast<uint8_t*>(chunk_data), length) != length) {
            Update.printError(Serial);
            return false;
        }
        return true;
    }

    // 3. Verify and reboot system upon success
    bool finalizeOTA() {
        if (!Update.end(true)) {
            Update.printError(Serial);
            return false;
        }
        Serial.println("OTA Update complete! Rebooting system...");
        ESP.restart();
        return true;
    }
};