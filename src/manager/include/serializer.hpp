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
    int arrival_time;
    int source_lane;
    int destination_lane;
    std::vector<int> zones;     // represent a trajectory
    // output
    std::vector<int> schedule;  // time to visit each conflict zone
} Vehicle;

// Point: 2D point for the position of a conflict zone
typedef struct Point{
    double x, y;
} Point;

// Data: all we need to perform intersection management
typedef struct Data{
    // info of intersection
    
    int zone_pass; // vertex passing time
    std::vector<int> edge_wait; // edge waiting time[3]
    int N; // number of conflict zone
    std::vector<Point> zone_pos_list; // position of all zones

    // info of vehicles
    
    int M; // number of vehicles
    std::vector<Vehicle> vehicles; // sort by arrival_time in ascending order
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
