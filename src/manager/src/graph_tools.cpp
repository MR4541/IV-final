#include <algorithm>
#include <cassert>
#include <queue>
#include <unordered_map>
#include <vector>
#include "graph_tools.hpp"
#include "serializer.hpp"

void TimingConflictGraph::buildGraph(const Data& d){
    // resize vertex_map to [M][N]
    this->vertex_map.resize(d.M);
    for(auto& vec : this->vertex_map){
        vec.resize(d.N);
    }
    // build vertex
    for(int i = 0; i < d.M; i++){
        const Vehicle& car = d.vehicles[i];
        for(const auto& j : car.zones){
            auto v = std::make_unique<Vertex>(
                    Vertex{i, j, d.zone_pass, {}, {},
                    nullptr, nullptr, V_WHITE, 0, 0, 0});
            this->vertex_map[i][j] = v.get();
            this->vertex_list.push_back(std::move(v));
        }
    }
    // build edges (type 1)
    for(int i = 0; i < d.M; i++){
        const std::vector<int>& path = d.vehicles[i].zones;
        for(int idx = 0; idx+1 < (int)path.size(); idx++){
            auto u = this->vertex_map[i][path[idx]];
            auto v = this->vertex_map[i][path[idx+1]];
            auto e = std::make_unique<Edge>(
                    Edge{u, v, d.edge_wait[E_TYPE_1], NULL,
                    E_TYPE_1, E_UNDECIDED});
            u->type1_edge = e.get();
            v->type1_in_edge = e.get();
            u->out_edges.push_back(e.get());
            v->in_edges.push_back(e.get());
            this->edge_list.push_back(std::move(e));
        }
    }
    // build edges (type 2, 3)
    int max_src_lane = -1;
    for(auto car : d.vehicles)
        max_src_lane = (car.source_lane > max_src_lane)?
            car.source_lane : max_src_lane;
    // [m] means the previous vertex from lane m in zone j
    std::vector<Vertex*> prev_car_in_lane(max_src_lane+1);
    
    // consier each conflict zone j
    for(int j = 0; j < d.N; j++){
        // type 2
        std::fill(prev_car_in_lane.begin(),
                prev_car_in_lane.end(), nullptr);
        for(int i = 0; i < d.M; i++){
            if(vertex_map[i][j] == NULL)
                continue;
            int src_lane = d.vehicles[i].source_lane;
            if(prev_car_in_lane[src_lane] == NULL){
                // first car in the lane that passes zone j
                prev_car_in_lane[src_lane] = vertex_map[i][j];
            }else{
                // create edge (prev car, j -> this car, j)
                Vertex* from = prev_car_in_lane[src_lane];
                Vertex* to = vertex_map[i][j];
                auto e = std::make_unique<Edge>(
                        Edge{from, to, d.edge_wait[E_TYPE_2], NULL,
                        E_TYPE_2, E_UNDECIDED});
                from->out_edges.push_back(e.get());
                to->in_edges.push_back(e.get());
                this->edge_list.push_back(std::move(e));
                prev_car_in_lane[src_lane] = to; // update
            }
        }
        // type 3
        for(int i1 = 0; i1 < d.M; i1++){
            if(vertex_map[i1][j] == NULL)
                continue;
            for(int i2 = i1; i2 < d.M; i2++){
                if(vertex_map[i2][j] == NULL
                        || d.vehicles[i1].source_lane 
                        == d.vehicles[i2].source_lane)
                    continue;
                Vertex *u = vertex_map[i1][j];
                Vertex *v = vertex_map[i2][j];
                auto e1 = std::make_unique<Edge>(
                        Edge{u, v, d.edge_wait[E_TYPE_3],
                        NULL, E_TYPE_3, E_UNDECIDED});
                auto e2 = std::make_unique<Edge>(
                        Edge{v, u, d.edge_wait[E_TYPE_3],
                        NULL, E_TYPE_3, E_UNDECIDED});
                e1->sibling = e2.get();
                e2->sibling = e1.get();
                u->out_edges.push_back(e1.get());
                v->in_edges.push_back(e1.get());
                v->out_edges.push_back(e2.get());
                u->in_edges.push_back(e2.get());
                this->type3_edge_list.push_back(e1.get());
                this->type3_edge_list.push_back(e2.get());
                this->edge_list.push_back(std::move(e1)); // u -> v
                this->edge_list.push_back(std::move(e2)); // v -> u
            }
        }
    }
    // copy arrival_time
    this->arrival_time.resize(d.M);
    for(int i = 0; i < d.M; i++)
        this->arrival_time[i] = d.vehicles[i].arrival_time;
    this->source_lane.resize(d.M);
    for(int i = 0; i < d.M; i++)
        this->source_lane[i] = d.vehicles[i].source_lane;
    this->same_src_delay = d.zone_pass + d.edge_wait[E_TYPE_2];
}

