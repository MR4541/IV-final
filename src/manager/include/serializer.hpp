#ifndef SERIALIZER_H
#define SERIALIZER_H

/**
 * JSON serializer
 * JSON <-> data (e.g. arrival time, conflict zones) that are
 * needed by Intersection Manager
 * 
 * Data data(inputFile);        // read input & initialize
 * data.readInput(inputFile);   // or this
 *
 * data.writeOutput(outputFile);// write result to JSON
 *
 * data.printContent();         // for debugging
 */


#include <vector>
#include <string>

// Vehicle: data of one vehicle
typedef struct {
    // input (read from JSON)
    int vid;                    // vehicle id
    int arrival_time;
    int source_lane;
    std::vector<int> zones;     // represent a trajectory
    // output
    std::vector<int> schedule;  // time to visit each conflict zone
} Vehicle;

// Data: all we need to perform intersection management
typedef struct Data{
    int zone_delay; // minimum time to pass a conflict zone
    std::vector<Vehicle> vehicles;
    int is_scheduled; // set to 0 before scheduling
    
    // methods
    
    // read input data from inputFile 
    void readInput(std::string inputFile);
    // write scheduled result to outputFile
    void writeOutput(std::string outputFile);
    // for debugging purpose
    void printContent();

    Data() : is_scheduled(0) {}
    Data(std::string inputFile){
        readInput(inputFile);
    }
} Data;

#endif
