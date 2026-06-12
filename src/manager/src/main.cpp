#include <cstdlib>
#include <iostream>
#include <chrono>
#include <string>
#include "serializer.hpp"
#include "graph_tools.hpp"
#include "scheduling_algo.hpp"

void printUsage(char *argv_0){
    std::cerr << "Usage: " << argv_0 << " <inputFile> <outputFile> <Algo=1|2|3|4>\n";
    std::cerr << "1 - 3D-intersecton\n2 - FCFS\n3 - Priority\n4 - Graph-based\n";
    exit(1);
}

// compute T_D
double calcAvgDelay(const Data& d){
    TimingConflictGraph G_3D(d);
    SchedAlgo::threeDimension(G_3D);
    int sum_delay = 0;
    for(int i = 0; i < d.M; i++){
        int t_3D = G_3D.vertex_map[i][d.vehicles[i].zones.back()]->s;
        int t = d.vehicles[i].schedule.back();
        sum_delay += t - t_3D;
    }
    return (double)sum_delay / d.M;
}

int main(int argc, char *argv[]){
    if(argc != 4){
        printUsage(argv[0]);
    }

    Data data(argv[1]);
    //data.printContent();
    
    auto start_full = std::chrono::high_resolution_clock::now();
    TimingConflictGraph G(data);
    //G.printContentAndCheck();
    
    auto start = std::chrono::high_resolution_clock::now();
    std::string method;
    switch (std::atoi(argv[3])) {
        case 1:
            method = "3D-intersecton";
            SchedAlgo::threeDimension(G);
            break;
        case 2:
            method = "FCFS";
            SchedAlgo::firstComeFirstServe(G);
            break;
        case 3:
            method = "Priority";
            SchedAlgo::priorityBased(G);
            break;
        case 4:
        default:
            std::cout << "Not Implemented Yet QAQ\n";
            printUsage(argv[0]);
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> runtime = end - start;

    //G.printContentAndCheck();

    data.getSchedOutput(G);
    auto end_full = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> runtime_full =
        end_full - start_full;

    std::cout << "Method: " << method << '\n';
    std::cout << "M: " << data.M << '\n';
    std::cout << "Algorithm Runtime (ms): " << runtime.count() << '\n';
    std::cout << "Full Runtime (ms): " << runtime_full.count() << '\n';
    std::cout << "T_L (ticks): " << data.getMaxLeavingTime() << '\n';
    std::cout << "T_D (ticks): " << calcAvgDelay(data) << '\n';

    data.writeOutput(argv[2]);

    return 0;
}
