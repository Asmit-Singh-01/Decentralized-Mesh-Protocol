#include "routing_failover_demo.h"
#include "routing_engine.h"
#include <iostream>

namespace {

bool check(bool condition, const char* description) {
    if (condition) {
        std::cout << "[SUCCESS] " << description << std::endl;
    } else {
        std::cerr << "[ERROR] " << description << std::endl;
    }
    return condition;
}

} // namespace

bool run_routing_failover_demo() {
    std::cout << "\n[SYSTEM] Running fallback route selection demo (issue #10)..." << std::endl;

    constexpr uint16_t LOCAL_ID = 0x1001;
    constexpr uint16_t DEST_ID = 0x2001;    // Also the beacon sender_id / table key,
                                             // per the pre-existing per-sender keying quirk.
    constexpr uint16_t BACKUP_HOP = 0x3001; // Proposed via the beacon's dest_id field.

    RoutingEngine engine(LOCAL_ID);
    bool ok = true;

    // 1. Establish a strong primary route directly from DEST_ID.
    engine.process_beacon(DEST_ID, DEST_ID, /*hops=*/1, /*rssi=*/-40, /*current_time=*/1000);
    uint16_t next_hop = 0;
    ok &= check(engine.get_next_hop(DEST_ID, next_hop) && next_hop == DEST_ID,
                "Primary route established via direct beacon");

    // 2. A weaker beacon for the same key proposes BACKUP_HOP as a fallback candidate.
    engine.process_beacon(DEST_ID, BACKUP_HOP, /*hops=*/3, /*rssi=*/-75, /*current_time=*/1100);
    uint16_t backup_hop = 0;
    ok &= check(engine.get_backup_hop(DEST_ID, backup_hop) && backup_hop == BACKUP_HOP,
                "Weaker secondary beacon populated the backup hop");

    // 3. A pathological beacon proposing our own node id as a hop must be rejected
    //    by the loop-avoidance guard, leaving the backup unchanged.
    engine.process_beacon(DEST_ID, LOCAL_ID, /*hops=*/2, /*rssi=*/-50, /*current_time=*/1150);
    uint16_t backup_after_loop_attempt = 0;
    ok &= check(engine.get_backup_hop(DEST_ID, backup_after_loop_attempt) &&
                    backup_after_loop_attempt == BACKUP_HOP,
                "Loop guard rejected a beacon proposing local_node_id as a hop");

    // 4. Simulate the primary route failing; we should fail over to the backup hop
    //    and see the [MESH WARN] switch-over line printed above.
    bool switched = engine.report_route_failure(DEST_ID, 2000);
    ok &= check(switched, "report_route_failure() switched over to the backup hop");
    ok &= check(engine.get_next_hop(DEST_ID, next_hop) && next_hop == BACKUP_HOP,
                "get_next_hop() now returns the former backup hop");

    // 5. Failing again with no backup available should be refused and counted
    //    separately (see the [MESH WARN] "No backup hop available" line above).
    bool switched_again = engine.report_route_failure(DEST_ID, 2100);
    ok &= check(!switched_again, "report_route_failure() correctly refused with no backup left");

    const FailoverStats& stats = engine.get_failover_stats();
    ok &= check(stats.total_failovers == 1 && stats.failed_with_no_backup == 1,
                "Failover metrics counted exactly one success and one no-backup failure");

    std::cout << "[SYSTEM] Fallback route selection demo "
              << (ok ? "complete." : "FAILED.") << std::endl;
    return ok;
}
