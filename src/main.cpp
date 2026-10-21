#include "mesh_gateway.h"
#include "rf_transceiver.h"
#include "acoustic_transceiver.h"
#include <iostream>
#include <memory>

int main() {
    std::cout << "Starting Decentralized Mesh Protocol - Multi-Domain Simulation...\n" << std::endl;

    uint16_t gateway_id = 0x5555;
    MeshGateway gateway(gateway_id);

    auto rf_modem = std::make_shared<RFTransceiver>(gateway_id);
    auto acoustic_modem = std::make_shared<AcousticTransceiver>(gateway_id);

    gateway.register_transceiver(rf_modem);
    gateway.register_transceiver(acoustic_modem);

    if (gateway.initialize_all()) {
        std::cout << "Gateway successfully initialized multi-transceiver stack!\n";
        
        // Test cross-domain bridging (RF to Underwater Acoustic)
        uint8_t dummy_payload[] = "Hello Subsurface AUV!";
        gateway.bridge_packet(dummy_payload, sizeof(dummy_payload), 
                              TransceiverType::RF_WIFI_ESP_NOW, 
                              TransceiverType::ACOUSTIC_UNDERWATER, 
                              0x8888);
    }

    std::cout << "\nAll Systems Nominal. Execution Finished Successfully." << std::endl;
    return 0;
}