// 1 if cycle exists
int prioritizedTopoSort(TimingConflictGraph *G,
        std::vector<Vertex*>& topoOrder){
    // kahn's algorithm but with pseudo edges
    std::unordered_map<Vertex*, int> in_degree;
    for(const auto& v : G->vertex_list)
        in_degree[v.get()] = 0;
    for(const auto& e : G->edge_list) // add ON edges
        if(e->state == E_ON)
            in_degree[e->v]++;

    // add pseudo edge
    std::unordered_map<Vertex*, std::vector<Vertex*>> pseudo_edges;
    for(const auto& u : G->vertex_list){
        // type 1 edge (u.i, u.j) -> (u.i, j2)
        if(u->type1_edge == NULL)
            continue;
        // for type 2/3 edge (u.i, u.j) -> (i2, u.j)
        // add pseudo edge (u.i, j2) -> (i2, u.j)
        for(const auto& e : u->out_edges){
            if(e->type == E_TYPE_1 || e->state != E_ON)
                continue;
            pseudo_edges[u->type1_edge->v].push_back(e->v);
            in_degree[e->v]++;
        }
    }

    std::queue<Vertex*> q;
    for(const auto& v : G->vertex_list)
        if(in_degree[v.get()] == 0)
            q.push(v.get());
    
    while(!q.empty()){
        Vertex* u = q.front();
        topoOrder.push_back(u);
        q.pop();

        for(auto& e : u->out_edges){ // real edges
            if(e->state != E_ON)
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
    // cycle detection
    return topoOrder.size() != G->vertex_list.size();
}

void TimingConflictGraph::calcVertexEnterTime(){
    for(const auto& e : edge_list)
        assert(e->state == E_ON || e->state == E_OFF);
    std::vector<Vertex*> topoOrder;
    // calculate global order, assert no cycle
    assert(prioritizedTopoSort(this, topoOrder) == 0);
    
    // calculate vertex enter time
    for(auto& v : topoOrder){
        int max_s = arrival_time[v->i];
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
    }
}

void TimingConflictGraph::updateTimeSlack(){
    std::vector<Vertex*> topoOrder;
    // assert no cycle
    assert(prioritizedTopoSort(this, topoOrder) == 0);

    // calculate vertex enter time
    for(auto& v : topoOrder){
        int max_s = arrival_time[v->i];
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
    }
    
    // calculate vertex slack
    int max_leave_t = 0;
    for(auto& v : vertex_list)
        max_leave_t = std::max(max_leave_t, v->s + v->p);
    // visit in reversed topoOrder
    for(auto it = topoOrder.rbegin(); it != topoOrder.rend(); ++it){
        Vertex* u = *it;
        u->slack = max_leave_t - u->s - u->p;
        for(auto& e : u->out_edges)
            u->slack = std::min(u->slack, e->v->slack);
    }
}

int isAcyclic(TimingConflictGraph *G){
    std::unordered_map<Vertex*, int> in_degree;
    for(const auto& v : G->vertex_list)
        in_degree[v.get()] = 0;
    for(const auto& e : G->edge_list)
        if(e->state == E_ON)
            in_degree[e->v]++;
    
    std::queue<Vertex*> q;
    for(const auto& v : G->vertex_list)
        if(in_degree[v.get()] == 0)
            q.push(v.get());
    int finished = 0;
    while(!q.empty()){
        Vertex *u = q.front();
        finished++;
        q.pop();
        for(auto& e : u->out_edges){
            if(e->state != E_ON)
                continue;
            in_degree[e->v]--;
            if(in_degree[e->v] == 0)
                q.push(e->v);
        }
    }
    return finished == (int)G->vertex_list.size();
}

struct HVertex{
    int i, j1, j2;

    // for unordered_map
    bool operator==(const HVertex& other) const {
        return i == other.i && j1 == other.j1 && j2 == other.j2;
    }
};

struct HVertexHash{
    size_t operator()(const HVertex& v) const {
        size_t h1 = std::hash<int>{}(v.i);
        size_t h2 = std::hash<int>{}(v.j1);
        size_t h3 = std::hash<int>{}(v.j2);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

int TimingConflictGraph::isDeadlockFree(){
    assert(isAcyclic(this));
    // build vertex (i, j1, j2) for u(i, j1) -> v(i, j2)
    std::vector<HVertex> V;
    std::unordered_map<HVertex, int, HVertexHash> in_degree;
    std::unordered_map<HVertex, std::vector<HVertex>,
        HVertexHash> out_edge_list;
    for(auto& u : vertex_list){
        if(u->type1_edge != nullptr){
            HVertex from = {u->i, u->j, u->type1_edge->v->j};
            in_degree[from] = 0;
            V.push_back(from);
        }
    }

    // build edge (type 1)
    for(auto& u : vertex_list){
        if(u->type1_edge != nullptr &&
                u->type1_edge->v->type1_edge != nullptr){
            Vertex* v = u->type1_edge->v;
            HVertex from = {u->i, u->j, v->j};
            HVertex to = {v->i, v->j, v->type1_edge->v->j};
            in_degree[to]++;
            out_edge_list[from].push_back(to);
        }
    }
    // build edge (other 4 types)
    for(auto& e : edge_list){ // e: the type 2/3 edge
        if(e->state != E_ON || e->type == E_TYPE_1)
            continue;
        std::vector<HVertex> froms = {{-1, 0, 0}, {-1, 0, 0}};
        std::vector<HVertex> tos = {{-1, 0, 0}, {-1, 0, 0}};
        if(e->u->type1_in_edge != nullptr)
            froms[0] = {e->u->i, e->u->type1_in_edge->u->j, e->u->j};
        if(e->u->type1_edge != nullptr)
            froms[1] = {e->u->i, e->u->j, e->u->type1_edge->v->j};
        if(e->v->type1_in_edge != nullptr)
            tos[0] = {e->v->i, e->v->type1_in_edge->u->j, e->v->j};
        if(e->v->type1_edge != nullptr)
            tos[1] = {e->v->i, e->v->j, e->v->type1_edge->v->j};
        for(auto& from : froms){
            if(from.i == -1) continue;
            for(auto& to : tos){
                if(to.i == -1) continue;
                in_degree[to]++;
                out_edge_list[from].push_back(to);
            }
        }
    }
    // cycle check (kahn's algorithm)
    int finished = 0;
    std::queue<HVertex> q;
    for(auto& v : V)
        if(in_degree[v] == 0)
            q.push(v);
    while(!q.empty()){
        HVertex from = q.front();
        finished++;
        q.pop();
        for(auto& to : out_edge_list[from]){
            in_degree[to]--;
            if(in_degree[to] == 0)
                q.push(to);
        }
    }
    
    return finished == (int)V.size();
}

int printEdge(const Edge* e){
    std::string state;
    switch(e->state){
        case E_ON: state = "ON"; break;
        case E_OFF: state = "OFF"; break;
        case E_UNDECIDED: state = "UNDECIDED"; break;
        case E_DONTCARE: state = "DOTCAR"; break;
    }
    printf("\t(%d,%d)->(%d,%d) T-%d w %d [%s]\n",
            e->u->i, e->u->j, e->v->i, e->v->j, e->type+1,
            e->w, state.c_str());
    if(e->type == E_TYPE_3){
        const Edge* sib = e->sibling;
        if(!(e->u == sib->v && e->v == sib->u)){
            printf("\tsibling check fail!\n");
            return 1;
        }
    }
    return 0;
}

void TimingConflictGraph::printContentAndCheck(){
    printf("===== TimingConflictGraph =====\n"
            "|E|: %d\n"
            "vertex_list:\n", (int)edge_list.size());
    int in_edges_num = 0, out_edges_num = 0, pair_error = 0;
    for(const auto& u : vertex_list){
        printf("[i %3d, j %3d], s %3d, pass_time: %d\n"
                "\ttype1_edges:", u->i, u->j, u->s, u->p);
        if(u->type1_edge != NULL) printf(" -> (%d, %d)\n",
                u->type1_edge->v->i, u->type1_edge->v->j);
        else printf(" DNE\n");
        printf("\tin_edges:\n");
        for(const auto& e : u->in_edges){
            pair_error += printEdge(e);
            in_edges_num++;
        }
        printf("\tout_edges:\n");
        for(const auto& e : u->out_edges){
            pair_error += printEdge(e);
            out_edges_num++;
        }
    }

    int M = vertex_map.size(), N = vertex_map[0].size();
    printf("check if vertex_map and vertex_list are sync\n"
            "none = 0, in map = 1, in list = 2, both = 3\n"
            "%d cars\\%d zones\n", M, N);
    int vertex_matrix[M][N];
    for(int i = 0; i < M*N; i++) vertex_matrix[i%M][i%N] = 0;
    for(const auto& u : vertex_list)
        vertex_matrix[u->i][u->j] += 2;
    for(int i = 0; i < M; i++)
        for(int j = 0; j < N; j++){
            if(vertex_map[i][j] != NULL)
                vertex_matrix[i][j]++;
            printf("%s%1d%s", j==0? "[ ":"",
                    vertex_matrix[i][j], j==N-1? " ]\n":" ");
        }
    
    int map_list_same = 1;
    for(const auto& u : vertex_list)
        if(vertex_map[u->i][u->j] != u.get())
            map_list_same = 0;
    printf("other checks:\n"
            "\t|E| = sum|in_edges| ? %s\n"
            "\t|E| = sum|out_edges| ? %s\n"
            "\tvertex map, list same content ? %s\n"
            "\tno Type-3 pair error? %s\n",
            (int)edge_list.size() == in_edges_num ? "Yes" : "No",
            (int)edge_list.size() == out_edges_num ? "Yes" : "No",
            map_list_same ? "Yes" : "No",
            pair_error == 0 ? "Yes" : "No");
}
