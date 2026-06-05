#ifndef GRAPH_TOOLS_H
#define GRAPH_TOOLS_H

/**
 * struct TimingConflictGraph
 *
 * // build timing conflict graph from data
 * // Vertex.s will be initialized to 0
 * buildGraph(data);
 * 
 * - to traverse V or E, use vertex_list, edge_list
 * - to get vertex (i, j) use vertex_map[i][j]
 * - to get edges regarding an vertex,
 *   use in_edges and out_edges
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
    
    // dontcare in initialization
    EdgeType type;
    EdgeState state;
}Edge;

typedef struct Vertex{
    int i; // vehicle id
    int j; // conflict zone id
    int p; // vertex passing time
    std::vector<Edge*> in_edges;
    std::vector<Edge*> out_edges;
    
    // dontcare in initialization
    VertexState state;
    
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
    // to access v_{i,j}, use vertex_map[i][j]
    std::vector<std::vector<Vertex*>> vertex_map;

    // methods

    // build graph from Data
    void buildGraph(const Data& d);
    // for debugging purpose
    void printContentAndCheck();

    TimingConflictGraph(){}
    TimingConflictGraph(const Data& d){
        buildGraph(d);
    }
}TimingConflictGraph;


#endif
