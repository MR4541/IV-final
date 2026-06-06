#include "scheduling_algo.hpp"
#include "graph_tools.hpp"

void SchedAlgo::threeDimension(TimingConflictGraph& G){
    // set all type 1,2 edge to ON, type 3 to OFF
    for(auto& e : G.edge_list){
        if(e->type == E_TYPE_3)
            e->state = E_OFF;
        else
            e->state = E_ON;
    }
    
    G.calcVertexEnterTime();
}
