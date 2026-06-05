#include <iostream>
#include "serializer.hpp"
#include "graph_tools.hpp"

int main(int argc, char *argv[]){
    if(argc != 3){
        std::cerr << "Error: Incorrect number of arguments\n";
        std::cerr << "Usage: " << argv[0] << " <inputFile> <outputFile>\n";
        return 1;
    }

    std::cout << argv[0] << " started\n";

    Data data(argv[1]);
    data.printContent();
    
    TimingConflictGraph G(data);
    G.printContentAndCheck();

    //data.writeOutput(argv[2]);

    return 0;
}
