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

void SchedAlgo::firstComeFirstServe(TimingConflictGraph& G){
    // for each type 3 edge pair between u, v
    // and arrival time u < v, turn ON u->v and turn OFF v->u
    // directly compare i since the cars are sorted
    for(auto& e : G.edge_list){
        if(e->type == E_TYPE_3){
            if(e->u->i < e->v->i)
                e->state = E_ON;
            else
                e->state = E_OFF;
        }else{
            e->state = E_ON;
        }
    }

    G.calcVertexEnterTime();
}
