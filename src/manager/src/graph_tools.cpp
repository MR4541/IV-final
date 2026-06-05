#include <memory>
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
                    Vertex{i, j, d.zone_pass, {}, {}, V_WHITE, 0});
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
                this->edge_list.push_back(std::move(e1)); // u -> v
                this->edge_list.push_back(std::move(e2)); // v -> u
            }
        }
    }
}

int printEdge(const Edge* e){
    printf("\t(%d,%d)->(%d,%d) T-%d w %d\n",
            e->u->i, e->u->j, e->v->i, e->v->j, e->type+1, e->w);
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
        printf("[i %3d, j %3d], passing_time: %d\n"
                "\tin_edges:\n", u->i, u->j, u->p);
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
