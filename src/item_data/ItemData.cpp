#include "ItemData.h"
#include <cmath>
#include <iostream>

std::mutex item_data_mutex;   // <--- single definition

double roundTo(double value, int decimals) {
    double factor = std::pow(10.0, decimals);
    return std::round(value * factor) / factor;
}

void roundItemData(ItemData& data, int decimals) {
    std::lock_guard<std::mutex> lock(item_data_mutex);

    data.Pos_x = roundTo(data.Pos_x, decimals);
    data.Pos_y = roundTo(data.Pos_y, decimals);

    data.Force_x = roundTo(data.Force_x, decimals);
    data.Force_y = roundTo(data.Force_y, decimals);

    data.Vel_x = roundTo(data.Vel_x, decimals);
    data.Vel_y = roundTo(data.Vel_y, decimals);

    data.mass = roundTo(data.mass, decimals);
    data.visc_damp_coef = roundTo(data.visc_damp_coef, decimals);
    data.repl_coef = roundTo(data.repl_coef, decimals);
    data.attr_coef = roundTo(data.attr_coef, decimals);

    data.timeStamp = roundTo(data.timeStamp, 3);
    data.distance_traveled = roundTo(data.distance_traveled, 3);
    data.score = roundTo(data.score, 0);
}

const char* type_to_string(ItemData::ItemType type) {
    switch (type) {
        case ItemData::ItemType::Drone: return "Drone";
        case ItemData::ItemType::Obstacle: return "Obstacle";
        case ItemData::ItemType::Target: return "Target";
        default: return "Unknown";
    }
}

void print_item_info(const ItemData& item) {
    std::lock_guard<std::mutex> lock(item_data_mutex);

    std::cout << "Item Data:\n";
    std::cout << "------------------------------\n";
    std::cout << "Type: " << type_to_string(item.type) << "\n";
    std::cout << "Position: (" << item.Pos_x << ", " << item.Pos_y << ")\n";
    std::cout << "Velocity: (" << item.Vel_x << ", " << item.Vel_y << ")\n";
    std::cout << "Force: (" << item.Force_x << ", " << item.Force_y << ")\n";
    std::cout << "Mass: " << item.mass << "\n";
    std::cout << "Viscous Damping: " << item.visc_damp_coef << "\n";
    std::cout << "Obstacle Repulsion: " << item.repl_coef << "\n";
    std::cout << "Target Attractiveness: " << item.attr_coef << "\n";
    std::cout << "Time Stamp: " << item.timeStamp << "\n";
    std::cout << "Distance Traveled: " << item.distance_traveled << "\n";
    std::cout << "Score: " << item.score << "\n";
    std::cout << "Hit Targets: " << item.number_of_hit_targets << "\n";
    std::cout << "Hit Obstacles: " << item.number_of_hit_obstacles << "\n";
    std::cout << "Active: " << (item.active ? "Yes" : "No") << "\n";
    std::cout << "------------------------------\n";
}
