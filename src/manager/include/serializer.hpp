#ifndef SERIALIZER_H
#define SERIALIZER_H

#include <vector>
#include <string>

// Vehicle: data of one vehicle
typedef struct {
    // input (read from JSON)
    int vid; // vehicle id
    int arrival_time;
    int source_lane;
    std::vector<int> zones; // represent a trajectory
    // output
    std::vector<int> schedule; // time to visit each conflict zone
} Vehicle;

// Data: all we need to perform intersection management
typedef struct Data{
    int zone_delay; // minimum time to pass a conflict zone
    std::vector<Vehicle> vehicles;
    int is_scheduled;
    // methods
    void readInput(std::string inputFile);
    void writeOutput(std::string outputFile);
    void printContent(); // for debugging purpose

    Data(std::string inputFile){
        readInput(inputFile);
    }
} Data;

#endif
