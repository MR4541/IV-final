#ifndef SCHEDULING_ALGO_H
#define SCHEDULING_ALGO_H

/**
 * scheduling algorithms
 * perform scheduling on TimingConflictGraph G
 * then update vertex enter time
 *
 * threeDimension(G)
 * - assume type 3 edges does no exist
 *
 * firstComeFirstServe(G)
 * - turn on type 3 edge u->v if u.i < v.i
 *   because vehicles are sorted by arrival time
 *
 * priorityBased(G)
 * - set simulated time = 0
 * - calculate estimated arrival time (esti-a) of each car
 * - remove type-3 edge and calculate s
 * - simulated time += 1 sec, update esti-a
 *
 * cycleRemovalBased(G)
 * - Cycle-Removal-Based Scheduling
 */

struct TimingConflictGraph;

typedef struct SchedAlgo{
    // 3D-intersection (performance upper bound)
    static void threeDimension(TimingConflictGraph& G);
    // First Come First Serve (performance lower bound)
    static void firstComeFirstServe(TimingConflictGraph& G);
    // Priority based (estimated arrival time)
    static void priorityBased(TimingConflictGraph& G);
    // Cycle-Removal-Based Scheduling
    static void cycleRemovalBased(TimingConflictGraph& G);
}SchedAlgo;

#endif
