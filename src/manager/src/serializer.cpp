#include <stdio.h>
#include <fstream>
#include <algorithm>
#include <vector>
#include "json.hpp"
#include "serializer.hpp"

using json = nlohmann::json;

// helper function to read array from JSON
void from_json(const json& j, Vehicle& v){
    j.at("src_lane").get_to(v.source_lane);
    j.at("dst_lane").get_to(v.destination_lane);
    j.at("path").get_to(v.zones);
    j.at("arrival").get_to(v.arrival_time);
}

void from_json(const json& j, Point& p){
    std::vector<double> vec = j.get<std::vector<double>>();
    p.x = vec[0];
    p.y = vec[1];
}

// read input from a JSON file and initialize this structure
void Data::readInput(std::string inputFile){
    std::ifstream f(inputFile);
    json inputData = json::parse(f);
    
    this->zone_pass = inputData["zone_passing_time"];
    this->edge_wait = inputData["edge_waiting_time"].get<std::vector<int>>();
    this->zone_pos_list = inputData["zones"].get<std::vector<Point>>();
    this->N = this->zone_pos_list.size();
    this->vehicles = inputData["vehicles"].get<std::vector<Vehicle>>();
    this->M = this->vehicles.size();
    // sort vehicles by arrival_time in ascending order
    std::sort(this->vehicles.begin(), this->vehicles.end(),
            [](const Vehicle& a, const Vehicle& b){
                return a.arrival_time < b.arrival_time;
            });
    this->is_scheduled = 0;
    for(auto& car : this->vehicles){
        car.schedule.resize(car.zones.size());
        car.schedule.assign(car.schedule.size(), 0);
    }
} // Data::readInput

// helper function to write vector to JSON
void to_json(json& j, const Vehicle& v){
    j = json{
        {"src_lane", v.source_lane},
        {"dst_lane", v.destination_lane},
        {"arrival", v.arrival_time},
        {"path", v.zones},
        {"schedule", v.schedule}
    };
}

void to_json(json& j, const Point& p){
    j = json{p.x, p.y}; // [Number, Number]
}

// write the generated schedule to an output JSON file
void Data::writeOutput(std::string outputFile){
    json outputData;
    outputData["zone_passing_time"] = this->zone_pass;
    outputData["edge_waiting_time"] = this->edge_wait;
    outputData["zones"] = this->zone_pos_list;
    outputData["vehicles"] = this->vehicles;

    std::ofstream f(outputFile);
    f << outputData.dump(2); // indent = 2
} // Data::writeOutput

// print the contents; for debugging purpose
void Data::printContent(){
    printf("===== printContent =====\n"
            "%s"
            "N: %d, M: %d\n"
            "zone_pass: %d, edge_wait: %d %d %d\n"
            "zone_positions:\n\t",
            this->is_scheduled? "" : "not scheduled yet!\n",
            this->N, this->M,
            this->zone_pass, this->edge_wait[0],
            this->edge_wait[1], this->edge_wait[2]);
    for(const auto& p : this->zone_pos_list)
        printf("(%f, %f), ", p.x, p.y);
    int counter = 0;
    printf("\nvehicles:\n");
    for(const auto& car : this->vehicles){
        counter++;
        printf("[%3d]\n"
                "\tarrive_time: %d, src: %d, dest: %d\n"
                "\tzones:",
                counter, car.arrival_time, car.source_lane, 
                car.destination_lane);
        int path_len = car.zones.size();
        for(int i = 0; i < path_len; i++){
            printf(" %3d", car.zones[i]);
        }
        printf("\n");
        if(this->is_scheduled){
            printf("\tsched:");
            for(int i = 0; i < path_len; i++){
                printf(" %3d", car.schedule[i]);
            }
            printf("\n");
        }
    }
    printf("====================\n");
} // Data::printContent
