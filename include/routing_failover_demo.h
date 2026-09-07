#pragma once

// Self-contained runtime demonstration of RoutingEngine's primary/backup
// fallback route selection (see routing_engine.h). There is no test framework
// in this repo, so this doubles as the verification for issue #10: it prints
// [SUCCESS]/[ERROR] lines and returns whether every check passed.
bool run_routing_failover_demo();
