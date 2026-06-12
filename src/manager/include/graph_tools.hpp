#ifndef GRAPH_TOOLS_H
#define GRAPH_TOOLS_H

/**
 * struct TimingConflictGraph
 *
 * buildGraph(data)
 * - build timing conflict graph from struct Data
 * - Vertex.s will be initialized to 0
 * 
 * calcVertexEnterTime()
 * - compute vertex (zone) entering time
 * - all edges must be either ON or OFF
 *
 * updateTimeSlack()
 * - compute vertex (zone) entering time
 * - and vertex slack at the same time
 * - edges don't need to be ON of OFF
 *
 * isDeadlockFree()
 * - perform graph-based verification
 *
 * Other Usage
 * - to traverse V or E, use vertex_list, edge_list
 * - to get vertex (i, j) use vertex_map[i][j]
 * - to get edges regarding an vertex,
 *   use in_edges and out_edges
 * - to get reverse edge of type-3 edge e, use e.sibling
 *
 * 
 *
 * deadlock-verification (resource conflict graph)
 * TODO
 * update-time-slack
 * TODO
 */

#include <memory>
#include <vector>

enum EdgeType{E_TYPE_1, E_TYPE_2, E_TYPE_3};
enum VertexState{V_WHITE, V_GRAY, V_BLACK};
enum EdgeState{E_ON, E_OFF, E_UNDECIDED, E_DONTCARE};

struct Vertex;

typedef struct Edge{
    Vertex* u; // from
    Vertex* v; // to
    int w; // weight (edge waiting time)
    Edge* sibling; // for type-3 edge
    EdgeType type;
    
    // dontcare in initialization
    EdgeState state;
}Edge;

typedef struct Vertex{
    int i; // vehicle id
    int j; // conflict zone id
    int p; // vertex passing time
    std::vector<Edge*> in_edges;
    std::vector<Edge*> out_edges;
    Edge* type1_edge; // (out) NULL if not exist
    Edge* type1_in_edge; // (in) NULL if not exist
    Edge* type2_edge; // (out) NULL if not exist
    
    // dontcare in initialization
    VertexState state;
    int is_locked; // for priority-based
    int slack; // for cycle-removal-based
    
    // our goal
    int s; // zone entering time
}Vertex;

struct Data; // forward declaration, avoid including .hpp

// TimingConflictGraph: G in the paper
typedef struct TimingConflictGraph{
    // data is stored by ptrs
    // we can traverse V and E using these
    std::vector<std::unique_ptr<Vertex>> vertex_list;
    std::vector<std::unique_ptr<Edge>> edge_list;
    // subset of the above
    std::vector<Edge*> type3_edge_list;
    // to access v_{i,j}, use vertex_map[i][j]
    std::vector<std::vector<Vertex*>> vertex_map;
    // arrival_time of vehicle i
    std::vector<int> arrival_time;
    // source lane of vehicle i
    std::vector<int> source_lane;
    // type-2 edge waiting time + vertex passing time
    int same_src_delay;

    // methods

    // build graph from Data
    void buildGraph(const Data& d);
    // calculate Vertex.s based on edge states (on/off)
    void calcVertexEnterTime();
    // update v.s and v.slack
    void updateTimeSlack();
    // deadlock freeness verification
    int isDeadlockFree();
    // for debugging purpose
    void printContentAndCheck();

    TimingConflictGraph(){}
    TimingConflictGraph(const Data& d){
        buildGraph(d);
    }
}TimingConflictGraph;


#endif
