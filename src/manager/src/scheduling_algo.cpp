#include "scheduling_algo.hpp"
#include "graph_tools.hpp"
#include <algorithm>
#include <cassert>
#include <climits>
#include <cstdio>
#include <queue>
#include <unordered_map>
#include <unordered_set>
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

void updateVertexStateAndPropagate(Vertex* v,
        std::unordered_set<Vertex*>& leaderVertices, int i_r);

void findLeaders(TimingConflictGraph* G, int i_l, int i_r,
        std::unordered_set<Vertex*>& leaderVertices){
    leaderVertices.clear();
    // v (first car of each lane, first zone of that car)
    std::unordered_map<int, Vertex*> leader_of_lane;
    for(auto& v : G->vertex_list){
        if(v->i < i_l || i_r <= v->i || v->type1_in_edge != nullptr)
            continue;
        int v_srclane = G->source_lane[v->i];
        auto it = leader_of_lane.find(v_srclane);
        // record DNE or v->i arrives earlier
        if(it == leader_of_lane.end() ||
                v->i < leader_of_lane[v_srclane]->i)
            leader_of_lane[v_srclane] = v.get();
    }
    // extract result (leader vertices)
    for(auto& pair : leader_of_lane){
        updateVertexStateAndPropagate(pair.second, leaderVertices, i_r);
    }
}

void findCandidates(const std::unordered_set<Vertex*>& leaderVertices,
        std::vector<Edge*>& candidateEdges){
    candidateEdges.clear();
    std::unordered_set<Edge*> candidate_set; // avoid duplicate edge
    for(auto& v : leaderVertices){
        for(auto& e : v->in_edges)
            if(e->state == E_UNDECIDED)
                candidate_set.insert(e);
        for(auto& e : v->out_edges)
            if(e->state == E_UNDECIDED)
                candidate_set.insert(e);
    }
    // extract result
    for(auto& e : candidate_set)
        candidateEdges.push_back(e);
}

void updateVertexStateAndPropagate(Vertex* v,
        std::unordered_set<Vertex*>& leaderVertices, int i_r){
    if(v->i >= i_r) // out of bound
        return;
    int is_gray = 1, is_black = 1;
    // GRAY: u = BLACK for all type 1,2 e = (u, v)
    // BLACK: GRAY && no undecided edge
    for(auto& e : v->in_edges){
        if(e->state == E_UNDECIDED){
            is_black = 0;
        }else if(e->type != E_TYPE_3 && e->u->state != V_BLACK){
            is_gray = 0;
            break;
        }
    }
    is_black = is_black && is_gray;

    if(is_black && is_gray){ // BLACK
        leaderVertices.erase(v); // nothing happen if not in set
        v->state = V_BLACK;
        // propogate to type 1/2 outedge
        if(v->type1_edge != nullptr)
            updateVertexStateAndPropagate(v->type1_edge->v,
                    leaderVertices, i_r);
        if(v->type2_edge != nullptr)
            updateVertexStateAndPropagate(v->type2_edge->v,
                    leaderVertices, i_r);
    }else if(is_gray){ // GRAY
        v->state = V_GRAY;
        leaderVertices.insert(v);
    }
}

void updateLeaders(Edge* e,
        std::unordered_set<Vertex*>& leaderVertices, int i_r){
    updateVertexStateAndPropagate(e->u, leaderVertices, i_r);
    updateVertexStateAndPropagate(e->v, leaderVertices, i_r);
}

// perform cycle removal on vehicles [i_l, i_r)
void removeType3Edges(TimingConflictGraph* G, int i_l, int i_r){
    // initialization
    for(auto& v : G->vertex_list){
        if(v->i >= i_l)
            v->state = V_WHITE;
        else
            v->state = V_BLACK;
    }
    for(auto& e : G->edge_list)
        if(e->type != E_TYPE_3)
            assert(e->state == E_ON);
    for(auto& e : G->type3_edge_list){
        int i1 = e->u->i, i2 = e->v->i;
        if(i1 < i_l || i2 < i_l){
            continue;
        }else if(i_r <= i1 && i_r <= i2){
            e->state = E_DONTCARE;
        }else if(i_l <= i1 && i1 < i_r && i_l <= i2 && i2 < i_r){
            e->state = E_UNDECIDED;
        }else if(i_l <= i1 && i1 < i_r && i_r <= i2){
            e->state = E_ON;
        }else if(i_l <= i2 && i2 < i_r && i_r <= i1){
            e->state = E_OFF;
        }
    }
    //G->updateTimeSlack();

    // start scheduling
    int is_failed = 0;
    std::unordered_set<Vertex*> leaderVertices;
    findLeaders(G, i_l, i_r, leaderVertices);
    while(leaderVertices.size() > 0){
        std::vector<Edge*> candidateEdges;
        findCandidates(leaderVertices, candidateEdges);
        assert(candidateEdges.size() > 0);
        // find-max-cost-edge
        Edge* e_max = nullptr;
        int cost_max = INT_MIN;
        for(auto& e : candidateEdges){
            assert(e->v->slack != INT_MAX);
            int cost = e->u->s + e->u->p + e->w
                - e->v->s - e->v->slack;
            if(cost > cost_max){
                cost_max = cost;
                e_max = e;
            }
        }
        Edge* e_max_sib = e_max->sibling;

        // make decision and deadlock freeness check
        e_max->state = E_OFF;
        e_max_sib->state = E_ON;
        if(!G->isDeadlockFree()){
            //printf("first try failed!\n");
            e_max->state = E_ON;
            e_max_sib->state = E_OFF;
            if(!G->isDeadlockFree()){
                //printf("second try failed!\n");
                is_failed = 1;
                break;
            }
        }
        updateLeaders(e_max, leaderVertices, i_r);
        G->updateTimeSlack();
    }


    // divide and conquer if failed
    if(is_failed){
        //printf("failed DC!\n");
        int i_mid = (i_l + i_r) / 2;
        removeType3Edges(G, i_l, i_mid);
        removeType3Edges(G, i_mid, i_r);
    }
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
    // start scheduling
    //printf("start sched\n");
    G.updateTimeSlack();
    removeType3Edges(&G, 0, G.arrival_time.size()); // [0. M)
    G.calcVertexEnterTime();
}
