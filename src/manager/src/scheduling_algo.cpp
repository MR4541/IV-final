#include "scheduling_algo.hpp"
#include "graph_tools.hpp"
#include <algorithm>
#include <cassert>
#include <climits>
#include <queue>
#include <unordered_map>
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

// at toposort, only consider unlocked vertices and edges with at
// least one unlocked vertex.
// lock vertices with v.s <= time since they are scheduled
void calcVertexEnterTimeUntil(TimingConflictGraph *G, int time){
    // kahn's algorithm but with pseudo edges
    std::unordered_map<Vertex*, int> in_degree;
    for(const auto& v : G->vertex_list)
        if(!v->is_locked)
            in_degree[v.get()] = 0;
    for(const auto& e : G->edge_list){ // add ON edges
        if(e->state == E_ON && !e->u->is_locked && !e->v->is_locked)
            in_degree[e->v]++;
    }

    // add pseudo edge
    std::unordered_map<Vertex*, std::vector<Vertex*>> pseudo_edges;
    for(const auto& u : G->vertex_list){
        // type 1 edge (u.i, u.j) -> (u.i, j2)
        if(u->type1_edge == NULL || u->type1_edge->v->is_locked)
            continue;
        // for type 2/3 edge (u.i, u.j) -> (i2, u.j)
        // add pseudo edge (u.i, j2) -> (i2, u.j)
        for(const auto& e : u->out_edges){
            if(e->type == E_TYPE_1 || e->state != E_ON ||
                    e->v->is_locked)
                continue;
            pseudo_edges[u->type1_edge->v].push_back(e->v);
            in_degree[e->v]++;
        }
    }

    std::queue<Vertex*> q;
    for(const auto& v : G->vertex_list)
        if(!v->is_locked && in_degree[v.get()] == 0)
            q.push(v.get());
    
    std::vector<Vertex*> topoOrder;
    while(!q.empty()){
        Vertex* u = q.front();
        topoOrder.push_back(u);
        q.pop();

        for(auto& e : u->out_edges){ // real edges
            if(e->state != E_ON || e->v->is_locked)
                continue;
            in_degree[e->v]--;
            if(in_degree[e->v] == 0)
                q.push(e->v);
        }
        if(pseudo_edges.count(u)){ // if [u] exists
            for(auto& fake_v : pseudo_edges[u]){
                in_degree[fake_v]--;
                if(in_degree[fake_v] == 0)
                    q.push(fake_v);
            }
        }
    }

    // calculate vertex enter time
    for(auto& v : topoOrder){
        int max_s = G->arrival_time[v->i];
        for(auto& e : v->in_edges){
            if(e->state != E_ON)
                continue;
            if(e->type == E_TYPE_1){
                max_s = std::max(max_s, e->u->s + e->u->p + e->w);
            }else{ // E_TYPE_2 || E_TYPE_3
                if(e->u->type1_edge == NULL){
                    max_s = std::max(max_s, e->u->s + e->u->p + e->w);
                }else{
                    // note that s-w+w >= s+p+w
                    // so we don't need to max(max_s, s+p+w)
                    max_s = std::max(max_s,
                            e->u->type1_edge->v->s 
                            - e->u->type1_edge->w + e->w);
                }
            }
        }
        v->s = max_s;
        // mark vertex as locked
        if(v->s <= time){
            v->is_locked = 1;
        }
    }
}

void SchedAlgo::priorityBased(TimingConflictGraph& G){
    // intialize
    int M = G.arrival_time.size();
    int n_lanes = 0;
    for(auto lane_i : G.source_lane)
        n_lanes = std::max(n_lanes, lane_i);
    n_lanes++; // e.g. lane = {0,1,2}, n_lanes = 3
    // always turn on type 1,2 edge
    std::vector<Edge*> active_type3_edge_list;
    for(auto& e : G.edge_list){
        if(e->type == E_TYPE_1 || e->type == E_TYPE_2){
            e->state = E_ON;
        }else{
            e->state = E_UNDECIDED;
            active_type3_edge_list.push_back(e.get());
        }
    }
    // [k] stores i of cars (earliest first) from lane k
    std::vector<std::vector<int>> car_of_lane(n_lanes);
    for(int i = 0; i < M; i++)
        car_of_lane[G.source_lane[i]].push_back(i);
    std::vector<int> esti_a(G.arrival_time); // estimated arrival time
    std::vector<int> entered(M, 0); // if entered the intersection
    std::vector<int> first_zone(M, -1); // first j on path(i)
    for(auto& v : G.vertex_list){
        v->is_locked = 0;
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
        std::vector<Edge*> next_active_t3_edge_list; //edges that might change
        for(auto& e : active_type3_edge_list){
            int i1 = e->u->i, i2 = e->v->i;
            if(entered[i1] || entered[i2]) // already fixed
                continue; // won't appear in next_active_edge_list
            if(esti_a[i1]<esti_a[i2] || (esti_a[i1]==esti_a[i2]
                        && i1 < i2))
                e->state = E_ON;
            else
                e->state = E_OFF;
            next_active_t3_edge_list.push_back(e);
        }
        // update active edge list
        active_type3_edge_list = std::move(next_active_t3_edge_list);
        
        calcVertexEnterTimeUntil(&G, time);

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

// update v.s and v.slack
void updateTimeSlack(TimingConflictGraph* G){
    // TODO:
}

// perform cycle removal on vehicles [l, r)
void removeType3Edges(TimingConflictGraph* G, int l, int r){
    // TODO:
}

void SchedAlgo::cycleRemovalBased(TimingConflictGraph& G){
    // initialization
    for(auto& v : G.vertex_list){
        v->state = V_WHITE;
        v->slack = INT_MAX;
    }
    for(auto& e : G.edge_list){
        if(e->type == E_TYPE_3)
            e->state = E_UNDECIDED;
        else
            e->state = E_ON;
    }

    updateTimeSlack(&G);
    removeType3Edges(&G, 0, G.arrival_time.size()); // [0. M)
    G.calcVertexEnterTime();
}
