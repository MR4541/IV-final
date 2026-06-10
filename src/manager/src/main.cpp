#include <cstdlib>
#include <iostream>
#include "serializer.hpp"
#include "graph_tools.hpp"
#include "scheduling_algo.hpp"

void printUsage(char *argv_0){
    std::cerr << "Error: Incorrect number of arguments\n";
    std::cerr << "Usage: " << argv_0 << " <inputFile> <outputFile> <Algo=1|2|3|4|5>\n";
    std::cerr << "1 - 3D-intersecton\n2 - FCFS\n3 - Priority\n4 - Graph-based\n5 - All\n";
    exit(1);
}

int main(int argc, char *argv[]){
    if(argc != 4){
        printUsage(argv[0]);
    }

    Data data(argv[1]);
    //data.printContent();
    
    TimingConflictGraph G(data);
    //G.printContentAndCheck();
    
    switch (std::atoi(argv[3])) {
        case 1:
            SchedAlgo::threeDimension(G);
            break;
        case 2:
            SchedAlgo::firstComeFirstServe(G);
            break;
        case 3:
            SchedAlgo::priorityBased(G);
            break;
        case 4:
        default:
            std::cout << "Not Implemented Yet QAQ\n";
            exit(0);
    }

    G.printContentAndCheck();

    data.getSchedOutput(G);
    data.writeOutput(argv[2]);

    return 0;
}
