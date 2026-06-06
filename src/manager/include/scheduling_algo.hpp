#ifndef SCHEDULING_ALGO_H
#define SCHEDULING_ALGO_H

/**
 * scheduling algorithms
 * perform scheduling on TimingConflictGraph G
 * then update vertex enter time
 *
 * threeDimension(G)
 * - assume type 3 edges does no exist
 */

struct TimingConflictGraph;

typedef struct SchedAlgo{
    // 3D-intersection (performance upper bound)
    static void threeDimension(TimingConflictGraph& G);
}SchedAlgo;

#endif
