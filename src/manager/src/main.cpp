#include <iostream>
#include "json.hpp"
#include "serializer.hpp"

using json = nlohmann::json;

int main(void){
    std::cout << "main started\n";
    Data data("mockdata/input_1.json");
    
    data.printContent();

    data.writeOutput("mockdata/output_1.json");
}
