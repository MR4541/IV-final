#include "scheduling_algo.hpp"
#include "graph_tools.hpp"
#include <algorithm>
#include <vector>

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

void SchedAlgo::priorityBased(TimingConflictGraph &G){
    // intialize
    int M = G.arrival_time.size();
    int n_lanes = 0;
    for(auto lane_i : G.source_lane)
        n_lanes = std::max(n_lanes, lane_i);
    n_lanes++; // e.g. lane = {0,1,2}, n_lanes = 3
    // always turn on type 1,2 edge
    std::vector<Edge*> type_3_edge_list;
    for(auto& e : G.edge_list){
        if(e->type == E_TYPE_1 || e->type == E_TYPE_2){
            e->state = E_ON;
        }else{
            e->state = E_UNDECIDED;
            type_3_edge_list.push_back(e.get());
        }
    }
    // [k] stores i of cars (earliest first) from lane k
    std::vector<std::vector<int>> car_of_lane(n_lanes);
    for(int i = 0; i < M; i++)
        car_of_lane[G.source_lane[i]].push_back(i);
    std::vector<int> esti_a(G.arrival_time); // estimated arrival time
    std::vector<int> entered(M, 0); // if entered the intersection
    std::vector<int> first_zone(M, -1); // first j on path(i)
    for(const auto& v : G.vertex_list){
        int isFirst = 1;
        for(const auto& e : v->in_edges)
            if(e->type == E_TYPE_1){
                isFirst = 0;
                break;
            }
        if(isFirst)
            first_zone[v->i] = v->j;
    }

    int time = 0; // simulated time
    int n_entered = 0;
    while(n_entered < M){
        // update esti_a
        for(auto& lane : car_of_lane){
            for(int idx = 0; idx < (int)lane.size(); idx++){
                int i = lane[idx];
                if(entered[i]){
                    esti_a[i] = G.vertex_map[i][first_zone[i]]->s;
                    continue;
                }
                if(idx == 0){ //first car
                    // = max(esti_a, simulated time now)
                    esti_a[i] = std::max(G.arrival_time[i], time);
                }else{ // other car
                    // = max(esti_a, time, esti_a(prev car)+w+p)
                    esti_a[i] = std::max(G.arrival_time[i],
                        esti_a[lane[idx-1]] + G.same_src_delay);
                    esti_a[i] = std::max(esti_a[i], time);
                }
            }
        }

        // update type 3 edge state by esti_a
        for(auto& e : type_3_edge_list){
            int i1 = e->u->i, i2 = e->v->i;
            if(esti_a[i1]<esti_a[i2] || (esti_a[i1]==esti_a[i2]
                        && i1 < i2))
                e->state = E_ON;
            else
                e->state = E_OFF;
        }
        
        G.calcVertexEnterTime();
        // update entered
        for(int i = 0; i < M; i++)
            if(!entered[i] &&
                    G.vertex_map[i][first_zone[i]]->s <= time){
                entered[i] = 1;
                n_entered++;
            }
        time += 10; // 1 second
    }
}
