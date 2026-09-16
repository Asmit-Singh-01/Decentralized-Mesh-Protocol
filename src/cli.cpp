#include "cli.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <algorithm>

SerialCLI::SerialCLI(MeshNode& mesh_node, uint32_t start_ms)
    : node(mesh_node), start_time_ms(start_ms) {}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

bool SerialCLI::process_command(const std::string& input, uint32_t current_time_ms) {
    std::string trimmed = trim(input);
    if (trimmed.empty()) {
        return true;
    }

    std::istringstream iss(trimmed);
    std::string token1, token2;
    iss >> token1;

    if (token1 == "mesh") {
        iss >> token2;
        if (token2 == "status") {
            cmd_status(current_time_ms);
            return true;
        } else if (token2 == "routes") {
            cmd_routes();
            return true;
        } else if (token2 == "ping") {
            std::string target;
            iss >> target;
            cmd_ping(target);
            return true;
        } else if (token2 == "help") {
            cmd_help();
            return true;
        } else {
            std::cout << "[CLI ERROR] Unknown mesh command: '" << token2 << "'. Type 'mesh help' for available commands." << std::endl;
            return false;
        }
    } else if (token1 == "help") {
        cmd_help();
        return true;
    }

    std::cout << "[CLI ERROR] Unknown command: '" << trimmed << "'. Type 'mesh help' for available commands." << std::endl;
    return false;
}

void SerialCLI::cmd_status(uint32_t current_time_ms) {
    uint32_t uptime = (current_time_ms >= start_time_ms) ? (current_time_ms - start_time_ms) : current_time_ms;
    size_t peer_count = node.get_routing_table().size();

    std::stringstream id_ss;
    id_ss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0')
          << node.get_node_id() << std::dec << " (" << node.get_node_id() << ")";

    std::cout << "+---------------------+-----------------------+" << std::endl;
    std::cout << "| Metric              | Value                 |" << std::endl;
    std::cout << "+---------------------+-----------------------+" << std::endl;
    std::cout << "| Local Node ID       | " << std::left << std::setw(21) << std::setfill(' ') << id_ss.str() << " |" << std::endl;
    std::cout << "| Active Neighbors    | " << std::left << std::setw(21) << std::setfill(' ') << peer_count << " |" << std::endl;
    std::cout << "| System Uptime       | " << std::left << std::setw(21) << std::setfill(' ') << (std::to_string(uptime) + " ms") << " |" << std::endl;
    std::cout << "+---------------------+-----------------------+" << std::endl;
}

void SerialCLI::cmd_routes() {
    const auto& table = node.get_routing_table();
    if (table.empty()) {
        std::cout << "[CLI] Routing table is empty. No active neighbors found." << std::endl;
        return;
    }

    std::cout << "+-------------+----------+------------+------------+" << std::endl;
    std::cout << "| Destination | Next Hop | RSSI (dBm) | Hop Count  |" << std::endl;
    std::cout << "+-------------+----------+------------+------------+" << std::endl;

    for (const auto& pair : table) {
        const PeerInfo& peer = pair.second;
        std::stringstream dest_ss, hop_ss;
        dest_ss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << peer.node_id;
        hop_ss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << peer.node_id;

        std::cout << "| " << std::left << std::setw(11) << dest_ss.str() << " "
                  << "| " << std::left << std::setw(8) << hop_ss.str() << " "
                  << "| " << std::left << std::setw(10) << static_cast<int>(peer.rssi) << " "
                  << "| " << std::left << std::setw(10) << static_cast<int>(peer.hop_count) << " "
                  << "|" << std::endl;
    }
    std::cout << "+-------------+----------+------------+------------+" << std::endl;
}

void SerialCLI::cmd_ping(const std::string& arg) {
    if (arg.empty()) {
        std::cout << "[CLI ERROR] Usage: mesh ping <node_id>" << std::endl;
        return;
    }

    uint16_t target_id = 0;
    try {
        if (arg.rfind("0x", 0) == 0 || arg.rfind("0X", 0) == 0) {
            target_id = static_cast<uint16_t>(std::stoul(arg, nullptr, 16));
        } else {
            target_id = static_cast<uint16_t>(std::stoul(arg, nullptr, 10));
        }
    } catch (...) {
        std::cout << "[CLI ERROR] Invalid node ID provided: '" << arg << "'" << std::endl;
        return;
    }

    if (target_id == 0) {
        std::cout << "[CLI ERROR] Cannot ping broadcast or zero address (0x0000)." << std::endl;
        return;
    }

    const uint8_t ping_payload[4] = {'P', 'I', 'N', 'G'};
    if (node.send_to_node(target_id, PacketType::BEACON, ping_payload, sizeof(ping_payload))) {
        std::stringstream ss;
        ss << "0x" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << target_id;
        std::cout << "[CLI] Ping packet sent to Node ID: " << ss.str() << " (" << std::dec << target_id << ")" << std::endl;
    } else {
        std::cout << "[CLI ERROR] Failed to serialize ping packet to Node ID: " << target_id << std::endl;
    }
}

void SerialCLI::cmd_help() {
    std::cout << "Available Mesh Serial CLI Commands:" << std::endl;
    std::cout << "  mesh status          - Display local node ID, active neighbor count, and uptime" << std::endl;
    std::cout << "  mesh routes          - Display formatted table of active mesh routing paths" << std::endl;
    std::cout << "  mesh ping <node_id>  - Send direct ping packet to target node ID (hex or dec)" << std::endl;
    std::cout << "  mesh help            - Show this list of available commands" << std::endl;
}
